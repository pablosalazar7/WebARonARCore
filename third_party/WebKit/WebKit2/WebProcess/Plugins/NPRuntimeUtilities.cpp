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

#include "NPRuntimeUtilities.h"

namespace WebKit {

NPObject* createNPObject(NPP npp, NPClass* npClass)
{
    ASSERT(npClass);
    
    NPObject* npObject;
    if (npClass->allocate)
        npObject = npClass->allocate(npp, npClass);
    else {
        // This should use NPN_MemAlloc, but we know that it uses malloc under the hood.
        npObject = static_cast<NPObject*>(malloc(sizeof(NPObject)));
    }

    npObject->_class = npClass;
    npObject->referenceCount = 1;
    
    return npObject;
}

void deallocateNPObject(NPObject* npObject)
{
    ASSERT(npObject);
    if (!npObject)
        return;

    if (npObject->_class->deallocate)
        npObject->_class->deallocate(npObject);
    else {
        // This should really call NPN_MemFree, but we know that it uses free
        // under the hood so it's fine.
        free(npObject);
    }
}

void retainNPObject(NPObject* npObject)
{
    ASSERT(npObject);
    if (!npObject)
        return;

    npObject->referenceCount++;
}

void releaseNPObject(NPObject* npObject)
{
    ASSERT(npObject);
    if (!npObject)
        return;
    
    ASSERT(npObject->referenceCount >= 1);
    npObject->referenceCount--;
    if (!npObject->referenceCount)
        deallocateNPObject(npObject);
}

void releaseNPVariantValue(NPVariant* variant)
{
    ASSERT(variant);
    
    switch (variant->type) {
    case NPVariantType_Void:
    case NPVariantType_Null:
    case NPVariantType_Bool:
    case NPVariantType_Int32:
    case NPVariantType_Double:
        // Nothing to do.
        break;
        
    case NPVariantType_String:
        free(const_cast<NPUTF8*>(variant->value.stringValue.UTF8Characters));
        variant->value.stringValue.UTF8Characters = 0;
        variant->value.stringValue.UTF8Length = 0;
        break;
    case NPVariantType_Object:
        releaseNPObject(variant->value.objectValue);
        variant->value.objectValue = 0;
        break;
    }

    variant->type = NPVariantType_Void;
}

} // namespace WebKit
