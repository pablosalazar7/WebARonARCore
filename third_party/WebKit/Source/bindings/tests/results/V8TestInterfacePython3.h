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

#ifndef V8TestInterfacePython3_h
#define V8TestInterfacePython3_h

#include "bindings/tests/idls/TestInterfacePython3.h"
#include "bindings/v8/V8Binding.h"
#include "bindings/v8/V8DOMWrapper.h"
#include "bindings/v8/WrapperTypeInfo.h"
#include "heap/Handle.h"

namespace WebCore {

class V8TestInterfacePython3 {
public:
    static bool hasInstance(v8::Handle<v8::Value>, v8::Isolate*);
    static v8::Handle<v8::FunctionTemplate> domTemplate(v8::Isolate*, WrapperWorldType);
    static TestInterfacePython3* toNative(v8::Handle<v8::Object> object)
    {
        return fromInternalPointer(object->GetAlignedPointerFromInternalField(v8DOMWrapperObjectIndex));
    }
    static const WrapperTypeInfo wrapperTypeInfo;
    static void derefObject(void*);
    static void visitDOMWrapper(void*, const v8::Persistent<v8::Object>&, v8::Isolate*);
    static const int internalFieldCount = v8DefaultWrapperInternalFieldCount + 0;
    static inline void* toInternalPointer(TestInterfacePython3* impl)
    {
        return impl;
    }

    static inline TestInterfacePython3* fromInternalPointer(void* object)
    {
        return static_cast<TestInterfacePython3*>(object);
    }
    static void installPerContextEnabledProperties(v8::Handle<v8::Object>, TestInterfacePython3*, v8::Isolate*) { }
    static void installPerContextEnabledMethods(v8::Handle<v8::Object>, v8::Isolate*) { }

private:
    friend v8::Handle<v8::Object> wrap(TestInterfacePython3*, v8::Handle<v8::Object> creationContext, v8::Isolate*);
    static v8::Handle<v8::Object> createWrapper(PassRefPtr<TestInterfacePython3>, v8::Handle<v8::Object> creationContext, v8::Isolate*);
};

template<>
class WrapperTypeTraits<TestInterfacePython3 > {
public:
    static const WrapperTypeInfo* wrapperTypeInfo() { return &V8TestInterfacePython3::wrapperTypeInfo; }
};

inline v8::Handle<v8::Object> wrap(TestInterfacePython3* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    ASSERT(impl);
    ASSERT(!DOMDataStore::containsWrapper<V8TestInterfacePython3>(impl, isolate));
    return V8TestInterfacePython3::createWrapper(impl, creationContext, isolate);
}

inline v8::Handle<v8::Value> toV8(TestInterfacePython3* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    if (UNLIKELY(!impl))
        return v8::Null(isolate);
    v8::Handle<v8::Value> wrapper = DOMDataStore::getWrapper<V8TestInterfacePython3>(impl, isolate);
    if (!wrapper.IsEmpty())
        return wrapper;
    return wrap(impl, creationContext, isolate);
}

template<typename CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, TestInterfacePython3* impl)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapper<V8TestInterfacePython3>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<typename CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, TestInterfacePython3* impl)
{
    ASSERT(worldType(callbackInfo.GetIsolate()) == MainWorld);
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperForMainWorld<V8TestInterfacePython3>(callbackInfo.GetReturnValue(), impl))
        return;
    v8::Handle<v8::Value> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, TestInterfacePython3* impl, Wrappable* wrappable)
{
    if (UNLIKELY(!impl)) {
        v8SetReturnValueNull(callbackInfo);
        return;
    }
    if (DOMDataStore::setReturnValueFromWrapperFast<V8TestInterfacePython3>(callbackInfo.GetReturnValue(), impl, callbackInfo.Holder(), wrappable))
        return;
    v8::Handle<v8::Object> wrapper = wrap(impl, callbackInfo.Holder(), callbackInfo.GetIsolate());
    v8SetReturnValue(callbackInfo, wrapper);
}

inline v8::Handle<v8::Value> toV8(PassRefPtr<TestInterfacePython3> impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    return toV8(impl.get(), creationContext, isolate);
}

template<class CallbackInfo>
inline void v8SetReturnValue(const CallbackInfo& callbackInfo, PassRefPtr<TestInterfacePython3> impl)
{
    v8SetReturnValue(callbackInfo, impl.get());
}

template<class CallbackInfo>
inline void v8SetReturnValueForMainWorld(const CallbackInfo& callbackInfo, PassRefPtr<TestInterfacePython3> impl)
{
    v8SetReturnValueForMainWorld(callbackInfo, impl.get());
}

template<class CallbackInfo, class Wrappable>
inline void v8SetReturnValueFast(const CallbackInfo& callbackInfo, PassRefPtr<TestInterfacePython3> impl, Wrappable* wrappable)
{
    v8SetReturnValueFast(callbackInfo, impl.get(), wrappable);
}

}
#endif // V8TestInterfacePython3_h
