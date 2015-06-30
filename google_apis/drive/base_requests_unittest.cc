// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/drive/base_requests.h"

#include "base/bind.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/values.h"
#include "google_apis/drive/drive_api_parser.h"
#include "google_apis/drive/drive_api_requests.h"
#include "google_apis/drive/dummy_auth_service.h"
#include "google_apis/drive/request_sender.h"
#include "google_apis/drive/test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/embedded_test_server/http_request.h"
#include "net/test/embedded_test_server/http_response.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace google_apis {

namespace {

const char kValidJsonString[] = "{ \"test\": 123 }";
const char kInvalidJsonString[] = "$$$";

class FakeUrlFetchRequest : public UrlFetchRequestBase {
 public:
  FakeUrlFetchRequest(RequestSender* sender,
                      const EntryActionCallback& callback,
                      const GURL& url)
      : UrlFetchRequestBase(sender),
        callback_(callback),
        url_(url) {
  }

  ~FakeUrlFetchRequest() override {}

 protected:
  GURL GetURL() const override { return url_; }
  void ProcessURLFetchResults(const net::URLFetcher* source) override {
    callback_.Run(GetErrorCode());
  }
  void RunCallbackOnPrematureFailure(DriveApiErrorCode code) override {
    callback_.Run(code);
  }

  EntryActionCallback callback_;
  GURL url_;
};

class FakeMultipartUploadRequest : public MultipartUploadRequestBase {
 public:
  FakeMultipartUploadRequest(
      base::SequencedTaskRunner* blocking_task_runner,
      const std::string& metadata_json,
      const std::string& content_type,
      int64 content_length,
      const base::FilePath& local_file_path,
      const FileResourceCallback& callback,
      const google_apis::ProgressCallback& progress_callback,
      const GURL& url,
      std::string* upload_content_type,
      std::string* upload_content_data)
      : MultipartUploadRequestBase(blocking_task_runner,
                                   metadata_json,
                                   content_type,
                                   content_length,
                                   local_file_path,
                                   callback,
                                   progress_callback),
        url_(url),
        upload_content_type_(upload_content_type),
        upload_content_data_(upload_content_data) {}

  ~FakeMultipartUploadRequest() override {}

  net::URLFetcher::RequestType GetRequestType() const override {
    return net::URLFetcher::POST;
  }

  bool GetContentData(std::string* content_type,
                      std::string* content_data) override {
    const bool result =
        MultipartUploadRequestBase::GetContentData(content_type, content_data);
    *upload_content_type_ = *content_type;
    *upload_content_data_ = *content_data;
    return result;
  }

 protected:
  GURL GetURL() const override { return url_; }

 private:
  const GURL url_;
  std::string* const upload_content_type_;
  std::string* const upload_content_data_;
};

}  // namespace

class BaseRequestsTest : public testing::Test {
 public:
  BaseRequestsTest() : response_code_(net::HTTP_OK) {}

  void SetUp() override {
    request_context_getter_ = new net::TestURLRequestContextGetter(
        message_loop_.task_runner());

    sender_.reset(new RequestSender(new DummyAuthService,
                                    request_context_getter_.get(),
                                    message_loop_.task_runner(),
                                    std::string() /* custom user agent */));

    ASSERT_TRUE(test_server_.InitializeAndWaitUntilReady());
    test_server_.RegisterRequestHandler(
        base::Bind(&BaseRequestsTest::HandleRequest, base::Unretained(this)));
  }

  scoped_ptr<net::test_server::HttpResponse> HandleRequest(
      const net::test_server::HttpRequest& request) {
    scoped_ptr<net::test_server::BasicHttpResponse> response(
        new net::test_server::BasicHttpResponse);
    response->set_code(response_code_);
    response->set_content(response_body_);
    response->set_content_type("application/json");
    return response.Pass();
  }

  base::MessageLoopForIO message_loop_;
  scoped_refptr<net::TestURLRequestContextGetter> request_context_getter_;
  scoped_ptr<RequestSender> sender_;
  net::test_server::EmbeddedTestServer test_server_;

