// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_MEDIA_STREAM_DEVICE_PERMISSION_CONTEXT_H_
#define CHROME_BROWSER_MEDIA_MEDIA_STREAM_DEVICE_PERMISSION_CONTEXT_H_

#include "base/macros.h"
#include "chrome/browser/permissions/permission_context_base.h"
#include "components/content_settings/core/common/content_settings_types.h"

// Common class which handles the mic and camera permissions.
// MediaStreamMicPermissionContextFactory and
// MediaStreamCameraPermissionContextFactory will instantiate this with
// corresponding |permission_type|.
class MediaStreamDevicePermissionContext : public PermissionContextBase {
 public:
  MediaStreamDevicePermissionContext(
      Profile* profile,
      const ContentSettingsType content_settings_type);
  ~MediaStreamDevicePermissionContext() override;

  // PermissionContextBase:
  void RequestPermission(content::WebContents* web_contents,
                         const PermissionRequestID& id,
                         const GURL& requesting_frame,
                         bool user_gesture,
                         const BrowserPermissionCallback& callback) override;

  // TODO(xhwang): GURL.GetOrigin() shouldn't be used as the origin. Need to
  // refactor to use url::Origin. crbug.com/527149 is filed for this.
  ContentSetting GetPermissionStatus(
      const GURL& requesting_origin,
      const GURL& embedding_origin) const override;

  // For Pepper request, insecure origins are allowed to be persisted, hence we
  // can't just call GetPermissionStatus when the requesting origin is insecure.
  // Please see crbug.com/512301.
  ContentSetting GetPermissionStatusForPepper(
      const GURL& requesting_origin,
      const GURL& embedding_origin) const;

  void ResetPermission(const GURL& requesting_origin,
                       const GURL& embedding_origin) override;

  void CancelPermissionRequest(content::WebContents* web_contents,
                               const PermissionRequestID& id) override;

 private:
  ContentSetting GetPermissionStatusInternal(const GURL& requesting_origin,
                                             const GURL& embedding_origin,
                                             bool is_pepper_request) const;

  // PermissionContextBase:
  bool IsRestrictedToSecureOrigins() const override;

  ContentSettingsType content_settings_type_;

  DISALLOW_COPY_AND_ASSIGN(MediaStreamDevicePermissionContext);
};

#endif  // CHROME_BROWSER_MEDIA_MEDIA_STREAM_DEVICE_PERMISSION_CONTEXT_H_
