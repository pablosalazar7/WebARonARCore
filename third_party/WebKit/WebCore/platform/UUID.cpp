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
#include "UUID.h"

#include "NotImplemented.h"

#if OS(WINDOWS)
#include <objbase.h>
#elif OS(DARWIN)
#include <CoreFoundation/CoreFoundation.h>
#elif OS(LINUX)
#include <stdio.h>
#endif

namespace WebCore {

String createCanonicalUUIDString()
{
#if OS(WINDOWS)
    GUID uuid = { 0 };
    HRESULT hr = CoCreateGuid(&uuid);
    if (FAILED(hr))
        return String();
    wchar_t uuidStr[40];
    int num = StringFromGUID2(uuid, reinterpret_cast<LPOLESTR>(uuidStr), ARRAYSIZE(uuidStr));
    ASSERT(num == 39);
    return String(uuidStr + 1, num - 3).lower(); // remove opening and closing bracket and make it lower.
#elif OS(DARWIN)
    CFUUIDRef uuid = CFUUIDCreate(0);
    CFStringRef uuidStrRef = CFUUIDCreateString(0, uuid);
    String uuidStr(uuidStrRef);
    CFRelease(uuidStrRef);
    CFRelease(uuid);
    return uuidStr.lower(); // make it lower.
#elif OS(LINUX)
    FILE* fptr = fopen("/proc/sys/kernel/random/uuid", "r");
    if (!fptr)
        return String();
    char uuidStr[37] = {0};
    fgets(uuidStr, sizeof(uuidStr) - 1, fptr);
    fclose(fptr);
    return String(uuidStr).lower(); // make it lower.
#else
    notImplemented();
    return String();
#endif    
}

}
