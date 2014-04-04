// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file has been auto-generated by code_generator_v8.py. DO NOT MODIFY!

#include "config.h"
#include "V8TestInterfaceNode.h"

#include "HTMLNames.h"
#include "RuntimeEnabledFeatures.h"
#include "V8TestInterfaceEmpty.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/V8AbstractEventListener.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8EventListenerList.h"
#include "bindings/v8/V8HiddenValue.h"
#include "bindings/v8/V8ObjectConstructor.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/dom/custom/CustomElementCallbackDispatcher.h"
#include "platform/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(TestInterfaceNode* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestInterfaceNode::wrapperTypeInfo);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

void webCoreInitializeScriptWrappableForInterface(WebCore::TestInterfaceNode* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
const WrapperTypeInfo V8TestInterfaceNode::wrapperTypeInfo = { gin::kEmbedderBlink, V8TestInterfaceNode::domTemplate, V8TestInterfaceNode::derefObject, 0, V8TestInterfaceNode::toEventTarget, 0, V8TestInterfaceNode::installPerContextEnabledMethods, &V8Node::wrapperTypeInfo, WrapperTypeObjectPrototype, false };

namespace TestInterfaceNodeV8Internal {

template <typename T> void V8_USE(T) { }

static void stringAttributeAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(holder);
    v8SetReturnValueString(info, impl->stringAttribute(), info.GetIsolate());
}

static void stringAttributeAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfaceNodeV8Internal::stringAttributeAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void stringAttributeAttributeSetter(v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(holder);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, v8Value);
    impl->setStringAttribute(cppValue);
}

static void stringAttributeAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestInterfaceNodeV8Internal::stringAttributeAttributeSetter(v8Value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void readonlyTestInterfaceEmptyAttributeAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(holder);
    v8SetReturnValueFast(info, WTF::getPtr(impl->readonlyTestInterfaceEmptyAttribute()), impl);
}

static void readonlyTestInterfaceEmptyAttributeAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfaceNodeV8Internal::readonlyTestInterfaceEmptyAttributeAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void eventHandlerAttributeAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(holder);
    EventListener* v8Value = impl->eventHandlerAttribute();
    v8SetReturnValue(info, v8Value ? v8::Handle<v8::Value>(V8AbstractEventListener::cast(v8Value)->getListenerObject(impl->executionContext())) : v8::Handle<v8::Value>(v8::Null(info.GetIsolate())));
}

static void eventHandlerAttributeAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfaceNodeV8Internal::eventHandlerAttributeAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void eventHandlerAttributeAttributeSetter(v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(holder);
    impl->setEventHandlerAttribute(V8EventListenerList::getEventListener(v8Value, true, ListenerFindOrCreate));
}

static void eventHandlerAttributeAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    TestInterfaceNodeV8Internal::eventHandlerAttributeAttributeSetter(v8Value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void perWorldBindingsReadonlyTestInterfaceEmptyAttributeAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(holder);
    v8SetReturnValueFast(info, WTF::getPtr(impl->perWorldBindingsReadonlyTestInterfaceEmptyAttribute()), impl);
}

static void perWorldBindingsReadonlyTestInterfaceEmptyAttributeAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfaceNodeV8Internal::perWorldBindingsReadonlyTestInterfaceEmptyAttributeAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void perWorldBindingsReadonlyTestInterfaceEmptyAttributeAttributeGetterForMainWorld(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(holder);
    v8SetReturnValueForMainWorld(info, WTF::getPtr(impl->perWorldBindingsReadonlyTestInterfaceEmptyAttribute()));
}

static void perWorldBindingsReadonlyTestInterfaceEmptyAttributeAttributeGetterCallbackForMainWorld(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfaceNodeV8Internal::perWorldBindingsReadonlyTestInterfaceEmptyAttributeAttributeGetterForMainWorld(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void reflectStringAttributeAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    Element* impl = V8Element::toNative(holder);
    v8SetReturnValueString(info, impl->fastGetAttribute(HTMLNames::reflectstringattributeAttr), info.GetIsolate());
}

static void reflectStringAttributeAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfaceNodeV8Internal::reflectStringAttributeAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void reflectStringAttributeAttributeSetter(v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    Element* impl = V8Element::toNative(holder);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, v8Value);
    impl->setAttribute(HTMLNames::reflectstringattributeAttr, cppValue);
}

