/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

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
*/

#ifndef CachedRawResource_h
#define CachedRawResource_h

#include "core/loader/cache/CachedResource.h"
#include "core/loader/cache/CachedResourceClient.h"

namespace WebCore {
class CachedRawResourceCallback;
class CachedRawResourceClient;

class CachedRawResource : public CachedResource {
public:
    CachedRawResource(ResourceRequest&, Type);

    // FIXME: AssociatedURLLoader shouldn't be a DocumentThreadableLoader and therefore shouldn't
    // use CachedRawResource. However, it is, and it needs to be able to defer loading.
    // This can be fixed by splitting CORS preflighting out of DocumentThreacableLoader.
    virtual void setDefersLoading(bool);

    virtual void setDataBufferingPolicy(DataBufferingPolicy);

    void clear();

    virtual bool canReuse(const ResourceRequest&) const;

    virtual void reportMemoryUsage(MemoryObjectInfo*) const OVERRIDE;

private:
    virtual void didAddClient(CachedResourceClient*);
    virtual void appendData(const char*, int) OVERRIDE;

    virtual bool shouldIgnoreHTTPStatusCodeErrors() const { return true; }

    virtual void willSendRequest(ResourceRequest&, const ResourceResponse&);
    virtual void responseReceived(const ResourceResponse&);
    virtual void didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent);
    virtual void didDownloadData(int);

    struct RedirectPair {
    public:
        explicit RedirectPair(const ResourceRequest& request, const ResourceResponse& redirectResponse)
            : m_request(request)
            , m_redirectResponse(redirectResponse)
        {
        }

        const ResourceRequest m_request;
        const ResourceResponse m_redirectResponse;
    };

    Vector<RedirectPair> m_redirectChain;
};


class CachedRawResourceClient : public CachedResourceClient {
public:
    virtual ~CachedRawResourceClient() { }
    static CachedResourceClientType expectedType() { return RawResourceType; }
    virtual CachedResourceClientType resourceClientType() const { return expectedType(); }

    virtual void dataSent(CachedResource*, unsigned long long /* bytesSent */, unsigned long long /* totalBytesToBeSent */) { }
    virtual void responseReceived(CachedResource*, const ResourceResponse&) { }
    virtual void dataReceived(CachedResource*, const char* /* data */, int /* length */) { }
    virtual void redirectReceived(CachedResource*, ResourceRequest&, const ResourceResponse&) { }
    virtual void dataDownloaded(CachedResource*, int) { }
};

}

#endif // CachedRawResource_h
