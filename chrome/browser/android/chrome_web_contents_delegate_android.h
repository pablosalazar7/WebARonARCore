// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_CHROME_WEB_CONTENTS_DELEGATE_ANDROID_H_
#define CHROME_BROWSER_ANDROID_CHROME_WEB_CONTENTS_DELEGATE_ANDROID_H_

#include <jni.h>

#include "base/files/file_path.h"
#include "components/web_contents_delegate_android/web_contents_delegate_android.h"
#include "content/public/browser/bluetooth_chooser.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class FindNotificationDetails;

namespace content {
struct FileChooserParams;
class WebContents;
}

namespace gfx {
class Rect;
class RectF;
}

namespace chrome {
namespace android {

// Chromium Android specific WebContentsDelegate.
// Should contain any WebContentsDelegate implementations required by
// the Chromium Android port but not to be shared with WebView.
class ChromeWebContentsDelegateAndroid
    : public web_contents_delegate_android::WebContentsDelegateAndroid,
      public content::NotificationObserver {
 public:
  ChromeWebContentsDelegateAndroid(JNIEnv* env, jobject obj);
  ~ChromeWebContentsDelegateAndroid() override;

  void LoadingStateChanged(content::WebContents* source,
                           bool to_different_document) override;
  void RunFileChooser(content::WebContents* web_contents,
                      const content::FileChooserParams& params) override;
  scoped_ptr<content::BluetoothChooser> RunBluetoothChooser(
      content::WebContents* web_contents,
      const content::BluetoothChooser::EventHandler& event_handler,
      const GURL& origin) override;

  void VisibleSSLStateChanged(content::WebContents* source) override;
  void CloseContents(content::WebContents* web_contents) override;
  blink::WebDisplayMode GetDisplayMode(
      const content::WebContents* web_contents) const override;
  void FindReply(content::WebContents* web_contents,
                 int request_id,
                 int number_of_matches,
                 const gfx::Rect& selection_rect,
                 int active_match_ordinal,
                 bool final_update) override;
  void FindMatchRectsReply(content::WebContents* web_contents,
                           int version,
                           const std::vector<gfx::RectF>& rects,
                           const gfx::RectF& active_rect) override;
  content::JavaScriptDialogManager* GetJavaScriptDialogManager(
      content::WebContents* source) override;
  void RequestMediaAccessPermission(
      content::WebContents* web_contents,
      const content::MediaStreamRequest& request,
      const content::MediaResponseCallback& callback) override;
  bool CheckMediaAccessPermission(content::WebContents* web_contents,
                                  const GURL& security_origin,
                                  content::MediaStreamType type) override;
  bool RequestPpapiBrokerPermission(
      content::WebContents* web_contents,
      const GURL& url,
      const base::FilePath& plugin_path,
      const base::Callback<void(bool)>& callback) override;
  content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  bool ShouldResumeRequestsForCreatedWindow() override;
  void AddNewContents(content::WebContents* source,
                      content::WebContents* new_contents,
                      WindowOpenDisposition disposition,
                      const gfx::Rect& initial_rect,
                      bool user_gesture,
                      bool* was_blocked) override;

 private:
  // NotificationObserver implementation.
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  void OnFindResultAvailable(content::WebContents* web_contents,
                             const FindNotificationDetails* find_result);

  content::NotificationRegistrar notification_registrar_;
};

// Register the native methods through JNI.
bool RegisterChromeWebContentsDelegateAndroid(JNIEnv* env);

}  // namespace android
}  // namespace chrome

#endif  // CHROME_BROWSER_ANDROID_CHROME_WEB_CONTENTS_DELEGATE_ANDROID_H_
