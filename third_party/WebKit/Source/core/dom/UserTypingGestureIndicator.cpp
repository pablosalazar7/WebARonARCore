/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "core/dom/UserTypingGestureIndicator.h"

#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/page/Frame.h"
#include "wtf/StdLibExtras.h"

namespace WebCore {

static bool s_processingUserTypingGesture;
bool UserTypingGestureIndicator::processingUserTypingGesture()
{
    return s_processingUserTypingGesture;
}

static RefPtr<Node>& focusedNode()
{
    DEFINE_STATIC_LOCAL(RefPtr<Node>, node, ());
    return node;
}

Node* UserTypingGestureIndicator::focusedElementAtGestureStart()
{
    return focusedNode().get();
}

UserTypingGestureIndicator::UserTypingGestureIndicator(Frame* frame)
    : m_previousProcessingUserTypingGesture(s_processingUserTypingGesture)
    , m_previousFocusedNode(focusedNode())
{
    s_processingUserTypingGesture = true;
    Node* node = frame->document() ? frame->document()->focusedElement() : 0;
    // FIXME: focusedNode() should be RefPtr<Element>.
    focusedNode() = node;
}

UserTypingGestureIndicator::~UserTypingGestureIndicator()
{
    s_processingUserTypingGesture = m_previousProcessingUserTypingGesture;
    focusedNode() = m_previousFocusedNode;
}

} // namespace WebCore
