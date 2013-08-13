/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StorageArea_h
#define StorageArea_h

#include "wtf/Forward.h"
#include "wtf/text/WTFString.h"

namespace WebCore {

class ExceptionState;
class Frame;
class SecurityOrigin;
class StorageSyncManager;
enum StorageType { LocalStorage, SessionStorage };

class StorageArea {
public:
    virtual ~StorageArea() { }

    // The HTML5 DOM Storage API
    // FIXME: We should pass Document instead of Frame. Also, that parameter should go first.
    virtual unsigned length(ExceptionState&, Frame* sourceFrame) = 0;
    virtual String key(unsigned index, ExceptionState&, Frame* sourceFrame) = 0;
    virtual String getItem(const String& key, ExceptionState&, Frame* sourceFrame) = 0;
    virtual void setItem(const String& key, const String& value, ExceptionState&, Frame* sourceFrame) = 0;
    virtual void removeItem(const String& key, ExceptionState&, Frame* sourceFrame) = 0;
    virtual void clear(ExceptionState&, Frame* sourceFrame) = 0;
    virtual bool contains(const String& key, ExceptionState&, Frame* sourceFrame) = 0;

    virtual bool canAccessStorage(Frame*) = 0;

    virtual size_t memoryBytesUsedByCache() = 0;
};

} // namespace WebCore

#endif // StorageArea_h
