// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_WIDGET_NATIVE_WIDGET_AURA_H_
#define UI_VIEWS_WIDGET_NATIVE_WIDGET_AURA_H_

#include "base/memory/weak_ptr.h"
#include "ui/aura/client/focus_change_observer.h"
#include "ui/aura/window_delegate.h"
#include "ui/aura/window_observer.h"
#include "ui/base/cursor/cursor.h"
#include "ui/events/event_constants.h"
#include "ui/views/views_export.h"
#include "ui/views/widget/native_widget_private.h"
#include "ui/wm/public/activation_change_observer.h"
#include "ui/wm/public/activation_delegate.h"
#include "ui/wm/public/drag_drop_delegate.h"

namespace aura {
class Window;
}
namespace gfx {
class FontList;
}

namespace views {

class DropHelper;
class TooltipManagerAura;
class WindowReorderer;

class VIEWS_EXPORT NativeWidgetAura
    : public internal::NativeWidgetPrivate,
      public aura::WindowDelegate,
      public aura::WindowObserver,
      public aura::client::ActivationDelegate,
      public aura::client::ActivationChangeObserver,
      public aura::client::FocusChangeObserver,
      public aura::client::DragDropDelegate {
 public:
  explicit NativeWidgetAura(internal::NativeWidgetDelegate* delegate);

  // Called internally by NativeWidgetAura and DesktopNativeWidgetAura to
  // associate |native_widget| with |window|.
  static void RegisterNativeWidgetForWindow(
      internal::NativeWidgetPrivate* native_widget,
      aura::Window* window);

  // Overridden from internal::NativeWidgetPrivate:
  void InitNativeWidget(const Widget::InitParams& params) override;
  NonClientFrameView* CreateNonClientFrameView() override;
  bool ShouldUseNativeFrame() const override;
  bool ShouldWindowContentsBeTransparent() const override;
  void FrameTypeChanged() override;
  Widget* GetWidget() override;
  const Widget* GetWidget() const override;
  gfx::NativeView GetNativeView() const override;
  gfx::NativeWindow GetNativeWindow() const override;
  Widget* GetTopLevelWidget() override;
  const ui::Compositor* GetCompositor() const override;
  const ui::Layer* GetLayer() const override;
  void ReorderNativeViews() override;
  void ViewRemoved(View* view) override;
  void SetNativeWindowProperty(const char* name, void* value) override;
  void* GetNativeWindowProperty(const char* name) const override;
  TooltipManager* GetTooltipManager() const override;
  void SetCapture() override;
  void ReleaseCapture() override;
  bool HasCapture() const override;
  ui::InputMethod* GetInputMethod() override;
  void CenterWindow(const gfx::Size& size) override;
  void GetWindowPlacement(gfx::Rect* bounds,
                          ui::WindowShowState* maximized) const override;
  bool SetWindowTitle(const base::string16& title) override;
  void SetWindowIcons(const gfx::ImageSkia& window_icon,
                      const gfx::ImageSkia& app_icon) override;
  void InitModalType(ui::ModalType modal_type) override;
  gfx::Rect GetWindowBoundsInScreen() const override;
  gfx::Rect GetClientAreaBoundsInScreen() const override;
  gfx::Rect GetRestoredBounds() const override;
  void SetBounds(const gfx::Rect& bounds) override;
  void SetSize(const gfx::Size& size) override;
  void StackAbove(gfx::NativeView native_view) override;
  void StackAtTop() override;
  void StackBelow(gfx::NativeView native_view) override;
  void SetShape(SkRegion* shape) override;
  void Close() override;
  void CloseNow() override;
  void Show() override;
  void Hide() override;
  void ShowMaximizedWithBounds(const gfx::Rect& restored_bounds) override;
  void ShowWithWindowState(ui::WindowShowState state) override;
  bool IsVisible() const override;
  void Activate() override;
  void Deactivate() override;
  bool IsActive() const override;
  void SetAlwaysOnTop(bool always_on_top) override;
  bool IsAlwaysOnTop() const override;
  void SetVisibleOnAllWorkspaces(bool always_visible) override;
  void Maximize() override;
  void Minimize() override;
  bool IsMaximized() const override;
  bool IsMinimized() const override;
  void Restore() override;
  void SetFullscreen(bool fullscreen) override;
  bool IsFullscreen() const override;
  void SetOpacity(unsigned char opacity) override;
  void SetUseDragFrame(bool use_drag_frame) override;
  void FlashFrame(bool flash_frame) override;
  void RunShellDrag(View* view,
                    const ui::OSExchangeData& data,
                    const gfx::Point& location,
                    int operation,
                    ui::DragDropTypes::DragEventSource source) override;
  void SchedulePaintInRect(const gfx::Rect& rect) override;
  void SetCursor(gfx::NativeCursor cursor) override;
  bool IsMouseEventsEnabled() const override;
  void ClearNativeFocus() override;
  gfx::Rect GetWorkAreaBoundsInScreen() const override;
  Widget::MoveLoopResult RunMoveLoop(
      const gfx::Vector2d& drag_offset,
      Widget::MoveLoopSource source,
      Widget::MoveLoopEscapeBehavior escape_behavior) override;
  void EndMoveLoop() override;
  void SetVisibilityChangedAnimationsEnabled(bool value) override;
  void SetVisibilityAnimationDuration(const base::TimeDelta& duration) override;
  void SetVisibilityAnimationTransition(
      Widget::VisibilityTransition transition) override;
  ui::NativeTheme* GetNativeTheme() const override;
  void OnRootViewLayout() override;
  bool IsTranslucentWindowOpacitySupported() const override;
  void OnSizeConstraintsChanged() override;
  void RepostNativeEvent(gfx::NativeEvent native_event) override;

  // Overridden from aura::WindowDelegate:
  gfx::Size GetMinimumSize() const override;
  gfx::Size GetMaximumSize() const override;
  void OnBoundsChanged(const gfx::Rect& old_bounds,
                       const gfx::Rect& new_bounds) override;
  gfx::NativeCursor GetCursor(const gfx::Point& point) override;
  int GetNonClientComponent(const gfx::Point& point) const override;
  bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) override;
  bool CanFocus() override;
  void OnCaptureLost() override;
  void OnPaint(const ui::PaintContext& context) override;
  void OnDeviceScaleFactorChanged(float device_scale_factor) override;
  void OnWindowDestroying(aura::Window* window) override;
  void OnWindowDestroyed(aura::Window* window) override;
  void OnWindowTargetVisibilityChanged(bool visible) override;
  bool HasHitTestMask() const override;
  void GetHitTestMask(gfx::Path* mask) const override;

