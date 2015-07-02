// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @typedef {function(!Array<string>):!Promise} */
var LaunchHandler;

/**
 * Root class of the background page.
 * @constructor
 * @struct
 */
function BackgroundBase() {
  /**
   * Map of all currently open app windows. The key is an app ID.
   * @type {Object<chrome.app.window.AppWindow>}
   */
  this.appWindows = {};

  /**
   * Map of all currently open file dialogs. The key is an app ID.
   * @type {!Object<!Window>}
   */
  this.dialogs = {};

  // Initializes the strings. This needs for the volume manager.
  var loadTimeDataPromise = new Promise(function(fulfill, reject) {
    chrome.fileManagerPrivate.getStrings(function(stringData) {
      loadTimeData.data = stringData;
      fulfill(stringData);
    });
  });

  // Volume Manager should be initialized after strings because volume name is
  // localized.
  this.volumeManager_ = null;
  this.initializationPromise_ = loadTimeDataPromise.then(function(strings) {
    return VolumeManager.getInstance().then(function(vm) {
      this.volumeManager_ = vm;
      return strings;
    }.bind(this));
  }.bind(this));

  /** @private {?LaunchHandler} */
  this.launchHandler_ = null;

  // Initialize handlers.
  chrome.app.runtime.onLaunched.addListener(this.onLaunched_.bind(this));
  chrome.app.runtime.onRestarted.addListener(this.onRestarted_.bind(this));
}

/**
 * Gets similar windows, it means with the same initial url.
 * @param {string} url URL that the obtained windows have.
 * @return {Array<chrome.app.window.AppWindow>} List of similar windows.
 */
BackgroundBase.prototype.getSimilarWindows = function(url) {
  var result = [];
  for (var appID in this.appWindows) {
    if (this.appWindows[appID].contentWindow.appInitialURL === url)
      result.push(this.appWindows[appID]);
  }
  return result;
};

/**
 * Called when an app is launched.
 *
 * @param {!Object} launchData Launch data. See the manual of chrome.app.runtime
 *     .onLaunched for detail.
 */
BackgroundBase.prototype.onLaunched_ = function(launchData) {
  // Skip if files are not selected.
  if (!launchData || !launchData.items || launchData.items.length == 0)
    return;

  this.initializationPromise_.then(function() {
    var isolatedEntries = launchData.items.map(function(item) {
      return item.entry;
    });

    // Obtains entries in non-isolated file systems.
    // The entries in launchData are stored in the isolated file system.
    // We need to map the isolated entries to the normal entries to retrieve
    // their parent directory.
    chrome.fileManagerPrivate.resolveIsolatedEntries(
        isolatedEntries,
        function(externalEntries) {
          var urls = util.entriesToURLs(externalEntries);
          if (this.launchHandler_)
            this.launchHandler_(urls);
        }.bind(this));
  }.bind(this));
};

/**
 * Set a handler which is called when an app is launched.
 * @param {!LaunchHandler} handler Function to be called.
 */
BackgroundBase.prototype.setLaunchHandler = function(handler) {
  this.launchHandler_ = handler;
};

/**
 * Called when an app is restarted.
 */
BackgroundBase.prototype.onRestarted_ = function() {
};
