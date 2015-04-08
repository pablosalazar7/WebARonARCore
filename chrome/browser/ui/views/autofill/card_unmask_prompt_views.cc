// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/autofill/card_unmask_prompt_views.h"

#include "base/basictypes.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/autofill/autofill_dialog_types.h"
#include "chrome/browser/ui/autofill/card_unmask_prompt_controller.h"
#include "chrome/browser/ui/views/autofill/decorated_textfield.h"
#include "chrome/browser/ui/views/autofill/tooltip_icon.h"
#include "chrome/grit/generated_resources.h"
#include "components/constrained_window/constrained_window_views.h"
#include "components/web_modal/web_contents_modal_dialog_host.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "components/web_modal/web_contents_modal_dialog_manager_delegate.h"
#include "grit/theme_resources.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/compositing_recorder.h"
#include "ui/compositor/paint_context.h"
#include "ui/gfx/canvas.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/combobox/combobox.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/throbber.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_client_view.h"

namespace autofill {

// The number of pixels of blank space on the outer horizontal edges of the
// dialog.
const int kEdgePadding = 19;

SkColor kGreyTextColor = SkColorSetRGB(0x64, 0x64, 0x64);

// static
CardUnmaskPromptView* CardUnmaskPromptView::CreateAndShow(
    CardUnmaskPromptController* controller) {
  CardUnmaskPromptViews* view = new CardUnmaskPromptViews(controller);
  view->Show();
  return view;
}

CardUnmaskPromptViews::CardUnmaskPromptViews(
    CardUnmaskPromptController* controller)
    : controller_(controller),
      main_contents_(nullptr),
      permanent_error_label_(nullptr),
      input_row_(nullptr),
      cvc_input_(nullptr),
      month_input_(nullptr),
      year_input_(nullptr),
      error_icon_(nullptr),
      error_label_(nullptr),
      storage_row_(nullptr),
      storage_checkbox_(nullptr),
      progress_overlay_(nullptr),
      progress_throbber_(nullptr),
      progress_label_(nullptr),
      overlay_animation_(this),
      weak_ptr_factory_(this) {
}

CardUnmaskPromptViews::~CardUnmaskPromptViews() {
  if (controller_)
    controller_->OnUnmaskDialogClosed();
}

void CardUnmaskPromptViews::Show() {
  constrained_window::ShowWebModalDialogViews(this,
                                              controller_->GetWebContents());
}

void CardUnmaskPromptViews::ControllerGone() {
  controller_ = nullptr;
  ClosePrompt();
}

void CardUnmaskPromptViews::DisableAndWaitForVerification() {
  SetInputsEnabled(false);
  progress_overlay_->SetOpacity(0.0);
  progress_overlay_->SetVisible(true);
  progress_throbber_->Start();
  overlay_animation_.Show();
  GetDialogClientView()->UpdateDialogButtons();
  Layout();
}

void CardUnmaskPromptViews::GotVerificationResult(
    const base::string16& error_message,
    bool allow_retry) {
  progress_throbber_->Stop();
  if (error_message.empty()) {
    progress_label_->SetText(l10n_util::GetStringUTF16(
        IDS_AUTOFILL_CARD_UNMASK_VERIFICATION_SUCCESS));
    progress_throbber_->SetChecked(true);
    base::MessageLoop::current()->PostDelayedTask(
        FROM_HERE, base::Bind(&CardUnmaskPromptViews::ClosePrompt,
                              weak_ptr_factory_.GetWeakPtr()),
        controller_->GetSuccessMessageDuration());
  } else {
    // TODO(estade): it's somewhat jarring when the error comes back too
    // quickly.
    overlay_animation_.Reset();
    storage_row_->SetOpacity(1.0);
    progress_overlay_->SetVisible(false);

    if (allow_retry) {
      SetInputsEnabled(true);

      // If there is more than one input showing, don't mark anything as
      // invalid since we don't know the location of the problem.
      if (!controller_->ShouldRequestExpirationDate())
        cvc_input_->SetInvalid(true);

      // TODO(estade): When do we hide |error_label_|?
      SetRetriableErrorMessage(error_message);
    } else {
      permanent_error_label_->SetText(error_message);
      permanent_error_label_->SetVisible(true);
      SetRetriableErrorMessage(base::string16());
    }
    GetDialogClientView()->UpdateDialogButtons();
  }

  Layout();
}

void CardUnmaskPromptViews::SetRetriableErrorMessage(
    const base::string16& message) {
  if (message.empty()) {
    error_label_->SetMultiLine(false);
    error_label_->SetText(base::ASCIIToUTF16(" "));
    error_icon_->SetVisible(false);
  } else {
    error_label_->SetMultiLine(true);
    error_label_->SetText(message);
    error_icon_->SetVisible(true);
  }

  // Update the dialog's size.
  if (GetWidget() && controller_->GetWebContents()) {
    constrained_window::UpdateWebContentsModalDialogPosition(
        GetWidget(), web_modal::WebContentsModalDialogManager::FromWebContents(
                         controller_->GetWebContents())
                         ->delegate()
                         ->GetWebContentsModalDialogHost());
  }

  Layout();
}

void CardUnmaskPromptViews::SetInputsEnabled(bool enabled) {
  cvc_input_->SetEnabled(enabled);
  if (storage_checkbox_)
    storage_checkbox_->SetEnabled(enabled);
  if (month_input_)
    month_input_->SetEnabled(enabled);
  if (year_input_)
    year_input_->SetEnabled(enabled);
}

views::View* CardUnmaskPromptViews::GetContentsView() {
  InitIfNecessary();
  return this;
}

views::View* CardUnmaskPromptViews::CreateFootnoteView() {
  if (!controller_->CanStoreLocally())
    return nullptr;

  // Local storage checkbox and (?) tooltip.
  storage_row_ = new FadeOutView();
  views::BoxLayout* storage_row_layout = new views::BoxLayout(
      views::BoxLayout::kHorizontal, kEdgePadding, kEdgePadding, 0);
  storage_row_->SetLayoutManager(storage_row_layout);
  storage_row_->SetBorder(
      views::Border::CreateSolidSidedBorder(1, 0, 0, 0, kSubtleBorderColor));
  storage_row_->set_background(
      views::Background::CreateSolidBackground(kLightShadingColor));

  storage_checkbox_ = new views::Checkbox(l10n_util::GetStringUTF16(
      IDS_AUTOFILL_CARD_UNMASK_PROMPT_STORAGE_CHECKBOX));
  storage_checkbox_->SetChecked(controller_->GetStoreLocallyStartState());
  storage_row_->AddChildView(storage_checkbox_);
  storage_row_layout->SetFlexForView(storage_checkbox_, 1);

  storage_row_->AddChildView(new TooltipIcon(l10n_util::GetStringUTF16(
      IDS_AUTOFILL_CARD_UNMASK_PROMPT_STORAGE_TOOLTIP)));

  return storage_row_;
}

gfx::Size CardUnmaskPromptViews::GetPreferredSize() const {
  // Must hardcode a width so the label knows where to wrap.
  const int kWidth = 375;
  return gfx::Size(kWidth, GetHeightForWidth(kWidth));
}

void CardUnmaskPromptViews::Layout() {
  gfx::Rect contents_bounds = GetContentsBounds();
  main_contents_->SetBoundsRect(contents_bounds);

  // The progress overlay extends from the top of the input row
  // to the bottom of the content area.
  gfx::RectF input_rect = input_row_->GetContentsBounds();
  View::ConvertRectToTarget(input_row_, this, &input_rect);
  input_rect.set_height(contents_bounds.height());
  contents_bounds.Intersect(gfx::ToNearestRect(input_rect));
  progress_overlay_->SetBoundsRect(contents_bounds);
}

int CardUnmaskPromptViews::GetHeightForWidth(int width) const {
  if (!has_children())
    return 0;
  const gfx::Insets insets = GetInsets();
  return main_contents_->GetHeightForWidth(width - insets.width()) +
         insets.height();
}

void CardUnmaskPromptViews::OnNativeThemeChanged(const ui::NativeTheme* theme) {
  SkColor bg_color =
      theme->GetSystemColor(ui::NativeTheme::kColorId_DialogBackground);
  progress_overlay_->set_background(
      views::Background::CreateSolidBackground(bg_color));
  progress_label_->SetBackgroundColor(bg_color);
}

ui::ModalType CardUnmaskPromptViews::GetModalType() const {
  return ui::MODAL_TYPE_CHILD;
}

base::string16 CardUnmaskPromptViews::GetWindowTitle() const {
  return controller_->GetWindowTitle();
}

void CardUnmaskPromptViews::DeleteDelegate() {
  delete this;
}

int CardUnmaskPromptViews::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK | ui::DIALOG_BUTTON_CANCEL;
}

