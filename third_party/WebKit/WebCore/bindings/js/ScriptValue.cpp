/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
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
#include "ScriptValue.h"

#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/JSValueRef.h>

#include "JSInspectedObjectWrapper.h"

#include <runtime/JSLock.h>
#include <runtime/Protect.h>
#include <runtime/UString.h>

using namespace JSC;

namespace WebCore {

#if ENABLE(INSPECTOR)
ScriptValue ScriptValue::quarantineValue(ScriptState* scriptState, const ScriptValue& value)
{
    JSLock lock(SilenceAssertionsOnly);
    return ScriptValue(JSInspectedObjectWrapper::wrap(scriptState, value.jsValue()));
}
#endif

bool ScriptValue::getString(ScriptState* scriptState, String& result) const
{
    if (!m_value)
        return false;
    JSLock lock(SilenceAssertionsOnly);
    UString ustring;
    if (!m_value.get().getString(scriptState, ustring))
        return false;
    result = ustring;
    return true;
}

bool ScriptValue::isEqual(ScriptState* scriptState, const ScriptValue& anotherValue) const
{
    if (hasNoValue())
        return anotherValue.hasNoValue();

    return JSValueIsEqual(toRef(scriptState), toRef(scriptState, jsValue()), toRef(scriptState, anotherValue.jsValue()), 0);
}

bool ScriptValue::isNull() const
{
    if (!m_value)
        return false;
    return m_value.get().isNull();
}

bool ScriptValue::isUndefined() const
{
    if (!m_value)
        return false;
    return m_value.get().isUndefined();
}

bool ScriptValue::isObject() const
{
    if (!m_value)
        return false;
    return m_value.get().isObject();
}

} // namespace WebCore
