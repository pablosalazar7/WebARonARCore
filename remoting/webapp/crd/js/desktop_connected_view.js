// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Class handling user-facing aspects of the client session.
 */

'use strict';

/** @suppress {duplicate} */
var remoting = remoting || {};

/**
 * @param {HTMLElement} container
 * @param {remoting.ConnectionInfo} connectionInfo
 * @constructor
 * @extends {base.EventSourceImpl}
 * @implements {base.Disposable}
 */
remoting.DesktopConnectedView = function(container, connectionInfo) {

  /** @private {HTMLElement} */
  this.container_ = container;

  /** @private {remoting.ClientPlugin} */
  this.plugin_ = connectionInfo.plugin();

  /** @private {remoting.ClientSession} */
  this.session_ = connectionInfo.session();

  /** @private */
  this.host_ = connectionInfo.host();

  /** @private */
  this.mode_ = connectionInfo.mode();

  /** @private {remoting.DesktopViewport} */
  this.viewport_ = null;

  /** private {remoting.ConnectedView} */
  this.view_ = null;

  /** @private {remoting.VideoFrameRecorder} */
  this.videoFrameRecorder_ = null;

  /** private {base.Disposable} */
  this.eventHooks_ = null;

  this.initPlugin_();
  this.initUI_();
};

/** @return {void} Nothing. */
remoting.DesktopConnectedView.prototype.dispose = function() {
  if (remoting.windowFrame) {
    remoting.windowFrame.setDesktopConnectedView(null);
  }
  if (remoting.toolbar) {
    remoting.toolbar.setDesktopConnectedView(null);
  }
  if (remoting.optionsMenu) {
    remoting.optionsMenu.setDesktopConnectedView(null);
  }

  document.body.classList.remove('connected');

  base.dispose(this.eventHooks_);
  this.eventHooks_ = null;

  base.dispose(this.viewport_);
  this.viewport_ = null;
};

// The mode of this session.
/** @enum {number} */
remoting.DesktopConnectedView.Mode = {
  IT2ME: 0,
  ME2ME: 1,
  APP_REMOTING: 2
};

// Keys for per-host settings.
remoting.DesktopConnectedView.KEY_REMAP_KEYS = 'remapKeys';
remoting.DesktopConnectedView.KEY_RESIZE_TO_CLIENT = 'resizeToClient';
remoting.DesktopConnectedView.KEY_SHRINK_TO_FIT = 'shrinkToFit';
remoting.DesktopConnectedView.KEY_DESKTOP_SCALE = 'desktopScale';

/**
 * Get host display name.
 *
 * @return {string}
 */
remoting.DesktopConnectedView.prototype.getHostDisplayName = function() {
  return this.host_.hostName;
};

/**
 * @return {remoting.DesktopConnectedView.Mode} The current state.
 */
remoting.DesktopConnectedView.prototype.getMode = function() {
  return this.mode_;
};

/**
 * @return {boolean} True if shrink-to-fit is enabled; false otherwise.
 */
remoting.DesktopConnectedView.prototype.getShrinkToFit = function() {
  if (this.viewport_) {
    return this.viewport_.getShrinkToFit();
  }
  return false;
};

/**
 * @return {boolean} True if resize-to-client is enabled; false otherwise.
 */
remoting.DesktopConnectedView.prototype.getResizeToClient = function() {
  if (this.viewport_) {
    return this.viewport_.getResizeToClient();
  }
  return false;
};

/**
 * @return {Element} The element that should host the plugin.
 * @private
 */
remoting.DesktopConnectedView.prototype.getPluginContainer_ = function() {
  return this.container_.querySelector('.client-plugin-container');
};

/** @return {remoting.DesktopViewport} */
remoting.DesktopConnectedView.prototype.getViewportForTesting = function() {
  return this.viewport_;
};

/** @private */
remoting.DesktopConnectedView.prototype.initPlugin_ = function() {
  // Show the Send Keys menu only if the plugin has the injectKeyEvent feature,
  // and the Ctrl-Alt-Del button only in Me2Me mode.
  if (!this.plugin_.hasFeature(
          remoting.ClientPlugin.Feature.INJECT_KEY_EVENT)) {
    var sendKeysElement = document.getElementById('send-keys-menu');
    sendKeysElement.hidden = true;
  } else if (this.mode_ != remoting.DesktopConnectedView.Mode.ME2ME &&
      this.mode_ != remoting.DesktopConnectedView.Mode.APP_REMOTING) {
    var sendCadElement = document.getElementById('send-ctrl-alt-del');
    sendCadElement.hidden = true;
  }
};

/**
 * This is a callback that gets called when the window is resized.
 *
 * @return {void} Nothing.
 * @private.
 */