base::string16 CardUnmaskPromptViews::GetDialogButtonLabel(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_OK)
    return l10n_util::GetStringUTF16(IDS_AUTOFILL_CARD_UNMASK_CONFIRM_BUTTON);

  return DialogDelegateView::GetDialogButtonLabel(button);
}

bool CardUnmaskPromptViews::ShouldDefaultButtonBeBlue() const {
  return true;
}

bool CardUnmaskPromptViews::IsDialogButtonEnabled(
    ui::DialogButton button) const {
  if (button == ui::DIALOG_BUTTON_CANCEL)
    return true;

  DCHECK_EQ(ui::DIALOG_BUTTON_OK, button);

  return cvc_input_->enabled() &&
         controller_->InputCvcIsValid(cvc_input_->text()) &&
         ExpirationDateIsValid();
}

views::View* CardUnmaskPromptViews::GetInitiallyFocusedView() {
  return cvc_input_;
}

bool CardUnmaskPromptViews::Cancel() {
  return true;
}

bool CardUnmaskPromptViews::Accept() {
  if (!controller_)
    return true;

  controller_->OnUnmaskResponse(
      cvc_input_->text(),
      month_input_ ? month_input_->GetTextForRow(month_input_->selected_index())
                   : base::string16(),
      year_input_ ? year_input_->GetTextForRow(year_input_->selected_index())
                  : base::string16(),
      storage_checkbox_ ? storage_checkbox_->checked() : false);
  return false;
}

