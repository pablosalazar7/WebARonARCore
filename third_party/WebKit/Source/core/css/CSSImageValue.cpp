/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "core/css/CSSImageValue.h"

#include "CSSValueKeywords.h"
#include "Document.h"
#include "Element.h"
#include "WebCoreMemoryInstrumentation.h"
#include "core/css/CSSCursorImageValue.h"
#include "core/css/CSSParser.h"
#include "core/loader/cache/CachedImage.h"
#include "core/loader/cache/CachedResourceLoader.h"
#include "core/loader/cache/CachedResourceRequest.h"
#include "core/loader/cache/CachedResourceRequestInitiators.h"
#include "core/loader/cache/MemoryCache.h"
#include "core/rendering/style/StyleCachedImage.h"
#include "core/rendering/style/StylePendingImage.h"

namespace WebCore {

CSSImageValue::CSSImageValue(const String& url)
    : CSSValue(ImageClass)
    , m_url(url)
    , m_accessedImage(false)
{
}

CSSImageValue::CSSImageValue(const String& url, StyleImage* image)
    : CSSValue(ImageClass)
    , m_url(url)
    , m_image(image)
    , m_accessedImage(true)
{
}

CSSImageValue::~CSSImageValue()
{
}

StyleImage* CSSImageValue::cachedOrPendingImage()
{
    if (!m_image)
        m_image = StylePendingImage::create(this);

    return m_image.get();
}

StyleCachedImage* CSSImageValue::cachedImage(CachedResourceLoader* loader)
{
    ASSERT(loader);

    if (!m_accessedImage) {
        m_accessedImage = true;

        CachedResourceRequest request(ResourceRequest(loader->document()->completeURL(m_url)));
        if (m_initiatorName.isEmpty())
            request.setInitiator(cachedResourceRequestInitiators().css);
        else
            request.setInitiator(m_initiatorName);
        if (CachedResourceHandle<CachedImage> cachedImage = loader->requestImage(request))
            m_image = StyleCachedImage::create(cachedImage.get());
    }

    return (m_image && m_image->isCachedImage()) ? static_cast<StyleCachedImage*>(m_image.get()) : 0;
}

bool CSSImageValue::hasFailedOrCanceledSubresources() const
{
    if (!m_image || !m_image->isCachedImage())
        return false;
    CachedResource* cachedResource = static_cast<StyleCachedImage*>(m_image.get())->cachedImage();
    if (!cachedResource)
        return true;
    return cachedResource->loadFailedOrCanceled();
}

bool CSSImageValue::equals(const CSSImageValue& other) const
{
    return m_url == other.m_url;
}

String CSSImageValue::customCssText() const
{
    return "url(" + quoteCSSURLIfNeeded(m_url) + ")";
}

PassRefPtr<CSSValue> CSSImageValue::cloneForCSSOM() const
{
    // NOTE: We expose CSSImageValues as URI primitive values in CSSOM to maintain old behavior.
    RefPtr<CSSPrimitiveValue> uriValue = CSSPrimitiveValue::create(m_url, CSSPrimitiveValue::CSS_URI);
    uriValue->setCSSOMSafe();
    return uriValue.release();
}

void CSSImageValue::reportDescendantMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    info.addMember(m_url, "url");
    // No need to report m_image as it is counted as part of RenderArena.
}

bool CSSImageValue::knownToBeOpaque(const RenderObject* renderer) const
{
    return m_image ? m_image->knownToBeOpaque(renderer) : false;
}

} // namespace WebCore
