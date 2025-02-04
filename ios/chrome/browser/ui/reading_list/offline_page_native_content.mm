// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/ui/reading_list/offline_page_native_content.h"

#include "base/files/file_path.h"
#include "base/logging.h"
#include "components/reading_list/core/reading_list_switches.h"
#include "components/reading_list/ios/reading_list_model.h"
#include "ios/chrome/browser/browser_state/chrome_browser_state.h"
#include "ios/chrome/browser/reading_list/offline_url_utils.h"
#include "ios/chrome/browser/reading_list/reading_list_download_service.h"
#include "ios/chrome/browser/reading_list/reading_list_download_service_factory.h"
#include "ios/chrome/browser/reading_list/reading_list_model_factory.h"
#import "ios/chrome/browser/ui/static_content/static_html_view_controller.h"
#include "ios/web/public/browser_state.h"
#import "ios/web/public/navigation_item.h"
#import "ios/web/public/navigation_manager.h"
#import "ios/web/public/web_state/web_state.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

@interface OfflinePageNativeContent ()
// Restores the last committed item to its initial state.
- (void)restoreOnlineURL;
@end

@implementation OfflinePageNativeContent {
  // The virtual URL that will be displayed to the user.
  GURL _virtualURL;

  // The Reading list model needed to reload the entry.
  ReadingListModel* _model;

  // The WebState of the current tab.
  web::WebState* _webState;
}

- (instancetype)initWithLoader:(id<UrlLoader>)loader
                  browserState:(web::BrowserState*)browserState
                      webState:(web::WebState*)webState
                           URL:(const GURL&)URL {
  DCHECK(loader);
  DCHECK(browserState);
  DCHECK(URL.is_valid());

  DCHECK(reading_list::switches::IsReadingListEnabled());
  _model = ReadingListModelFactory::GetForBrowserState(
      ios::ChromeBrowserState::FromBrowserState(browserState));
  base::FilePath offline_root =
      ReadingListDownloadServiceFactory::GetForBrowserState(
          ios::ChromeBrowserState::FromBrowserState(browserState))
          ->OfflineRoot();

  _webState = webState;
  GURL resourcesRoot;
  GURL fileURL =
      reading_list::FileURLForDistilledURL(URL, offline_root, &resourcesRoot);

  StaticHtmlViewController* HTMLViewController =
      [[StaticHtmlViewController alloc] initWithFileURL:fileURL
                                allowingReadAccessToURL:resourcesRoot
                                           browserState:browserState];
  _virtualURL = reading_list::VirtualURLForDistilledURL(URL);

  return [super initWithLoader:loader
      staticHTMLViewController:HTMLViewController
                           URL:URL];
}

- (void)willBeDismissed {
  [self restoreOnlineURL];
}

- (void)close {
  [self restoreOnlineURL];
  [super close];
}

- (GURL)virtualURL {
  return _virtualURL;
}

- (void)reload {
  [self restoreOnlineURL];
  _webState->GetNavigationManager()->Reload(false);
}

- (void)restoreOnlineURL {
  web::NavigationItem* item =
      _webState->GetNavigationManager()->GetLastCommittedItem();
  DCHECK(item && item->GetVirtualURL() == [self virtualURL]);
  item->SetURL([self virtualURL]);
  item->SetVirtualURL([self virtualURL]);
}

@end
