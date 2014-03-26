// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#include "config.h"
#include "V8TestInterfaceNamedConstructor2.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8HiddenValue.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/frame/DOMWindow.h"
#include "platform/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestInterfaceNamedConstructor2* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestInterfaceNamedConstructor2::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

void webCoreInitializeScriptWrappableForInterface(WebCore::TestInterfaceNamedConstructor2* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestInterfaceNamedConstructor2::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterfaceNamedConstructor2::domTemplate, V8TestInterfaceNamedConstructor2::derefObject, 0, 0, 0, V8TestInterfaceNamedConstructor2::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype, false };

namespace TestInterfaceNamedConstructor2V8Internal {

template <typename T> void V8_USE(T) { }

} // namespace TestInterfaceNamedConstructor2V8Internal

const WrapperTypeInfo V8TestInterfaceNamedConstructor2Constructor::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterfaceNamedConstructor2Constructor::domTemplate, V8TestInterfaceNamedConstructor2::derefObject, 0, 0, 0, V8TestInterfaceNamedConstructor2::installPerContextEnabledMethods, 0, WrapperTypeObjectPrototype, false };

static void V8TestInterfaceNamedConstructor2ConstructorCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    if (!info.IsConstructCall()) {
        throwTypeError(ExceptionMessages::constructorNotCallableAsFunction("Audio"), info.GetIsolate());
        return;
    }

    if (ConstructorMode::current() == ConstructorMode::WrapExistingObject) {
        v8SetReturnValue(info, info.Holder());
        return;
    }

    Document* document = currentDOMWindow(info.GetIsolate())->document();
    ASSERT(document);

    // Make sure the document is added to the DOM Node map. Otherwise, the TestInterfaceNamedConstructor2 instance
    // may end up being the only node in the map and get garbage-collected prematurely.
    toV8(document, info.Holder(), info.GetIsolate());

    if (UNLIKELY(info.Length() < 1)) {
        throwTypeError(ExceptionMessages::failedToConstruct("TestInterfaceNamedConstructor2", ExceptionMessages::notEnoughArguments(1, info.Length())), info.GetIsolate());
        return;
    }
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, stringArg, info[0]);
    RefPtr<TestInterfaceNamedConstructor2> impl = TestInterfaceNamedConstructor2::createForJSConstructor(*document, stringArg);

    v8::Handle<v8::Object> wrapper = info.Holder();
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceNamedConstructor2>(impl.release(), &V8TestInterfaceNamedConstructor2Constructor::wrapperTypeInfo, wrapper, info.GetIsolate(), WrapperConfiguration::Independent);
    v8SetReturnValue(info, wrapper);
}

v8::Handle<v8::FunctionTemplate> V8TestInterfaceNamedConstructor2Constructor::domTemplate(v8::Isolate* isolate)
{
    static int domTemplateKey; // This address is used for a key to look up the dom template.
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    v8::Local<v8::FunctionTemplate> result = data->existingDOMTemplate(&domTemplateKey);
    if (!result.IsEmpty())
        return result;

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    result = v8::FunctionTemplate::New(isolate, V8TestInterfaceNamedConstructor2ConstructorCallback);
    v8::Local<v8::ObjectTemplate> instanceTemplate = result->InstanceTemplate();
    instanceTemplate->SetInternalFieldCount(V8TestInterfaceNamedConstructor2::internalFieldCount);
    result->SetClassName(v8AtomicString(isolate, "TestInterfaceNamedConstructor2"));
    result->Inherit(V8TestInterfaceNamedConstructor2::domTemplate(isolate));
    data->setDOMTemplate(&domTemplateKey, result);
    return result;
}

static void configureV8TestInterfaceNamedConstructor2Template(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestInterfaceNamedConstructor2", v8::Local<v8::FunctionTemplate>(), V8TestInterfaceNamedConstructor2::internalFieldCount,
        0, 0,
        0, 0,
        0, 0,
        isolate);
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED prototypeTemplate = functionTemplate->PrototypeTemplate();

    // Custom toString template
    functionTemplate->Set(v8AtomicString(isolate, "toString"), V8PerIsolateData::current()->toStringTemplate());
}

v8::Handle<v8::FunctionTemplate> V8TestInterfaceNamedConstructor2::domTemplate(v8::Isolate* isolate)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    v8::Local<v8::FunctionTemplate> result = data->existingDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo));
    if (!result.IsEmpty())
        return result;

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    result = v8::FunctionTemplate::New(isolate, V8ObjectConstructor::isValidConstructorMode);
    configureV8TestInterfaceNamedConstructor2Template(result, isolate);
    data->setDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo), result);
    return result;
}

bool V8TestInterfaceNamedConstructor2::hasInstance(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, jsValue);
}

v8::Handle<v8::Object> V8TestInterfaceNamedConstructor2::findInstanceInPrototypeChain(v8::Handle<v8::Value> jsValue, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->findInstanceInPrototypeChain(&wrapperTypeInfo, jsValue);
}

TestInterfaceNamedConstructor2* V8TestInterfaceNamedConstructor2::toNativeWithTypeCheck(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
    return hasInstance(value, isolate) ? fromInternalPointer(v8::Handle<v8::Object>::Cast(value)->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex)) : 0;
}

v8::Handle<v8::Object> V8TestInterfaceNamedConstructor2::createWrapper(PassRefPtr<TestInterfaceNamedConstructor2> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestInterfaceNamedConstructor2>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceNamedConstructor2>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestInterfaceNamedConstructor2::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestInterfaceNamedConstructor2* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
