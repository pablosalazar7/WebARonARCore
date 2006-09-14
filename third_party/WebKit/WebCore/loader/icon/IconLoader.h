/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
#ifndef ICONLOADER_H_
#define ICONLOADER_H_

#include <ResourceLoader.h>

namespace WebCore {

class Frame;

class IconLoader : public ResourceLoaderClient
{
public:
    static IconLoader* createForFrame(Frame* frame);
    ~IconLoader();
    
    void startLoading();
    void stopLoading();
    
// ResourceLoaderClient delegate methods
    virtual void receivedResponse(ResourceLoader*, PlatformResponse);
    virtual void receivedData(ResourceLoader*, const char*, int);
    virtual void receivedAllData(ResourceLoader*);
private:
    IconLoader(Frame* frame);
    
    void notifyIconChanged(const KURL& iconURL);
    
    ResourceLoader* m_resourceLoader;
    Frame* m_frame;
    
    static const int IconLoaderDefaultBuffer = 4096;
    Vector<char, IconLoaderDefaultBuffer> m_data;
    int m_httpStatusCode;
}; // class Iconloader

}; // namespace WebCore

#endif
