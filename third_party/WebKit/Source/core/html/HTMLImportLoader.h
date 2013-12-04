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

#ifndef HTMLImportLoader_h
#define HTMLImportLoader_h

#include "core/html/HTMLImport.h"
#include "core/html/HTMLImportDataClient.h"
#include "core/html/HTMLImportResourceOwner.h"
#include "platform/weborigin/KURL.h"

namespace WebCore {

class HTMLImportData;
class HTMLImportLoaderClient;

//
// An import tree node subclas to encapsulate imported document
// lifecycle. This class is owned by LinkStyle. The actual loading
// is done by HTMLImportData, which can be shared among multiple
// HTMLImportLoader of same link URL.
//
// HTMLImportLoader implements ResourceClient through HTMLImportResourceOwner
// so that it can speculatively request linked resources while it is unblocked.
//
// FIXME: Should be renamed to HTMLImportChild
//
class HTMLImportLoader : public HTMLImport, public HTMLImportDataClient, public HTMLImportResourceOwner {
public:
    HTMLImportLoader(const KURL&, HTMLImportLoaderClient*);
    virtual ~HTMLImportLoader();

    Document* importedDocument() const;
    const KURL& url() const { return m_url; }

    void wasAlreadyLoadedAs(HTMLImportLoader* found);
    void startLoading(ResourceFetcher*, const ResourcePtr<RawResource>&);
    void importDestroyed();
    bool isDone() const;
    bool isLoaded() const;

    // HTMLImport
    virtual HTMLImportRoot* root() OVERRIDE;
    virtual Document* document() const OVERRIDE;
    virtual void wasDetachedFromDocument() OVERRIDE;
    virtual void didFinishParsing() OVERRIDE;
    virtual bool isProcessing() const OVERRIDE;

    void clearClient() { m_client = 0; }

private:
    // RawResourceOwner doing nothing.
    // HTMLImportLoader owns the resource so that the contents of prefetched Resource doesn't go away.
    virtual void responseReceived(Resource*, const ResourceResponse&) OVERRIDE { }
    virtual void dataReceived(Resource*, const char*, int) OVERRIDE { }
    virtual void notifyFinished(Resource*) OVERRIDE { }

    // HTMLImportDataClient
    virtual void didFinish() OVERRIDE;

    KURL m_url;
    HTMLImportLoaderClient* m_client;
    RefPtr<HTMLImportData> m_data;
};

} // namespace WebCore

#endif // HTMLImportLoader_h