  // Overridden from aura::WindowObserver:
  void OnWindowPropertyChanged(aura::Window* window,
                               const void* key,
                               intptr_t old) override;

  // Overridden from ui::EventHandler:
  void OnKeyEvent(ui::KeyEvent* event) override;
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnScrollEvent(ui::ScrollEvent* event) override;
  void OnGestureEvent(ui::GestureEvent* event) override;

  // Overridden from aura::client::ActivationDelegate:
  bool ShouldActivate() const override;

  // Overridden from aura::client::ActivationChangeObserver:
  void OnWindowActivated(
      aura::client::ActivationChangeObserver::ActivationReason reason,
      aura::Window* gained_active,
      aura::Window* lost_active) override;

  // Overridden from aura::client::FocusChangeObserver:
  void OnWindowFocused(aura::Window* gained_focus,
                       aura::Window* lost_focus) override;

  // Overridden from aura::client::DragDropDelegate:
  void OnDragEntered(const ui::DropTargetEvent& event) override;
  int OnDragUpdated(const ui::DropTargetEvent& event) override;
  void OnDragExited() override;
  int OnPerformDrop(const ui::DropTargetEvent& event) override;

 protected:
  ~NativeWidgetAura() override;

  internal::NativeWidgetDelegate* delegate() { return delegate_; }

 private:
  class ActiveWindowObserver;

  bool IsDocked() const;
  void SetInitialFocus(ui::WindowShowState show_state);

  internal::NativeWidgetDelegate* delegate_;

  // WARNING: set to NULL when destroyed. As the Widget is not necessarily
  // destroyed along with |window_| all usage of |window_| should first verify
  // non-NULL.
  aura::Window* window_;

  // See class documentation for Widget in widget.h for a note about ownership.
  Widget::InitParams::Ownership ownership_;

  // Are we in the destructor?
  bool destroying_;

  gfx::NativeCursor cursor_;

  // The saved window state for exiting full screen state.
  ui::WindowShowState saved_window_state_;

  scoped_ptr<TooltipManagerAura> tooltip_manager_;

  // Reorders child windows of |window_| associated with a view based on the
  // order of the associated views in the widget's view hierarchy.
  scoped_ptr<WindowReorderer> window_reorderer_;

  scoped_ptr<DropHelper> drop_helper_;
  int last_drop_operation_;

  // The following factory is used for calls to close the NativeWidgetAura
  // instance.
  base::WeakPtrFactory<NativeWidgetAura> close_widget_factory_;

  DISALLOW_COPY_AND_ASSIGN(NativeWidgetAura);
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_NATIVE_WIDGET_AURA_H_
