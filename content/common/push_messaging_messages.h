// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// IPC messages for push messaging.
// Multiply-included message file, hence no include guard.

#include "content/public/common/push_messaging_status.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/WebKit/public/platform/WebPushPermissionStatus.h"
#include "url/gurl.h"

#define IPC_MESSAGE_START PushMessagingMsgStart

IPC_ENUM_TRAITS_MAX_VALUE(content::PushRegistrationStatus,
                          content::PUSH_REGISTRATION_STATUS_LAST)

IPC_ENUM_TRAITS_MAX_VALUE(
    blink::WebPushPermissionStatus,
    blink::WebPushPermissionStatus::WebPushPermissionStatusLast)

// Messages sent from the browser to the child process.

IPC_MESSAGE_ROUTED3(PushMessagingMsg_RegisterFromDocumentSuccess,
                    int32 /* request_id */,
                    GURL /* push_endpoint */,
                    std::string /* push_registration_id */)

IPC_MESSAGE_CONTROL3(PushMessagingMsg_RegisterFromWorkerSuccess,
                     int32 /* request_id */,
                     GURL /* push_endpoint */,
                     std::string /* push_registration_id */)

IPC_MESSAGE_ROUTED2(PushMessagingMsg_RegisterFromDocumentError,
                    int32 /* request_id */,
                    content::PushRegistrationStatus /* status */)

IPC_MESSAGE_CONTROL2(PushMessagingMsg_RegisterFromWorkerError,
                     int32 /* request_id */,
                     content::PushRegistrationStatus /* status */)

// TODO(mvanouwerkerk): Delete this after switching to the platform code path.
IPC_MESSAGE_ROUTED2(PushMessagingMsg_PermissionStatusResult,
                    int32 /* callback_id */,
                    blink::WebPushPermissionStatus /* status */)

IPC_MESSAGE_CONTROL2(PushMessagingMsg_GetPermissionStatusSuccess,
                     int32 /* request_id */,
                     blink::WebPushPermissionStatus /* status */)

// TODO(mvanouwerkerk): Delete this after switching to the platform code path.
IPC_MESSAGE_ROUTED1(PushMessagingMsg_PermissionStatusFailure,
                    int32 /* callback_id */)

IPC_MESSAGE_CONTROL1(PushMessagingMsg_GetPermissionStatusError,
                     int32 /* request_id */)

// Messages sent from the child process to the browser.

IPC_MESSAGE_CONTROL5(PushMessagingHostMsg_RegisterFromDocument,
                     int32 /* render_frame_id */,
                     int32 /* request_id */,
                     std::string /* sender_id */,
                     bool /* user_gesture */,
                     int32 /* service_worker_provider_id */)

IPC_MESSAGE_CONTROL2(PushMessagingHostMsg_RegisterFromWorker,
                     int32 /* request_id */,
                     int64 /* service_worker_registration_id */)

// TODO(mvanouwerkerk): Delete this after switching to the platform code path.
IPC_MESSAGE_CONTROL3(PushMessagingHostMsg_PermissionStatus,
                     int32 /* render_frame_id */,
                     int32 /* service_worker_provider_id */,
                     int32 /* permission_callback_id */)

IPC_MESSAGE_CONTROL2(PushMessagingHostMsg_GetPermissionStatus,
                     int32 /* request_id */,
                     int64 /* service_worker_registration_id */)
