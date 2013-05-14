/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "core/loader/LinkLoader.h"

#include "core/css/CSSStyleSheet.h"
#include "core/css/resolver/StyleResolver.h"
#include "core/dom/ContainerNode.h"
#include "core/dom/Document.h"
#include "core/html/LinkRelAttribute.h"
#include "core/loader/Prerenderer.h"
#include "core/loader/cache/CachedCSSStyleSheet.h"
#include "core/loader/cache/CachedResourceLoader.h"
#include "core/loader/cache/CachedResourceRequest.h"
#include "core/page/Frame.h"
#include "core/page/FrameView.h"
#include "core/page/Settings.h"
#include "core/platform/PrerenderHandle.h"
#include "core/platform/network/DNS.h"

namespace WebCore {

LinkLoader::LinkLoader(LinkLoaderClient* client)
    : m_client(client)
    , m_linkLoadTimer(this, &LinkLoader::linkLoadTimerFired)
    , m_linkLoadingErrorTimer(this, &LinkLoader::linkLoadingErrorTimerFired)
{
}

LinkLoader::~LinkLoader()
{
    if (m_cachedLinkResource)
        m_cachedLinkResource->removeClient(this);
    if (m_prerenderHandle)
        m_prerenderHandle->removeClient();
}

void LinkLoader::linkLoadTimerFired(Timer<LinkLoader>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_linkLoadTimer);
    m_client->linkLoaded();
}

void LinkLoader::linkLoadingErrorTimerFired(Timer<LinkLoader>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_linkLoadingErrorTimer);
    m_client->linkLoadingErrored();
}

void LinkLoader::notifyFinished(CachedResource* resource)
{
    ASSERT_UNUSED(resource, m_cachedLinkResource.get() == resource);

    if (m_cachedLinkResource->errorOccurred())
        m_linkLoadingErrorTimer.startOneShot(0);
    else 
        m_linkLoadTimer.startOneShot(0);

    m_cachedLinkResource->removeClient(this);
    m_cachedLinkResource = 0;
}

void LinkLoader::didStartPrerender()
{
    m_client->didStartLinkPrerender();
}

void LinkLoader::didStopPrerender()
{
    m_client->didStopLinkPrerender();
}

void LinkLoader::didSendLoadForPrerender()
{
    m_client->didSendLoadForLinkPrerender();
}

void LinkLoader::didSendDOMContentLoadedForPrerender()
{
    m_client->didSendDOMContentLoadedForLinkPrerender();
}

bool LinkLoader::loadLink(const LinkRelAttribute& relAttribute, const String& type, const KURL& href, Document* document)
{
    if (relAttribute.isDNSPrefetch()) {
        Settings* settings = document->settings();
        // FIXME: The href attribute of the link element can be in "//hostname" form, and we shouldn't attempt
        // to complete that as URL <https://bugs.webkit.org/show_bug.cgi?id=48857>.
        if (settings && settings->dnsPrefetchingEnabled() && href.isValid() && !href.isEmpty())
            prefetchDNS(href.host());
    }

    if ((relAttribute.isLinkPrefetch() || relAttribute.isLinkSubresource()) && href.isValid() && document->frame()) {
        if (!m_client->shouldLoadLink())
            return false;
        ResourceLoadPriority priority = ResourceLoadPriorityUnresolved;
        CachedResource::Type type = CachedResource::LinkPrefetch;
        // We only make one request to the cachedresourcelodaer if multiple rel types are
        // specified, 
        if (relAttribute.isLinkSubresource()) {
            priority = ResourceLoadPriorityLow;
            type = CachedResource::LinkSubresource;
        }
        CachedResourceRequest linkRequest(ResourceRequest(document->completeURL(href)), priority);
        
        if (m_cachedLinkResource) {
            m_cachedLinkResource->removeClient(this);
            m_cachedLinkResource = 0;
        }
        m_cachedLinkResource = document->cachedResourceLoader()->requestLinkResource(type, linkRequest);
        if (m_cachedLinkResource)
            m_cachedLinkResource->addClient(this);
    }

    if (relAttribute.isLinkPrerender()) {
        if (!m_prerenderHandle) {
            m_prerenderHandle = document->prerenderer()->render(this, href);
        } else if (m_prerenderHandle->url() != href) {
            m_prerenderHandle->cancel();
            m_prerenderHandle = document->prerenderer()->render(this, href);
        }
    } else if (m_prerenderHandle) {
        m_prerenderHandle->cancel();
        m_prerenderHandle = 0;
    }
    return true;
}

void LinkLoader::released()
{
    // Only prerenders need treatment here; other links either use the CachedResource interface, or are notionally
    // atomic (dns prefetch).
    if (m_prerenderHandle) {
        m_prerenderHandle->cancel();
        m_prerenderHandle->removeClient();
        m_prerenderHandle.clear();
    }
}

}
