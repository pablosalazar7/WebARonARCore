// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_BANNERS_APP_BANNER_MANAGER_H_
#define CHROME_BROWSER_ANDROID_BANNERS_APP_BANNER_MANAGER_H_

#include "base/android/jni_android.h"
#include "base/android/jni_weak_ref.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "chrome/browser/android/banners/app_banner_infobar_delegate.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher.h"
#include "chrome/browser/ui/android/infobars/app_banner_infobar.h"
#include "components/infobars/core/infobar_manager.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/common/manifest.h"

namespace content {
struct FrameNavigateParams;
struct LoadCommittedDetails;
}  // namespace content

namespace infobars {
class InfoBar;
}  // namspace infobars

/**
 * Manages when an app banner is created or dismissed.
 *
 * Hooks the wiring together for getting the data for a particular app.
 * Monitors at most one package at a time, and tracks the info for the
 * most recent app that was requested.  Any work in progress for other apps is
 * discarded.
 *
 * The procedure for creating a banner spans multiple asynchronous calls across
 * the JNI boundary, as well as querying a Service to get info about the app.
 *
 * 0) A navigation of the main frame is triggered.  Upon completion of the load,
 *    the page is parsed for the correct meta tag.  If it doesn't exist, abort.
 *
 * 1) The AppBannerManager is alerted about the tag's contents, which should
 *    be the Play Store package name.  This is sent to the Java side
 *    AppBannerManager.
 *
 * 2) The AppBannerManager's ServiceDelegate is asynchronously queried about the
 *    package name.
 *
 * 3) At some point, the Java-side AppBannerManager is alerted of the completed
 *    query and is given back data about the requested package, which includes a
 *    URL for the app's icon.  This URL is sent to native code for retrieval.
 *
 * 4) The process of fetching the icon begins by invoking the BitmapFetcher,
 *    which works asynchonously.
 *
 * 5) Once the icon has been downloaded, the icon is sent to the Java-side
 *    AppBannerManager to (finally) create a AppBannerView, assuming that the
 *    app we retrieved the details for is still for the page that requested it.
 *
 * Because of the asynchronous nature of this pipeline, it's entirely possible
 * that a request to show a banner is interrupted by another request.  The
 * Java side manages what happens in these situations, which will usually result
 * in dropping the old banner request on the floor.
 */

namespace banners {

class AppBannerManager : public content::WebContentsObserver {
 public:
  class BannerBitmapFetcher;

  AppBannerManager(JNIEnv* env, jobject obj);
  ~AppBannerManager() override;

  // Destroys the AppBannerManager.
  void Destroy(JNIEnv* env, jobject obj);

  // Observes a new WebContents, if necessary.
  void ReplaceWebContents(JNIEnv* env,
                          jobject obj,
                          jobject jweb_contents);

  // Called when the Java-side has retrieved information for the app.
  // Returns |false| if an icon fetch couldn't be kicked off.
  bool OnAppDetailsRetrieved(JNIEnv* env,
                             jobject obj,
                             jobject japp_data,
                             jstring japp_title,
                             jstring japp_package,
                             jstring jicon_url);

  // Fetches the icon at the given URL asynchronously.
  // Returns |false| if this couldn't be kicked off.
  bool FetchIcon(const GURL& image_url);

  // Called when everything required to show a banner is ready.
  void OnFetchComplete(BannerBitmapFetcher* fetcher,
                       const GURL url,
                       const SkBitmap* icon);

  // Return whether a BitmapFetcher is active.
  bool IsFetcherActive(JNIEnv* env, jobject jobj);

  // Returns the current time.
  static base::Time GetCurrentTime();

  // WebContentsObserver overrides.
  void DidNavigateMainFrame(
      const content::LoadCommittedDetails& details,
      const content::FrameNavigateParams& params) override;
  void DidFinishLoad(content::RenderFrameHost* render_frame_host,
                     const GURL& validated_url) override;
  bool OnMessageReceived(const IPC::Message& message) override;

 private:
  // Gets the preferred icon size for the banner icons.
  int GetPreferredIconSize();

  // Called when the manifest has been retrieved, or if there is no manifest to
  // retrieve.
  void OnDidGetManifest(const content::Manifest& manifest);

  // Called when the renderer has returned information about the meta tag.
  // If there is some metadata for the play store tag, this kicks off the
  // process of showing a banner for the package designated by |tag_content| on
  // the page at the |expected_url|.
  void OnDidRetrieveMetaTagContent(bool success,
                                   const std::string& tag_name,
                                   const std::string& tag_content,
                                   const GURL& expected_url);

  // Called when the result of the CheckHasServiceWorker query has completed.
  void OnDidCheckHasServiceWorker(bool has_service_worker);

  // Record that the banner could be shown at this point, if the triggering
  // heuristic allowed.
  void RecordCouldShowBanner(const std::string& package_or_start_url);

  // Check if the banner should be shown.
  bool CheckIfShouldShow(const std::string& package_or_start_url);

  // Cancels an active BitmapFetcher, stopping its banner from appearing.
  void CancelActiveFetcher();

  // Fetches the icon for an app.  Weakly held because they delete themselves.
  BannerBitmapFetcher* fetcher_;

  GURL validated_url_;
  GURL app_icon_url_;

  base::string16 app_title_;

  content::Manifest web_app_data_;

  base::android::ScopedJavaGlobalRef<jobject> native_app_data_;
  std::string native_app_package_;

  // AppBannerManager on the Java side.
  JavaObjectWeakGlobalRef weak_java_banner_view_manager_;

  // A weak pointer is used as the lifetime of the ServiceWorkerContext is
  // longer than the lifetime of this banner manager. The banner manager
  // might be gone when calls sent to the ServiceWorkerContext are completed.
  base::WeakPtrFactory<AppBannerManager> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppBannerManager);
};  // class AppBannerManager

// Register native methods
bool RegisterAppBannerManager(JNIEnv* env);

}  // namespace banners

#endif  // CHROME_BROWSER_ANDROID_BANNERS_APP_BANNER_MANAGER_H_
