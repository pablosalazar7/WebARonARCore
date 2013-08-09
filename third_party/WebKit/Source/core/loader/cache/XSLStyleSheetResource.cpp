/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "core/loader/cache/XSLStyleSheetResource.h"

#include "core/loader/TextResourceDecoder.h"
#include "core/loader/cache/ResourceClientWalker.h"
#include "core/loader/cache/StyleSheetResourceClient.h"
#include "core/platform/SharedBuffer.h"
#include "wtf/Vector.h"

namespace WebCore {

XSLStyleSheetResource::XSLStyleSheetResource(const ResourceRequest& resourceRequest)
    : Resource(resourceRequest, XSLStyleSheet)
    , m_decoder(TextResourceDecoder::create("text/xsl"))
{
    DEFINE_STATIC_LOCAL(const AtomicString, acceptXSLT, ("text/xml, application/xml, application/xhtml+xml, text/xsl, application/rss+xml, application/atom+xml", AtomicString::ConstructFromLiteral));

    // It's XML we want.
    // FIXME: This should accept more general xml formats */*+xml, image/svg+xml for example.
    setAccept(acceptXSLT);
}

void XSLStyleSheetResource::didAddClient(ResourceClient* c)
{
    ASSERT(c->resourceClientType() == StyleSheetResourceClient::expectedType());
    if (!isLoading())
        static_cast<StyleSheetResourceClient*>(c)->setXSLStyleSheet(m_resourceRequest.url(), m_response.url(), m_sheet);
}

void XSLStyleSheetResource::setEncoding(const String& chs)
{
    m_decoder->setEncoding(chs, TextResourceDecoder::EncodingFromHTTPHeader);
}

String XSLStyleSheetResource::encoding() const
{
    return m_decoder->encoding().name();
}

void XSLStyleSheetResource::checkNotify()
{
    if (m_data.get()) {
        m_sheet = m_decoder->decode(m_data->data(), encodedSize());
        m_sheet.append(m_decoder->flush());
    }

    ResourceClientWalker<StyleSheetResourceClient> w(m_clients);
    while (StyleSheetResourceClient* c = w.next())
        c->setXSLStyleSheet(m_resourceRequest.url(), m_response.url(), m_sheet);
}

} // namespace WebCore