remoting.DesktopConnectedView.prototype.onResize_ = function() {
  if (this.viewport_) {
    this.viewport_.onResize();
  }
};

/** @private */
remoting.DesktopConnectedView.prototype.initUI_ = function() {
  document.body.classList.add('connected');

  this.view_ = new remoting.ConnectedView(
      this.plugin_, this.container_,
      this.container_.querySelector('.mouse-cursor-overlay'));

  var scrollerElement = document.getElementById('scroller');
  this.viewport_ = new remoting.DesktopViewport(
      scrollerElement || document.body,
      this.plugin_.hostDesktop(),
      this.host_.options);

  if (remoting.windowFrame) {
    remoting.windowFrame.setDesktopConnectedView(this);
  }
  if (remoting.toolbar) {
    remoting.toolbar.setDesktopConnectedView(this);
  }
  if (remoting.optionsMenu) {
    remoting.optionsMenu.setDesktopConnectedView(this);
  }

  // Activate full-screen related UX.
  this.eventHooks_ = new base.Disposables(
    this.view_,
    new base.EventHook(this.session_,
                       remoting.ClientSession.Events.videoChannelStateChanged,
                       this.view_.onConnectionReady.bind(this.view_)),
    new base.DomEventHook(window, 'resize', this.onResize_.bind(this), false),
    new remoting.Fullscreen.EventHook(this.onFullScreenChanged_.bind(this)));
  this.onFullScreenChanged_(remoting.fullscreen.isActive());
};

/**
 * Set the shrink-to-fit and resize-to-client flags and save them if this is
 * a Me2Me connection.
 *
 * @param {boolean} shrinkToFit True if the remote desktop should be scaled
 *     down if it is larger than the client window; false if scroll-bars
 *     should be added in this case.
 * @param {boolean} resizeToClient True if window resizes should cause the
 *     host to attempt to resize its desktop to match the client window size;
 *     false to disable this behaviour for subsequent window resizes--the
 *     current host desktop size is not restored in this case.
 * @return {void} Nothing.
 */
remoting.DesktopConnectedView.prototype.setScreenMode =
    function(shrinkToFit, resizeToClient) {
  this.viewport_.setScreenMode(shrinkToFit, resizeToClient);
};

/**
 * Called when the full-screen status has changed, either via the
 * remoting.Fullscreen class, or via a system event such as the Escape key
 *
 * @param {boolean=} fullscreen True if the app is entering full-screen mode;
 *     false if it is leaving it.
 * @private
 */
remoting.DesktopConnectedView.prototype.onFullScreenChanged_ = function (
    fullscreen) {
  if (this.viewport_) {
    // When a window goes full-screen, a resize event is triggered, but the
    // Fullscreen.isActive call is not guaranteed to return true until the
    // full-screen event is triggered. In apps v2, the size of the window's
    // client area is calculated differently in full-screen mode, so register
    // for both events.
    this.viewport_.onResize();
    this.viewport_.enableBumpScroll(Boolean(fullscreen));
  }
};

/**
 * Sends a Ctrl-Alt-Del sequence to the remoting client.
 *
 * @return {void} Nothing.
 */
remoting.DesktopConnectedView.prototype.sendCtrlAltDel = function() {
  console.log('Sending Ctrl-Alt-Del.');
  this.plugin_.injectKeyCombination([0x0700e0, 0x0700e2, 0x07004c]);
};

/**
 * Sends a Print Screen keypress to the remoting client.
 *
 * @return {void} Nothing.
 */
remoting.DesktopConnectedView.prototype.sendPrintScreen = function() {
  console.log('Sending Print Screen.');
  this.plugin_.injectKeyCombination([0x070046]);
};

/** @param {remoting.VideoFrameRecorder} recorder */
remoting.DesktopConnectedView.prototype.setVideoFrameRecorder =
    function(recorder) {
  this.videoFrameRecorder_ = recorder;
};

/**
 * Returns true if the ClientSession can record video frames to a file.
 * @return {boolean}
 */
remoting.DesktopConnectedView.prototype.canRecordVideo = function() {
  return !!this.videoFrameRecorder_;
};

/**
 * Returns true if the ClientSession is currently recording video frames.
 * @return {boolean}
 */
remoting.DesktopConnectedView.prototype.isRecordingVideo = function() {
  if (!this.videoFrameRecorder_) {
    return false;
  }
  return this.videoFrameRecorder_.isRecording();
};

/**
 * Starts or stops recording of video frames.
 */
remoting.DesktopConnectedView.prototype.startStopRecording = function() {
  if (this.videoFrameRecorder_) {
    this.videoFrameRecorder_.startStopRecording();
  }
};
