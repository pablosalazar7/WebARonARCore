// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_
#pragma once

#include <deque>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/process_util.h"
#include "base/property_bag.h"
#include "base/string16.h"
#include "base/timer.h"
#include "build/build_config.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/common/page_zoom.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/gfx/native_widget_types.h"

class BackingStore;
class MockRenderWidgetHost;
class WebCursor;
struct EditCommand;
struct ViewHostMsg_UpdateRect_Params;

namespace base {
class TimeTicks;
}

namespace ui {
class Range;
}

namespace WebKit {
class WebInputEvent;
class WebMouseEvent;
struct WebCompositionUnderline;
struct WebScreenInfo;
}

namespace content {

class RenderWidgetHostViewPort;

// This implements the RenderWidgetHost interface that is exposed to
// embedders of content, and adds things only visible to content.
class CONTENT_EXPORT RenderWidgetHostImpl : virtual public RenderWidgetHost,
                                            public IPC::Channel::Listener {
 public:
  // routing_id can be MSG_ROUTING_NONE, in which case the next available
  // routing id is taken from the RenderProcessHost.
  RenderWidgetHostImpl(RenderProcessHost* process, int routing_id);
  virtual ~RenderWidgetHostImpl();

  // Use RenderWidgetHostImpl::From(rwh) to downcast a
  // RenderWidgetHost to a RenderWidgetHostImpl.  Internally, this
  // uses RenderWidgetHost::AsRenderWidgetHostImpl().
  static RenderWidgetHostImpl* From(RenderWidgetHost* rwh);

  // RenderWidgetHost implementation.
  virtual void Undo() OVERRIDE;
  virtual void Redo() OVERRIDE;
  virtual void Cut() OVERRIDE;
  virtual void Copy() OVERRIDE;
  virtual void CopyToFindPboard() OVERRIDE;
  virtual void Paste() OVERRIDE;
  virtual void PasteAndMatchStyle() OVERRIDE;
  virtual void Delete() OVERRIDE;
  virtual void SelectAll() OVERRIDE;
  virtual void UpdateTextDirection(WebKit::WebTextDirection direction) OVERRIDE;
  virtual void NotifyTextDirection() OVERRIDE;
  virtual void Blur() OVERRIDE;
  virtual bool CopyFromBackingStore(const gfx::Rect& src_rect,
                                    const gfx::Size& accelerated_dest_size,
                                    skia::PlatformCanvas* output) OVERRIDE;
#if defined(TOOLKIT_GTK)
  virtual bool CopyFromBackingStoreToGtkWindow(const gfx::Rect& dest_rect,
                                               GdkWindow* target) OVERRIDE;
#elif defined(OS_MACOSX)
  virtual gfx::Size GetBackingStoreSize() OVERRIDE;
  virtual bool CopyFromBackingStoreToCGContext(const CGRect& dest_rect,
                                               CGContextRef target) OVERRIDE;
#endif
  virtual void EnableRendererAccessibility() OVERRIDE;
  virtual void ForwardMouseEvent(
      const WebKit::WebMouseEvent& mouse_event) OVERRIDE;
  virtual void ForwardWheelEvent(
      const WebKit::WebMouseWheelEvent& wheel_event) OVERRIDE;
  virtual void ForwardKeyboardEvent(
      const NativeWebKeyboardEvent& key_event) OVERRIDE;
  virtual const gfx::Point& GetLastScrollOffset() const OVERRIDE;
  virtual RenderProcessHost* GetProcess() const OVERRIDE;
  virtual int GetRoutingID() const OVERRIDE;
  virtual RenderWidgetHostView* GetView() const OVERRIDE;
  virtual bool IsRenderView() const OVERRIDE;
  virtual void PaintAtSize(TransportDIB::Handle dib_handle,
                           int tag,
                           const gfx::Size& page_size,
                           const gfx::Size& desired_size) OVERRIDE;
  virtual void Replace(const string16& word) OVERRIDE;
  virtual void ResizeRectChanged(const gfx::Rect& new_rect) OVERRIDE;
  virtual void RestartHangMonitorTimeout() OVERRIDE;
  virtual void SetIgnoreInputEvents(bool ignore_input_events) OVERRIDE;
  virtual void Stop() OVERRIDE;
  virtual void WasResized() OVERRIDE;
  virtual bool OnMessageReceivedForTesting(const IPC::Message& msg) OVERRIDE;
  virtual void AddKeyboardListener(KeyboardListener* listener) OVERRIDE;
  virtual void RemoveKeyboardListener(KeyboardListener* listener) OVERRIDE;

  // Sets the View of this RenderWidgetHost.
  void SetView(RenderWidgetHostView* view);

  int surface_id() const { return surface_id_; }
  bool renderer_accessible() { return renderer_accessible_; }

  bool empty() const { return current_size_.IsEmpty(); }

  // Returns the property bag for this widget, where callers can add extra data
  // they may wish to associate with it. Returns a pointer rather than a
  // reference since the PropertyAccessors expect this.
  const base::PropertyBag* property_bag() const { return &property_bag_; }
  base::PropertyBag* property_bag() { return &property_bag_; }

  // Called when a renderer object already been created for this host, and we
  // just need to be attached to it. Used for window.open, <select> dropdown
  // menus, and other times when the renderer initiates creating an object.
  void Init();

  // Tells the renderer to die and then calls Destroy().
  virtual void Shutdown();

  // IPC::Channel::Listener
  virtual bool OnMessageReceived(const IPC::Message& msg) OVERRIDE;

  // Sends a message to the corresponding object in the renderer.
  virtual bool Send(IPC::Message* msg) OVERRIDE;

  // Called to notify the RenderWidget that it has been hidden or restored from
  // having been hidden.
  void WasHidden();
  void WasRestored();

  // Called to notify the RenderWidget that its associated native window got
  // focused.
  virtual void GotFocus();

  // Tells the renderer it got/lost focus.
  void Focus();
  virtual void LostCapture();

  // Sets whether the renderer should show controls in an active state.  On all
  // platforms except mac, that's the same as focused. On mac, the frontmost
  // window will show active controls even if the focus is not in the web
  // contents, but e.g. in the omnibox.
  void SetActive(bool active);

  // Called to notify the RenderWidget that it has lost the mouse lock.
  virtual void LostMouseLock();

  // Tells us whether the page is rendered directly via the GPU process.
  bool is_accelerated_compositing_active() {
    return is_accelerated_compositing_active_;
  }

  // Notifies the RenderWidgetHost that the View was destroyed.
  void ViewDestroyed();

  // Indicates if the page has finished loading.
  void SetIsLoading(bool is_loading);

  // Get access to the widget's backing store.  If a resize is in progress,
  // then the current size of the backing store may be less than the size of
  // the widget's view.  If you pass |force_create| as true, then the backing
  // store will be created if it doesn't exist. Otherwise, NULL will be returned
  // if the backing store doesn't already exist. It will also return NULL if the
  // backing store could not be created.
  BackingStore* GetBackingStore(bool force_create);

  // Allocate a new backing store of the given size. Returns NULL on failure
  // (for example, if we don't currently have a RenderWidgetHostView.)
  BackingStore* AllocBackingStore(const gfx::Size& size);

  // When a backing store does asynchronous painting, it will call this function
  // when it is done with the DIB. We will then forward a message to the
  // renderer to send another paint.
  void DonePaintingToBackingStore();

  // GPU accelerated version of GetBackingStore function. This will
  // trigger a re-composite to the view. If a resize is pending, it will
  // block briefly waiting for an ack from the renderer.
  void ScheduleComposite();

  // Starts a hang monitor timeout. If there's already a hang monitor timeout
  // the new one will only fire if it has a shorter delay than the time
  // left on the existing timeouts.
  void StartHangMonitorTimeout(base::TimeDelta delay);

  // Stops all existing hang monitor timeouts and assumes the renderer is
  // responsive.
  void StopHangMonitorTimeout();

  // Forwards the given message to the renderer. These are called by the view
  // when it has received a message.
  void ForwardGestureEvent(const WebKit::WebGestureEvent& gesture_event);
  virtual void ForwardTouchEvent(const WebKit::WebTouchEvent& touch_event);

#if defined(TOOLKIT_USES_GTK)
  // Give key press listeners a chance to handle this key press. This allow
  // widgets that don't have focus to still handle key presses.
  bool KeyPressListenersHandleEvent(GdkEventKey* event);
#endif  // defined(TOOLKIT_USES_GTK)

  void CancelUpdateTextDirection();

  // Called when a mouse click activates the renderer.
  virtual void OnMouseActivate();

  // Notifies the renderer whether or not the input method attached to this
  // process is activated.
  // When the input method is activated, a renderer process sends IPC messages
  // to notify the status of its composition node. (This message is mainly used
  // for notifying the position of the input cursor so that the browser can
  // display input method windows under the cursor.)
  void SetInputMethodActive(bool activate);

  // Update the composition node of the renderer (or WebKit).
  // WebKit has a special node (a composition node) for input method to change
  // its text without affecting any other DOM nodes. When the input method
  // (attached to the browser) updates its text, the browser sends IPC messages
  // to update the composition node of the renderer.
  // (Read the comments of each function for its detail.)

  // Sets the text of the composition node.
  // This function can also update the cursor position and mark the specified
  // range in the composition node.
  // A browser should call this function:
  // * when it receives a WM_IME_COMPOSITION message with a GCS_COMPSTR flag
  //   (on Windows);
  // * when it receives a "preedit_changed" signal of GtkIMContext (on Linux);
  // * when markedText of NSTextInput is called (on Mac).
  void ImeSetComposition(
      const string16& text,
      const std::vector<WebKit::WebCompositionUnderline>& underlines,
      int selection_start,
      int selection_end);

  // Finishes an ongoing composition with the specified text.
  // A browser should call this function:
  // * when it receives a WM_IME_COMPOSITION message with a GCS_RESULTSTR flag
  //   (on Windows);
  // * when it receives a "commit" signal of GtkIMContext (on Linux);
  // * when insertText of NSTextInput is called (on Mac).
  void ImeConfirmComposition(const string16& text);
  void ImeConfirmComposition(const string16& text,
                             const ui::Range& replacement_range);

  // Finishes an ongoing composition with the composition text set by last
  // SetComposition() call.
  void ImeConfirmComposition();

  // Cancels an ongoing composition.
  void ImeCancelComposition();

  // This is for derived classes to give us access to the resizer rect.
  // And to also expose it to the RenderWidgetHostView.
  virtual gfx::Rect GetRootWindowResizerRect() const;

  bool ignore_input_events() const {
    return ignore_input_events_;
  }

  // Activate deferred plugin handles.
  void ActivateDeferredPluginHandles();

  bool has_touch_handler() const { return has_touch_handler_; }

  // Notification that the user has made some kind of input that could
  // perform an action. See OnUserGesture for more details.
  void StartUserGesture();

  // Set the RenderView background.
  void SetBackground(const SkBitmap& background);

  // Notifies the renderer that the next key event is bound to one or more
  // pre-defined edit commands
  void SetEditCommandsForNextKeyEvent(
      const std::vector<EditCommand>& commands);

  // Relay a request from assistive technology to perform the default action
  // on a given node.
  void AccessibilityDoDefaultAction(int object_id);

  // Relay a request from assistive technology to set focus to a given node.
  void AccessibilitySetFocus(int object_id);

  // Relay a request from assistive technology to make a given object
  // visible by scrolling as many scrollable containers as necessary.
  // In addition, if it's not possible to make the entire object visible,
  // scroll so that the |subfocus| rect is visible at least. The subfocus
  // rect is in local coordinates of the object itself.
  void AccessibilityScrollToMakeVisible(
      int acc_obj_id, gfx::Rect subfocus);

  // Relay a request from assistive technology to move a given object
  // to a specific location, in the tab content area coordinate space, i.e.
  // (0, 0) is the top-left corner of the tab contents.
  void AccessibilityScrollToPoint(int acc_obj_id, gfx::Point point);

  // Relay a request from assistive technology to set text selection.
  void AccessibilitySetTextSelection(
      int acc_obj_id, int start_offset, int end_offset);

  // Executes the edit command on the RenderView.
  void ExecuteEditCommand(const std::string& command,
                          const std::string& value);

  // Tells the renderer to scroll the currently focused node into rect only if
  // the currently focused node is a Text node (textfield, text area or content
  // editable divs).
  void ScrollFocusedEditableNodeIntoRect(const gfx::Rect& rect);

  // Requests the renderer to select the region between two points.
  void SelectRange(const gfx::Point& start, const gfx::Point& end);

  // Called when the reponse to a pending mouse lock request has arrived.
  // Returns true if |allowed| is true and the mouse has been successfully
  // locked.
  bool GotResponseToLockMouseRequest(bool allowed);

  // Called by the view in response to AcceleratedSurfaceBuffersSwapped.
  static void AcknowledgeSwapBuffers(int32 route_id, int gpu_host_id);
  static void AcknowledgePostSubBuffer(int32 route_id, int gpu_host_id);

  // Signals that the compositing surface was updated, e.g. after a lost context
  // event.
  void CompositingSurfaceUpdated();

 protected:
  virtual RenderWidgetHostImpl* AsRenderWidgetHostImpl() OVERRIDE;

  // Internal implementation of the public Forward*Event() methods.
  void ForwardInputEvent(const WebKit::WebInputEvent& input_event,
                         int event_size, bool is_keyboard_shortcut);

  // Called when we receive a notification indicating that the renderer
  // process has gone. This will reset our state so that our state will be
  // consistent if a new renderer is created.
  void RendererExited(base::TerminationStatus status, int exit_code);

  // Retrieves an id the renderer can use to refer to its view.
  // This is used for various IPC messages, including plugins.
  gfx::NativeViewId GetNativeViewId() const;

  // Retrieves an id for the surface that the renderer can draw to
  // when accelerated compositing is enabled.
  gfx::GLSurfaceHandle GetCompositingSurface();

  // Called to handled a keyboard event before sending it to the renderer.
  // This is overridden by RenderViewHost to send upwards to its delegate.
  // Returns true if the event was handled, and then the keyboard event will
  // not be sent to the renderer anymore. Otherwise, if the |event| would
  // be handled in HandleKeyboardEvent() method as a normal keyboard shortcut,
  // |*is_keyboard_shortcut| should be set to true.
  virtual bool PreHandleKeyboardEvent(const NativeWebKeyboardEvent& event,
                                      bool* is_keyboard_shortcut);

  // "RenderWidgetHostDelegate" ------------------------------------------------
  // There is no RenderWidgetHostDelegate but the following methods serve the
  // same purpose. They are overridden by RenderViewHost to send upwards to its
  // delegate.

  // Called when a keyboard event was not processed by the renderer.
  virtual void UnhandledKeyboardEvent(const NativeWebKeyboardEvent& event) {}

  // Called when a mousewheel event was not processed by the renderer.
  virtual void UnhandledWheelEvent(const WebKit::WebMouseWheelEvent& event) {}

  // Notification that the user has made some kind of input that could
  // perform an action. The gestures that count are 1) any mouse down
  // event and 2) enter or space key presses.
  virtual void OnUserGesture() {}

  // Callbacks for notification when the renderer becomes unresponsive to user
  // input events, and subsequently responsive again.
  virtual void NotifyRendererUnresponsive() {}
  virtual void NotifyRendererResponsive() {}

  // Called when auto-resize resulted in the renderer size changing.
  virtual void OnRenderAutoResized(const gfx::Size& new_size) {}

  // ---------------------------------------------------------------------------

  // RenderViewHost overrides this method to impose further restrictions on when
  // to allow mouse lock.
  // Once the request is approved or rejected, GotResponseToLockMouseRequest()
  // will be called.
  virtual void RequestToLockMouse();

  void RejectMouseLockOrUnlockIfNecessary();
  bool IsMouseLocked() const;

  // RenderViewHost overrides this method to report when in fullscreen mode.
  virtual bool IsFullscreen() const;

  // Indicates if the render widget host should track the render widget's size
  // as opposed to visa versa.
  void SetShouldAutoResize(bool enable);

 protected:
  // The View associated with the RenderViewHost. The lifetime of this object
  // is associated with the lifetime of the Render process. If the Renderer
  // crashes, its View is destroyed and this pointer becomes NULL, even though
  // render_view_host_ lives on to load another URL (creating a new View while
  // doing so).
  RenderWidgetHostViewPort* view_;

  // true if a renderer has once been valid. We use this flag to display a sad
  // tab only when we lose our renderer and not if a paint occurs during
  // initialization.
  bool renderer_initialized_;

  // This value indicates how long to wait before we consider a renderer hung.
  int hung_renderer_delay_ms_;

 private:
  friend class ::MockRenderWidgetHost;

  // Tell this object to destroy itself.
  void Destroy();

  // Checks whether the renderer is hung and calls NotifyRendererUnresponsive
  // if it is.
  void CheckRendererIsUnresponsive();

  // Called if we know the renderer is responsive. When we currently think the
  // renderer is unresponsive, this will clear that state and call
  // NotifyRendererResponsive.
  void RendererIsResponsive();

  // IPC message handlers
  void OnMsgRenderViewReady();
  void OnMsgRenderViewGone(int status, int error_code);
  void OnMsgClose();
  void OnMsgRequestMove(const gfx::Rect& pos);
  void OnMsgSetTooltipText(const string16& tooltip_text,
                           WebKit::WebTextDirection text_direction_hint);
  void OnMsgPaintAtSizeAck(int tag, const gfx::Size& size);
  void OnMsgUpdateRect(const ViewHostMsg_UpdateRect_Params& params);
  void OnMsgUpdateIsDelayed();
  void OnMsgInputEventAck(WebKit::WebInputEvent::Type event_type,
                          bool processed);
  virtual void OnMsgFocus();
  virtual void OnMsgBlur();
  void OnMsgDidChangeNumTouchEvents(int count);

  void OnMsgSetCursor(const WebCursor& cursor);
  void OnMsgTextInputStateChanged(ui::TextInputType type,
                                  bool can_compose_inline);
  void OnMsgImeCompositionRangeChanged(const ui::Range& range);
  void OnMsgImeCancelComposition();

  void OnMsgDidActivateAcceleratedCompositing(bool activated);

  void OnMsgLockMouse();
  void OnMsgUnlockMouse();

#if defined(OS_POSIX) || defined(USE_AURA)
  void OnMsgGetWindowRect(gfx::NativeViewId window_id, gfx::Rect* results);
  void OnMsgGetRootWindowRect(gfx::NativeViewId window_id, gfx::Rect* results);
#endif
#if defined(OS_MACOSX)
  void OnMsgPluginFocusChanged(bool focused, int plugin_id);
  void OnMsgStartPluginIme();
  void OnAllocateFakePluginWindowHandle(bool opaque,
                                        bool root,
                                        gfx::PluginWindowHandle* id);
  void OnDestroyFakePluginWindowHandle(gfx::PluginWindowHandle id);
  void OnAcceleratedSurfaceSetIOSurface(gfx::PluginWindowHandle window,
                                        int32 width,
                                        int32 height,
                                        uint64 mach_port);
  void OnAcceleratedSurfaceSetTransportDIB(gfx::PluginWindowHandle window,
                                           int32 width,
                                           int32 height,
                                           TransportDIB::Handle transport_dib);
  void OnAcceleratedSurfaceBuffersSwapped(gfx::PluginWindowHandle window,
                                          uint64 surface_handle);
#endif
#if defined(TOOLKIT_USES_GTK)
  void OnMsgCreatePluginContainer(gfx::PluginWindowHandle id);
  void OnMsgDestroyPluginContainer(gfx::PluginWindowHandle id);
#endif

  // Called (either immediately or asynchronously) after we're done with our
  // BackingStore and can send an ACK to the renderer so it can paint onto it
  // again.
  void DidUpdateBackingStore(const ViewHostMsg_UpdateRect_Params& params,
                             const base::TimeTicks& paint_start);

  // Paints the given bitmap to the current backing store at the given
  // location.  Returns true if the passed callback was asynchronously
  // scheduled in the future (and thus the caller must manually synchronously
  // call the callback function).
  bool PaintBackingStoreRect(TransportDIB::Id bitmap,
                             const gfx::Rect& bitmap_rect,
                             const std::vector<gfx::Rect>& copy_rects,
                             const gfx::Size& view_size,
                             const base::Closure& completion_callback);

  // Scrolls the given |clip_rect| in the backing by the given dx/dy amount. The
  // |dib| and its corresponding location |bitmap_rect| in the backing store
  // is the newly painted pixels by the renderer.
  void ScrollBackingStoreRect(int dx, int dy, const gfx::Rect& clip_rect,
                              const gfx::Size& view_size);

  // Called by OnMsgInputEventAck() to process a keyboard event ack message.
  void ProcessKeyboardEventAck(int type, bool processed);

  // Called by OnMsgInputEventAck() to process a wheel event ack message.
  // This could result in a task being posted to allow additional wheel
  // input messages to be coalesced.
  void ProcessWheelAck(bool processed);

  // Called on OnMsgInputEventAck() to process a touch event ack message.
  // This can result in a gesture event being generated and sent back to the
  // renderer.
  void ProcessTouchAck(WebKit::WebInputEvent::Type type, bool processed);

  // Called when there is a new auto resize (using a post to avoid a stack
  // which may get in recursive loops).
  void DelayedAutoResized();

  // Created during construction but initialized during Init*(). Therefore, it
  // is guaranteed never to be NULL, but its channel may be NULL if the
  // renderer crashed, so you must always check that.
  RenderProcessHost* process_;

  // The ID of the corresponding object in the Renderer Instance.
  int routing_id_;

  // True if renderer accessibility is enabled. This should only be set when a
  // screenreader is detected as it can potentially slow down Chrome.
  bool renderer_accessible_;

  // Stores random bits of data for others to associate with this object.
  base::PropertyBag property_bag_;

  // The ID of the surface corresponding to this render widget.
  int surface_id_;

  // Indicates whether a page is loading or not.
  bool is_loading_;

  // Indicates whether a page is hidden or not.
  bool is_hidden_;

  // Indicates whether a page is fullscreen or not.
  bool is_fullscreen_;

  // True when a page is rendered directly via the GPU process.
  bool is_accelerated_compositing_active_;

  // Set if we are waiting for a repaint ack for the view.
  bool repaint_ack_pending_;

  // True when waiting for RESIZE_ACK.
  bool resize_ack_pending_;

  // The current size of the RenderWidget.
  gfx::Size current_size_;

  // The size we last sent as requested size to the renderer. |current_size_|
  // is only updated once the resize message has been ack'd. This on the other
  // hand is updated when the resize message is sent. This is very similar to
  // |resize_ack_pending_|, but the latter is not set if the new size has width
  // or height zero, which is why we need this too.
  gfx::Size in_flight_size_;

  // The next auto resize to send.
  gfx::Size new_auto_size_;

  // True if the render widget host should track the render widget's size as
  // opposed to visa versa.
  bool should_auto_resize_;

  // True if a mouse move event was sent to the render view and we are waiting
  // for a corresponding ViewHostMsg_HandleInputEvent_ACK message.
  bool mouse_move_pending_;

  // The next mouse move event to send (only non-null while mouse_move_pending_
  // is true).
  scoped_ptr<WebKit::WebMouseEvent> next_mouse_move_;

  // (Similar to |mouse_move_pending_|.) True if a mouse wheel event was sent
  // and we are waiting for a corresponding ack.
  bool mouse_wheel_pending_;
  WebKit::WebMouseWheelEvent current_wheel_event_;

  typedef std::deque<WebKit::WebMouseWheelEvent> WheelEventQueue;

  // (Similar to |next_mouse_move_|.) The next mouse wheel events to send.
  // Unlike mouse moves, mouse wheel events received while one is pending are
  // coalesced (by accumulating deltas) if they match the previous event in
  // modifiers. On the Mac, in particular, mouse wheel events are received at a
  // high rate; not waiting for the ack results in jankiness, and using the same
  // mechanism as for mouse moves (just dropping old events when multiple ones
  // would be queued) results in very slow scrolling.
  WheelEventQueue coalesced_mouse_wheel_events_;

  // The time when an input event was sent to the RenderWidget.
  base::TimeTicks input_event_start_time_;

  // Keyboard event listeners.
  std::list<KeyboardListener*> keyboard_listeners_;

  // If true, then we should repaint when restoring even if we have a
  // backingstore.  This flag is set to true if we receive a paint message
  // while is_hidden_ to true.  Even though we tell the render widget to hide
  // itself, a paint message could already be in flight at that point.
  bool needs_repainting_on_restore_;

  // This is true if the renderer is currently unresponsive.
  bool is_unresponsive_;

  // The following value indicates a time in the future when we would consider
  // the renderer hung if it does not generate an appropriate response message.
  base::Time time_when_considered_hung_;

  // This value denotes the number of input events yet to be acknowledged
  // by the renderer.
  int in_flight_event_count_;

  // This timer runs to check if time_when_considered_hung_ has past.
  base::OneShotTimer<RenderWidgetHostImpl> hung_renderer_timer_;

  // Flag to detect recursive calls to GetBackingStore().
  bool in_get_backing_store_;

  // Set when we call DidPaintRect/DidScrollRect on the view.
  bool view_being_painted_;

  // Used for UMA histogram logging to measure the time for a repaint view
  // operation to finish.
  base::TimeTicks repaint_start_time_;

  // Queue of keyboard events that we need to track.
  typedef std::deque<NativeWebKeyboardEvent> KeyQueue;

  // A queue of keyboard events. We can't trust data from the renderer so we
  // stuff key events into a queue and pop them out on ACK, feeding our copy
  // back to whatever unhandled handler instead of the returned version.
  KeyQueue key_queue_;

  // Set to true if we shouldn't send input events from the render widget.
  bool ignore_input_events_;

  // Set when we update the text direction of the selected input element.
  bool text_direction_updated_;
  WebKit::WebTextDirection text_direction_;

  // Set when we cancel updating the text direction.
  // This flag also ignores succeeding update requests until we call
  // NotifyTextDirection().
  bool text_direction_canceled_;

  // Indicates if the next sequence of Char events should be suppressed or not.
  // System may translate a RawKeyDown event into zero or more Char events,
  // usually we send them to the renderer directly in sequence. However, If a
  // RawKeyDown event was not handled by the renderer but was handled by
  // our UnhandledKeyboardEvent() method, e.g. as an accelerator key, then we
  // shall not send the following sequence of Char events, which was generated
  // by this RawKeyDown event, to the renderer. Otherwise the renderer may
  // handle the Char events and cause unexpected behavior.
  // For example, pressing alt-2 may let the browser switch to the second tab,
  // but the Char event generated by alt-2 may also activate a HTML element
  // if its accesskey happens to be "2", then the user may get confused when
  // switching back to the original tab, because the content may already be
  // changed.
  bool suppress_next_char_events_;

  std::vector<gfx::PluginWindowHandle> deferred_plugin_handles_;

  // The last scroll offset of the render widget.
  gfx::Point last_scroll_offset_;

  bool pending_mouse_lock_request_;

  // Keeps track of whether the webpage has any touch event handler. If it does,
  // then touch events are sent to the renderer. Otherwise, the touch events are
  // not sent to the renderer.
  bool has_touch_handler_;

  base::WeakPtrFactory<RenderWidgetHostImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(RenderWidgetHostImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_RENDER_WIDGET_HOST_IMPL_H_
