/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "NPRuntimeObjectMap.h"

#include "JSNPObject.h"
#include "NPJSObject.h"
#include "NPRuntimeUtilities.h"
#include "NotImplemented.h"
#include "PluginView.h"
#include <JavaScriptCore/Error.h>
#include <JavaScriptCore/JSLock.h>
#include <JavaScriptCore/SourceCode.h>
#include <WebCore/Frame.h>

using namespace JSC;
using namespace WebCore;

namespace WebKit {


NPRuntimeObjectMap::NPRuntimeObjectMap(PluginView* pluginView)
    : m_pluginView(pluginView)
{
}

NPRuntimeObjectMap::PluginProtector::PluginProtector(NPRuntimeObjectMap* npRuntimeObjectMap)
{
    // If we're already in the plug-in view destructor, we shouldn't try to keep it alive.
    if (!npRuntimeObjectMap->m_pluginView->isBeingDestroyed())
        m_pluginView = npRuntimeObjectMap->m_pluginView;
}

NPRuntimeObjectMap::PluginProtector::~PluginProtector()
{
}

NPObject* NPRuntimeObjectMap::getOrCreateNPObject(JSObject* jsObject)
{
    // If this is a JSNPObject, we can just get its underlying NPObject.
    if (jsObject->classInfo() == &JSNPObject::s_info) {
        JSNPObject* jsNPObject = static_cast<JSNPObject*>(jsObject);
        NPObject* npObject = jsNPObject->npObject();
        
        retainNPObject(npObject);
        return npObject;
    }
    
    // First, check if we already know about this object.
    if (NPJSObject* npJSObject = m_npJSObjects.get(jsObject)) {
        retainNPObject(npJSObject);
        return npJSObject;
    }

    NPJSObject* npJSObject = NPJSObject::create(this, jsObject);
    m_npJSObjects.set(jsObject, npJSObject);

    return npJSObject;
}

void NPRuntimeObjectMap::npJSObjectDestroyed(NPJSObject* npJSObject)
{
    // Remove the object from the map.
    ASSERT(m_npJSObjects.contains(npJSObject->jsObject()));
    m_npJSObjects.remove(npJSObject->jsObject());
}

JSObject* NPRuntimeObjectMap::getOrCreateJSObject(JSGlobalObject* globalObject, NPObject* npObject)
{
    // If this is an NPJSObject, we can just get the JSObject that it's wrapping.
    if (NPJSObject::isNPJSObject(npObject))
        return NPJSObject::toNPJSObject(npObject)->jsObject();
    
    if (JSNPObject* jsNPObject = m_jsNPObjects.get(npObject))
        return jsNPObject;

    JSNPObject* jsNPObject = new (&globalObject->globalData()) JSNPObject(globalObject, this, npObject);
    m_jsNPObjects.set(npObject, jsNPObject);

    return jsNPObject;
}

void NPRuntimeObjectMap::jsNPObjectDestroyed(JSNPObject* jsNPObject)
{
    // Remove the object from the map.
    ASSERT(m_jsNPObjects.contains(jsNPObject->npObject()));
    m_jsNPObjects.remove(jsNPObject->npObject());
}

JSValue NPRuntimeObjectMap::convertNPVariantToJSValue(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject, const NPVariant& variant)
{
    switch (variant.type) {
    case NPVariantType_Void:
        return jsUndefined();

    case NPVariantType_Null:
        return jsNull();

    case NPVariantType_Bool:
        return jsBoolean(variant.value.boolValue);

    case NPVariantType_Int32:
        return jsNumber(variant.value.intValue);

    case NPVariantType_Double:
        return jsNumber(variant.value.doubleValue);

    case NPVariantType_String:
        return jsString(exec, String::fromUTF8WithLatin1Fallback(variant.value.stringValue.UTF8Characters, 
                                                                 variant.value.stringValue.UTF8Length));
    case NPVariantType_Object:
        return getOrCreateJSObject(globalObject, variant.value.objectValue);
    }

    ASSERT_NOT_REACHED();
    return jsUndefined();
}

void NPRuntimeObjectMap::convertJSValueToNPVariant(ExecState* exec, JSValue value, NPVariant& variant)
{
    JSLock lock(SilenceAssertionsOnly);

    VOID_TO_NPVARIANT(variant);
    
    if (value.isNull()) {
        NULL_TO_NPVARIANT(variant);
        return;
    }

    if (value.isUndefined()) {
        VOID_TO_NPVARIANT(variant);
        return;
    }

    if (value.isBoolean()) {
        BOOLEAN_TO_NPVARIANT(value.toBoolean(exec), variant);
        return;
    }

    if (value.isNumber()) {
        DOUBLE_TO_NPVARIANT(value.toNumber(exec), variant);
        return;
    }

    if (value.isString()) {
        CString utf8String = value.toString(exec).utf8();

        // This should use NPN_MemAlloc, but we know that it uses malloc under the hood.
        char* utf8Characters = static_cast<char*>(malloc(utf8String.length()));
        memcpy(utf8Characters, utf8String.data(), utf8String.length());
        
        STRINGN_TO_NPVARIANT(utf8Characters, utf8String.length(), variant);
        return;
    }

    if (value.isObject()) {
        NPObject* npObject = getOrCreateNPObject(asObject(value));
        OBJECT_TO_NPVARIANT(npObject, variant);
        return;
    }

    ASSERT_NOT_REACHED();
}

bool NPRuntimeObjectMap::evaluate(NPObject* npObject, const String&scriptString, NPVariant* result)
{
    ProtectedPtr<JSGlobalObject> globalObject = this->globalObject();
    if (!globalObject)
        return false;

    ExecState* exec = globalObject->globalExec();
    
    JSLock lock(SilenceAssertionsOnly);
    JSValue thisValue = getOrCreateJSObject(globalObject, npObject);

    globalObject->globalData().timeoutChecker.start();
    Completion completion = JSC::evaluate(exec, globalObject->globalScopeChain(), makeSource(UString(scriptString.impl())), thisValue);
    globalObject->globalData().timeoutChecker.stop();

    ComplType completionType = completion.complType();

    JSValue resultValue;
    if (completionType == Normal) {
        resultValue = completion.value();
        if (!resultValue)
            resultValue = jsUndefined();
    } else
        resultValue = jsUndefined();

    exec->clearException();
    
    convertJSValueToNPVariant(exec, resultValue, *result);
    return true;
}

void NPRuntimeObjectMap::invalidate()
{
    Vector<NPJSObject*> npJSObjects;
    copyValuesToVector(m_npJSObjects, npJSObjects);

    // Deallocate all the object wrappers so we won't leak any JavaScript objects.
    for (size_t i = 0; i < npJSObjects.size(); ++i)
        deallocateNPObject(npJSObjects[i]);
    
    // We shouldn't have any NPJSObjects left now.
    ASSERT(m_npJSObjects.isEmpty());

    Vector<JSNPObject*> jsNPObjects;
    copyValuesToVector(m_jsNPObjects, jsNPObjects);

    // Invalidate all the JSObjects that wrap NPObjects.
    for (size_t i = 0; i < jsNPObjects.size(); ++i)
        jsNPObjects[i]->invalidate();

    m_jsNPObjects.clear();
}

JSGlobalObject* NPRuntimeObjectMap::globalObject() const
{
    Frame* frame = m_pluginView->frame();
    if (!frame)
        return 0;

    return frame->script()->globalObject(pluginWorld());
}

ExecState* NPRuntimeObjectMap::globalExec() const
{
    JSGlobalObject* globalObject = this->globalObject();
    if (!globalObject)
        return 0;
    
    return globalObject->globalExec();
}

static String& globalExceptionString()
{
    DEFINE_STATIC_LOCAL(String, exceptionString, ());
    return exceptionString;
}

void NPRuntimeObjectMap::setGlobalException(const String& exceptionString)
{
    globalExceptionString() = exceptionString;
}
    
void NPRuntimeObjectMap::moveGlobalExceptionToExecState(ExecState* exec)
{
    if (globalExceptionString().isNull())
        return;

    {
        JSLock lock(SilenceAssertionsOnly);
        throwError(exec, createError(exec, stringToUString(globalExceptionString())));
    }
    
    globalExceptionString() = String();
}

} // namespace WebKit
