// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/infobars/confirm_infobar.h"

#include "base/logging.h"
#include "chrome/browser/infobars/confirm_infobar_delegate.h"
#include "ui/base/window_open_disposition.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/link.h"


// ConfirmInfoBarDelegate -----------------------------------------------------

// static
scoped_ptr<InfoBar> ConfirmInfoBarDelegate::CreateInfoBar(
    scoped_ptr<ConfirmInfoBarDelegate> delegate) {
  return scoped_ptr<InfoBar>(new ConfirmInfoBar(delegate.Pass()));
}


// ConfirmInfoBar -------------------------------------------------------------

ConfirmInfoBar::ConfirmInfoBar(scoped_ptr<ConfirmInfoBarDelegate> delegate)
    : InfoBarView(delegate.PassAs<InfoBarDelegate>()),
      label_(NULL),
      ok_button_(NULL),
      cancel_button_(NULL),
      link_(NULL) {
}

ConfirmInfoBar::~ConfirmInfoBar() {
}

void ConfirmInfoBar::Layout() {
  InfoBarView::Layout();

  int x = StartX();
  int available_width = std::max(0, EndX() - x - ContentMinimumWidth());
  gfx::Size label_size = label_->GetPreferredSize();
  label_->SetBounds(x, OffsetY(label_),
                    std::min(label_size.width(), available_width),
                    label_size.height());
  available_width = std::max(0, available_width - label_size.width());
  if (!label_->text().empty())
    x = label_->bounds().right() + kEndOfLabelSpacing;

  if (ok_button_) {
    ok_button_->SetPosition(gfx::Point(x, OffsetY(ok_button_)));
    x = ok_button_->bounds().right() + kButtonButtonSpacing;
  }

  if (cancel_button_)
    cancel_button_->SetPosition(gfx::Point(x, OffsetY(cancel_button_)));

  gfx::Size link_size = link_->GetPreferredSize();
  int link_width = std::min(link_size.width(), available_width);
  link_->SetBounds(EndX() - link_width, OffsetY(link_), link_width,
                   link_size.height());
}

void ConfirmInfoBar::ViewHierarchyChanged(
    const ViewHierarchyChangedDetails& details) {
  if (details.is_add && details.child == this && (label_ == NULL)) {
    ConfirmInfoBarDelegate* delegate = GetDelegate();
    label_ = CreateLabel(delegate->GetMessageText());
    AddChildView(label_);

    if (delegate->GetButtons() & ConfirmInfoBarDelegate::BUTTON_OK) {
      ok_button_ = CreateLabelButton(this,
          delegate->GetButtonLabel(ConfirmInfoBarDelegate::BUTTON_OK),
          delegate->NeedElevation(ConfirmInfoBarDelegate::BUTTON_OK));
      AddChildView(ok_button_);
    }

    if (delegate->GetButtons() & ConfirmInfoBarDelegate::BUTTON_CANCEL) {
      cancel_button_ = CreateLabelButton(this,
          delegate->GetButtonLabel(ConfirmInfoBarDelegate::BUTTON_CANCEL),
          delegate->NeedElevation(ConfirmInfoBarDelegate::BUTTON_CANCEL));
      AddChildView(cancel_button_);
    }

    base::string16 link_text(delegate->GetLinkText());
    link_ = CreateLink(link_text, this);
    AddChildView(link_);
  }

  // This must happen after adding all other children so InfoBarView can ensure
  // the close button is the last child.
  InfoBarView::ViewHierarchyChanged(details);
}

void ConfirmInfoBar::ButtonPressed(views::Button* sender,
                                   const ui::Event& event) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.
  ConfirmInfoBarDelegate* delegate = GetDelegate();
  if ((ok_button_ != NULL) && sender == ok_button_) {
    if (delegate->Accept())
      RemoveSelf();
  } else if ((cancel_button_ != NULL) && (sender == cancel_button_)) {
    if (delegate->Cancel())
      RemoveSelf();
  } else {
    InfoBarView::ButtonPressed(sender, event);
  }
}

int ConfirmInfoBar::ContentMinimumWidth() const {
  int width = (label_->text().empty() || (!ok_button_ && !cancel_button_)) ?
      0 : kEndOfLabelSpacing;
  if (ok_button_)
    width += ok_button_->width() + (cancel_button_ ? kButtonButtonSpacing : 0);
  width += (cancel_button_ ? cancel_button_->width() : 0);
  return width + ((link_->text().empty() || !width) ? 0 : kEndOfLabelSpacing);
}

void ConfirmInfoBar::LinkClicked(views::Link* source, int event_flags) {
  if (!owner())
    return;  // We're closing; don't call anything, it might access the owner.
  DCHECK_EQ(link_, source);
  if (GetDelegate()->LinkClicked(ui::DispositionFromEventFlags(event_flags)))
    RemoveSelf();
}

ConfirmInfoBarDelegate* ConfirmInfoBar::GetDelegate() {
  return delegate()->AsConfirmInfoBarDelegate();
}
