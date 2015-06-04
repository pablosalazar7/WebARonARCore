// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/task_management/providers/web_contents/background_contents_task.h"

#include "base/i18n/rtl.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/background/background_contents_service.h"
#include "chrome/browser/background/background_contents_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/view_type_utils.h"
#include "extensions/common/extension_set.h"
#include "grit/theme_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image_skia.h"

namespace task_management {

namespace {

// The default icon for the background webcontents task.
gfx::ImageSkia* g_default_icon = nullptr;

gfx::ImageSkia* GetDefaultIcon() {
  if (!g_default_icon && ResourceBundle::HasSharedInstance()) {
    g_default_icon = ResourceBundle::GetSharedInstance().GetImageSkiaNamed(
        IDR_PLUGINS_FAVICON);
  }

  return g_default_icon;
}

base::string16 AdjustAndLocalizeTitle(const base::string16& title,
                                      const std::string& url_spec) {
  base::string16 localized_title(title);
  if (localized_title.empty()) {
    // No title (can't locate the parent app for some reason) so just display
    // the URL (properly forced to be LTR).
    localized_title = base::i18n::GetDisplayStringInLTRDirectionality(
        base::UTF8ToUTF16(url_spec));
  }

  // Ensure that the string has the appropriate direction markers.
  base::i18n::AdjustStringForLocaleDirection(&localized_title);
  return l10n_util::GetStringFUTF16(IDS_TASK_MANAGER_BACKGROUND_PREFIX,
                                    localized_title);
}

}  // namespace

BackgroundContentsTask::BackgroundContentsTask(
    const base::string16& title,
    BackgroundContents* background_contents)
    : RendererTask(AdjustAndLocalizeTitle(title,
                                          background_contents->GetURL().spec()),
                   GetDefaultIcon(),
                   background_contents->web_contents()->GetRenderProcessHost()->
                   GetHandle(),
                   background_contents->web_contents()->
                   GetRenderProcessHost()) {
}

BackgroundContentsTask::~BackgroundContentsTask() {
}

void BackgroundContentsTask::OnTitleChanged(content::NavigationEntry* entry) {
  // TODO(afakhry): At the time of integration testing figure out whether we
  // need to change the title of the task here.
}

void BackgroundContentsTask::OnFaviconChanged() {
  // We don't do anything here. For background contents we always use the
  // default icon.
}

}  // namespace task_management
