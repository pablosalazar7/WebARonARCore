/*
    This file is part of the Blink open source project.
    This file has been auto-generated by CodeGeneratorV8.pm. DO NOT MODIFY!

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include "config.h"
#include "V8TestInterfaceImplementedAs.h"

#include "RuntimeEnabledFeatures.h"
#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ScriptController.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMConfiguration.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "core/dom/ContextFeatures.h"
#include "core/dom/Document.h"
#include "core/page/Frame.h"
#include "core/platform/chromium/TraceEvent.h"
#include "wtf/GetPtr.h"
#include "wtf/RefPtr.h"
#include "wtf/UnusedParam.h"

namespace WebCore {

static void initializeScriptWrappableForInterface(RealClass* object)
{
    if (ScriptWrappable::wrapperCanBeStoredInObject(object))
        ScriptWrappable::setTypeInfoInObject(object, &V8TestInterfaceImplementedAs::info);
    else
        ASSERT_NOT_REACHED();
}

} // namespace WebCore

// In ScriptWrappable::init, the use of a local function declaration has an issue on Windows:
// the local declaration does not pick up the surrounding namespace. Therefore, we provide this function
// in the global namespace.
// (More info on the MSVC bug here: http://connect.microsoft.com/VisualStudio/feedback/details/664619/the-namespace-of-local-function-declarations-in-c)
void webCoreInitializeScriptWrappableForInterface(WebCore::RealClass* object)
{
    WebCore::initializeScriptWrappableForInterface(object);
}

namespace WebCore {
WrapperTypeInfo V8TestInterfaceImplementedAs::info = { V8TestInterfaceImplementedAs::GetTemplate, V8TestInterfaceImplementedAs::derefObject, 0, 0, 0, V8TestInterfaceImplementedAs::installPerContextPrototypeProperties, 0, WrapperTypeObjectPrototype };

namespace RealClassV8Internal {

template <typename T> void V8_USE(T) { }

static void aAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    v8SetReturnValueString(info, imp->a(), info.GetIsolate());
    return;
}

static void aAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    RealClassV8Internal::aAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void aAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, v, value);
    imp->setA(v);
    return;
}

static void aAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    RealClassV8Internal::aAttributeSetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void bAttributeGetter(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    v8SetReturnValueFast(info, imp->b(), imp);
    return;
}

static void bAttributeGetterCallback(v8::Local<v8::String> name, const v8::PropertyCallbackInfo<v8::Value>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMGetter");
    RealClassV8Internal::bAttributeGetter(name, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void bAttributeSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(info.Holder());
    V8TRYCATCH_VOID(RealClass*, v, V8TestInterfaceImplementedAs::HasInstance(value, info.GetIsolate(), worldType(info.GetIsolate())) ? V8TestInterfaceImplementedAs::toNative(v8::Handle<v8::Object>::Cast(value)) : 0);
    imp->setB(WTF::getPtr(v));
    return;
}

static void bAttributeSetterCallback(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::PropertyCallbackInfo<void>& info)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMSetter");
    RealClassV8Internal::bAttributeSetter(name, value, info);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void func1Method(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(args.Holder());
    V8TRYCATCH_FOR_V8STRINGRESOURCE_VOID(V8StringResource<>, a, args[0]);
    v8SetReturnValueString(args, imp->func1(a), args.GetIsolate());
    return;
}

static void func1MethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    RealClassV8Internal::func1Method(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

static void funcTestInterfaceImplementedAsParamMethod(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    if (args.Length() < 1) {
        throwNotEnoughArgumentsError(args.GetIsolate());
        return;
    }
    RealClass* imp = V8TestInterfaceImplementedAs::toNative(args.Holder());
    V8TRYCATCH_VOID(RealClass*, orange, V8TestInterfaceImplementedAs::HasInstance(args[0], args.GetIsolate(), worldType(args.GetIsolate())) ? V8TestInterfaceImplementedAs::toNative(v8::Handle<v8::Object>::Cast(args[0])) : 0);
    v8SetReturnValueString(args, imp->funcTestInterfaceImplementedAsParam(orange), args.GetIsolate());
    return;
}

static void funcTestInterfaceImplementedAsParamMethodCallback(const v8::FunctionCallbackInfo<v8::Value>& args)
{
    TRACE_EVENT_SET_SAMPLING_STATE("Blink", "DOMMethod");
    RealClassV8Internal::funcTestInterfaceImplementedAsParamMethod(args);
    TRACE_EVENT_SET_SAMPLING_STATE("V8", "Execution");
}

} // namespace RealClassV8Internal

static const V8DOMConfiguration::AttributeConfiguration V8TestInterfaceImplementedAsAttributes[] = {
    {"a", RealClassV8Internal::aAttributeGetterCallback, RealClassV8Internal::aAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
    {"b", RealClassV8Internal::bAttributeGetterCallback, RealClassV8Internal::bAttributeSetterCallback, 0, 0, 0, static_cast<v8::AccessControl>(v8::DEFAULT), static_cast<v8::PropertyAttribute>(v8::None), 0 /* on instance */},
};

static const V8DOMConfiguration::MethodConfiguration V8TestInterfaceImplementedAsMethods[] = {
    {"func1", RealClassV8Internal::func1MethodCallback, 0, 1},
};

