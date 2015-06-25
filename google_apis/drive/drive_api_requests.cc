// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/drive/drive_api_requests.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/sparse_histogram.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner_util.h"
#include "base/values.h"
#include "google_apis/drive/request_sender.h"
#include "google_apis/drive/request_util.h"
#include "google_apis/drive/time_util.h"
#include "net/base/url_util.h"
#include "net/http/http_response_headers.h"

namespace google_apis {
namespace drive {
namespace {

// Format of one request in batch uploading request.
const char kBatchUploadRequestFormat[] =
    "%s %s HTTP/1.1\n"
    "Host: %s\n"
    "X-Goog-Upload-Protocol: multipart\n"
    "Content-Type: %s\n"
    "\n";

// Request header for specifying batch upload.
const char kBatchUploadHeader[] = "X-Goog-Upload-Protocol: batch";

// Content type of HTTP request.
const char kHttpContentType[] = "application/http";

// Break line in HTTP message.
const char kHttpBr[] = "\r\n";

// Mime type of multipart mixed.
const char kMultipartMixedMimeTypePrefix[] = "multipart/mixed; boundary=";

// UMA names.
const char kUMADriveBatchUploadResponseCode[] = "Drive.BatchUploadResponseCode";
const char kUMADriveTotalFileCountInBatchUpload[] =
    "Drive.TotalFileCountInBatchUpload";
const char kUMADriveTotalFileSizeInBatchUpload[] =
    "Drive.TotalFileSizeInBatchUpload";

// Parses the JSON value to FileResource instance and runs |callback| on the
// UI thread once parsing is done.
// This is customized version of ParseJsonAndRun defined above to adapt the
// remaining response type.
void ParseFileResourceWithUploadRangeAndRun(const UploadRangeCallback& callback,
                                            const UploadRangeResponse& response,
                                            scoped_ptr<base::Value> value) {
  DCHECK(!callback.is_null());

  scoped_ptr<FileResource> file_resource;
  if (value) {
    file_resource = FileResource::CreateFrom(*value);
    if (!file_resource) {
      callback.Run(
          UploadRangeResponse(DRIVE_PARSE_ERROR,
                              response.start_position_received,
                              response.end_position_received),
          scoped_ptr<FileResource>());
      return;
    }
  }

  callback.Run(response, file_resource.Pass());
}

// Attaches |properties| to the |request_body| if |properties| is not empty.
// |request_body| must not be NULL.
void AttachProperties(const Properties& properties,
                      base::DictionaryValue* request_body) {
  DCHECK(request_body);
  if (properties.empty())
    return;

  base::ListValue* const properties_value = new base::ListValue;
  for (const auto& property : properties) {
    base::DictionaryValue* const property_value = new base::DictionaryValue;
    std::string visibility_as_string;
    switch (property.visibility()) {
      case Property::VISIBILITY_PRIVATE:
        visibility_as_string = "PRIVATE";
        break;
      case Property::VISIBILITY_PUBLIC:
        visibility_as_string = "PUBLIC";
        break;
    }
    property_value->SetString("visibility", visibility_as_string);
    property_value->SetString("key", property.key());
    property_value->SetString("value", property.value());
    properties_value->Append(property_value);
  }
  request_body->Set("properties", properties_value);
}

// Creates metadata JSON string for multipart uploading.
// All the values are optional. If the value is empty or null, the value does
// not appear in the metadata.
std::string CreateMultipartUploadMetadataJson(
    const std::string& title,
    const std::string& parent_resource_id,
    const base::Time& modified_date,
    const base::Time& last_viewed_by_me_date,
    const Properties& properties) {
  base::DictionaryValue root;
  if (!title.empty())
    root.SetString("title", title);

  // Fill parent link.
  if (!parent_resource_id.empty()) {
    scoped_ptr<base::ListValue> parents(new base::ListValue);
    parents->Append(
        google_apis::util::CreateParentValue(parent_resource_id).release());
    root.Set("parents", parents.release());
  }

  if (!modified_date.is_null()) {
    root.SetString("modifiedDate",
                   google_apis::util::FormatTimeAsString(modified_date));
  }

  if (!last_viewed_by_me_date.is_null()) {
    root.SetString("lastViewedByMeDate", google_apis::util::FormatTimeAsString(
                                             last_viewed_by_me_date));
  }

  AttachProperties(properties, &root);
  std::string json_string;
  base::JSONWriter::Write(root, &json_string);
  return json_string;
}

// Splits |string| into lines by |kHttpBr|.
// Each line does not include |kHttpBr|.
void SplitIntoLines(const std::string& string,
                    std::vector<base::StringPiece>* output) {
  const size_t br_size = std::string(kHttpBr).size();
  std::string::const_iterator it = string.begin();
  std::vector<base::StringPiece> lines;
  while (true) {
    const std::string::const_iterator next_pos =
        std::search(it, string.end(), kHttpBr, kHttpBr + br_size);
    lines.push_back(base::StringPiece(it, next_pos));
    if (next_pos == string.end())
      break;
    it = next_pos + br_size;
  }
  output->swap(lines);
}

// Remove transport padding (spaces and tabs at the end of line) from |piece|.
base::StringPiece TrimTransportPadding(const base::StringPiece& piece) {
  size_t trim_size = 0;
  while (trim_size < piece.size() &&
         (piece[piece.size() - 1 - trim_size] == ' ' ||
          piece[piece.size() - 1 - trim_size] == '\t')) {
    ++trim_size;
  }
  return piece.substr(0, piece.size() - trim_size);
}

void EmptyClosure(scoped_ptr<BatchableDelegate>) {
}

}  // namespace

MultipartHttpResponse::MultipartHttpResponse() : code(HTTP_SUCCESS) {
}

MultipartHttpResponse::~MultipartHttpResponse() {
}

// The |response| must be multipart/mixed format that contains child HTTP
// response of drive batch request.
// https://www.ietf.org/rfc/rfc2046.txt
//
// It looks like:
// --Boundary
// Content-type: application/http
//
// HTTP/1.1 200 OK
// Header of child response
//
// Body of child response
// --Boundary
// Content-type: application/http
//
// HTTP/1.1 404 Not Found
// Header of child response
//
// Body of child response
// --Boundary--
bool ParseMultipartResponse(const std::string& content_type,
                            const std::string& response,
                            std::vector<MultipartHttpResponse>* parts) {
  if (response.empty())
    return false;

  base::StringPiece content_type_piece(content_type);
  if (!content_type_piece.starts_with(kMultipartMixedMimeTypePrefix)) {
    return false;
  }
  content_type_piece.remove_prefix(
      base::StringPiece(kMultipartMixedMimeTypePrefix).size());

  if (content_type_piece.empty())
    return false;
  if (content_type_piece[0] == '"') {
    if (content_type_piece.size() <= 2 ||
        content_type_piece[content_type_piece.size() - 1] != '"') {
      return false;
    }
    content_type_piece =
        content_type_piece.substr(1, content_type_piece.size() - 2);
  }

  std::string boundary;
  content_type_piece.CopyToString(&boundary);
  const std::string header = "--" + boundary;
  const std::string terminator = "--" + boundary + "--";

  std::vector<base::StringPiece> lines;
  SplitIntoLines(response, &lines);

  enum {
    STATE_START,
    STATE_PART_HEADER,
    STATE_PART_HTTP_STATUS_LINE,
    STATE_PART_HTTP_HEADER,
    STATE_PART_HTTP_BODY
  } state = STATE_START;

  const std::string kHttpStatusPrefix = "HTTP/1.1 ";
  std::vector<MultipartHttpResponse> responses;
  DriveApiErrorCode code = DRIVE_PARSE_ERROR;
  std::string body;
  for (const auto& line : lines) {
    if (state == STATE_PART_HEADER && line.empty()) {
      state = STATE_PART_HTTP_STATUS_LINE;
      continue;
    }

    if (state == STATE_PART_HTTP_STATUS_LINE) {
      if (line.starts_with(kHttpStatusPrefix)) {
        int int_code;
        base::StringToInt(
            line.substr(base::StringPiece(kHttpStatusPrefix).size()),
            &int_code);
        if (int_code > 0)
          code = static_cast<DriveApiErrorCode>(int_code);
        else
          code = DRIVE_PARSE_ERROR;
      } else {
        code = DRIVE_PARSE_ERROR;
      }
      state = STATE_PART_HTTP_HEADER;
      continue;
    }

    if (state == STATE_PART_HTTP_HEADER && line.empty()) {
      state = STATE_PART_HTTP_BODY;
      body.clear();
      continue;
    }
    const base::StringPiece chopped_line = TrimTransportPadding(line);
    const bool is_new_part = chopped_line == header;
    const bool was_last_part = chopped_line == terminator;
    if (is_new_part || was_last_part) {
      switch (state) {
        case STATE_START:
          break;
        case STATE_PART_HEADER:
        case STATE_PART_HTTP_STATUS_LINE:
          responses.push_back(MultipartHttpResponse());
          responses.back().code = DRIVE_PARSE_ERROR;
          break;
        case STATE_PART_HTTP_HEADER:
          responses.push_back(MultipartHttpResponse());
          responses.back().code = code;
          break;
        case STATE_PART_HTTP_BODY:
          // Drop the last kHttpBr.
          if (!body.empty())
            body.resize(body.size() - 2);
          responses.push_back(MultipartHttpResponse());
          responses.back().code = code;
          responses.back().body.swap(body);
          break;
      }
      if (is_new_part)
        state = STATE_PART_HEADER;
      if (was_last_part)
        break;
    } else if (state == STATE_PART_HTTP_BODY) {
      line.AppendToString(&body);
      body.append(kHttpBr);
    }
  }

  parts->swap(responses);
  return true;
}

Property::Property() : visibility_(VISIBILITY_PRIVATE) {
}

Property::~Property() {
}

//============================ DriveApiPartialFieldRequest ====================

DriveApiPartialFieldRequest::DriveApiPartialFieldRequest(
    RequestSender* sender) : UrlFetchRequestBase(sender) {
}

DriveApiPartialFieldRequest::~DriveApiPartialFieldRequest() {
}

GURL DriveApiPartialFieldRequest::GetURL() const {
  GURL url = GetURLInternal();
  if (!fields_.empty())
    url = net::AppendOrReplaceQueryParameter(url, "fields", fields_);
  return url;
}

//=============================== FilesGetRequest =============================

FilesGetRequest::FilesGetRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    bool use_internal_endpoint,
    const FileResourceCallback& callback)
    : DriveApiDataRequest<FileResource>(sender, callback),
      url_generator_(url_generator),
      use_internal_endpoint_(use_internal_endpoint) {
  DCHECK(!callback.is_null());
}

FilesGetRequest::~FilesGetRequest() {}

GURL FilesGetRequest::GetURLInternal() const {
  return url_generator_.GetFilesGetUrl(file_id_,
                                       use_internal_endpoint_,
                                       embed_origin_);
}

//============================ FilesAuthorizeRequest ===========================

FilesAuthorizeRequest::FilesAuthorizeRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const FileResourceCallback& callback)
    : DriveApiDataRequest<FileResource>(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

FilesAuthorizeRequest::~FilesAuthorizeRequest() {}

net::URLFetcher::RequestType FilesAuthorizeRequest::GetRequestType() const {
  return net::URLFetcher::POST;
}

GURL FilesAuthorizeRequest::GetURLInternal() const {
  return url_generator_.GetFilesAuthorizeUrl(file_id_, app_id_);
}

//============================ FilesInsertRequest ============================

FilesInsertRequest::FilesInsertRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const FileResourceCallback& callback)
    : DriveApiDataRequest<FileResource>(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

FilesInsertRequest::~FilesInsertRequest() {}

net::URLFetcher::RequestType FilesInsertRequest::GetRequestType() const {
  return net::URLFetcher::POST;
}

bool FilesInsertRequest::GetContentData(std::string* upload_content_type,
                                        std::string* upload_content) {
  *upload_content_type = util::kContentTypeApplicationJson;

  base::DictionaryValue root;

  if (!last_viewed_by_me_date_.is_null()) {
    root.SetString("lastViewedByMeDate",
                   util::FormatTimeAsString(last_viewed_by_me_date_));
  }

  if (!mime_type_.empty())
    root.SetString("mimeType", mime_type_);

  if (!modified_date_.is_null())
    root.SetString("modifiedDate", util::FormatTimeAsString(modified_date_));

  if (!parents_.empty()) {
    base::ListValue* parents_value = new base::ListValue;
    for (size_t i = 0; i < parents_.size(); ++i) {
      base::DictionaryValue* parent = new base::DictionaryValue;
      parent->SetString("id", parents_[i]);
      parents_value->Append(parent);
    }
    root.Set("parents", parents_value);
  }

  if (!title_.empty())
    root.SetString("title", title_);

  AttachProperties(properties_, &root);
  base::JSONWriter::Write(root, upload_content);

  DVLOG(1) << "FilesInsert data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

GURL FilesInsertRequest::GetURLInternal() const {
  return url_generator_.GetFilesInsertUrl();
}

//============================== FilesPatchRequest ============================

FilesPatchRequest::FilesPatchRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const FileResourceCallback& callback)
    : DriveApiDataRequest<FileResource>(sender, callback),
      url_generator_(url_generator),
      set_modified_date_(false),
      update_viewed_date_(true) {
  DCHECK(!callback.is_null());
}

FilesPatchRequest::~FilesPatchRequest() {}

net::URLFetcher::RequestType FilesPatchRequest::GetRequestType() const {
  return net::URLFetcher::PATCH;
}

std::vector<std::string> FilesPatchRequest::GetExtraRequestHeaders() const {
  std::vector<std::string> headers;
  headers.push_back(util::kIfMatchAllHeader);
  return headers;
}

GURL FilesPatchRequest::GetURLInternal() const {
  return url_generator_.GetFilesPatchUrl(
      file_id_, set_modified_date_, update_viewed_date_);
}

bool FilesPatchRequest::GetContentData(std::string* upload_content_type,
                                       std::string* upload_content) {
  if (title_.empty() &&
      modified_date_.is_null() &&
      last_viewed_by_me_date_.is_null() &&
      parents_.empty())
    return false;

  *upload_content_type = util::kContentTypeApplicationJson;

  base::DictionaryValue root;
  if (!title_.empty())
    root.SetString("title", title_);

  if (!modified_date_.is_null())
    root.SetString("modifiedDate", util::FormatTimeAsString(modified_date_));

  if (!last_viewed_by_me_date_.is_null()) {
    root.SetString("lastViewedByMeDate",
                   util::FormatTimeAsString(last_viewed_by_me_date_));
  }

  if (!parents_.empty()) {
    base::ListValue* parents_value = new base::ListValue;
    for (size_t i = 0; i < parents_.size(); ++i) {
      base::DictionaryValue* parent = new base::DictionaryValue;
      parent->SetString("id", parents_[i]);
      parents_value->Append(parent);
    }
    root.Set("parents", parents_value);
  }

  AttachProperties(properties_, &root);
  base::JSONWriter::Write(root, upload_content);

  DVLOG(1) << "FilesPatch data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//============================= FilesCopyRequest ==============================

FilesCopyRequest::FilesCopyRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const FileResourceCallback& callback)
    : DriveApiDataRequest<FileResource>(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

FilesCopyRequest::~FilesCopyRequest() {
}

net::URLFetcher::RequestType FilesCopyRequest::GetRequestType() const {
  return net::URLFetcher::POST;
}

GURL FilesCopyRequest::GetURLInternal() const {
  return url_generator_.GetFilesCopyUrl(file_id_);
}

bool FilesCopyRequest::GetContentData(std::string* upload_content_type,
                                      std::string* upload_content) {
  if (parents_.empty() && title_.empty())
    return false;

  *upload_content_type = util::kContentTypeApplicationJson;

  base::DictionaryValue root;

  if (!modified_date_.is_null())
    root.SetString("modifiedDate", util::FormatTimeAsString(modified_date_));

  if (!parents_.empty()) {
    base::ListValue* parents_value = new base::ListValue;
    for (size_t i = 0; i < parents_.size(); ++i) {
      base::DictionaryValue* parent = new base::DictionaryValue;
      parent->SetString("id", parents_[i]);
      parents_value->Append(parent);
    }
    root.Set("parents", parents_value);
  }

  if (!title_.empty())
    root.SetString("title", title_);

  base::JSONWriter::Write(root, upload_content);
  DVLOG(1) << "FilesCopy data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//============================= FilesListRequest =============================

FilesListRequest::FilesListRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const FileListCallback& callback)
    : DriveApiDataRequest<FileList>(sender, callback),
      url_generator_(url_generator),
      max_results_(100) {
  DCHECK(!callback.is_null());
}

FilesListRequest::~FilesListRequest() {}

GURL FilesListRequest::GetURLInternal() const {
  return url_generator_.GetFilesListUrl(max_results_, page_token_, q_);
}

//======================== FilesListNextPageRequest =========================

FilesListNextPageRequest::FilesListNextPageRequest(
    RequestSender* sender,
    const FileListCallback& callback)
    : DriveApiDataRequest<FileList>(sender, callback) {
  DCHECK(!callback.is_null());
}

FilesListNextPageRequest::~FilesListNextPageRequest() {
}

GURL FilesListNextPageRequest::GetURLInternal() const {
  return next_link_;
}

//============================ FilesDeleteRequest =============================

FilesDeleteRequest::FilesDeleteRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const EntryActionCallback& callback)
    : EntryActionRequest(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

FilesDeleteRequest::~FilesDeleteRequest() {}

net::URLFetcher::RequestType FilesDeleteRequest::GetRequestType() const {
  return net::URLFetcher::DELETE_REQUEST;
}

GURL FilesDeleteRequest::GetURL() const {
  return url_generator_.GetFilesDeleteUrl(file_id_);
}

std::vector<std::string> FilesDeleteRequest::GetExtraRequestHeaders() const {
  std::vector<std::string> headers(
      EntryActionRequest::GetExtraRequestHeaders());
  headers.push_back(util::GenerateIfMatchHeader(etag_));
  return headers;
}

//============================ FilesTrashRequest =============================

FilesTrashRequest::FilesTrashRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const FileResourceCallback& callback)
    : DriveApiDataRequest<FileResource>(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

FilesTrashRequest::~FilesTrashRequest() {}

net::URLFetcher::RequestType FilesTrashRequest::GetRequestType() const {
  return net::URLFetcher::POST;
}

GURL FilesTrashRequest::GetURLInternal() const {
  return url_generator_.GetFilesTrashUrl(file_id_);
}

//============================== AboutGetRequest =============================

AboutGetRequest::AboutGetRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const AboutResourceCallback& callback)
    : DriveApiDataRequest<AboutResource>(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

AboutGetRequest::~AboutGetRequest() {}

GURL AboutGetRequest::GetURLInternal() const {
  return url_generator_.GetAboutGetUrl();
}

//============================ ChangesListRequest ===========================

ChangesListRequest::ChangesListRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const ChangeListCallback& callback)
    : DriveApiDataRequest<ChangeList>(sender, callback),
      url_generator_(url_generator),
      include_deleted_(true),
      max_results_(100),
      start_change_id_(0) {
  DCHECK(!callback.is_null());
}

ChangesListRequest::~ChangesListRequest() {}

GURL ChangesListRequest::GetURLInternal() const {
  return url_generator_.GetChangesListUrl(
      include_deleted_, max_results_, page_token_, start_change_id_);
}

//======================== ChangesListNextPageRequest =========================

ChangesListNextPageRequest::ChangesListNextPageRequest(
    RequestSender* sender,
    const ChangeListCallback& callback)
    : DriveApiDataRequest<ChangeList>(sender, callback) {
  DCHECK(!callback.is_null());
}

ChangesListNextPageRequest::~ChangesListNextPageRequest() {
}

GURL ChangesListNextPageRequest::GetURLInternal() const {
  return next_link_;
}

//============================== AppsListRequest ===========================

AppsListRequest::AppsListRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    bool use_internal_endpoint,
    const AppListCallback& callback)
    : DriveApiDataRequest<AppList>(sender, callback),
      url_generator_(url_generator),
      use_internal_endpoint_(use_internal_endpoint) {
  DCHECK(!callback.is_null());
}

