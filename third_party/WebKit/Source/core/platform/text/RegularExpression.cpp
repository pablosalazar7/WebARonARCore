/*
 * Copyright (C) 2004, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2011 Peter Varga (pvarga@webkit.org), University of Szeged
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

#include "config.h"
#include "RegularExpression.h"

#include <wtf/BumpPointerAllocator.h>
#include <yarr/Yarr.h>
#include "Logging.h"

namespace WebCore {

RegularExpression::RegularExpression(const String& pattern, TextCaseSensitivity caseSensitivity, MultilineMode multilineMode)
    : m_numSubpatterns(0)
    , m_regExpByteCode(compile(pattern, caseSensitivity, multilineMode))
{
}

PassOwnPtr<JSC::Yarr::BytecodePattern> RegularExpression::compile(const String& patternString, TextCaseSensitivity caseSensitivity, MultilineMode multilineMode)
{
    const char* constructionError = 0;
    JSC::Yarr::YarrPattern pattern(patternString, (caseSensitivity == TextCaseInsensitive), (multilineMode == MultilineEnabled), &constructionError);
    if (constructionError) {
        LOG_ERROR("RegularExpression: YARR compile failed with '%s'", constructionError);
        return nullptr;
    }

    m_numSubpatterns = pattern.m_numSubpatterns;

    return JSC::Yarr::byteCompile(pattern, &m_regexAllocator);
}

int RegularExpression::match(const String& str, int startFrom, int* matchLength) const
{
    if (!m_regExpByteCode)
        return -1;

    if (str.isNull())
        return -1;

    int offsetVectorSize = (m_numSubpatterns + 1) * 2;
    unsigned* offsetVector;
    Vector<unsigned, 32> nonReturnedOvector;

    nonReturnedOvector.resize(offsetVectorSize);
    offsetVector = nonReturnedOvector.data();

    ASSERT(offsetVector);
    for (unsigned j = 0, i = 0; i < m_numSubpatterns + 1; j += 2, i++)
        offsetVector[j] = JSC::Yarr::offsetNoMatch;

    unsigned result;
    if (str.length() <= INT_MAX)
        result = JSC::Yarr::interpret(m_regExpByteCode.get(), str, startFrom, offsetVector);
    else {
        // This code can't handle unsigned offsets. Limit our processing to strings with offsets that 
        // can be represented as ints.
        result = JSC::Yarr::offsetNoMatch;
    }

    if (result == JSC::Yarr::offsetNoMatch)
        return -1;

    // 1 means 1 match; 0 means more than one match. First match is recorded in offsetVector.
    if (matchLength)
        *matchLength = offsetVector[1] - offsetVector[0];
    return offsetVector[0];
}

void replace(String& string, const RegularExpression& target, const String& replacement)
{
    int index = 0;
    while (index < static_cast<int>(string.length())) {
        int matchLength;
        index = target.match(string, index, &matchLength);
        if (index < 0)
            break;
        string.replace(index, matchLength, replacement);
        index += replacement.length();
        if (!matchLength)
            break;  // Avoid infinite loop on 0-length matches, e.g. [a-z]*
    }
}

} // namespace WebCore
