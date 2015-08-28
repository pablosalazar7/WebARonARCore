// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview This file contains the |MockFeedback| class which is a
 * combined mock class for speech and braille feedback.  A test that uses
 * this class may add expectations for speech utterances and braille display
 * content to be output.  The |install| method sets appropriate mock classes
 * as the |cvox.ChromeVox.tts| and |cvox.ChromeVox.braille| objects,
 * respectively.  Output sent to those objects will then be collected in
 * an internal queue.
 *
 * Expectations can be added using the |expectSpeech| and |expectBraille|
 * methods.  These methods take either strings or regular expressions to match
 * against.  Strings must match a full utterance (or display content) exactly,
 * while a regular expression must match a substring (use anchor operators if
 * needed).
 *
 * Function calls may be inserted in the stream of expectations using the
 * |call| method.  Such callbacks are called after all preceding expectations
 * have been met, and before any further expectations are matched.  Callbacks
 * are called in the order they were added to the mock.
 *
 * The |replay| method starts processing any pending utterances and braille
 * display content and will try to match expectations as new feedback enters
 * the queue asynchronously.  When all expectations have been met and callbacks
 * called, the finish callback, if any was provided to the constructor, is
 * called.
 *
 * This mock class is lean, meaning that feedback that doesn't match
 * any expectations is silently ignored.
 *
 * NOTE: for asynchronous tests, the processing will never finish if there
 * are unmet expectations.  To help debugging in such situations, the mock
 * will output its pending state if there are pending expectations and no
 * output is received within a few seconds.
 *
 * See mock_feedback_test.js for example usage of this class.
 */

/**
 * Combined mock class for braille and speech output.
 * @param {function=} opt_finishedCallback Called when all expectations have
 *     been met.
 * @constructor
 */
var MockFeedback = function(opt_finishedCallback) {
  /**
   * @type {function}
   * @private
   */
  this.finishedCallback_ = opt_finishedCallback || null;
  /**
   * True when |replay| has been called and actions are being replayed.
   * @type {boolean}
   * @private
   */
  this.replaying_ = false;
  /**
   * True when inside the |process| function to prevent nested calls.
   * @type {boolean}
   * @private
   */
  this.inProcess_ = false;
  /**
   * Pending expectations and callbacks.
   * @type {Array<{perform: function(): boolean, toString: function(): string}>}
   * @private
   */
  this.pendingActions_ = [];
  /**
   * Pending speech utterances.
   * @type {Array<{text: string, callback: (function|undefined)}>}
   * @private
   */
  this.pendingUtterances_ = [];
  /**
   * Pending braille output.
   * @type {Array<{text: string, callback: (function|undefined)}>}
   * @private
   */
  this.pendingBraille_ = [];
  /**
   * Handle for the timeout set for debug logging.
   * @type {number}
   * @private
   */
  this.logTimeoutId_ = 0;
  /**
   * @type {cvox.NavBraille}
   * @private
   */
  this.lastMatchedBraille_ = null;
};