AppsListRequest::~AppsListRequest() {}

GURL AppsListRequest::GetURLInternal() const {
  return url_generator_.GetAppsListUrl(use_internal_endpoint_);
}

//============================== AppsDeleteRequest ===========================

AppsDeleteRequest::AppsDeleteRequest(RequestSender* sender,
                                     const DriveApiUrlGenerator& url_generator,
                                     const EntryActionCallback& callback)
    : EntryActionRequest(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

AppsDeleteRequest::~AppsDeleteRequest() {}

net::URLFetcher::RequestType AppsDeleteRequest::GetRequestType() const {
  return net::URLFetcher::DELETE_REQUEST;
}

GURL AppsDeleteRequest::GetURL() const {
  return url_generator_.GetAppsDeleteUrl(app_id_);
}

//========================== ChildrenInsertRequest ============================

ChildrenInsertRequest::ChildrenInsertRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const EntryActionCallback& callback)
    : EntryActionRequest(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

ChildrenInsertRequest::~ChildrenInsertRequest() {}

net::URLFetcher::RequestType ChildrenInsertRequest::GetRequestType() const {
  return net::URLFetcher::POST;
}

GURL ChildrenInsertRequest::GetURL() const {
  return url_generator_.GetChildrenInsertUrl(folder_id_);
}

bool ChildrenInsertRequest::GetContentData(std::string* upload_content_type,
                                           std::string* upload_content) {
  *upload_content_type = util::kContentTypeApplicationJson;

  base::DictionaryValue root;
  root.SetString("id", id_);

  base::JSONWriter::Write(root, upload_content);
  DVLOG(1) << "InsertResource data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//========================== ChildrenDeleteRequest ============================

ChildrenDeleteRequest::ChildrenDeleteRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const EntryActionCallback& callback)
    : EntryActionRequest(sender, callback),
      url_generator_(url_generator) {
  DCHECK(!callback.is_null());
}

ChildrenDeleteRequest::~ChildrenDeleteRequest() {}

net::URLFetcher::RequestType ChildrenDeleteRequest::GetRequestType() const {
  return net::URLFetcher::DELETE_REQUEST;
}

GURL ChildrenDeleteRequest::GetURL() const {
  return url_generator_.GetChildrenDeleteUrl(child_id_, folder_id_);
}

//======================= InitiateUploadNewFileRequest =======================

InitiateUploadNewFileRequest::InitiateUploadNewFileRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const std::string& content_type,
    int64 content_length,
    const std::string& parent_resource_id,
    const std::string& title,
    const InitiateUploadCallback& callback)
    : InitiateUploadRequestBase(sender,
                                callback,
                                content_type,
                                content_length),
      url_generator_(url_generator),
      parent_resource_id_(parent_resource_id),
      title_(title) {
}

