/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "WebHistoryItem.h"

#include "COMPtr.h"
#include "MarshallingHelpers.h"
#include "WebKit.h"

#pragma warning(push, 0)
#include <WebCore/BString.h>
#include <WebCore/HistoryItem.h>
#include <WebCore/KURL.h>
#pragma warning(pop)

#include <wtf/RetainPtr.h>

using namespace WebCore;

// WebHistoryItem ----------------------------------------------------------------

static HashMap<HistoryItem*, WebHistoryItem*>& historyItemWrappers()
{
    static HashMap<HistoryItem*, WebHistoryItem*> staticHistoryItemWrappers;
    return staticHistoryItemWrappers;
}

WebHistoryItem::WebHistoryItem(PassRefPtr<HistoryItem> historyItem)
: m_refCount(0)
, m_historyItem(historyItem)
{
    ASSERT(!historyItemWrappers().contains(m_historyItem.get()));
    historyItemWrappers().set(m_historyItem.get(), this);

    gClassCount++;
    gClassNameCount.add("WebHistoryItem");
}

WebHistoryItem::~WebHistoryItem()
{
    ASSERT(historyItemWrappers().contains(m_historyItem.get()));
    historyItemWrappers().remove(m_historyItem.get());

    gClassCount--;
    gClassNameCount.remove("WebHistoryItem");
}

WebHistoryItem* WebHistoryItem::createInstance()
{
    WebHistoryItem* instance = new WebHistoryItem(HistoryItem::create());
    instance->AddRef();
    return instance;
}

WebHistoryItem* WebHistoryItem::createInstance(PassRefPtr<HistoryItem> historyItem)
{
    WebHistoryItem* instance;

    instance = historyItemWrappers().get(historyItem.get());

    if (!instance)
        instance = new WebHistoryItem(historyItem);

    instance->AddRef();
    return instance;
}

// IWebHistoryItemPrivate -----------------------------------------------------

static CFStringRef urlKey = CFSTR("");
static CFStringRef lastVisitedDateKey = CFSTR("lastVisitedDate");
static CFStringRef titleKey = CFSTR("title");
static CFStringRef visitCountKey = CFSTR("visitCount");
static CFStringRef lastVisitWasFailureKey = CFSTR("lastVisitWasFailure");
static CFStringRef lastVisitWasHTTPNonGetKey = CFSTR("lastVisitWasHTTPNonGet");

