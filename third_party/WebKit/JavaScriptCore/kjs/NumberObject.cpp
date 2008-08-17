/*
 *  Copyright (C) 1999-2000,2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "NumberObject.h"

#include "JSGlobalObject.h"
#include "NumberPrototype.h"

namespace KJS {

ASSERT_CLASS_FITS_IN_CELL(NumberObject);

const ClassInfo NumberObject::info = { "Number", 0, 0, 0 };

NumberObject::NumberObject(JSObject* prototype)
    : JSWrapperObject(prototype)
{
}

JSValue* NumberObject::getJSNumber()
{
    return internalValue();
}

NumberObject* constructNumber(ExecState* exec, JSNumberCell* number)
{
    NumberObject* obj = new (exec) NumberObject(exec->lexicalGlobalObject()->numberPrototype());
    obj->setInternalValue(number);
    return obj;
}

NumberObject* constructNumberFromImmediateNumber(ExecState* exec, JSValue* value)
{
    NumberObject* obj = new (exec) NumberObject(exec->lexicalGlobalObject()->numberPrototype());
    obj->setInternalValue(value);
    return obj;
}

} // namespace KJS
