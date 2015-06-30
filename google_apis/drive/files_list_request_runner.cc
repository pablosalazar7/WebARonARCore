// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "google_apis/drive/files_list_request_runner.h"

#include "base/bind.h"
#include "google_apis/drive/drive_api_error_codes.h"
#include "google_apis/drive/drive_api_requests.h"
#include "google_apis/drive/request_sender.h"

namespace google_apis {

FilesListRequestRunner::FilesListRequestRunner(
    RequestSender* request_sender,
    const google_apis::DriveApiUrlGenerator& url_generator)
    : request_sender_(request_sender),
      url_generator_(url_generator),
      weak_ptr_factory_(this) {
}

FilesListRequestRunner::~FilesListRequestRunner() {
}

CancelCallback FilesListRequestRunner::CreateAndStartWithSizeBackoff(
    int max_results,
    const std::string& q,
    const std::string& fields,
    const FileListCallback& callback) {
  base::Closure* const cancel_callback = new base::Closure;
  drive::FilesListRequest* const request = new drive::FilesListRequest(
      request_sender_, url_generator_,
      base::Bind(&FilesListRequestRunner::OnCompleted,
                 weak_ptr_factory_.GetWeakPtr(), max_results, q, fields,
                 callback, base::Owned(cancel_callback)));
  request->set_max_results(max_results);
  request->set_q(q);
  request->set_fields(fields);
  *cancel_callback = request_sender_->StartRequestWithAuthRetry(request);

  // The cancellation callback is owned by the completion callback, so it must
  // not be used after |callback| is called.
  return base::Bind(&FilesListRequestRunner::OnCancel,
                    weak_ptr_factory_.GetWeakPtr(),
                    base::Unretained(cancel_callback));
}

void FilesListRequestRunner::OnCancel(base::Closure* cancel_callback) {
  DCHECK(cancel_callback);
  DCHECK(!cancel_callback->is_null());
  cancel_callback->Run();
}

void FilesListRequestRunner::OnCompleted(int max_results,
                                         const std::string& q,
                                         const std::string& fields,
                                         const FileListCallback& callback,
                                         CancelCallback* cancel_callback,
                                         DriveApiErrorCode error,
                                         scoped_ptr<FileList> entry) {
  if (!request_completed_callback_for_testing_.is_null())
    request_completed_callback_for_testing_.Run();

  if (error == google_apis::DRIVE_RESPONSE_TOO_LARGE && max_results > 1) {
    CreateAndStartWithSizeBackoff(max_results / 2, q, fields, callback);
    return;
  }

  callback.Run(error, entry.Pass());
}

void FilesListRequestRunner::SetRequestCompletedCallbackForTesting(
    const base::Closure& callback) {
  request_completed_callback_for_testing_ = callback;
}

}  // namespace google_apis
