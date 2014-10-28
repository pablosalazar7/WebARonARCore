// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/values.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/base/web_ui_browser_test.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_message_handler.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest-spi.h"

using content::WebUIMessageHandler;

// According to the interface for EXPECT_FATAL_FAILURE
// (http://code.google.com/p/googletest/wiki/AdvancedGuide#Catching_Failures)
// the statement must be statically available. Therefore, we make a static
// global s_test_ which should point to |this| for the duration of the test run
// and be cleared afterward.
class WebUIBrowserExpectFailTest : public WebUIBrowserTest {
 public:
  WebUIBrowserExpectFailTest() {
    EXPECT_FALSE(s_test_);
    s_test_ = this;
  }

 protected:
  ~WebUIBrowserExpectFailTest() override {
    EXPECT_TRUE(s_test_);
    s_test_ = NULL;
  }

  static void RunJavascriptTestNoReturn(const std::string& testname) {
    EXPECT_TRUE(s_test_);
    s_test_->RunJavascriptTest(testname);
  }

  static void RunJavascriptAsyncTestNoReturn(const std::string& testname) {
    EXPECT_TRUE(s_test_);
    s_test_->RunJavascriptAsyncTest(testname);
  }

 private:
  static WebUIBrowserTest* s_test_;
};

WebUIBrowserTest* WebUIBrowserExpectFailTest::s_test_ = NULL;

// Test that bogus javascript fails fast - no timeout waiting for result.
IN_PROC_BROWSER_TEST_F(WebUIBrowserExpectFailTest, TestFailsFast) {
  AddLibrary(base::FilePath(FILE_PATH_LITERAL("sample_downloads.js")));
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIDownloadsURL));
  EXPECT_FATAL_FAILURE(RunJavascriptTestNoReturn("DISABLED_BogusFunctionName"),
                       "WebUITestHandler::JavaScriptComplete");
}

// Test that bogus javascript fails fast - no timeout waiting for result.
IN_PROC_BROWSER_TEST_F(WebUIBrowserExpectFailTest, TestRuntimeErrorFailsFast) {
  AddLibrary(base::FilePath(FILE_PATH_LITERAL("runtime_error.js")));
  ui_test_utils::NavigateToURL(browser(), GURL(kDummyURL));
  EXPECT_FATAL_FAILURE(RunJavascriptTestNoReturn("TestRuntimeErrorFailsFast"),
                       "WebUITestHandler::JavaScriptComplete");
}

// Test that bogus javascript fails async test fast as well - no timeout waiting
// for result.
IN_PROC_BROWSER_TEST_F(WebUIBrowserExpectFailTest, TestFailsAsyncFast) {
  AddLibrary(base::FilePath(FILE_PATH_LITERAL("sample_downloads.js")));
  ui_test_utils::NavigateToURL(browser(), GURL(chrome::kChromeUIDownloadsURL));
  EXPECT_FATAL_FAILURE(
      RunJavascriptAsyncTestNoReturn("DISABLED_BogusFunctionName"),
      "WebUITestHandler::JavaScriptComplete");
}

// Tests that the async framework works.
class WebUIBrowserAsyncTest : public WebUIBrowserTest {
 public:
  // Calls the testDone() function from test_api.js
  void TestDone() {
    RunJavascriptFunction("testDone");
  }

  // Starts a failing test.
  void RunTestFailsAssert() {
    RunJavascriptFunction("runAsync", new base::StringValue("testFailsAssert"));
  }

  // Starts a passing test.
  void RunTestPasses() {
    RunJavascriptFunction("runAsync", new base::StringValue("testPasses"));
  }

 protected:
  WebUIBrowserAsyncTest() {}

  // Class to synchronize asynchronous javascript activity with the tests.
  class AsyncWebUIMessageHandler : public WebUIMessageHandler {
   public:
    AsyncWebUIMessageHandler() {}

    MOCK_METHOD1(HandleTestContinues, void(const base::ListValue*));
    MOCK_METHOD1(HandleTestFails, void(const base::ListValue*));
    MOCK_METHOD1(HandleTestPasses, void(const base::ListValue*));

   private:
    virtual void RegisterMessages() override {
      web_ui()->RegisterMessageCallback("startAsyncTest",
          base::Bind(&AsyncWebUIMessageHandler::HandleStartAsyncTest,
                     base::Unretained(this)));
      web_ui()->RegisterMessageCallback("testContinues",
          base::Bind(&AsyncWebUIMessageHandler::HandleTestContinues,
                     base::Unretained(this)));
      web_ui()->RegisterMessageCallback("testFails",
          base::Bind(&AsyncWebUIMessageHandler::HandleTestFails,
                     base::Unretained(this)));
      web_ui()->RegisterMessageCallback("testPasses",
          base::Bind(&AsyncWebUIMessageHandler::HandleTestPasses,
                     base::Unretained(this)));
    }

    // Starts the test in |list_value|[0] with the runAsync wrapper.
    void HandleStartAsyncTest(const base::ListValue* list_value) {
      const base::Value* test_name;
      ASSERT_TRUE(list_value->Get(0, &test_name));
      web_ui()->CallJavascriptFunction("runAsync", *test_name);
    }

