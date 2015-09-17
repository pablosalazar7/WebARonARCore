// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTIONS_BAR_H_
#define CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTIONS_BAR_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/scoped_vector.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_bar_bubble_delegate.h"
#include "chrome/browser/ui/toolbar/toolbar_actions_model.h"
#include "ui/gfx/animation/tween.h"
#include "ui/gfx/geometry/size.h"

namespace extensions {
class Extension;
class ExtensionMessageBubbleController;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

class ToolbarActionsBarDelegate;
class ToolbarActionViewController;

// A platform-independent version of the container for toolbar actions,
// including extension actions and component actions.
//
// This is a per-window instance, unlike the ToolbarActionsModel, which is
// per-profile. In most cases, ordering and visible count will be identical
// between the base model and the window; however, there are exceptions in the
// case of very small windows (which may be too narrow to display all the
// icons), or windows in which an action is "popped out", resulting in a
// re-ordering.
//
// This can come in two flavors, main and "overflow". The main bar is visible
// next to the omnibox, and the overflow bar is visible inside the chrome
// (fka wrench) menu. The main bar can have only a single row of icons with
// flexible width, whereas the overflow bar has multiple rows of icons with a
// fixed width (the width of the menu).
class ToolbarActionsBar : public ToolbarActionsModel::Observer {
 public:
  // A struct to contain the platform settings.
  struct PlatformSettings {
    explicit PlatformSettings(bool in_overflow_mode);

    // The padding that comes before the first icon in the container.
    int left_padding;
    // The padding following the final icon in the container.
    int right_padding;
    // The spacing between each of the icons.
    int item_spacing;
    // The number of icons per row in the overflow menu.
    int icons_per_overflow_menu_row;
    // Whether or not the overflow menu is displayed as a chevron (this is being
    // phased out).
    bool chevron_enabled;
  };

  // The type of drag that occurred in a drag-and-drop operation.
  enum DragType {
    // The icon was dragged to the same container it started in.
    DRAG_TO_SAME,
    // The icon was dragged from the main container to the overflow.
    DRAG_TO_OVERFLOW,
    // The icon was dragged from the overflow container to the main.
    DRAG_TO_MAIN,
  };

  enum HighlightType {
    HIGHLIGHT_NONE,
    HIGHLIGHT_INFO,
    HIGHLIGHT_WARNING,
  };

  ToolbarActionsBar(ToolbarActionsBarDelegate* delegate,
                    Browser* browser,
                    ToolbarActionsBar* main_bar);
  ~ToolbarActionsBar() override;

  // Returns the width of a browser action icon, optionally including the
  // following padding.
  static int IconWidth(bool include_padding);

  // Returns the height of a browser action icon.
  static int IconHeight();

  // Registers profile preferences.
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns the preferred size for the toolbar; this does *not* reflect any
  // animations that may be running.
  gfx::Size GetPreferredSize() const;

  // Returns the [minimum|maximum] possible width for the toolbar.
  int GetMinimumWidth() const;
  int GetMaximumWidth() const;

  // Returns the width for the given number of icons.
  int IconCountToWidth(int icons) const;

  // Returns the number of icons that can fit within the given width.
  size_t WidthToIconCount(int width) const;

  // Returns the number of icons that should be displayed if space allows.
  size_t GetIconCount() const;

  // Returns the starting index (inclusive) for displayable icons.
  size_t GetStartIndexInBounds() const;

  // Returns the ending index (exclusive) for displayable icons.
  size_t GetEndIndexInBounds() const;

  // Returns true if an overflow container is necessary to display any other
  // icons for this particular window. This is different than
  // ToolbarActionsModel::all_icons_visible() because the ToolbarActionsBar
  // is limited to a single window, whereas the model is the underlying model
  // of *all* windows, independent of size. As such, the model is identical
  // between a very wide window and a very narrow window, and the user's stored
  // preference may be to have all icons visible. But if the very narrow window
  // doesn't have the width to display all those actions, some will need to be
  // implicitly pushed to the overflow, even though the user's global preference
  // has not changed.
  bool NeedsOverflow() const;

  // Returns the frame (bounds) that the specified index should have, taking
  // into account if this is the main or overflow bar. If this is the overflow
  // bar and the index should not be displayed (i.e., it is shown on the main
  // bar), returns an empty rect.
  gfx::Rect GetFrameForIndex(size_t index) const;

  // Returns the actions in the proper order; this may differ from the
  // underlying order in the case of actions being popped out to show a popup.
  std::vector<ToolbarActionViewController*> GetActions() const;

  // Creates the toolbar actions.
  void CreateActions();

  // Deletes all toolbar actions.
  void DeleteActions();

  // Updates all the toolbar actions.
  void Update();

  // Shows the popup for the action with |id|, returning true if a popup is
  // shown. If |grant_active_tab| is true, then active tab permissions should
  // be given to the action (only do this if this is through a user action).
  bool ShowToolbarActionPopup(const std::string& id, bool grant_active_tab);

  // Sets the width for the overflow menu rows.
  void SetOverflowRowWidth(int width);

  // Notifies the ToolbarActionsBar that a user completed a resize gesture, and
  // the new width is |width|.
  void OnResizeComplete(int width);

  // Notifies the ToolbarActionsBar that a user completed a drag and drop event,
  // and dragged the view from |dragged_index| to |dropped_index|.
  // |drag_type| indicates whether or not the icon was dragged between the
  // overflow and main containers.
  // The main container should handle all drag/drop notifications.
  void OnDragDrop(int dragged_index,
                  int dropped_index,
                  DragType drag_type);

