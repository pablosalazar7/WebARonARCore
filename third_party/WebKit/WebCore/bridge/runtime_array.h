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

#ifndef RUNTIME_ARRAY_H_
#define RUNTIME_ARRAY_H_

#include <wtf/OwnPtr.h>

#include "array_instance.h"
#include "runtime.h"

namespace KJS {
    
class RuntimeArray : public JSObject {
public:
    RuntimeArray(ExecState *exec, Bindings::Array *i);
    
    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    virtual bool getOwnPropertySlot(ExecState *, unsigned, PropertySlot&);
    virtual void put(ExecState *exec, const Identifier &propertyName, JSValue *value);
    virtual void put(ExecState *exec, unsigned propertyName, JSValue *value);
    
    virtual bool deleteProperty(ExecState *exec, const Identifier &propertyName);
    virtual bool deleteProperty(ExecState *exec, unsigned propertyName);
    
    virtual const ClassInfo *classInfo() const { return &info; }
    
    unsigned getLength() const { return getConcreteArray()->getLength(); }
    
    Bindings::Array *getConcreteArray() const { return _array.get(); }

    static const ClassInfo info;

private:
    static JSValue *lengthGetter(ExecState *, JSObject *, const Identifier&, const PropertySlot&);
    static JSValue *indexGetter(ExecState *, JSObject *, const Identifier&, const PropertySlot&);

    OwnPtr<Bindings::Array> _array;
};
    
} // namespace KJS

#endif // RUNTIME_ARRAY_H_
