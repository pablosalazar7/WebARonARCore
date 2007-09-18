/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetscapePlugInStreamLoader.h"

#include "DocumentLoader.h"

namespace WebCore {

NetscapePlugInStreamLoader::NetscapePlugInStreamLoader(Frame* frame, NetscapePlugInStreamLoaderClient* client)
    : ResourceLoader(frame, true, true)
    , m_client(client)
{
}

NetscapePlugInStreamLoader::~NetscapePlugInStreamLoader()
{
}

PassRefPtr<NetscapePlugInStreamLoader> NetscapePlugInStreamLoader::create(Frame* frame, NetscapePlugInStreamLoaderClient* client)
{
    return new NetscapePlugInStreamLoader(frame, client);
}

bool NetscapePlugInStreamLoader::isDone() const
{
    return !m_client;
}

void NetscapePlugInStreamLoader::releaseResources()
{
    m_client = 0;
    ResourceLoader::releaseResources();
}

void NetscapePlugInStreamLoader::didReceiveResponse(const ResourceResponse& response)
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    m_client->didReceiveResponse(this, response);
    ResourceLoader::didReceiveResponse(response);
}

void NetscapePlugInStreamLoader::didReceiveData(const char* data, int length, long long lengthReceived, bool allAtOnce)
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    m_client->didReceiveData(this, data, length);
    
    ResourceLoader::didReceiveData(data, length, lengthReceived, allAtOnce);
}

void NetscapePlugInStreamLoader::didFinishLoading()
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    m_documentLoader->removePlugInStreamLoader(this);
    m_client->didFinishLoading(this);
    ResourceLoader::didFinishLoading();
}

void NetscapePlugInStreamLoader::didFail(const ResourceError& error)
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    m_documentLoader->removePlugInStreamLoader(this);
    m_client->didFail(this, error);
    ResourceLoader::didFail(error);
}

void NetscapePlugInStreamLoader::didCancel(const ResourceError& error)
{
    RefPtr<NetscapePlugInStreamLoader> protect(this);

    m_documentLoader->removePlugInStreamLoader(this);
    m_client->didFail(this, error);
    ResourceLoader::didCancel(error);
}

}