InitiateUploadNewFileRequest::~InitiateUploadNewFileRequest() {}

GURL InitiateUploadNewFileRequest::GetURL() const {
  return url_generator_.GetInitiateUploadNewFileUrl(!modified_date_.is_null());
}

net::URLFetcher::RequestType
InitiateUploadNewFileRequest::GetRequestType() const {
  return net::URLFetcher::POST;
}

bool InitiateUploadNewFileRequest::GetContentData(
    std::string* upload_content_type,
    std::string* upload_content) {
  *upload_content_type = util::kContentTypeApplicationJson;

  base::DictionaryValue root;
  root.SetString("title", title_);

  // Fill parent link.
  scoped_ptr<base::ListValue> parents(new base::ListValue);
  parents->Append(util::CreateParentValue(parent_resource_id_).release());
  root.Set("parents", parents.release());

  if (!modified_date_.is_null())
    root.SetString("modifiedDate", util::FormatTimeAsString(modified_date_));

  if (!last_viewed_by_me_date_.is_null()) {
    root.SetString("lastViewedByMeDate",
                   util::FormatTimeAsString(last_viewed_by_me_date_));
  }

  AttachProperties(properties_, &root);
  base::JSONWriter::Write(root, upload_content);

  DVLOG(1) << "InitiateUploadNewFile data: " << *upload_content_type << ", ["
           << *upload_content << "]";
  return true;
}

