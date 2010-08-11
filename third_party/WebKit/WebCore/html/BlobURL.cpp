/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(BLOB)

#include "BlobURL.h"

#include "KURL.h"
#include "PlatformString.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include "UUID.h"

namespace WebCore {

KURL BlobURL::createURL(ScriptExecutionContext* scriptExecutionContext)
{
    // Create the blob URL in the following format:
    //     blob:%escaped_origin%/%UUID%
    // The origin of the host page is encoded in the URL value to allow easy lookup of the origin when the security check needs
    // to be performed.
    String urlString = "blob:";
    urlString += encodeWithURLEscapeSequences(scriptExecutionContext->securityOrigin()->toString());
    urlString += "/";
    urlString += createCanonicalUUIDString();
    return KURL(ParsedURLString, urlString);
}

KURL BlobURL::getOrigin(const KURL& url)
{
    ASSERT(url.protocolIs("blob"));

    unsigned startIndex = url.pathStart();
    unsigned afterEndIndex = url.pathAfterLastSlash();
    String origin = url.string().substring(startIndex, afterEndIndex - startIndex);
    return KURL(ParsedURLString, decodeURLEscapeSequences(origin));
}

} // namespace WebCore

#endif // ENABLE(BLOB)