  net::HttpStatusCode response_code_;
  std::string response_body_;
};

typedef BaseRequestsTest MultipartUploadRequestBaseTest;

TEST_F(BaseRequestsTest, ParseValidJson) {
  scoped_ptr<base::Value> json(ParseJson(kValidJsonString));

  base::DictionaryValue* root_dict = NULL;
  ASSERT_TRUE(json);
  ASSERT_TRUE(json->GetAsDictionary(&root_dict));

  int int_value = 0;
  ASSERT_TRUE(root_dict->GetInteger("test", &int_value));
  EXPECT_EQ(123, int_value);
}

TEST_F(BaseRequestsTest, ParseInvalidJson) {
  EXPECT_FALSE(ParseJson(kInvalidJsonString));
}

TEST_F(BaseRequestsTest, UrlFetchRequestBaseResponseCodeOverride) {
  response_code_ = net::HTTP_FORBIDDEN;
  response_body_ =
      "{\"error\": {\n"
      "  \"errors\": [\n"
      "   {\n"
      "    \"domain\": \"usageLimits\",\n"
      "    \"reason\": \"rateLimitExceeded\",\n"
      "    \"message\": \"Rate Limit Exceeded\"\n"
      "   }\n"
      "  ],\n"
      "  \"code\": 403,\n"
      "  \"message\": \"Rate Limit Exceeded\"\n"
      " }\n"
      "}\n";

  DriveApiErrorCode error = DRIVE_OTHER_ERROR;
  base::RunLoop run_loop;
  sender_->StartRequestWithAuthRetry(new FakeUrlFetchRequest(
      sender_.get(),
      test_util::CreateQuitCallback(
          &run_loop, test_util::CreateCopyResultCallback(&error)),
      test_server_.base_url()));
  run_loop.Run();

  // HTTP_FORBIDDEN (403) is overridden by the error reason.
  EXPECT_EQ(HTTP_SERVICE_UNAVAILABLE, error);
}

TEST_F(MultipartUploadRequestBaseTest, Basic) {
  response_code_ = net::HTTP_OK;
  response_body_ = "{\"kind\": \"drive#file\", \"id\": \"file_id\"}";
  scoped_ptr<google_apis::FileResource> file;
  DriveApiErrorCode error = DRIVE_OTHER_ERROR;
  base::RunLoop run_loop;
  const base::FilePath source_path =
      google_apis::test_util::GetTestFilePath("chromeos/file_manager/text.txt");
  std::string upload_content_type;
  std::string upload_content_data;
  FakeMultipartUploadRequest* const multipart_request =
      new FakeMultipartUploadRequest(
          sender_->blocking_task_runner(), "{json:\"test\"}", "text/plain", 10,
          source_path,
          test_util::CreateQuitCallback(
              &run_loop, test_util::CreateCopyResultCallback(&error, &file)),
          ProgressCallback(), test_server_.base_url(), &upload_content_type,
          &upload_content_data);
  multipart_request->SetBoundaryForTesting("TESTBOUNDARY");
  scoped_ptr<drive::SingleBatchableDelegateRequest> request(
      new drive::SingleBatchableDelegateRequest(
          sender_.get(), multipart_request));
  sender_->StartRequestWithAuthRetry(request.release());
  run_loop.Run();
  EXPECT_EQ("multipart/related; boundary=TESTBOUNDARY", upload_content_type);
  EXPECT_EQ(
      "--TESTBOUNDARY\n"
      "Content-Type: application/json\n"
      "\n"
      "{json:\"test\"}\n"
      "--TESTBOUNDARY\n"
      "Content-Type: text/plain\n"
      "\n"
      "This is a sample file. I like chocolate and chips.\n"
      "\n"
      "--TESTBOUNDARY--",
      upload_content_data);
  ASSERT_EQ(HTTP_SUCCESS, error);
  EXPECT_EQ("file_id", file->file_id());
}

}  // Namespace google_apis