MockFeedback.prototype = {

  /**
   * Install mock objects as |cvox.ChromeVox.tts| and |cvox.ChromeVox.braille|
   * to collect feedback.
   */
  install: function() {
    assertFalse(this.replaying_);

    var MockTts = function() {};
    MockTts.prototype = {
      __proto__: cvox.TtsInterface.prototype,
      speak: this.addUtterance_.bind(this)
    };

    cvox.ChromeVox.tts = new MockTts();

    var MockBraille = function() {};
    MockBraille.prototype = {
      __proto__: cvox.BrailleInterface.prototype,
      write: this.addBraille_.bind(this)
    };

    cvox.ChromeVox.braille = new MockBraille();
  },

  /**
   * Adds an expectation for one or more spoken utterances.
   * @param {...(string|RegExp)} var_args One or more utterance to add as
   *     expectations.
   * @return {MockFeedback} |this| for chaining
   */
  expectSpeech: function() {
    assertFalse(this.replaying_);
    Array.prototype.forEach.call(arguments, function(text) {
      this.pendingActions_.push({
        perform: function() {
          return !!MockFeedback.matchAndConsume_(
              text, {}, this.pendingUtterances_);
        }.bind(this),
        toString: function() { return 'Speak \'' + text + '\''; }
      });
    }.bind(this));
    return this;
  },

  /**
   * Adds an expectation for braille output.
   * @param {string|RegExp} text
   * @param {Object=} opt_props Additional properties to match in the
   *     |NavBraille|
   * @return {MockFeedback} |this| for chaining
   */
  expectBraille: function(text, opt_props) {
    assertFalse(this.replaying_);
    var props = opt_props || {};
    this.pendingActions_.push({
      perform: function() {
        var match = MockFeedback.matchAndConsume_(
            text, props, this.pendingBraille_);
        if (match)
          this.lastMatchedBraille_ = match;
        return !!match;
      }.bind(this),
      toString: function() {
        return 'Braille \'' + text + '\' ' + JSON.stringify(props);
      }
    });
    return this;
  },

  /**
   * Arranges for a callback to be invoked when all expectations that were
   * added before this call have been met.  Callbacks are called in the
   * order they are added.
   * @param {Function} callback
   * @return {MockFeedback} |this| for chaining
   */
  call: function(callback) {
    assertFalse(this.replaying_);
    this.pendingActions_.push({
      perform: function() {
        callback();
        return true;
      },
      toString: function() {
        return 'Callback';
      }
    });
    return this;
  },

  /**
   * Processes any feedback that has been received so far and treis to
   * satisfy the registered expectations.  Any feedback that is received
   * after this call (via the installed mock objects) is processed immediately.
   * When all expectations are satisfied and registered callbacks called,
   * the finish callbcak, if any, is called.
   * This function may only be called once.
   */
  replay: function() {
    assertFalse(this.replaying_);
    this.replaying_ = true;
    this.process_();
  },

  /**
   * Returns the |NavBraille| that matched an expectation.  This is
   * intended to be used by a callback to invoke braille commands that
   * depend on display contents.
   * @type {cvox.NavBraille}
   */
  get lastMatchedBraille() {
    assertTrue(this.replaying_);
    return this.lastMatchedBraille_;
  },

  /**
   * @param {string} textString
   * @param {cvox.QueueMode} queueMode
   * @param {Object=} properties
   * @private
   */
  addUtterance_: function(textString, queueMode, properties) {
    var callback;
    if (properties && (properties.startCallback || properties.endCallback)) {
      var startCallback = properties.startCallback;
      var endCallback = properties.endCallback;
      callback = function() {
        startCallback && startCallback();
        endCallback && endCallback();
      };
    }
    this.pendingUtterances_.push(
        {text: textString,
         callback: callback});
    this.process_();
  },

  /** @private */
  addBraille_: function(navBraille) {
    this.pendingBraille_.push(navBraille);
    this.process_();
  },

  /*** @private */
  process_: function() {
    if (!this.replaying_ || this.inProcess_)
      return;
    try {
      this.inProcess_ = true;
      while (this.pendingActions_.length > 0) {
        var action = this.pendingActions_[0];
        if (action.perform()) {
          this.pendingActions_.shift();
          if (this.logTimeoutId_) {
            window.clearTimeout(this.logTimeoutId_);
            this.logTimeoutId_ = 0;
          }
        } else {
          break;
        }
      }
      if (this.pendingActions_.length == 0) {
        if (this.finishedCallback_) {
          this.finishedCallback_();
          this.finishedCallback_ = null;
        }
      } else {
        // If there are pending actions and no matching feedback for a few
        // seconds, log the pending state to ease debugging.
        if (!this.logTimeoutId_) {
          this.logTimeoutId_ = window.setTimeout(
              this.logPendingState_.bind(this), 2000);
        }
      }
    } finally {
      this.inProcess_ = false;
    }
  },

  /** @private */
  logPendingState_: function() {
    if (this.pendingActions_.length > 0)
      console.log('Still waiting for ' + this.pendingActions_[0].toString());
    function logPending(desc, list) {
      if (list.length > 0)
        console.log('Pending ' + desc + ':\n  ' +
            list.map(function(i) {
              var ret = '\'' + i.text + '\'';
              if ('startIndex' in i)
                ret += ' startIndex=' + i.startIndex;
              if ('endIndex' in i)
                ret += ' endIndex=' + i.endIndex;
              return ret;
            }).join('\n  ') + '\n  ');
    }
    logPending('speech utterances', this.pendingUtterances_);
    logPending('braille', this.pendingBraille_);
    this.logTimeoutId_ = 0;
  },
};

/**
 * @param {string} text
 * @param {Object} props
 * @param {Array<{text: (string|RegExp), callback: (function|undefined)}>}
 *     pending
 * @return {Object}
 * @private
 */
MockFeedback.matchAndConsume_ = function(text, props, pending) {
  for (var i = 0, candidate; candidate = pending[i]; ++i) {
    var candidateText = candidate.text.toString();
    if (text === candidateText ||
        (text instanceof RegExp && text.test(candidateText))) {
      var matched = true;
      for (prop in props) {
        if (candidate[prop] !== props[prop]) {
          matched = false;
          break;
        }
      }
      if (matched)
        break;
    }
  }
  if (candidate) {
    var consumed = pending.splice(0, i + 1);
    consumed.forEach(function(item) {
      if (item.callback)
        item.callback();
    });
  }
  return candidate;
};
