/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebKitMediaSource_h
#define WebKitMediaSource_h

#include "bindings/v8/ScriptWrappable.h"
#include "core/dom/ActiveDOMObject.h"
#include "core/dom/GenericEventQueue.h"
#include "core/platform/graphics/MediaSourcePrivate.h"
#include "modules/mediasource/WebKitSourceBuffer.h"
#include "modules/mediasource/WebKitSourceBufferList.h"
#include "wtf/RefCounted.h"

namespace WebCore {

class WebKitMediaSource : public RefCounted<WebKitMediaSource>, public ScriptWrappable, public EventTarget, public ActiveDOMObject {
public:
    static const String& openKeyword();
    static const String& closedKeyword();
    static const String& endedKeyword();

    static PassRefPtr<WebKitMediaSource> create(ScriptExecutionContext*);
    virtual ~WebKitMediaSource() { }

    // WebKitMediaSource.idl methods
    WebKitSourceBufferList* sourceBuffers();
    WebKitSourceBufferList* activeSourceBuffers();
    double duration() const;
    void setDuration(double, ExceptionCode&);
    WebKitSourceBuffer* addSourceBuffer(const String& type, ExceptionCode&);
    void removeSourceBuffer(WebKitSourceBuffer*, ExceptionCode&);
    const String& readyState() const;
    void setReadyState(const String&);
    void endOfStream(const String& error, ExceptionCode&);
    static bool isTypeSupported(const String& type);

    void setPrivateAndOpen(PassOwnPtr<MediaSourcePrivate>);

    // EventTarget interface
    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;

    // ActiveDOMObject interface
    virtual bool hasPendingActivity() const OVERRIDE;
    virtual void stop() OVERRIDE;

    using RefCounted<WebKitMediaSource>::ref;
    using RefCounted<WebKitMediaSource>::deref;

    virtual void reportMemoryUsage(MemoryObjectInfo*) const OVERRIDE;

private:
    explicit WebKitMediaSource(ScriptExecutionContext*);

    virtual EventTargetData* eventTargetData() OVERRIDE;
    virtual EventTargetData* ensureEventTargetData() OVERRIDE;

    virtual void refEventTarget() OVERRIDE { ref(); }
    virtual void derefEventTarget() OVERRIDE { deref(); }

    void scheduleEvent(const AtomicString& eventName);

    EventTargetData m_eventTargetData;

    String m_readyState;
    OwnPtr<MediaSourcePrivate> m_private;

    RefPtr<WebKitSourceBufferList> m_sourceBuffers;
    RefPtr<WebKitSourceBufferList> m_activeSourceBuffers;
    OwnPtr<GenericEventQueue> m_asyncEventQueue;
};

} // namespace WebCore

#endif
