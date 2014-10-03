// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_GUEST_VIEW_EXTENSION_OPTIONS_EXTENSION_OPTIONS_GUEST_H_
#define EXTENSIONS_BROWSER_GUEST_VIEW_EXTENSION_OPTIONS_EXTENSION_OPTIONS_GUEST_H_

#include "base/macros.h"
#include "extensions/browser/extension_function_dispatcher.h"
#include "extensions/browser/guest_view/extension_options/extension_options_guest_delegate.h"
#include "extensions/browser/guest_view/guest_view.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class ExtensionOptionsGuest
    : public extensions::GuestView<ExtensionOptionsGuest>,
      public extensions::ExtensionFunctionDispatcher::Delegate {
 public:
  static const char Type[];
  static extensions::GuestViewBase* Create(
      content::BrowserContext* browser_context,
      int guest_instance_id);

  // GuestViewBase implementation.
  virtual void CreateWebContents(
      const std::string& embedder_extension_id,
      int embedder_render_process_id,
      const GURL& embedder_site_url,
      const base::DictionaryValue& create_params,
      const WebContentsCreatedCallback& callback) override;
  virtual void DidAttachToEmbedder() override;
  virtual void DidInitialize() override;
  virtual void DidStopLoading() override;
  virtual const char* GetAPINamespace() const override;
  virtual int GetTaskPrefix() const override;
  virtual void GuestSizeChangedDueToAutoSize(
      const gfx::Size& old_size,
      const gfx::Size& new_size) override;
  virtual bool IsAutoSizeSupported() const override;

  // ExtensionFunctionDispatcher::Delegate implementation.
  virtual content::WebContents* GetAssociatedWebContents() const override;

  // content::WebContentsDelegate implementation.
  virtual content::WebContents* OpenURLFromTab(
      content::WebContents* source,
      const content::OpenURLParams& params) override;
  virtual void CloseContents(content::WebContents* source) override;
  virtual bool HandleContextMenu(
      const content::ContextMenuParams& params) override;
  virtual bool ShouldCreateWebContents(
      content::WebContents* web_contents,
      int route_id,
      WindowContainerType window_container_type,
      const base::string16& frame_name,
      const GURL& target_url,
      const std::string& partition_id,
      content::SessionStorageNamespace* session_storage_namespace) override;

  // content::WebContentsObserver implementation.
  virtual bool OnMessageReceived(const IPC::Message& message) override;

 private:
  ExtensionOptionsGuest(content::BrowserContext* browser_context,
                        int guest_instance_id);
  virtual ~ExtensionOptionsGuest();
  void OnRequest(const ExtensionHostMsg_Request_Params& params);
  void SetUpAutoSize();

  scoped_ptr<extensions::ExtensionFunctionDispatcher>
      extension_function_dispatcher_;
  scoped_ptr<extensions::ExtensionOptionsGuestDelegate>
      extension_options_guest_delegate_;
  GURL options_page_;
  bool has_navigated_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionOptionsGuest);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_GUEST_VIEW_EXTENSION_OPTIONS_EXTENSION_OPTIONS_GUEST_H_
