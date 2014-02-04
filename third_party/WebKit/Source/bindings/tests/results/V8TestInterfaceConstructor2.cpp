/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

// This file has been auto-generated by code_generator_v8.pm. DO NOT MODIFY!

#include "config.h"
#include "V8TestInterfaceConstructor2.h"

#include "RuntimeEnabledFeatures.h"
#include "V8TestInterfaceEmpty.h"
#include "bindings/v8/Dictionary.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "platform/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestInterfaceConstructor2* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestInterfaceConstructor2::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::TestInterfaceConstructor2* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestInterfaceConstructor2::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterfaceConstructor2::domTemplate, V8TestInterfaceConstructor2::derefObject, 0, 0, 0, V8TestInterfaceConstructor2::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype };

namespace TestInterfaceConstructor2V8Internal {

template <typename T> void V8_USE(T) { }

static void constructor1(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, stringArg, info[0]);
    RefPtr<TestInterfaceConstructor2> impl = TestInterfaceConstructor2::create(stringArg);
    v8::Handle<v8::Object> wrapper = info.Holder();

    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceConstructor2>(impl.release(), &V8TestInterfaceConstructor2::wrapperTypeInfo, wrapper, info.GetIsolate(), WrapperConfiguration::Dependent);
    v8SetReturnValue(info, wrapper);
}

static void constructor2(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    V8TRYCATCH_VOID(TestInterfaceEmpty*, testInterfaceEmptyArg, V8TestInterfaceEmpty::hasInstance(info[0], info.GetIsolate()) ? V8TestInterfaceEmpty::toNative(v8::Handle<v8::Object>::Cast(info[0])) : 0);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, defaultUndefinedOptionalStringArg, info[1]);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, defaultNullStringOptionalStringArg, argumentOrNull(info, 2));
    V8TRYCATCH_VOID(Dictionary, defaultUndefinedOptionalDictionaryArg, Dictionary(info[3], info.GetIsolate()));
    if (!defaultUndefinedOptionalDictionaryArg.isUndefinedOrNull() && !defaultUndefinedOptionalDictionaryArg.isObject()) {
        throwTypeError(ExceptionMessages::failedToConstruct("TestInterfaceConstructor2", "parameter 4 ('defaultUndefinedOptionalDictionaryArg') is not an object."), info.GetIsolate());
        return;
    }
    RefPtr<TestInterfaceConstructor2> impl = TestInterfaceConstructor2::create(testInterfaceEmptyArg, defaultUndefinedOptionalStringArg, defaultNullStringOptionalStringArg, defaultUndefinedOptionalDictionaryArg);
    v8::Handle<v8::Object> wrapper = info.Holder();

    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceConstructor2>(impl.release(), &V8TestInterfaceConstructor2::wrapperTypeInfo, wrapper, info.GetIsolate(), WrapperConfiguration::Dependent);
    v8SetReturnValue(info, wrapper);
}

static void constructor(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (((info.Length() == 1))) {
        TestInterfaceConstructor2V8Internal::constructor1(info);
        return;
    }
    if (((info.Length() == 1) && (V8TestInterfaceEmpty::hasInstance(info[0], info.GetIsolate()))) || ((info.Length() == 2) && (V8TestInterfaceEmpty::hasInstance(info[0], info.GetIsolate()))) || ((info.Length() == 3) && (V8TestInterfaceEmpty::hasInstance(info[0], info.GetIsolate()))) || ((info.Length() == 4) && (V8TestInterfaceEmpty::hasInstance(info[0], info.GetIsolate())) && (info[3]->IsUndefined() || info[3]->IsObject()))) {
        TestInterfaceConstructor2V8Internal::constructor2(info);
        return;
    }
    ExceptionState exceptionState(ExceptionState::ConstructionContext, "TestInterfaceConstructor2", info.Holder(), info.GetIsolate());
    if (UNLIKELY(info.Length() < 1)) {
        exceptionState.throwTypeError(ExceptionMessages::notEnoughArguments(1, info.Length()));
        exceptionState.throwIfNeeded();
        return;
    }
    exceptionState.throwTypeError("No matching constructor signature.");
    exceptionState.throwIfNeeded();
}

} // namespace TestInterfaceConstructor2V8Internal

void V8TestInterfaceConstructor2::constructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "DOMConstructor");
    if (!info.IsConstructCall()) {
        throwTypeError(ExceptionMessages::failedToConstruct("TestInterfaceConstructor2", "Please use the 'new' operator, this DOM object constructor cannot be called as a function."), info.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        v8SetReturnValue(info, info.Holder());
        return;
    }

    TestInterfaceConstructor2V8Internal::constructor(info);
}

static void configureV8TestInterfaceConstructor2Template(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestInterfaceConstructor2", v8::Local<v8::FunctionTemplate>(), V8TestInterfaceConstructor2::internalFieldCount,
        0, 0,
        0, 0,
        0, 0,
        isolate, currentWorldType);
    functionTemplate->SetCallHandler(V8TestInterfaceConstructor2::constructorCallback);
    functionTemplate->SetLength(1);
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED prototypeTemplate = functionTemplate->PrototypeTemplate();

    // Custom toString template
    functionTemplate->Set(v8AtomicString(isolate, "toString"), V8PerIsolateData::current()->toStringTemplate());
}

v8::Handle<v8::FunctionTemplate> V8TestInterfaceConstructor2::domTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&wrapperTypeInfo);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::EscapableHandleScope handleScope(isolate);
    v8::Local<v8::FunctionTemplate> templ = v8::FunctionTemplate::New(isolate, V8ObjectConstructor::isValidConstructorMode);
    configureV8TestInterfaceConstructor2Template(templ, isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&wrapperTypeInfo, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Escape(templ);
}

bool V8TestInterfaceConstructor2::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstanceInMainWorld(&wrapperTypeInfo, jsValue)
        || V8PerIsolateData::from(isolate)->hasInstanceInNonMainWorld(&wrapperTypeInfo, jsValue);
}

v8::Handle<v8::Object> V8TestInterfaceConstructor2::createWrapper(PassRefPtr<TestInterfaceConstructor2> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestInterfaceConstructor2>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::wrapperTypeInfo instead of an XXX::wrapperTypeInfo. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == wrapperTypeInfo.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &wrapperTypeInfo, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextEnabledProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceConstructor2>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestInterfaceConstructor2::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestInterfaceConstructor2* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