//===================== InitiateUploadExistingFileRequest ====================

InitiateUploadExistingFileRequest::InitiateUploadExistingFileRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const std::string& content_type,
    int64 content_length,
    const std::string& resource_id,
    const std::string& etag,
    const InitiateUploadCallback& callback)
    : InitiateUploadRequestBase(sender,
                                callback,
                                content_type,
                                content_length),
      url_generator_(url_generator),
      resource_id_(resource_id),
      etag_(etag) {
}

InitiateUploadExistingFileRequest::~InitiateUploadExistingFileRequest() {}

GURL InitiateUploadExistingFileRequest::GetURL() const {
  return url_generator_.GetInitiateUploadExistingFileUrl(
      resource_id_, !modified_date_.is_null());
}

net::URLFetcher::RequestType
InitiateUploadExistingFileRequest::GetRequestType() const {
  return net::URLFetcher::PUT;
}

std::vector<std::string>
InitiateUploadExistingFileRequest::GetExtraRequestHeaders() const {
  std::vector<std::string> headers(
      InitiateUploadRequestBase::GetExtraRequestHeaders());
  headers.push_back(util::GenerateIfMatchHeader(etag_));
  return headers;
}

bool InitiateUploadExistingFileRequest::GetContentData(
    std::string* upload_content_type,
    std::string* upload_content) {
  base::DictionaryValue root;
  if (!parent_resource_id_.empty()) {
    scoped_ptr<base::ListValue> parents(new base::ListValue);
    parents->Append(util::CreateParentValue(parent_resource_id_).release());
    root.Set("parents", parents.release());
  }

  if (!title_.empty())
    root.SetString("title", title_);

  if (!modified_date_.is_null())
    root.SetString("modifiedDate", util::FormatTimeAsString(modified_date_));

  if (!last_viewed_by_me_date_.is_null()) {
    root.SetString("lastViewedByMeDate",
                   util::FormatTimeAsString(last_viewed_by_me_date_));
  }

  AttachProperties(properties_, &root);
  if (root.empty())
    return false;

  *upload_content_type = util::kContentTypeApplicationJson;
  base::JSONWriter::Write(root, upload_content);
  DVLOG(1) << "InitiateUploadExistingFile data: " << *upload_content_type
           << ", [" << *upload_content << "]";
  return true;
}