void CardUnmaskPromptViews::ContentsChanged(
    views::Textfield* sender,
    const base::string16& new_contents) {
  if (controller_->InputCvcIsValid(new_contents))
    cvc_input_->SetInvalid(false);

  GetDialogClientView()->UpdateDialogButtons();
}

void CardUnmaskPromptViews::OnPerformAction(views::Combobox* combobox) {
  if (ExpirationDateIsValid()) {
    if (month_input_->invalid()) {
      month_input_->SetInvalid(false);
      year_input_->SetInvalid(false);
      SetRetriableErrorMessage(base::string16());
    }
  } else if (month_input_->selected_index() !=
                 month_combobox_model_.GetDefaultIndex() &&
             year_input_->selected_index() !=
                 year_combobox_model_.GetDefaultIndex()) {
    month_input_->SetInvalid(true);
    year_input_->SetInvalid(true);
    SetRetriableErrorMessage(l10n_util::GetStringUTF16(
        IDS_AUTOFILL_CARD_UNMASK_INVALID_EXPIRATION_DATE));
  }

  GetDialogClientView()->UpdateDialogButtons();
}

void CardUnmaskPromptViews::AnimationProgressed(
    const gfx::Animation* animation) {
  progress_overlay_->SetOpacity(animation->GetCurrentValue());
  storage_row_->SetOpacity(1.0 - animation->GetCurrentValue());
}

