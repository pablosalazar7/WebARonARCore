/*
 * Copyright (C) 2009 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef StorageNamespaceProxy_h
#define StorageNamespaceProxy_h

#include "core/storage/StorageArea.h"
#include "core/storage/StorageNamespace.h"

namespace blink { class WebStorageNamespace; }

namespace WebCore {

// Instances of StorageNamespaceProxy are only used to interact with
// SessionStorage, never LocalStorage.
class StorageNamespaceProxy : public StorageNamespace {
public:
    explicit StorageNamespaceProxy(PassOwnPtr<blink::WebStorageNamespace>);
    virtual ~StorageNamespaceProxy();
    virtual PassOwnPtr<StorageArea> storageArea(SecurityOrigin*);

    bool isSameNamespace(const blink::WebStorageNamespace&);

private:
    OwnPtr<blink::WebStorageNamespace> m_storageNamespace;
};

} // namespace WebCore

#endif // StorageNamespaceProxy_h