//============================ ResumeUploadRequest ===========================

ResumeUploadRequest::ResumeUploadRequest(
    RequestSender* sender,
    const GURL& upload_location,
    int64 start_position,
    int64 end_position,
    int64 content_length,
    const std::string& content_type,
    const base::FilePath& local_file_path,
    const UploadRangeCallback& callback,
    const ProgressCallback& progress_callback)
    : ResumeUploadRequestBase(sender,
                              upload_location,
                              start_position,
                              end_position,
                              content_length,
                              content_type,
                              local_file_path),
      callback_(callback),
      progress_callback_(progress_callback) {
  DCHECK(!callback_.is_null());
}

ResumeUploadRequest::~ResumeUploadRequest() {}

void ResumeUploadRequest::OnRangeRequestComplete(
    const UploadRangeResponse& response,
    scoped_ptr<base::Value> value) {
  DCHECK(CalledOnValidThread());
  ParseFileResourceWithUploadRangeAndRun(callback_, response, value.Pass());
}

void ResumeUploadRequest::OnURLFetchUploadProgress(
    const net::URLFetcher* source, int64 current, int64 total) {
  if (!progress_callback_.is_null())
    progress_callback_.Run(current, total);
}

//========================== GetUploadStatusRequest ==========================

GetUploadStatusRequest::GetUploadStatusRequest(
    RequestSender* sender,
    const GURL& upload_url,
    int64 content_length,
    const UploadRangeCallback& callback)
    : GetUploadStatusRequestBase(sender,
                                 upload_url,
                                 content_length),
      callback_(callback) {
  DCHECK(!callback.is_null());
}