void CardUnmaskPromptViews::InitIfNecessary() {
  if (has_children())
    return;

  main_contents_ = new views::View();
  main_contents_->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kVertical, 0, 0, 12));
  AddChildView(main_contents_);

  permanent_error_label_ = new views::Label();
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();
  permanent_error_label_->SetFontList(
      rb.GetFontList(ui::ResourceBundle::BoldFont));
  permanent_error_label_->set_background(
      views::Background::CreateSolidBackground(kWarningColor));
  permanent_error_label_->SetBorder(
      views::Border::CreateEmptyBorder(12, kEdgePadding, 12, kEdgePadding));
  permanent_error_label_->SetEnabledColor(SK_ColorWHITE);
  permanent_error_label_->SetAutoColorReadabilityEnabled(false);
  permanent_error_label_->SetVisible(false);
  permanent_error_label_->SetMultiLine(true);
  permanent_error_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  main_contents_->AddChildView(permanent_error_label_);

  views::View* controls_container = new views::View();
  controls_container->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kVertical, kEdgePadding, 0, 0));
  main_contents_->AddChildView(controls_container);

  views::Label* instructions =
      new views::Label(controller_->GetInstructionsMessage());
  instructions->SetEnabledColor(kGreyTextColor);
  instructions->SetMultiLine(true);
  instructions->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  instructions->SetBorder(views::Border::CreateEmptyBorder(0, 0, 16, 0));
  controls_container->AddChildView(instructions);

  input_row_ = new views::View();
  input_row_->SetLayoutManager(
      new views::BoxLayout(views::BoxLayout::kHorizontal, 0, 0, 5));
  controls_container->AddChildView(input_row_);

  if (controller_->ShouldRequestExpirationDate()) {
    month_input_ = new views::Combobox(&month_combobox_model_);
    month_input_->set_listener(this);
    input_row_->AddChildView(month_input_);
    views::Label* separator = new views::Label(l10n_util::GetStringUTF16(
        IDS_AUTOFILL_CARD_UNMASK_EXPIRATION_DATE_SEPARATOR));
    separator->SetEnabledColor(kGreyTextColor);
    input_row_->AddChildView(separator);
    year_input_ = new views::Combobox(&year_combobox_model_);
    year_input_->set_listener(this);
    input_row_->AddChildView(year_input_);
    input_row_->AddChildView(new views::Label(base::ASCIIToUTF16("  ")));
  }

  cvc_input_ = new DecoratedTextfield(
      base::string16(),
      l10n_util::GetStringUTF16(IDS_AUTOFILL_DIALOG_PLACEHOLDER_CVC), this);
  cvc_input_->set_default_width_in_chars(8);
  input_row_->AddChildView(cvc_input_);

  views::ImageView* cvc_image = new views::ImageView();
  cvc_image->SetImage(rb.GetImageSkiaNamed(controller_->GetCvcImageRid()));
  input_row_->AddChildView(cvc_image);

  views::View* temporary_error = new views::View();
  views::BoxLayout* temporary_error_layout =
      new views::BoxLayout(views::BoxLayout::kHorizontal, 0, 0, 4);
  temporary_error->SetLayoutManager(temporary_error_layout);
  temporary_error_layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_START);
  temporary_error->SetBorder(views::Border::CreateEmptyBorder(8, 0, 0, 0));
  controls_container->AddChildView(temporary_error);

  error_icon_ = new views::ImageView();
  error_icon_->SetVisible(false);
  error_icon_->SetImage(ui::ResourceBundle::GetSharedInstance()
                            .GetImageNamed(IDR_ALERT_RED)
                            .ToImageSkia());
  temporary_error->AddChildView(error_icon_);

  // Reserve vertical space for the error label, assuming it's one line.
  error_label_ = new views::Label(base::ASCIIToUTF16(" "));
  error_label_->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  error_label_->SetEnabledColor(kWarningColor);
  temporary_error->AddChildView(error_label_);
  temporary_error_layout->SetFlexForView(error_label_, 1);

  progress_overlay_ = new FadeOutView();
  progress_overlay_->set_fade_everything(true);
  views::BoxLayout* progress_layout =
      new views::BoxLayout(views::BoxLayout::kHorizontal, 0, 0, 5);
  progress_layout->set_cross_axis_alignment(
      views::BoxLayout::CROSS_AXIS_ALIGNMENT_CENTER);
  progress_layout->set_main_axis_alignment(
      views::BoxLayout::MAIN_AXIS_ALIGNMENT_CENTER);
  progress_overlay_->SetLayoutManager(progress_layout);

  progress_overlay_->SetVisible(false);
  AddChildView(progress_overlay_);

  progress_throbber_ = new views::CheckmarkThrobber();
  progress_overlay_->AddChildView(progress_throbber_);

  progress_label_ = new views::Label(l10n_util::GetStringUTF16(
      IDS_AUTOFILL_CARD_UNMASK_VERIFICATION_IN_PROGRESS));
  // Material blue. TODO(estade): find an appropriate place for this color.
  progress_label_->SetEnabledColor(SkColorSetRGB(0x42, 0x85, 0xF4));
  progress_overlay_->AddChildView(progress_label_);
}

bool CardUnmaskPromptViews::ExpirationDateIsValid() const {
  if (!controller_->ShouldRequestExpirationDate())
    return true;

  return controller_->InputExpirationIsValid(
      month_input_->GetTextForRow(month_input_->selected_index()),
      year_input_->GetTextForRow(year_input_->selected_index()));
}

void CardUnmaskPromptViews::ClosePrompt() {
  GetWidget()->Close();
}

CardUnmaskPromptViews::FadeOutView::FadeOutView()
    : fade_everything_(false), opacity_(1.0) {
}
CardUnmaskPromptViews::FadeOutView::~FadeOutView() {
}

void CardUnmaskPromptViews::FadeOutView::PaintChildren(
    const ui::PaintContext& context) {
  uint8_t alpha = static_cast<uint8_t>(255 * opacity_);
  ui::CompositingRecorder recorder(context, alpha);
  views::View::PaintChildren(context);
}

void CardUnmaskPromptViews::FadeOutView::OnPaint(gfx::Canvas* canvas) {
  if (!fade_everything_ || opacity_ > 0.99)
    return views::View::OnPaint(canvas);

  canvas->SaveLayerAlpha(0xff * opacity_);
  views::View::OnPaint(canvas);
  canvas->Restore();
}

void CardUnmaskPromptViews::FadeOutView::SetOpacity(double opacity) {
  opacity_ = opacity;
  SchedulePaint();
}

}  // namespace autofill
