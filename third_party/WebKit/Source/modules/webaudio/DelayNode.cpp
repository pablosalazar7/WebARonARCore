/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "modules/webaudio/DelayNode.h"

#include "bindings/v8/ExceptionState.h"
#include "core/dom/ExceptionCode.h"

namespace WebCore {

const double maximumAllowedDelayTime = 180;

DelayNode::DelayNode(AudioContext* context, float sampleRate, double maxDelayTime, ExceptionState& es)
    : AudioBasicProcessorNode(context, sampleRate)
{
    ScriptWrappable::init(this);
    if (maxDelayTime <= 0 || maxDelayTime >= maximumAllowedDelayTime) {
        es.throwDOMException(NotSupportedError);
        return;
    }
    m_processor = adoptPtr(new DelayProcessor(context, sampleRate, 1, maxDelayTime));
    setNodeType(NodeTypeDelay);
}

AudioParam* DelayNode::delayTime()
{
    return delayProcessor()->delayTime();
}

} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
