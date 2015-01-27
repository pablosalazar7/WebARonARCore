// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var mobileNav = false;

/**
 * For small screen mobile the navigation buttons are moved
 * below the advanced text.
 */
function onResize() {
  var helpOuterBox = document.querySelector('#details');
  var mainContent = document.querySelector('#main-content');
  var mediaQuery = '(max-width: 420px) and (orientation: portrait),' +
      '(max-width: 736px) and (max-height: 420px) and (orientation: landscape)';

  mobileNav = window.matchMedia(mediaQuery).matches;

  mainContent.classList.toggle('hidden', false);
  helpOuterBox.classList.toggle('hidden', true);
}

function setupMobileNav() {
  window.addEventListener('resize', onResize);
  onResize();
}

document.addEventListener('DOMContentLoaded', setupMobileNav);
