// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/extensions_api_client.h"

#include "base/logging.h"
#include "extensions/browser/api/device_permissions_prompt.h"
#include "extensions/browser/api/virtual_keyboard_private/virtual_keyboard_delegate.h"
#include "extensions/browser/api/web_request/web_request_event_router_delegate.h"
#include "extensions/browser/guest_view/mime_handler_view/mime_handler_view_guest_delegate.h"
#include "extensions/browser/guest_view/web_view/web_view_permission_helper_delegate.h"

namespace extensions {
class AppViewGuestDelegate;

namespace {
ExtensionsAPIClient* g_instance = NULL;
}  // namespace

ExtensionsAPIClient::ExtensionsAPIClient() { g_instance = this; }

ExtensionsAPIClient::~ExtensionsAPIClient() { g_instance = NULL; }

// static
ExtensionsAPIClient* ExtensionsAPIClient::Get() { return g_instance; }

void ExtensionsAPIClient::AddAdditionalValueStoreCaches(
    content::BrowserContext* context,
    const scoped_refptr<SettingsStorageFactory>& factory,
    const scoped_refptr<base::ObserverListThreadSafe<SettingsObserver>>&
        observers,
    std::map<settings_namespace::Namespace, ValueStoreCache*>* caches) {
}

AppViewGuestDelegate* ExtensionsAPIClient::CreateAppViewGuestDelegate() const {
  return NULL;
}

ExtensionOptionsGuestDelegate*
ExtensionsAPIClient::CreateExtensionOptionsGuestDelegate(
    ExtensionOptionsGuest* guest) const {
  return NULL;
}

ExtensionViewGuestDelegate*
ExtensionsAPIClient::CreateExtensionViewGuestDelegate(
    ExtensionViewGuest* guest) const {
  return NULL;
}

scoped_ptr<MimeHandlerViewGuestDelegate>
ExtensionsAPIClient::CreateMimeHandlerViewGuestDelegate(
    MimeHandlerViewGuest* guest) const {
  return scoped_ptr<MimeHandlerViewGuestDelegate>();
}

WebViewGuestDelegate* ExtensionsAPIClient::CreateWebViewGuestDelegate(
    WebViewGuest* web_view_guest) const {
  return NULL;
}

WebViewPermissionHelperDelegate* ExtensionsAPIClient::
    CreateWebViewPermissionHelperDelegate(
        WebViewPermissionHelper* web_view_permission_helper) const {
  return new WebViewPermissionHelperDelegate(web_view_permission_helper);
}

WebRequestEventRouterDelegate*
ExtensionsAPIClient::CreateWebRequestEventRouterDelegate() const {
  return new WebRequestEventRouterDelegate();
}

scoped_refptr<ContentRulesRegistry>
ExtensionsAPIClient::CreateContentRulesRegistry(
    content::BrowserContext* browser_context,
    RulesCacheDelegate* cache_delegate) const {
  return scoped_refptr<ContentRulesRegistry>();
}

scoped_ptr<DevicePermissionsPrompt>
ExtensionsAPIClient::CreateDevicePermissionsPrompt(
    content::WebContents* web_contents) const {
  return nullptr;
}

scoped_ptr<VirtualKeyboardDelegate>
ExtensionsAPIClient::CreateVirtualKeyboardDelegate() const {
  return nullptr;
}

ManagementAPIDelegate* ExtensionsAPIClient::CreateManagementAPIDelegate()
    const {
  return nullptr;
}

}  // namespace extensions