static void reflectStringAttributeAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    CustomElementCallbackDispatcher::CallbackDeliveryScope deliveryScope;
    TestInterfaceNodeV8Internal::reflectStringAttributeAttributeSetter(v8Value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void reflectUrlStringAttributeAttributeGetter(const v8::PropertyCallbackInfo<v8::Value>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(holder);
    v8SetReturnValueString(info, impl->getURLAttribute(HTMLNames::reflecturlstringattributeAttr), info.GetIsolate());
}

static void reflectUrlStringAttributeAttributeGetterCallback(v8::Local<v8::String>, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    TestInterfaceNodeV8Internal::reflectUrlStringAttributeAttributeGetter(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void reflectUrlStringAttributeAttributeSetter(v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    v8::Handle<v8::Object> holder = info.Holder();
    Element* impl = V8Element::toNative(holder);
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, cppValue, v8Value);
    impl->setAttribute(HTMLNames::reflecturlstringattributeAttr, cppValue);
}

static void reflectUrlStringAttributeAttributeSetterCallback(v8::Local<v8::String>, v8::Local<v8::Value> v8Value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    CustomElementCallbackDispatcher::CallbackDeliveryScope deliveryScope;
    TestInterfaceNodeV8Internal::reflectUrlStringAttributeAttributeSetter(v8Value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void testInterfaceEmptyMethodMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(info.Holder());
    v8SetReturnValueFast(info, WTF::getPtr(impl->testInterfaceEmptyMethod()), impl);
}

static void testInterfaceEmptyMethodMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterfaceNodeV8Internal::testInterfaceEmptyMethodMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void perWorldBindingsTestInterfaceEmptyMethodMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(info.Holder());
    v8SetReturnValueFast(info, WTF::getPtr(impl->perWorldBindingsTestInterfaceEmptyMethod()), impl);
}

static void perWorldBindingsTestInterfaceEmptyMethodMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterfaceNodeV8Internal::perWorldBindingsTestInterfaceEmptyMethodMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void perWorldBindingsTestInterfaceEmptyMethodMethodForMainWorld(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(info.Holder());
    v8SetReturnValueForMainWorld(info, WTF::getPtr(impl->perWorldBindingsTestInterfaceEmptyMethod()));
}

static void perWorldBindingsTestInterfaceEmptyMethodMethodCallbackForMainWorld(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterfaceNodeV8Internal::perWorldBindingsTestInterfaceEmptyMethodMethodForMainWorld(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArgMethod(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(info.Holder());
    if (UNLIKELY(info.Length() <= 0)) {
        v8SetReturnValueFast(info, WTF::getPtr(impl->perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArg()), impl);
        return;
    }
    V8TRYCATCH_VOID(bool, optionalBooleanArgument, info[0]->BooleanValue());
    v8SetReturnValueFast(info, WTF::getPtr(impl->perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArg(optionalBooleanArgument)), impl);
}

static void perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArgMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterfaceNodeV8Internal::perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArgMethod(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

static void perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArgMethodForMainWorld(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TestInterfaceNode* impl = V8TestInterfaceNode::toNative(info.Holder());
    if (UNLIKELY(info.Length() <= 0)) {
        v8SetReturnValueForMainWorld(info, WTF::getPtr(impl->perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArg()));
        return;
    }
    V8TRYCATCH_VOID(bool, optionalBooleanArgument, info[0]->BooleanValue());
    v8SetReturnValueForMainWorld(info, WTF::getPtr(impl->perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArg(optionalBooleanArgument)));
}

static void perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArgMethodCallbackForMainWorld(const v8::FunctionCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    TestInterfaceNodeV8Internal::perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArgMethodForMainWorld(info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "V8Execution");
}

} // namespace TestInterfaceNodeV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8TestInterfaceNodeAttributes[] = {
    {"stringAttribute", TestInterfaceNodeV8Internal::stringAttributeAttributeGetterCallback, TestInterfaceNodeV8Internal::stringAttributeAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"readonlyTestInterfaceEmptyAttribute", TestInterfaceNodeV8Internal::readonlyTestInterfaceEmptyAttributeAttributeGetterCallback, 0, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"eventHandlerAttribute", TestInterfaceNodeV8Internal::eventHandlerAttributeAttributeGetterCallback, TestInterfaceNodeV8Internal::eventHandlerAttributeAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"perWorldBindingsReadonlyTestInterfaceEmptyAttribute", TestInterfaceNodeV8Internal::perWorldBindingsReadonlyTestInterfaceEmptyAttributeAttributeGetterCallback, 0, TestInterfaceNodeV8Internal::perWorldBindingsReadonlyTestInterfaceEmptyAttributeAttributeGetterCallbackForMainWorld, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"reflectStringAttribute", TestInterfaceNodeV8Internal::reflectStringAttributeAttributeGetterCallback, TestInterfaceNodeV8Internal::reflectStringAttributeAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"reflectUrlStringAttribute", TestInterfaceNodeV8Internal::reflectUrlStringAttributeAttributeGetterCallback, TestInterfaceNodeV8Internal::reflectUrlStringAttributeAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static const V8DOMConfiguration::MethodConfiguration V8TestInterfaceNodeMethods[] = {
    {"testInterfaceEmptyMethod", TestInterfaceNodeV8Internal::testInterfaceEmptyMethodMethodCallback, 0, 0},
    {"perWorldBindingsTestInterfaceEmptyMethod", TestInterfaceNodeV8Internal::perWorldBindingsTestInterfaceEmptyMethodMethodCallback, TestInterfaceNodeV8Internal::perWorldBindingsTestInterfaceEmptyMethodMethodCallbackForMainWorld, 0},
    {"perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArg", TestInterfaceNodeV8Internal::perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArgMethodCallback, TestInterfaceNodeV8Internal::perWorldBindingsTestInterfaceEmptyMethodOptionalBooleanArgMethodCallbackForMainWorld, 0},
};

static void configureV8TestInterfaceNodeTemplate(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Isolate* isolate)
{
    functionTemplate->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(functionTemplate, "TestInterfaceNode", V8Node::domTemplate(isolate), V8TestInterfaceNode::internalFieldCount,
        V8TestInterfaceNodeAttributes, WTF_ARRAY_LENGTH(V8TestInterfaceNodeAttributes),
        0, 0,
        V8TestInterfaceNodeMethods, WTF_ARRAY_LENGTH(V8TestInterfaceNodeMethods),
        isolate);
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED instanceTemplate = functionTemplate->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> ALLOW_UNUSED prototypeTemplate = functionTemplate->PrototypeTemplate();

    // Custom toString template
    functionTemplate->Set(v8AtomicString(isolate, "toString"), V8PerIsolateData::from(isolate)->toStringTemplate());
}

v8::Handle<v8::FunctionTemplate> V8TestInterfaceNode::domTemplate(v8::Isolate* isolate)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    v8::Local<v8::FunctionTemplate> result = data->existingDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo));
    if (!result.IsEmpty())
        return result;

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    result = v8::FunctionTemplate::New(isolate, V8ObjectConstructor::isValidConstructorMode);
    configureV8TestInterfaceNodeTemplate(result, isolate);
    data->setDOMTemplate(const_cast<WrapperTypeInfo*>(&wrapperTypeInfo), result);
    return result;
}

bool V8TestInterfaceNode::hasInstance(v8::Handle<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&wrapperTypeInfo, v8Value);
}

v8::Handle<v8::Object> V8TestInterfaceNode::findInstanceInPrototypeChain(v8::Handle<v8::Value> v8Value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->findInstanceInPrototypeChain(&wrapperTypeInfo, v8Value);
}

TestInterfaceNode* V8TestInterfaceNode::toNativeWithTypeCheck(v8::Isolate* isolate, v8::Handle<v8::Value> value)
{
    return hasInstance(value, isolate) ? fromInternalPointer(v8::Handle<v8::Object>::Cast(value)->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex)) : 0;
}

EventTarget* V8TestInterfaceNode::toEventTarget(v8::Handle<v8::Object> object)
{
    return toNative(object);
}

v8::Handle<v8::Object> V8TestInterfaceNode::createWrapper(PassRefPtr<TestInterfaceNode> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestInterfaceNode>(impl.get(), isolate));
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
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceNode>(impl, &wrapperTypeInfo, wrapper, isolate, WrapperConfiguration::Dependent);
    return wrapper;
}

void V8TestInterfaceNode::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

template<>
v8::Handle<v8::Value> toV8NoInline(TestInterfaceNode* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl, creationContext, isolate);
}

} // namespace WebCore