HRESULT STDMETHODCALLTYPE WebHistoryItem::initFromDictionaryRepresentation(void* dictionary)
{
    CFDictionaryRef dictionaryRef = (CFDictionaryRef) dictionary;

    CFStringRef urlStringRef = (CFStringRef) CFDictionaryGetValue(dictionaryRef, urlKey);
    if (urlStringRef && CFGetTypeID(urlStringRef) != CFStringGetTypeID())
        return E_FAIL;

    CFStringRef lastVisitedRef = (CFStringRef) CFDictionaryGetValue(dictionaryRef, lastVisitedDateKey);
    if (!lastVisitedRef || CFGetTypeID(lastVisitedRef) != CFStringGetTypeID())
        return E_FAIL;
    CFAbsoluteTime lastVisitedTime = CFStringGetDoubleValue(lastVisitedRef);

    CFStringRef titleRef = (CFStringRef) CFDictionaryGetValue(dictionaryRef, titleKey);
    if (titleRef && CFGetTypeID(titleRef) != CFStringGetTypeID())
        return E_FAIL;

    CFNumberRef visitCountRef = (CFNumberRef) CFDictionaryGetValue(dictionaryRef, visitCountKey);
    if (!visitCountRef || CFGetTypeID(visitCountRef) != CFNumberGetTypeID())
        return E_FAIL;
    int visitedCount = 0;
    if (!CFNumberGetValue(visitCountRef, kCFNumberIntType, &visitedCount))
        return E_FAIL;

    CFBooleanRef lastVisitWasFailureRef = static_cast<CFBooleanRef>(CFDictionaryGetValue(dictionaryRef, lastVisitWasFailureKey));
    if (lastVisitWasFailureRef && CFGetTypeID(lastVisitWasFailureRef) != CFBooleanGetTypeID())
        return E_FAIL;
    bool lastVisitWasFailure = lastVisitWasFailureRef && CFBooleanGetValue(lastVisitWasFailureRef);

    CFBooleanRef lastVisitWasHTTPNonGetRef = static_cast<CFBooleanRef>(CFDictionaryGetValue(dictionaryRef, lastVisitWasHTTPNonGetKey));
    if (lastVisitWasHTTPNonGetRef && CFGetTypeID(lastVisitWasHTTPNonGetRef) != CFBooleanGetTypeID())
        return E_FAIL;
    bool lastVisitWasHTTPNonGet = lastVisitWasHTTPNonGetRef && CFBooleanGetValue(lastVisitWasHTTPNonGetRef);

    historyItemWrappers().remove(m_historyItem.get());
    m_historyItem = HistoryItem::create(urlStringRef, titleRef, lastVisitedTime);
    historyItemWrappers().set(m_historyItem.get(), this);

    m_historyItem->setVisitCount(visitedCount);
    if (lastVisitWasFailure)
        m_historyItem->setLastVisitWasFailure(true);

    if (lastVisitWasHTTPNonGet && (protocolIs(m_historyItem->urlString(), "http") || protocolIs(m_historyItem->urlString(), "https")))
        m_historyItem->setLastVisitWasHTTPNonGet(true);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::dictionaryRepresentation(void** dictionary)
{
    CFDictionaryRef* dictionaryRef = (CFDictionaryRef*) dictionary;
    static CFStringRef lastVisitedFormat = CFSTR("%.1lf");
    CFStringRef lastVisitedStringRef =
        CFStringCreateWithFormat(0, 0, lastVisitedFormat, m_historyItem->lastVisitedTime());
    if (!lastVisitedStringRef)
        return E_FAIL;

    int keyCount = 0;
    CFTypeRef keys[6];
    CFTypeRef values[6];

    if (!m_historyItem->urlString().isEmpty()) {
        keys[keyCount] = urlKey;
        values[keyCount++] = m_historyItem->urlString().createCFString();
    }

    keys[keyCount] = lastVisitedDateKey;
    values[keyCount++] = lastVisitedStringRef;

    if (!m_historyItem->title().isEmpty()) {
        keys[keyCount] = titleKey;
        values[keyCount++] = m_historyItem->title().createCFString();
    }

    keys[keyCount] = visitCountKey;
    int visitCount = m_historyItem->visitCount();
    values[keyCount++] = CFNumberCreate(0, kCFNumberIntType, &visitCount);

    if (m_historyItem->lastVisitWasFailure()) {
        keys[keyCount] = lastVisitWasFailureKey;
        values[keyCount++] = CFRetain(kCFBooleanTrue);
    }

    if (m_historyItem->lastVisitWasHTTPNonGet()) {
        ASSERT(m_historyItem->urlString().startsWith("http:", false) || m_historyItem->urlString().startsWith("https:", false));
        keys[keyCount] = lastVisitWasHTTPNonGetKey;
        values[keyCount++] = CFRetain(kCFBooleanTrue);
    }

    *dictionaryRef = CFDictionaryCreate(0, keys, values, keyCount, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    
    for (int i = 0; i < keyCount; ++i)
        CFRelease(values[i]);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::hasURLString(BOOL *hasURL)
{
    *hasURL = m_historyItem->urlString().isEmpty() ? FALSE : TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::visitCount(int *count)
{
    *count = m_historyItem->visitCount();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setVisitCount(int count)
{
    m_historyItem->setVisitCount(count);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::mergeAutoCompleteHints(IWebHistoryItem* otherItem)
{
    if (!otherItem)
        return E_FAIL;

    COMPtr<WebHistoryItem> otherWebHistoryItem(Query, otherItem);
    if (!otherWebHistoryItem)
        return E_FAIL;

    m_historyItem->mergeAutoCompleteHints(otherWebHistoryItem->historyItem());

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setLastVisitedTimeInterval(DATE time)
{
    m_historyItem->setLastVisitedTime(MarshallingHelpers::DATEToCFAbsoluteTime(time));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setTitle(BSTR title)
{
    m_historyItem->setTitle(String(title, SysStringLen(title)));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::RSSFeedReferrer(BSTR* url)
{
    BString str(m_historyItem->rssFeedReferrer());
    *url = str.release();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setRSSFeedReferrer(BSTR url)
{
    m_historyItem->setRSSFeedReferrer(String(url, SysStringLen(url)));

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::hasPageCache(BOOL* /*hasCache*/)
{
    // FIXME - TODO
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setHasPageCache(BOOL /*hasCache*/)
{
    // FIXME - TODO
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::target(BSTR* target)
{
    if (!target) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *target = BString(m_historyItem->target()).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::isTargetItem(BOOL* result)
{
    if (!result) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *result = m_historyItem->isTargetItem() ? TRUE : FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::children(unsigned* outChildCount, SAFEARRAY** outChildren)
{
    if (!outChildCount || !outChildren) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *outChildCount = 0;
    *outChildren = 0;

    const HistoryItemVector& coreChildren = m_historyItem->children();
    if (coreChildren.isEmpty())
        return S_OK;
    size_t childCount = coreChildren.size();

    SAFEARRAY* children = SafeArrayCreateVector(VT_UNKNOWN, 0, static_cast<ULONG>(childCount));
    if (!children)
        return E_OUTOFMEMORY;

    for (unsigned i = 0; i < childCount; ++i) {
        WebHistoryItem* item = WebHistoryItem::createInstance(coreChildren[i]);
        if (!item) {
            SafeArrayDestroy(children);
            return E_OUTOFMEMORY;
        }

        COMPtr<IUnknown> unknown;
        HRESULT hr = item->QueryInterface(IID_IUnknown, (void**)&unknown);
        if (FAILED(hr)) {
            SafeArrayDestroy(children);
            return hr;
        }

        LONG longI = i;
        hr = SafeArrayPutElement(children, &longI, unknown.get());
        if (FAILED(hr)) {
            SafeArrayDestroy(children);
            return hr;
        }
    }

    *outChildCount = static_cast<unsigned>(childCount);
    *outChildren = children;
    return S_OK;

}

HRESULT STDMETHODCALLTYPE WebHistoryItem::lastVisitWasFailure(BOOL* wasFailure)
{
    if (!wasFailure) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *wasFailure = m_historyItem->lastVisitWasFailure();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setLastVisitWasFailure(BOOL wasFailure)
{
    m_historyItem->setLastVisitWasFailure(wasFailure);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::lastVisitWasHTTPNonGet(BOOL* HTTPNonGet)
{
    if (!HTTPNonGet) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *HTTPNonGet = m_historyItem->lastVisitWasHTTPNonGet();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setLastVisitWasHTTPNonGet(BOOL HTTPNonGet)
{
    m_historyItem->setLastVisitWasHTTPNonGet(HTTPNonGet);
    return S_OK;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebHistoryItem::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, __uuidof(WebHistoryItem)))
        *ppvObject = this;
    else if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebHistoryItem*>(this);
    else if (IsEqualGUID(riid, IID_IWebHistoryItem))
        *ppvObject = static_cast<IWebHistoryItem*>(this);
    else if (IsEqualGUID(riid, IID_IWebHistoryItemPrivate))
        *ppvObject = static_cast<IWebHistoryItemPrivate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebHistoryItem::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebHistoryItem::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebHistoryItem -------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebHistoryItem::initWithURLString(
    /* [in] */ BSTR urlString,
    /* [in] */ BSTR title,
    /* [in] */ DATE lastVisited)
{
    historyItemWrappers().remove(m_historyItem.get());
    m_historyItem = HistoryItem::create(String(urlString, SysStringLen(urlString)), String(title, SysStringLen(title)), MarshallingHelpers::DATEToCFAbsoluteTime(lastVisited));
    historyItemWrappers().set(m_historyItem.get(), this);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::originalURLString( 
    /* [retval][out] */ BSTR* url)
{
    if (!url)
        return E_POINTER;

    BString str = m_historyItem->originalURLString();
    *url = str.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::URLString( 
    /* [retval][out] */ BSTR* url)
{
    if (!url)
        return E_POINTER;

    BString str = m_historyItem->urlString();
    *url = str.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::title( 
    /* [retval][out] */ BSTR* pageTitle)
{
    if (!pageTitle)
        return E_POINTER;

    BString str(m_historyItem->title());
    *pageTitle = str.release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::lastVisitedTimeInterval( 
    /* [retval][out] */ DATE* lastVisited)
{
    if (!lastVisited)
        return E_POINTER;

    *lastVisited = MarshallingHelpers::CFAbsoluteTimeToDATE(m_historyItem->lastVisitedTime());
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::setAlternateTitle( 
    /* [in] */ BSTR title)
{
    m_alternateTitle = String(title, SysStringLen(title));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::alternateTitle( 
    /* [retval][out] */ BSTR* title)
{
    if (!title) {
        ASSERT_NOT_REACHED();
        return E_POINTER;
    }

    *title = BString(m_alternateTitle).release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebHistoryItem::icon( 
    /* [out, retval] */ OLE_HANDLE* /*hBitmap*/)
{
    ASSERT_NOT_REACHED();
    return E_NOTIMPL;
}

// WebHistoryItem -------------------------------------------------------------

HistoryItem* WebHistoryItem::historyItem() const
{
    return m_historyItem.get();
}