    DISALLOW_COPY_AND_ASSIGN(AsyncWebUIMessageHandler);
  };

  // Handler for this object.
  ::testing::StrictMock<AsyncWebUIMessageHandler> message_handler_;

 private:
  // Provide this object's handler.
  virtual WebUIMessageHandler* GetMockMessageHandler() override {
    return &message_handler_;
  }

  // Set up and browse to kDummyURL for all tests.
  virtual void SetUpOnMainThread() override {
    WebUIBrowserTest::SetUpOnMainThread();
    AddLibrary(base::FilePath(FILE_PATH_LITERAL("async.js")));
    ui_test_utils::NavigateToURL(browser(), GURL(kDummyURL));
  }

  DISALLOW_COPY_AND_ASSIGN(WebUIBrowserAsyncTest);
};

// Test that assertions fail immediately after assertion fails (no testContinues
// message). (Sync version).
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestSyncOkTestFail) {
  ASSERT_FALSE(RunJavascriptTest("testFailsAssert"));
}

// Test that assertions fail immediately after assertion fails (no testContinues
// message). (Async version).
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestAsyncFailsAssert) {
  EXPECT_CALL(message_handler_, HandleTestFails(::testing::_));
  ASSERT_FALSE(RunJavascriptAsyncTest(
      "startAsyncTest", new base::StringValue("testFailsAssert")));
}

// Test that expectations continue the function, but fail the test.
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestAsyncFailsExpect) {
  ::testing::InSequence s;
  EXPECT_CALL(message_handler_, HandleTestContinues(::testing::_));
  EXPECT_CALL(message_handler_, HandleTestFails(::testing::_));
  ASSERT_FALSE(RunJavascriptAsyncTest(
      "startAsyncTest", new base::StringValue("testFailsExpect")));
}

// Test that test continues and passes. (Sync version).
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestSyncPasses) {
  EXPECT_CALL(message_handler_, HandleTestContinues(::testing::_));
  ASSERT_TRUE(RunJavascriptTest("testPasses"));
}

// Test that test continues and passes. (Async version).
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestAsyncPasses) {
  ::testing::InSequence s;
  EXPECT_CALL(message_handler_, HandleTestContinues(::testing::_));
  EXPECT_CALL(message_handler_, HandleTestPasses(::testing::_))
      .WillOnce(::testing::InvokeWithoutArgs(
          this, &WebUIBrowserAsyncTest::TestDone));
  ASSERT_TRUE(RunJavascriptAsyncTest(
      "startAsyncTest", new base::StringValue("testPasses")));
}

// Test that two tests pass.
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestAsyncPassPass) {
  ::testing::InSequence s;
  EXPECT_CALL(message_handler_, HandleTestContinues(::testing::_));
  EXPECT_CALL(message_handler_, HandleTestPasses(::testing::_))
      .WillOnce(::testing::InvokeWithoutArgs(
          this, &WebUIBrowserAsyncTest::RunTestPasses));
  EXPECT_CALL(message_handler_, HandleTestContinues(::testing::_));
  EXPECT_CALL(message_handler_, HandleTestPasses(::testing::_))
      .WillOnce(::testing::InvokeWithoutArgs(
          this, &WebUIBrowserAsyncTest::TestDone));
  ASSERT_TRUE(RunJavascriptAsyncTest(
      "startAsyncTest", new base::StringValue("testPasses")));
}

// Test that first test passes; second fails.
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestAsyncPassThenFail) {
  ::testing::InSequence s;
  EXPECT_CALL(message_handler_, HandleTestContinues(::testing::_));
  EXPECT_CALL(message_handler_, HandleTestPasses(::testing::_))
      .WillOnce(::testing::InvokeWithoutArgs(
          this, &WebUIBrowserAsyncTest::RunTestFailsAssert));
  EXPECT_CALL(message_handler_, HandleTestFails(::testing::_));
  ASSERT_FALSE(RunJavascriptAsyncTest(
      "startAsyncTest", new base::StringValue("testPasses")));
}

// Test that testDone() with failure first then sync pass still fails.
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestAsyncDoneFailFirstSyncPass) {
  ::testing::InSequence s;
  EXPECT_CALL(message_handler_, HandleTestContinues(::testing::_));
  EXPECT_CALL(message_handler_, HandleTestFails(::testing::_));

  // Call runAsync directly instead of deferring through startAsyncTest. It will
  // call testDone() on failure, then return.
  ASSERT_FALSE(RunJavascriptAsyncTest(
      "runAsync", new base::StringValue("testAsyncDoneFailFirstSyncPass")));
}

// Test that calling testDone during RunJavascriptAsyncTest still completes
// when waiting for async result. This is similar to the previous test, but call
// testDone directly and expect pass result.
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestTestDoneEarlyPassesAsync) {
  ASSERT_TRUE(RunJavascriptAsyncTest("testDone"));
}

// Test that calling testDone during RunJavascriptTest still completes when
// waiting for async result.
IN_PROC_BROWSER_TEST_F(WebUIBrowserAsyncTest, TestTestDoneEarlyPasses) {
  ASSERT_TRUE(RunJavascriptTest("testDone"));
}
