// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_MESSAGE_FILTER_H_
#define CONTENT_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_MESSAGE_FILTER_H_

#include <string>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/browser_message_filter.h"
#include "content/public/common/push_messaging_status.h"
#include "url/gurl.h"

class GURL;

namespace content {

class PushMessagingService;
class ServiceWorkerContextWrapper;

class PushMessagingMessageFilter : public BrowserMessageFilter {
 public:
  PushMessagingMessageFilter(
      int render_process_id,
      ServiceWorkerContextWrapper* service_worker_context);

 private:
  ~PushMessagingMessageFilter() override;

  // BrowserMessageFilter implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

  void OnRegisterFromDocument(int render_frame_id,
                              int request_id,
                              const std::string& sender_id,
                              bool user_gesture,
                              int service_worker_provider_id);

  void OnRegisterFromWorker(int request_id,
                            int64 service_worker_registration_id);

  // TODO(mvanouwerkerk): Delete once the Push API flows through platform.
  // https://crbug.com/389194
  void OnPermissionStatusRequest(int render_frame_id,
                                 int service_worker_provider_id,
                                 int permission_callback_id);

  void OnGetPermissionStatus(int request_id,
                             int64 service_worker_registration_id);

  void DoRegisterFromDocument(int render_frame_id,
                              int request_id,
                              const std::string& sender_id,
                              bool user_gesture,
                              const GURL& requesting_origin,
                              int64 service_worker_registration_id);

  // TODO(mvanouwerkerk): Delete once the Push API flows through platform.
  // https://crbug.com/389194
  void DoPermissionStatusRequest(const GURL& requesting_origin,
                                 int render_frame_id,
                                 int callback_id);

  void DidRegisterFromDocument(int render_frame_id,
                               int request_id,
                               const std::string& push_registration_id,
                               PushRegistrationStatus status);

  PushMessagingService* service();

  int render_process_id_;
  scoped_refptr<ServiceWorkerContextWrapper> service_worker_context_;

  // Owned by the content embedder's browsing context.
  PushMessagingService* service_;

  base::WeakPtrFactory<PushMessagingMessageFilter> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PushMessagingMessageFilter);
};

}  // namespace content

#endif  // CONTENT_BROWSER_PUSH_MESSAGING_PUSH_MESSAGING_MESSAGE_FILTER_H_