  // Notifies the ToolbarActionsBar that the delegate finished animating.
  void OnAnimationEnded();

  // Returns true if the given |action| is visible on the main toolbar.
  bool IsActionVisibleOnMainBar(const ToolbarActionViewController* action)
      const;

  // Pops out a given |action|, ensuring it is visible.
  // |closure| will be called once any animation is complete.
  void PopOutAction(ToolbarActionViewController* action,
                    const base::Closure& closure);

  // Undoes the current "pop out"; i.e., moves the popped out action back into
  // overflow.
  void UndoPopOut();

  // Sets the active popup owner to be |popup_owner|.
  void SetPopupOwner(ToolbarActionViewController* popup_owner);

  // Hides the actively showing popup, if any.
  void HideActivePopup();

  // Returns the main (i.e., not overflow) controller for the given action.
  ToolbarActionViewController* GetMainControllerForAction(
      ToolbarActionViewController* action);

  // Returns the underlying toolbar actions, but does not order them. Primarily
  // for use in testing.
  const std::vector<ToolbarActionViewController*>& toolbar_actions_unordered()
      const {
    return toolbar_actions_.get();
  }
  bool enabled() const { return model_ != nullptr; }
  bool suppress_layout() const { return suppress_layout_; }
  bool suppress_animation() const {
    return suppress_animation_ || disable_animations_for_testing_;
  }
  bool is_highlighting() const { return model_ && model_->is_highlighting(); }
  ToolbarActionsModel::HighlightType highlight_type() const {
    return model_ ? model_->highlight_type()
                  : ToolbarActionsModel::HIGHLIGHT_NONE;
  }
  const PlatformSettings& platform_settings() const {
    return platform_settings_;
  }
  ToolbarActionViewController* popup_owner() { return popup_owner_; }
  ToolbarActionViewController* popped_out_action() {
    return popped_out_action_;
  }
  bool in_overflow_mode() const { return main_bar_ != nullptr; }

  ToolbarActionsBarDelegate* delegate_for_test() { return delegate_; }

  // During testing we can disable animations by setting this flag to true,
  // so that the bar resizes instantly, instead of having to poll it while it
  // animates to open/closed status.
  static bool disable_animations_for_testing_;

 private:
  using ToolbarActions = ScopedVector<ToolbarActionViewController>;

  // ToolbarActionsModel::Observer:
  void OnToolbarActionAdded(const std::string& action_id, int index) override;
  void OnToolbarActionRemoved(const std::string& action_id) override;
  void OnToolbarActionMoved(const std::string& action_id, int index) override;
  void OnToolbarActionUpdated(const std::string& action_id) override;
  void OnToolbarVisibleCountChanged() override;
  void OnToolbarHighlightModeChanged(bool is_highlighting) override;
  void OnToolbarModelInitialized() override;

  // Resizes the delegate (if necessary) to the preferred size using the given
  // |tween_type| and optionally suppressing the chevron.
  void ResizeDelegate(gfx::Tween::Type tween_type, bool suppress_chevron);

  // Returns the action for the given |id|, if one exists.
  ToolbarActionViewController* GetActionForId(const std::string& action_id);

  // Returns the current web contents.
  content::WebContents* GetCurrentWebContents();

  // Reorders the toolbar actions to reflect the model and, optionally, to
  // "pop out" any overflowed actions that want to run (depending on the
  // value of |pop_out_actions_to_run|.
  void ReorderActions();

  // Shows an extension message bubble, if any should be shown.
  void MaybeShowExtensionBubble(
      scoped_ptr<extensions::ExtensionMessageBubbleController> controller);

  // The delegate for this object (in a real build, this is the view).
  ToolbarActionsBarDelegate* delegate_;

  // The associated browser.
  Browser* browser_;

  // The observed toolbar model.
  ToolbarActionsModel* model_;

  // The controller for the main toolbar actions bar. This will be null if this
  // is the main bar.
  ToolbarActionsBar* main_bar_;

  // Platform-specific settings for dimensions and the overflow chevron.
  PlatformSettings platform_settings_;

  // The toolbar actions.
  ToolbarActions toolbar_actions_;

  // The action that triggered the current popup (just a reference to an action
  // from toolbar_actions_).
  ToolbarActionViewController* popup_owner_;

  ScopedObserver<ToolbarActionsModel, ToolbarActionsModel::Observer>
      model_observer_;

  // True if we should suppress layout, such as when we are creating or
  // adjusting a lot of actions at once.
  bool suppress_layout_;

  // True if we should suppress animation; we do this when first creating the
  // toolbar, and also when switching tabs changes the state of the icons.
  bool suppress_animation_;

  // If this is true, actions that want to run (e.g., an extension's page
  // action) will pop out of overflow to draw more attention.
  // See also TabOrderHelper in the .cc file.
  static bool pop_out_actions_to_run_;

  // True if we have checked to see if there is an extension bubble that should
  // be displayed, and, if there is, shown that bubble.
  bool checked_extension_bubble_;

  // The action, if any, which is currently "popped out" of the overflow in
  // order to show a popup.
  ToolbarActionViewController* popped_out_action_;

  // The task to alert the |popped_out_action_| that animation has finished, and
  // it is fully popped out.
  base::Closure popped_out_closure_;

  // The controller of the bubble to show once animation finishes, if any.
  scoped_ptr<extensions::ExtensionMessageBubbleController>
      pending_extension_bubble_controller_;

  base::WeakPtrFactory<ToolbarActionsBar> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ToolbarActionsBar);
};

#endif  // CHROME_BROWSER_UI_TOOLBAR_TOOLBAR_ACTIONS_BAR_H_
