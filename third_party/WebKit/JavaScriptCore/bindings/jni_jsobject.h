/*
 * Copyright (C) 2003 Apple Computer, Inc.  All rights reserved.
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
#ifndef _JNI_JSOBJECT_H_
#define _JNI_JSOBJECT_H_

#include <CoreFoundation/CoreFoundation.h>

#include <JavaScriptCore/interpreter.h>
#include <JavaScriptCore/object.h>

#include <JavaVM/jni.h>

#define jlong_to_ptr(a) ((void*)(uintptr_t)(a))
#define jlong_to_impptr(a) (static_cast<KJS::ObjectImp*>(((void*)(uintptr_t)(a))))
#define ptr_to_jlong(a) ((jlong)(uintptr_t)(a))

namespace Bindings {

class RootObject;

typedef RootObject *(*FindRootObjectForNativeHandleFunctionPtr)(void *);

class RootObject
{
public:
    RootObject (const void *nativeHandle) : _nativeHandle(nativeHandle), _imp(0), _interpreter(0) {}
    ~RootObject (){
        _imp->deref();
        CFRelease (_runLoop);
    }
    
    void setRootObjectImp (KJS::ObjectImp *i) { 
        _imp = i;
        _imp->ref();
    }
    
    KJS::ObjectImp *rootObjectImp() const { return _imp; }
    
    void setInterpreter (KJS::Interpreter *i) { _interpreter = i; }
    KJS::Interpreter *interpreter() const { return _interpreter; }

    // Must be called from the thread that will be used to access JavaScript.
    static void setFindRootObjectForNativeHandleFunction(FindRootObjectForNativeHandleFunctionPtr aFunc);
    static FindRootObjectForNativeHandleFunctionPtr findRootObjectForNativeHandleFunction() {
        return _findRootObjectForNativeHandleFunctionPtr;
    }

    static void removeAllJavaReferencesForRoot (Bindings::RootObject *root);
    static CFRunLoopRef runLoop() { return _runLoop; }
    static CFRunLoopSourceRef performJavaScriptSource() { return _performJavaScriptSource; }
    
private:
    const void *_nativeHandle;
    KJS::ObjectImp *_imp;
    KJS::Interpreter *_interpreter;

    static FindRootObjectForNativeHandleFunctionPtr _findRootObjectForNativeHandleFunctionPtr;
    static CFRunLoopRef _runLoop;
    static CFRunLoopSourceRef _performJavaScriptSource;
};

enum JSObjectCallType {
    GetWindow,
    Call,
    Eval,
    GetMember,
    SetMember,
    RemoveMember,
    GetSlot,
    SetSlot,
    ToString,
    Finalize
};

struct JSObjectCallContext
{
    JSObjectCallType type;
    jlong nativeHandle;
    jstring string;
    jobjectArray args;
    jint index;
    jobject value;
    CFRunLoopRef originatingLoop;
    jvalue result;
};

class JSObject
{
public:
    JSObject(jlong nativeHandle);
    
    jobject call(const char *methodName, jobjectArray args) const;
    jobject eval(const char *script) const;
    jobject getMember(const char *memberName) const;
    void setMember(const char *memberName, jobject value) const;
    void removeMember(const char *memberName) const;
    jobject getSlot(jint index) const;
    void setSlot(jint index, jobject value) const;
    jstring toString() const;
    void finalize() const;

    static jlong getWindow(jlong nativeHandle);
    
    static jvalue invoke (JSObjectCallContext *context);
    
private:
    const Bindings::RootObject *_root;
    KJS::ObjectImp *_imp;
};


}

extern "C" {

// Functions called from the Java VM when making calls to the JSObject class.
jlong KJS_JSCreateNativeJSObject (JNIEnv *env, jclass clazz, jstring jurl, jlong nativeHandle, jboolean ctx);
void KJS_JSObject_JSFinalize (JNIEnv *env, jclass jsClass, jlong nativeJSObject);
jobject KJS_JSObject_JSObjectCall (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring methodName, jobjectArray args, jboolean ctx);
jobject KJS_JSObject_JSObjectEval (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring jscript, jboolean ctx);
jobject KJS_JSObject_JSObjectGetMember (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring jname, jboolean ctx);
void KJS_JSObject_JSObjectSetMember (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring jname, jobject value, jboolean ctx);
void KJS_JSObject_JSObjectRemoveMember (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jstring jname, jboolean ctx);
jobject KJS_JSObject_JSObjectGetSlot (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jint jindex, jboolean ctx);
void KJS_JSObject_JSObjectSetSlot (JNIEnv *env, jclass jsClass, jlong nativeJSObject, jstring jurl, jint jindex, jobject value, jboolean ctx);
jstring KJS_JSObject_JSObjectToString (JNIEnv *env, jclass clazz, jlong nativeJSObject);

}

#endif