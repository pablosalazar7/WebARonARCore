// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/media_stream_device_permission_context.h"
#include "chrome/browser/media/media_stream_device_permissions.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "content/public/common/url_constants.h"
#include "extensions/common/constants.h"

MediaStreamDevicePermissionContext::MediaStreamDevicePermissionContext(
    Profile* profile,
    const ContentSettingsType content_settings_type)
    : PermissionContextBase(profile, content_settings_type),
      content_settings_type_(content_settings_type) {
  DCHECK(content_settings_type_ == CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC ||
         content_settings_type_ == CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
}

MediaStreamDevicePermissionContext::~MediaStreamDevicePermissionContext() {}

void MediaStreamDevicePermissionContext::RequestPermission(
    content::WebContents* web_contents,
    const PermissionRequestID& id,
    const GURL& requesting_frame,
    bool user_gesture,
    const BrowserPermissionCallback& callback) {
  NOTREACHED() << "RequestPermission is not implemented";
  callback.Run(CONTENT_SETTING_BLOCK);
}

ContentSetting MediaStreamDevicePermissionContext::GetPermissionStatus(
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  return GetPermissionStatusInternal(requesting_origin, embedding_origin,
                                     false);
}

ContentSetting MediaStreamDevicePermissionContext::GetPermissionStatusForPepper(
    const GURL& requesting_origin,
    const GURL& embedding_origin) const {
  return GetPermissionStatusInternal(requesting_origin, embedding_origin, true);
}

void MediaStreamDevicePermissionContext::ResetPermission(
    const GURL& requesting_origin,
    const GURL& embedding_origin) {
  NOTREACHED() << "ResetPermission is not implemented";
}

void MediaStreamDevicePermissionContext::CancelPermissionRequest(
    content::WebContents* web_contents,
    const PermissionRequestID& id) {
  NOTREACHED() << "CancelPermissionRequest is not implemented";
}

ContentSetting MediaStreamDevicePermissionContext::GetPermissionStatusInternal(
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    bool is_pepper_request) const {
  // TODO(raymes): Merge this policy check into content settings
  // crbug.com/244389.
  const char* policy_name = nullptr;
  const char* urls_policy_name = nullptr;
  if (content_settings_type_ == CONTENT_SETTINGS_TYPE_MEDIASTREAM_MIC) {
    policy_name = prefs::kAudioCaptureAllowed;
    urls_policy_name = prefs::kAudioCaptureAllowedUrls;
  } else {
    DCHECK(content_settings_type_ == CONTENT_SETTINGS_TYPE_MEDIASTREAM_CAMERA);
    policy_name = prefs::kVideoCaptureAllowed;
    urls_policy_name = prefs::kVideoCaptureAllowedUrls;
  }

  MediaStreamDevicePolicy policy = GetDevicePolicy(
      profile(), requesting_origin, policy_name, urls_policy_name);

  switch (policy) {
    case ALWAYS_DENY:
      return CONTENT_SETTING_BLOCK;
    case ALWAYS_ALLOW:
      return CONTENT_SETTING_ALLOW;
    default:
      DCHECK_EQ(POLICY_NOT_SET, policy);
  }

  // Check the content setting. TODO(raymes): currently mic/camera permission
  // doesn't consider the embedder.
  ContentSetting setting = PermissionContextBase::GetPermissionStatus(
      requesting_origin, requesting_origin);

  if (setting == CONTENT_SETTING_DEFAULT)
    setting = CONTENT_SETTING_ASK;

  // TODO(raymes): This is here for safety to ensure that we always ask the user
  // even if a content setting is set to "allow" if the origin is insecure. In
  // reality we shouldn't really need to check this here as we should respect
  // the user's content setting. The problem is that pepper requests allow
  // insecure origins to be persisted. We should stop allowing this, do some
  // sort of migration and remove this check. See crbug.com/512301.
  if (!ShouldPersistContentSetting(setting, requesting_origin,
                                   is_pepper_request) &&
      !requesting_origin.SchemeIs(extensions::kExtensionScheme) &&
      !requesting_origin.SchemeIs(content::kChromeUIScheme) &&
      !requesting_origin.SchemeIs(content::kChromeDevToolsScheme)) {
    return CONTENT_SETTING_ASK;
  }

  return setting;
}

bool MediaStreamDevicePermissionContext::IsRestrictedToSecureOrigins() const {
  // Flash currently doesn't require secure origin to use mic/camera. If we
  // return true here, it'll break the use case like http://tinychat.com. Please
  // see crbug.com/512301.
  return false;
}
