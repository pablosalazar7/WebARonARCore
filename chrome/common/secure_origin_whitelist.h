// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_SECURE_ORIGIN_WHITELIST_H_
#define CHROME_COMMON_SECURE_ORIGIN_WHITELIST_H_

#include <set>

#include "url/gurl.h"

// Returns whitelisted origins that need to be considered trustworthy.
// The whitelist is given by kUnsafetyTreatInsecureOriginAsSecure
// command-line option.
// See https://www.w3.org/TR/powerful-features/#is-origin-trustworthy.
void GetSecureOriginWhitelist(std::set<GURL>* origins);

#endif  // CHROME_COMMON_SECURE_ORIGIN_WHITELIST_H_
