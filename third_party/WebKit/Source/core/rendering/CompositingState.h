// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CompositingState_h
#define CompositingState_h

namespace WebCore {

enum CompositingState {
    // The layer paints into its enclosing composited ancestor.
    NotComposited = 0,

    PaintsIntoOwnBacking = 1,
};

} // namespace WebCore

#endif // CompositingState_h
