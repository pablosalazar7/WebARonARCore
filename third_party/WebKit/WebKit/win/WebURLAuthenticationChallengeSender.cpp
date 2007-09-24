/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#include "config.h"
#include "WebKitDLL.h"
#include <initguid.h>
#include "WebURLAuthenticationChallengeSender.h"

#include "COMPtr.h"
#include "WebKit.h"
#include "WebURLAuthenticationChallenge.h"
#include "WebURLCredential.h"

#pragma warning(push, 0)
#include <WebCore/ResourceHandle.h>
#pragma warning(pop)

using namespace WebCore;

// WebURLAuthenticationChallengeSender ----------------------------------------------------------------

WebURLAuthenticationChallengeSender::WebURLAuthenticationChallengeSender(PassRefPtr<ResourceHandle> handle)
    : m_refCount(0)
    , m_handle(handle)
{
    ASSERT(m_handle);
    gClassCount++;
}

WebURLAuthenticationChallengeSender::~WebURLAuthenticationChallengeSender()
{
    gClassCount--;
}

WebURLAuthenticationChallengeSender* WebURLAuthenticationChallengeSender::createInstance(PassRefPtr<WebCore::ResourceHandle> handle)
{
    WebURLAuthenticationChallengeSender* instance = new WebURLAuthenticationChallengeSender(handle);
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IUnknown*>(this);
    else if (IsEqualGUID(riid, __uuidof(WebURLAuthenticationChallengeSender)))
        *ppvObject = static_cast<WebURLAuthenticationChallengeSender*>(this);
    else if (IsEqualGUID(riid, IID_IWebURLAuthenticationChallengeSender))
        *ppvObject = static_cast<IWebURLAuthenticationChallengeSender*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebURLAuthenticationChallengeSender -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::cancelAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge* challenge)
{
    COMPtr<WebURLAuthenticationChallenge> webChallenge(Query, challenge);
    if (!webChallenge)
        return E_FAIL;

    m_handle->receivedCancellation(webChallenge->authenticationChallenge());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::continueWithoutCredentialForAuthenticationChallenge(
        /* [in] */ IWebURLAuthenticationChallenge* challenge)
{
    COMPtr<WebURLAuthenticationChallenge> webChallenge(Query, challenge);
    if (!webChallenge)
        return E_FAIL;

    m_handle->receivedRequestToContinueWithoutCredential(webChallenge->authenticationChallenge());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebURLAuthenticationChallengeSender::useCredential(
        /* [in] */ IWebURLCredential* credential, 
        /* [in] */ IWebURLAuthenticationChallenge* challenge)
{
    COMPtr<WebURLAuthenticationChallenge> webChallenge(Query, challenge);
    if (!webChallenge)
        return E_FAIL;
    
    COMPtr<WebURLCredential> webCredential;
    if (!credential || FAILED(credential->QueryInterface(__uuidof(WebURLCredential), (void**)&webCredential)))
        return E_FAIL;

    m_handle->receivedCredential(webChallenge->authenticationChallenge(), webCredential->credential());
    return S_OK;
}

// WebURLAuthenticationChallengeSender ----------------------------------------------------------------

ResourceHandle* WebURLAuthenticationChallengeSender::resourceHandle() const
{
    return m_handle.get();
}