GetUploadStatusRequest::~GetUploadStatusRequest() {}

void GetUploadStatusRequest::OnRangeRequestComplete(
    const UploadRangeResponse& response,
    scoped_ptr<base::Value> value) {
  DCHECK(CalledOnValidThread());
  ParseFileResourceWithUploadRangeAndRun(callback_, response, value.Pass());
}

//======================= MultipartUploadNewFileDelegate =======================

MultipartUploadNewFileDelegate::MultipartUploadNewFileDelegate(
    base::SequencedTaskRunner* task_runner,
    const std::string& title,
    const std::string& parent_resource_id,
    const std::string& content_type,
    int64 content_length,
    const base::Time& modified_date,
    const base::Time& last_viewed_by_me_date,
    const base::FilePath& local_file_path,
    const Properties& properties,
    const DriveApiUrlGenerator& url_generator,
    const FileResourceCallback& callback,
    const ProgressCallback& progress_callback)
    : MultipartUploadRequestBase(
          task_runner,
          CreateMultipartUploadMetadataJson(title,
                                            parent_resource_id,
                                            modified_date,
                                            last_viewed_by_me_date,
                                            properties),
          content_type,
          content_length,
          local_file_path,
          callback,
          progress_callback),
      has_modified_date_(!modified_date.is_null()),
      url_generator_(url_generator) {
}

MultipartUploadNewFileDelegate::~MultipartUploadNewFileDelegate() {
}

GURL MultipartUploadNewFileDelegate::GetURL() const {
  return url_generator_.GetMultipartUploadNewFileUrl(has_modified_date_);
}

net::URLFetcher::RequestType MultipartUploadNewFileDelegate::GetRequestType()
    const {
  return net::URLFetcher::POST;
}

//====================== MultipartUploadExistingFileDelegate ===================

MultipartUploadExistingFileDelegate::MultipartUploadExistingFileDelegate(
    base::SequencedTaskRunner* task_runner,
    const std::string& title,
    const std::string& resource_id,
    const std::string& parent_resource_id,
    const std::string& content_type,
    int64 content_length,
    const base::Time& modified_date,
    const base::Time& last_viewed_by_me_date,
    const base::FilePath& local_file_path,
    const std::string& etag,
    const Properties& properties,
    const DriveApiUrlGenerator& url_generator,
    const FileResourceCallback& callback,
    const ProgressCallback& progress_callback)
    : MultipartUploadRequestBase(
          task_runner,
          CreateMultipartUploadMetadataJson(title,
                                            parent_resource_id,
                                            modified_date,
                                            last_viewed_by_me_date,
                                            properties),
          content_type,
          content_length,
          local_file_path,
          callback,
          progress_callback),
      resource_id_(resource_id),
      etag_(etag),
      has_modified_date_(!modified_date.is_null()),
      url_generator_(url_generator) {
}

MultipartUploadExistingFileDelegate::~MultipartUploadExistingFileDelegate() {
}

std::vector<std::string>
MultipartUploadExistingFileDelegate::GetExtraRequestHeaders() const {
  std::vector<std::string> headers(
      MultipartUploadRequestBase::GetExtraRequestHeaders());
  headers.push_back(util::GenerateIfMatchHeader(etag_));
  return headers;
}

GURL MultipartUploadExistingFileDelegate::GetURL() const {
  return url_generator_.GetMultipartUploadExistingFileUrl(resource_id_,
                                                          has_modified_date_);
}

net::URLFetcher::RequestType
MultipartUploadExistingFileDelegate::GetRequestType() const {
  return net::URLFetcher::PUT;
}

//========================== DownloadFileRequest ==========================

DownloadFileRequest::DownloadFileRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const std::string& resource_id,
    const base::FilePath& output_file_path,
    const DownloadActionCallback& download_action_callback,
    const GetContentCallback& get_content_callback,
    const ProgressCallback& progress_callback)
    : DownloadFileRequestBase(
          sender,
          download_action_callback,
          get_content_callback,
          progress_callback,
          url_generator.GenerateDownloadFileUrl(resource_id),
          output_file_path) {
}

DownloadFileRequest::~DownloadFileRequest() {
}

//========================== PermissionsInsertRequest ==========================

PermissionsInsertRequest::PermissionsInsertRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator,
    const EntryActionCallback& callback)
    : EntryActionRequest(sender, callback),
      url_generator_(url_generator),
      type_(PERMISSION_TYPE_USER),
      role_(PERMISSION_ROLE_READER) {
}

PermissionsInsertRequest::~PermissionsInsertRequest() {
}

GURL PermissionsInsertRequest::GetURL() const {
  return url_generator_.GetPermissionsInsertUrl(id_);
}

