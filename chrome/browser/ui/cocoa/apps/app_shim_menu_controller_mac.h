// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_APPS_APP_SHIM_MENU_CONTROLLER_MAC_H_
#define CHROME_BROWSER_UI_COCOA_APPS_APP_SHIM_MENU_CONTROLLER_MAC_H_

#import <Cocoa/Cocoa.h>

#include "base/mac/scoped_nsobject.h"

// This controller listens to NSWindowDidBecomeMainNotification and
// NSWindowDidResignMainNotification and modifies the main menu bar to mimic a
// main menu for the app. When an app window becomes main, all Chrome menu items
// are hidden and menu items for the app are appended to the main menu. When the
// app window resigns main, its menu items are removed and all Chrome menu items
// are unhidden.
@interface AppShimMenuController : NSObject {
 @private
  // The extension id of the currently focused packaged app.
  base::scoped_nsobject<NSString> appId_;
  // A menu item for the currently focused packaged app.
  base::scoped_nsobject<NSMenuItem> appMenuItem_;
}

@end

#endif  // CHROME_BROWSER_UI_COCOA_APPS_APP_SHIM_MENU_CONTROLLER_MAC_H_
