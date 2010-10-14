/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WebGLProgram_h
#define WebGLProgram_h

#include "WebGLObject.h"

#include "WebGLShader.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class WebGLProgram : public WebGLObject {
public:
    virtual ~WebGLProgram() { deleteObject(); }

    static PassRefPtr<WebGLProgram> create(WebGLRenderingContext*);

    // cacheActiveAttribLocation() is only called once after linkProgram()
    // succeeds.
    bool cacheActiveAttribLocations();
    int numActiveAttribLocations() const;
    int getActiveAttribLocation(int index) const;

    bool isUsingVertexAttrib0() const;

    bool getLinkStatus() const { return m_linkStatus; }
    void setLinkStatus(bool status) { m_linkStatus = status; }

    WebGLShader* getAttachedShader(GraphicsContext3D::WebGLEnumType);
    bool attachShader(WebGLShader*);
    bool detachShader(WebGLShader*);

protected:
    WebGLProgram(WebGLRenderingContext*);

    virtual void deleteObjectImpl(Platform3DObject);

private:
    virtual bool isProgram() const { return true; }

    Vector<int> m_activeAttribLocations;

    bool m_linkStatus;

    RefPtr<WebGLShader> m_vertexShader;
    RefPtr<WebGLShader> m_fragmentShader;
};

} // namespace WebCore

#endif // WebGLProgram_h