net::URLFetcher::RequestType
PermissionsInsertRequest::GetRequestType() const {
  return net::URLFetcher::POST;
}

bool PermissionsInsertRequest::GetContentData(std::string* upload_content_type,
                                              std::string* upload_content) {
  *upload_content_type = util::kContentTypeApplicationJson;

  base::DictionaryValue root;
  switch (type_) {
    case PERMISSION_TYPE_ANYONE:
      root.SetString("type", "anyone");
      break;
    case PERMISSION_TYPE_DOMAIN:
      root.SetString("type", "domain");
      break;
    case PERMISSION_TYPE_GROUP:
      root.SetString("type", "group");
      break;
    case PERMISSION_TYPE_USER:
      root.SetString("type", "user");
      break;
  }
  switch (role_) {
    case PERMISSION_ROLE_OWNER:
      root.SetString("role", "owner");
      break;
    case PERMISSION_ROLE_READER:
      root.SetString("role", "reader");
      break;
    case PERMISSION_ROLE_WRITER:
      root.SetString("role", "writer");
      break;
    case PERMISSION_ROLE_COMMENTER:
      root.SetString("role", "reader");
      {
        base::ListValue* list = new base::ListValue;
        list->AppendString("commenter");
        root.Set("additionalRoles", list);
      }
      break;
  }
  root.SetString("value", value_);
  base::JSONWriter::Write(root, upload_content);
  return true;
}

//======================= SingleBatchableDelegateRequest =======================

SingleBatchableDelegateRequest::SingleBatchableDelegateRequest(
    RequestSender* sender,
    BatchableDelegate* delegate)
    : UrlFetchRequestBase(sender),
      delegate_(delegate),
      weak_ptr_factory_(this) {
}

SingleBatchableDelegateRequest::~SingleBatchableDelegateRequest() {
}

GURL SingleBatchableDelegateRequest::GetURL() const {
  return delegate_->GetURL();
}

net::URLFetcher::RequestType SingleBatchableDelegateRequest::GetRequestType()
    const {
  return delegate_->GetRequestType();
}

std::vector<std::string>
SingleBatchableDelegateRequest::GetExtraRequestHeaders() const {
  return delegate_->GetExtraRequestHeaders();
}

void SingleBatchableDelegateRequest::Prepare(const PrepareCallback& callback) {
  delegate_->Prepare(callback);
}

bool SingleBatchableDelegateRequest::GetContentData(
    std::string* upload_content_type,
    std::string* upload_content) {
  return delegate_->GetContentData(upload_content_type, upload_content);
}

void SingleBatchableDelegateRequest::ProcessURLFetchResults(
    const net::URLFetcher* source) {
  delegate_->NotifyResult(
      GetErrorCode(), response_writer()->data(),
      base::Bind(
          &SingleBatchableDelegateRequest::OnProcessURLFetchResultsComplete,
          weak_ptr_factory_.GetWeakPtr()));
}

void SingleBatchableDelegateRequest::RunCallbackOnPrematureFailure(
    DriveApiErrorCode code) {
  delegate_->NotifyError(code);
}

void SingleBatchableDelegateRequest::OnURLFetchUploadProgress(
    const net::URLFetcher* source,
    int64 current,
    int64 total) {
  delegate_->NotifyUploadProgress(source, current, total);
}

//========================== BatchUploadRequest ==========================

BatchUploadChildEntry::BatchUploadChildEntry(BatchableDelegate* request)
    : request(request), prepared(false), data_offset(0), data_size(0) {
}

BatchUploadChildEntry::~BatchUploadChildEntry() {
}

BatchUploadRequest::BatchUploadRequest(
    RequestSender* sender,
    const DriveApiUrlGenerator& url_generator)
    : UrlFetchRequestBase(sender),
      sender_(sender),
      url_generator_(url_generator),
      committed_(false),
      last_progress_value_(0),
      weak_ptr_factory_(this) {
}

BatchUploadRequest::~BatchUploadRequest() {
}

void BatchUploadRequest::SetBoundaryForTesting(const std::string& boundary) {
  boundary_ = boundary;
}

void BatchUploadRequest::AddRequest(BatchableDelegate* request) {
  DCHECK(CalledOnValidThread());
  DCHECK(request);
  DCHECK(GetChildEntry(request) == child_requests_.end());
  DCHECK(!committed_);
  child_requests_.push_back(new BatchUploadChildEntry(request));
  request->Prepare(base::Bind(&BatchUploadRequest::OnChildRequestPrepared,
                              weak_ptr_factory_.GetWeakPtr(), request));
}

void BatchUploadRequest::OnChildRequestPrepared(RequestID request_id,
                                                DriveApiErrorCode result) {
  DCHECK(CalledOnValidThread());
  auto const child = GetChildEntry(request_id);
  DCHECK(child != child_requests_.end());
  if (IsSuccessfulDriveApiErrorCode(result)) {
    (*child)->prepared = true;
  } else {
    (*child)->request->NotifyError(result);
    child_requests_.erase(child);
  }
  MayCompletePrepare();
}

void BatchUploadRequest::Commit() {
  DCHECK(CalledOnValidThread());
  DCHECK(!committed_);
  if (child_requests_.empty()) {
    Cancel();
  } else {
    committed_ = true;
    MayCompletePrepare();
  }
}

void BatchUploadRequest::Prepare(const PrepareCallback& callback) {
  DCHECK(CalledOnValidThread());
  DCHECK(!callback.is_null());
  prepare_callback_ = callback;
  MayCompletePrepare();
}

void BatchUploadRequest::Cancel() {
  child_requests_.clear();
  UrlFetchRequestBase::Cancel();
}

