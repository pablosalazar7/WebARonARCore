/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef WebMIDIClientMock_h
#define WebMIDIClientMock_h

#include "../platform/WebCommon.h"
#include "../platform/WebPrivateOwnPtr.h"
#include "WebMIDIClient.h"

namespace WebCore {
class MIDIClientMock;
}

namespace blink {

class WebMIDIClientMock : public WebMIDIClient {
public:
    BLINK_EXPORT WebMIDIClientMock();
    virtual ~WebMIDIClientMock() { reset(); }

    BLINK_EXPORT void setSysexPermission(bool);
    BLINK_EXPORT void resetMock();

    // FIXME: Remove following function once Chromium stop using it.
    void setSysExPermission(bool permission) { setSysexPermission(permission); }

    // WebMIDIClient
    virtual void requestSysexPermission(const WebMIDIPermissionRequest&) OVERRIDE;
    virtual void cancelSysexPermissionRequest(const WebMIDIPermissionRequest&) OVERRIDE;

    // FIXME: Remove following old functions once Chromium uses new functions.
    virtual void requestSysExPermission(const WebMIDIPermissionRequest& request) { requestSysexPermission(request); }
    virtual void cancelSysExPermissionRequest(const WebMIDIPermissionRequest& request) { cancelSysexPermissionRequest(request); }


private:
    BLINK_EXPORT void reset();

    WebPrivateOwnPtr<WebCore::MIDIClientMock> m_clientMock;
};

} // namespace blink

#endif // WebMIDIClientMock_h
