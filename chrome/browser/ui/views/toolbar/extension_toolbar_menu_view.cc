// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/toolbar/extension_toolbar_menu_view.h"

#include "base/bind.h"
#include "base/location.h"
#include "base/single_thread_task_runner.h"
#include "base/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/browser_actions_container.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/browser/ui/views/toolbar/wrench_menu.h"
#include "ui/views/controls/menu/menu_item_view.h"
#include "ui/views/controls/menu/submenu_view.h"

ExtensionToolbarMenuView::ExtensionToolbarMenuView(Browser* browser,
                                                   WrenchMenu* wrench_menu)
    : browser_(browser),
      wrench_menu_(wrench_menu),
      container_(nullptr),
      max_height_(0),
      browser_actions_container_observer_(this),
      weak_factory_(this) {
  BrowserActionsContainer* main =
      BrowserView::GetBrowserViewForBrowser(browser_)
          ->toolbar()->browser_actions();
  container_ = new BrowserActionsContainer(browser_, main);
  container_->Init();
  SetContents(container_);
  // We Layout() the container here so that we know the number of actions
  // that will be visible in ShouldShow().
  container_->Layout();

  // If we were opened for a drop command, we have to wait for the drop to
  // finish so we can close the wrench menu.
  if (wrench_menu_->for_drop()) {
    browser_actions_container_observer_.Add(container_);
    browser_actions_container_observer_.Add(main);
  }

  // In *very* extreme cases, it's possible that there are so many overflowed
  // actions, we won't be able to show them all. Cap the height so that the
  // overflow won't be excessively tall (at 8 icons per row, this allows for
  // 104 total extensions).
  const int kMaxOverflowRows = 13;
  max_height_ = ToolbarActionsBar::IconHeight() * kMaxOverflowRows;
  ClipHeightTo(0, max_height_);
}

ExtensionToolbarMenuView::~ExtensionToolbarMenuView() {
}

bool ExtensionToolbarMenuView::ShouldShow() {
  return wrench_menu_->for_drop() ||
      container_->VisibleBrowserActionsAfterAnimation();
}

gfx::Size ExtensionToolbarMenuView::GetPreferredSize() const {
  gfx::Size s = views::ScrollView::GetPreferredSize();
  // views::ScrollView::GetPreferredSize() includes the contents' size, but
  // not the scrollbar width. Add it in if necessary.
  if (container_->GetPreferredSize().height() > max_height_)
    s.Enlarge(GetScrollBarWidth(), 0);
  return s;
}

int ExtensionToolbarMenuView::GetHeightForWidth(int width) const {
  // The width passed in here includes the full width of the menu, so we need
  // to omit the necessary padding.
  const views::MenuConfig& menu_config =
      static_cast<const views::MenuItemView*>(parent())->GetMenuConfig();
  int end_padding = menu_config.arrow_to_edge_padding -
      container_->toolbar_actions_bar()->platform_settings().item_spacing;
  width -= start_padding() + end_padding;

  return views::ScrollView::GetHeightForWidth(width);
}

void ExtensionToolbarMenuView::Layout() {
  SetPosition(gfx::Point(start_padding(), 0));
  SizeToPreferredSize();
  views::ScrollView::Layout();
}

void ExtensionToolbarMenuView::OnBrowserActionDragDone() {
  // The delay before we close the wrench menu if this was opened for a drop so
  // that the user can see a browser action if one was moved.
  static const int kCloseMenuDelay = 300;

  DCHECK(wrench_menu_->for_drop());
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE, base::Bind(&ExtensionToolbarMenuView::CloseWrenchMenu,
                            weak_factory_.GetWeakPtr()),
      base::TimeDelta::FromMilliseconds(kCloseMenuDelay));
}

void ExtensionToolbarMenuView::CloseWrenchMenu() {
  wrench_menu_->CloseMenu();
}

int ExtensionToolbarMenuView::start_padding() const {
  // We pad enough on the left so that the first icon starts at the same point
  // as the labels. We subtract kItemSpacing because there needs to be padding
  // so we can see the drop indicator.
  return views::MenuItemView::label_start() -
      container_->toolbar_actions_bar()->platform_settings().item_spacing;
}