// Obtains corresponding child entry of |request_id|. Returns NULL if the
// entry is not found.
ScopedVector<BatchUploadChildEntry>::iterator BatchUploadRequest::GetChildEntry(
    RequestID request_id) {
  for (auto it = child_requests_.begin(); it != child_requests_.end(); ++it) {
    if ((*it)->request.get() == request_id)
      return it;
  }
  return child_requests_.end();
}

void BatchUploadRequest::MayCompletePrepare() {
  if (!committed_ || prepare_callback_.is_null())
    return;
  for (const auto& child : child_requests_) {
    if (!child->prepared)
      return;
  }

  // Build multipart body here.
  int64 total_size = 0;
  std::vector<ContentTypeAndData> parts;
  for (auto& child : child_requests_) {
    std::string type;
    std::string data;
    const bool result = child->request->GetContentData(&type, &data);
    // Upload request must have content data.
    DCHECK(result);

    const GURL url = child->request->GetURL();
    std::string method;
    switch (child->request->GetRequestType()) {
      case net::URLFetcher::POST:
        method = "POST";
        break;
      case net::URLFetcher::PUT:
        method = "PUT";
        break;
      default:
        NOTREACHED();
        break;
    }
    const std::string header = base::StringPrintf(
        kBatchUploadRequestFormat, method.c_str(), url.path().c_str(),
        url_generator_.GetBatchUploadUrl().host().c_str(), type.c_str());

    child->data_offset = header.size();
    child->data_size = data.size();
    total_size += data.size();

    parts.push_back(ContentTypeAndData());
    parts.back().type = kHttpContentType;
    parts.back().data = header;
    parts.back().data.append(data);
  }

  UMA_HISTOGRAM_COUNTS_100(kUMADriveTotalFileCountInBatchUpload, parts.size());
  UMA_HISTOGRAM_MEMORY_KB(kUMADriveTotalFileSizeInBatchUpload,
                          total_size / 1024);

  std::vector<uint64> part_data_offset;
  GenerateMultipartBody(MULTIPART_MIXED, boundary_, parts, &upload_content_,
                        &part_data_offset);
  DCHECK(part_data_offset.size() == child_requests_.size());
  for (size_t i = 0; i < child_requests_.size(); ++i) {
    child_requests_[i]->data_offset += part_data_offset[i];
  }
  prepare_callback_.Run(HTTP_SUCCESS);
}

bool BatchUploadRequest::GetContentData(std::string* upload_content_type,
                                        std::string* upload_content_data) {
  upload_content_type->assign(upload_content_.type);
  upload_content_data->assign(upload_content_.data);
  return true;
}

base::WeakPtr<BatchUploadRequest>
BatchUploadRequest::GetWeakPtrAsBatchUploadRequest() {
  return weak_ptr_factory_.GetWeakPtr();
}

GURL BatchUploadRequest::GetURL() const {
  return url_generator_.GetBatchUploadUrl();
}

net::URLFetcher::RequestType BatchUploadRequest::GetRequestType() const {
  return net::URLFetcher::PUT;
}

std::vector<std::string> BatchUploadRequest::GetExtraRequestHeaders() const {
  std::vector<std::string> headers;
  headers.push_back(kBatchUploadHeader);
  return headers;
}

void BatchUploadRequest::ProcessURLFetchResults(const net::URLFetcher* source) {
  UMA_HISTOGRAM_SPARSE_SLOWLY(kUMADriveBatchUploadResponseCode, GetErrorCode());

  if (!IsSuccessfulDriveApiErrorCode(GetErrorCode())) {
    RunCallbackOnPrematureFailure(GetErrorCode());
    sender_->RequestFinished(this);
    return;
  }

  std::string content_type;
  source->GetResponseHeaders()->EnumerateHeader(
      /* need only first header */ NULL, "Content-Type", &content_type);

  std::vector<MultipartHttpResponse> parts;
  if (!ParseMultipartResponse(content_type, response_writer()->data(),
                              &parts) ||
      child_requests_.size() != parts.size()) {
    RunCallbackOnPrematureFailure(DRIVE_PARSE_ERROR);
    sender_->RequestFinished(this);
    return;
  }

  for (size_t i = 0; i < parts.size(); ++i) {
    BatchableDelegate* const delegate = child_requests_[i]->request.get();
    // Pass onwership of |delegate| so that child_requests_.clear() won't
    // kill the delegate. It has to be deleted after the notification.
    delegate->NotifyResult(
        parts[i].code, parts[i].body,
        base::Bind(&EmptyClosure, base::Passed(&child_requests_[i]->request)));
  }
  child_requests_.clear();

  sender_->RequestFinished(this);
}

void BatchUploadRequest::RunCallbackOnPrematureFailure(DriveApiErrorCode code) {
  for (auto child : child_requests_) {
    child->request->NotifyError(code);
  }
  child_requests_.clear();
}

void BatchUploadRequest::OnURLFetchUploadProgress(const net::URLFetcher* source,
                                                  int64 current,
                                                  int64 total) {
  for (auto child : child_requests_) {
    if (child->data_offset <= current &&
        current <= child->data_offset + child->data_size) {
      child->request->NotifyUploadProgress(source, current - child->data_offset,
                                           child->data_size);
    } else if (last_progress_value_ < child->data_offset + child->data_size &&
               child->data_offset + child->data_size < current) {
      child->request->NotifyUploadProgress(source, child->data_size,
                                           child->data_size);
    }
  }
  last_progress_value_ = current;
}
}  // namespace drive
}  // namespace google_apis
