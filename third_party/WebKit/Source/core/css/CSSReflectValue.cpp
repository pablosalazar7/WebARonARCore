/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
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
#include "core/css/CSSReflectValue.h"

#include "WebCoreMemoryInstrumentation.h"
#include "core/css/CSSPrimitiveValue.h"
#include <wtf/text/StringBuilder.h>

using namespace std;

namespace WebCore {

String CSSReflectValue::customCssText() const
{
    if (m_mask)
        return m_direction->cssText() + ' ' + m_offset->cssText() + ' ' + m_mask->cssText();
    return m_direction->cssText() + ' ' + m_offset->cssText();
}

String CSSReflectValue::customSerializeResolvingVariables(const HashMap<AtomicString, String>& variables) const
{
    if (m_mask)
        return m_direction->customSerializeResolvingVariables(variables) + ' ' + m_offset->customSerializeResolvingVariables(variables) + ' ' + m_mask->serializeResolvingVariables(variables);
    return m_direction->customSerializeResolvingVariables(variables) + ' ' + m_offset->customSerializeResolvingVariables(variables);
}

void CSSReflectValue::addSubresourceStyleURLs(ListHashSet<KURL>& urls, const StyleSheetContents* styleSheet) const
{
    if (m_mask)
        m_mask->addSubresourceStyleURLs(urls, styleSheet);
}

bool CSSReflectValue::equals(const CSSReflectValue& other) const
{
    return m_direction == other.m_direction
        && compareCSSValuePtr(m_offset, other.m_offset)
        && compareCSSValuePtr(m_mask, other.m_mask);
}

void CSSReflectValue::reportDescendantMemoryUsage(MemoryObjectInfo* memoryObjectInfo) const
{
    MemoryClassInfo info(memoryObjectInfo, this, WebCoreMemoryTypes::CSS);
    info.addMember(m_offset, "offset");
    info.addMember(m_mask, "mask");
}

} // namespace WebCore
