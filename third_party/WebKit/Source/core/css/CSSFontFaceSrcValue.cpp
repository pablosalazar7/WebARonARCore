/*
 * Copyright (C) 2007, 2010 Apple Inc. All rights reserved.
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
#include "core/css/CSSFontFaceSrcValue.h"

#include "FetchInitiatorTypeNames.h"
#include "core/css/StyleSheetContents.h"
#include "core/dom/Document.h"
#include "core/dom/Node.h"
#include "core/fetch/FetchRequest.h"
#include "core/fetch/FontResource.h"
#include "core/fetch/ResourceFetcher.h"
#include "core/platform/graphics/FontCustomPlatformData.h"
#include "core/svg/SVGFontFaceElement.h"
#include "wtf/text/StringBuilder.h"

namespace WebCore {

#if ENABLE(SVG_FONTS)
bool CSSFontFaceSrcValue::isSVGFontFaceSrc() const
{
    return equalIgnoringCase(m_format, "svg");
}
#endif

bool CSSFontFaceSrcValue::isSupportedFormat() const
{
    // Normally we would just check the format, but in order to avoid conflicts with the old WinIE style of font-face,
    // we will also check to see if the URL ends with .eot.  If so, we'll go ahead and assume that we shouldn't load it.
    if (m_format.isEmpty()) {
        // Check for .eot.
        if (!m_resource.startsWith("data:", false) && m_resource.endsWith(".eot", false))
            return false;
        return true;
    }

    return FontCustomPlatformData::supportsFormat(m_format)
#if ENABLE(SVG_FONTS)
           || isSVGFontFaceSrc()
#endif
           ;
}

String CSSFontFaceSrcValue::customCSSText() const
{
    StringBuilder result;
    if (isLocal())
        result.appendLiteral("local(");
    else
        result.appendLiteral("url(");
    result.append(m_resource);
    result.append(')');
    if (!m_format.isEmpty()) {
        result.appendLiteral(" format(");
        result.append(m_format);
        result.append(')');
    }
    return result.toString();
}

void CSSFontFaceSrcValue::addSubresourceStyleURLs(ListHashSet<KURL>& urls, const StyleSheetContents* styleSheet) const
{
    if (!isLocal())
        addSubresourceURL(urls, styleSheet->completeURL(m_resource));
}

bool CSSFontFaceSrcValue::hasFailedOrCanceledSubresources() const
{
    if (!m_fetched)
        return false;
    return m_fetched->loadFailedOrCanceled();
}

FontResource* CSSFontFaceSrcValue::fetch(Document* document)
{
    if (!m_fetched) {
        FetchRequest request(ResourceRequest(document->completeURL(m_resource)), FetchInitiatorTypeNames::css);
        m_fetched = document->fetcher()->fetchFont(request);
    }
    return m_fetched.get();
}

bool CSSFontFaceSrcValue::equals(const CSSFontFaceSrcValue& other) const
{
    return m_isLocal == other.m_isLocal && m_format == other.m_format && m_resource == other.m_resource;
}

}
