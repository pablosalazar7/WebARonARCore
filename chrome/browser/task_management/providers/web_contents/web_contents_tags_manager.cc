// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_management/providers/web_contents/web_contents_tags_manager.h"

#include "base/memory/singleton.h"
#include "chrome/browser/task_management/providers/web_contents/web_contents_task_provider.h"

namespace task_management {

// static
WebContentsTagsManager* WebContentsTagsManager::GetInstance() {
  return Singleton<WebContentsTagsManager>::get();
}

void WebContentsTagsManager::AddTag(WebContentsTag* tag) {
  DCHECK(tag);
  tracked_tags_.insert(tag);

  if (provider_)
    provider_->OnWebContentsTagCreated(tag);
}

void WebContentsTagsManager::RemoveTag(WebContentsTag* tag) {
  DCHECK(tag);
  tracked_tags_.erase(tag);

  // No need to inform the provider here. The provider will create an entry
  // for each WebContents it's tracking which is a WebContentsObserver and
  // can be used to track the lifetime of the WebContents.

  // We must however make sure that the provider has already forgotten about the
  // tag and its associated web_contents.
  if (provider_)
    CHECK(!provider_->HasWebContents(tag->web_contents()));
}

void WebContentsTagsManager::SetProvider(WebContentsTaskProvider* provider) {
  DCHECK(provider);
  DCHECK(!provider_);
  provider_ = provider;
}

void WebContentsTagsManager::ClearProvider() {
  DCHECK(provider_);
  provider_ = nullptr;
}

WebContentsTagsManager::WebContentsTagsManager()
    : provider_(nullptr) {
}

WebContentsTagsManager::~WebContentsTagsManager() {
}

}  // namespace task_management
