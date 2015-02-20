// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Interface abstracting the functionality of the HostDesktop.
 */

var remoting = remoting || {};

(function() {

'use strict';

/**
 * @interface
 * @extends {base.EventSource}
 */
remoting.HostDesktop = function() {};

/** @return {boolean} Whether the host supports desktop resizing. */
remoting.HostDesktop.prototype.isResizable = function() {};

/** @enum {string} */
remoting.HostDesktop.Events = {
  // Fired when the size of the host desktop changes with the desktop dimensions
  //  {{width:number, height:number, xDpi:number, yDpi:number}}
  sizeChanged: 'sizeChanged',
  // Fired when the shape of the host desktop changes with an array of
  // rectangles of desktop shapes as the event data.
  //   Array<{left:number, top:number, width:number, height:number}>
  shapeChanged: 'shapeChanged'
};

/**
 * @return {{width:number, height:number, xDpi:number, yDpi:number}}
 *  The dimensions and DPI settings of the host desktop.
 */
remoting.HostDesktop.prototype.getDimensions = function() {};

/**
 * Resize the desktop of the host to |width|, |height| and |deviceScale|.
 *
 * @param {number} width The width of the desktop in DIPs.
 * @param {number} height The height of the desktop in DIPs.
 * @param {number} deviceScale
 */
remoting.HostDesktop.prototype.resize = function(width, height, deviceScale) {};

})();