static v8::Handle<v8::FunctionTemplate> ConfigureV8TestInterfaceImplementedAsTemplate(v8::Handle<v8::FunctionTemplate> desc, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    desc->ReadOnlyPrototype();

    v8::Local<v8::Signature> defaultSignature;
    defaultSignature = V8DOMConfiguration::installDOMClassTemplate(desc, "TestInterfaceImplementedAs", v8::Local<v8::FunctionTemplate>(), V8TestInterfaceImplementedAs::internalFieldCount,
        V8TestInterfaceImplementedAsAttributes, WTF_ARRAY_LENGTH(V8TestInterfaceImplementedAsAttributes),
        V8TestInterfaceImplementedAsMethods, WTF_ARRAY_LENGTH(V8TestInterfaceImplementedAsMethods), isolate, currentWorldType);
    UNUSED_PARAM(defaultSignature);
    v8::Local<v8::ObjectTemplate> instance = desc->InstanceTemplate();
    v8::Local<v8::ObjectTemplate> proto = desc->PrototypeTemplate();
    UNUSED_PARAM(instance);
    UNUSED_PARAM(proto);

    // Custom Signature 'funcTestInterfaceImplementedAsParam'
    const int funcTestInterfaceImplementedAsParamArgc = 1;
    v8::Handle<v8::FunctionTemplate> funcTestInterfaceImplementedAsParamArgv[funcTestInterfaceImplementedAsParamArgc] = { V8PerIsolateData::from(isolate)->rawTemplate(&V8TestInterfaceImplementedAs::info, currentWorldType) };
    v8::Handle<v8::Signature> funcTestInterfaceImplementedAsParamSignature = v8::Signature::New(desc, funcTestInterfaceImplementedAsParamArgc, funcTestInterfaceImplementedAsParamArgv);
    proto->Set(v8::String::NewSymbol("funcTestInterfaceImplementedAsParam"), v8::FunctionTemplate::New(RealClassV8Internal::funcTestInterfaceImplementedAsParamMethodCallback, v8Undefined(), funcTestInterfaceImplementedAsParamSignature, 1));

    // Custom toString template
    desc->Set(v8::String::NewSymbol("toString"), V8PerIsolateData::current()->toStringTemplate());
    return desc;
}

v8::Handle<v8::FunctionTemplate> V8TestInterfaceImplementedAs::GetTemplate(v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    V8PerIsolateData* data = V8PerIsolateData::from(isolate);
    V8PerIsolateData::TemplateMap::iterator result = data->templateMap(currentWorldType).find(&info);
    if (result != data->templateMap(currentWorldType).end())
        return result->value.newLocal(isolate);

    TRACE_EVENT_SCOPED_SAMPLING_STATE("Blink", "BuildDOMTemplate");
    v8::HandleScope handleScope(isolate);
    v8::Handle<v8::FunctionTemplate> templ =
        ConfigureV8TestInterfaceImplementedAsTemplate(data->rawTemplate(&info, currentWorldType), isolate, currentWorldType);
    data->templateMap(currentWorldType).add(&info, UnsafePersistent<v8::FunctionTemplate>(isolate, templ));
    return handleScope.Close(templ);
}

bool V8TestInterfaceImplementedAs::HasInstance(v8::Handle<v8::Value> value, v8::Isolate* isolate, WrapperWorldType currentWorldType)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, currentWorldType);
}

bool V8TestInterfaceImplementedAs::HasInstanceInAnyWorld(v8::Handle<v8::Value> value, v8::Isolate* isolate)
{
    return V8PerIsolateData::from(isolate)->hasInstance(&info, value, MainWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, IsolatedWorld)
        || V8PerIsolateData::from(isolate)->hasInstance(&info, value, WorkerWorld);
}

v8::Handle<v8::Object> V8TestInterfaceImplementedAs::createWrapper(PassRefPtr<RealClass> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl.get());
    ASSERT(!DOMDataStore::containsWrapper<V8TestInterfaceImplementedAs>(impl.get(), isolate));
    if (ScriptWrappable::wrapperCanBeStoredInObject(impl.get())) {
        const WrapperTypeInfo* actualInfo = ScriptWrappable::getTypeInfoFromObject(impl.get());
        // Might be a XXXConstructor::info instead of an XXX::info. These will both have
        // the same object de-ref functions, though, so use that as the basis of the check.
        RELEASE_ASSERT_WITH_SECURITY_IMPLICATION(actualInfo->derefObjectFunction == info.derefObjectFunction);
    }

    v8::Handle<v8::Object> wrapper = V8DOMWrapper::createWrapper(creationContext, &info, toInternalPointer(impl.get()), isolate);
    if (UNLIKELY(wrapper.IsEmpty()))
        return wrapper;

    installPerContextProperties(wrapper, impl.get(), isolate);
    V8DOMWrapper::associateObjectWithWrapper<V8TestInterfaceImplementedAs>(impl, &info, wrapper, isolate, WrapperConfiguration::Independent);
    return wrapper;
}

void V8TestInterfaceImplementedAs::derefObject(void* object)
{
    fromInternalPointer(object)->deref();
}

} // namespace WebCore
