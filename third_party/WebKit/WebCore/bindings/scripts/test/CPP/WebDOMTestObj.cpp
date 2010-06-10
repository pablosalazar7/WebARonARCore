/*
 * This file is part of the WebKit open source project.
 * This file has been generated by generate-bindings.pl. DO NOT MODIFY!
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebDOMTestObj.h"

#include "AtomicString.h"
#include "KURL.h"
#include "SerializedScriptValue.h"
#include "TestObj.h"
#include "WebDOMString.h"
#include "WebExceptionHandler.h"
#include "WebNativeEventListener.h"
#include <wtf/GetPtr.h>
#include <wtf/RefPtr.h>

struct WebDOMTestObj::WebDOMTestObjPrivate {
    WebDOMTestObjPrivate(WebCore::TestObj* object = 0)
        : impl(object)
    {
    }

    RefPtr<WebCore::TestObj> impl;
};

WebDOMTestObj::WebDOMTestObj()
    : WebDOMObject()
    , m_impl(0)
{
}

WebDOMTestObj::WebDOMTestObj(WebCore::TestObj* impl)
    : WebDOMObject()
    , m_impl(new WebDOMTestObjPrivate(impl))
{
}

WebDOMTestObj::WebDOMTestObj(const WebDOMTestObj& copy)
    : WebDOMObject()
{
    m_impl = copy.impl() ? new WebDOMTestObjPrivate(copy.impl()) : 0;
}

WebCore::TestObj* WebDOMTestObj::impl() const
{
    return m_impl ? m_impl->impl.get() : 0;
}

WebDOMTestObj::~WebDOMTestObj()
{
    delete m_impl;
    m_impl = 0;
}

int WebDOMTestObj::readOnlyIntAttr() const
{
    if (!impl())
        return 0;

    return impl()->readOnlyIntAttr();
}

WebDOMString WebDOMTestObj::readOnlyStringAttr() const
{
    if (!impl())
        return WebDOMString();

    return static_cast<const WebCore::String&>(impl()->readOnlyStringAttr());
}

WebDOMTestObj WebDOMTestObj::readOnlyTestObjAttr() const
{
    if (!impl())
        return WebDOMTestObj();

    return toWebKit(WTF::getPtr(impl()->readOnlyTestObjAttr()));
}

int WebDOMTestObj::intAttr() const
{
    if (!impl())
        return 0;

    return impl()->intAttr();
}

void WebDOMTestObj::setIntAttr(int newIntAttr)
{
    if (!impl())
        return;

    impl()->setIntAttr(newIntAttr);
}

long long WebDOMTestObj::longLongAttr() const
{
    if (!impl())
        return 0;

    return impl()->longLongAttr();
}

void WebDOMTestObj::setLongLongAttr(long long newLongLongAttr)
{
    if (!impl())
        return;

    impl()->setLongLongAttr(newLongLongAttr);
}

unsigned long long WebDOMTestObj::unsignedLongLongAttr() const
{
    if (!impl())
        return 0;

    return impl()->unsignedLongLongAttr();
}

void WebDOMTestObj::setUnsignedLongLongAttr(unsigned long long newUnsignedLongLongAttr)
{
    if (!impl())
        return;

    impl()->setUnsignedLongLongAttr(newUnsignedLongLongAttr);
}

WebDOMString WebDOMTestObj::stringAttr() const
{
    if (!impl())
        return WebDOMString();

    return static_cast<const WebCore::String&>(impl()->stringAttr());
}

void WebDOMTestObj::setStringAttr(const WebDOMString& newStringAttr)
{
    if (!impl())
        return;

    impl()->setStringAttr(newStringAttr);
}

WebDOMTestObj WebDOMTestObj::testObjAttr() const
{
    if (!impl())
        return WebDOMTestObj();

    return toWebKit(WTF::getPtr(impl()->testObjAttr()));
}

void WebDOMTestObj::setTestObjAttr(const WebDOMTestObj& newTestObjAttr)
{
    if (!impl())
        return;

    impl()->setTestObjAttr(toWebCore(newTestObjAttr));
}

int WebDOMTestObj::attrWithException() const
{
    if (!impl())
        return 0;

    return impl()->attrWithException();
}

void WebDOMTestObj::setAttrWithException(int newAttrWithException)
{
    if (!impl())
        return;

    impl()->setAttrWithException(newAttrWithException);
}

int WebDOMTestObj::attrWithSetterException() const
{
    if (!impl())
        return 0;

    WebCore::ExceptionCode ec = 0;
    int result = impl()->attrWithSetterException(ec);
    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
    return result;
}

void WebDOMTestObj::setAttrWithSetterException(int newAttrWithSetterException)
{
    if (!impl())
        return;

    WebCore::ExceptionCode ec = 0;
    impl()->setAttrWithSetterException(newAttrWithSetterException, ec);
    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
}

int WebDOMTestObj::attrWithGetterException() const
{
    if (!impl())
        return 0;

    return impl()->attrWithGetterException();
}

void WebDOMTestObj::setAttrWithGetterException(int newAttrWithGetterException)
{
    if (!impl())
        return;

    WebCore::ExceptionCode ec = 0;
    impl()->setAttrWithGetterException(newAttrWithGetterException, ec);
    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
}

WebDOMString WebDOMTestObj::scriptStringAttr() const
{
    if (!impl())
        return WebDOMString();

    return static_cast<const WebCore::String&>(impl()->scriptStringAttr());
}

void WebDOMTestObj::voidMethod()
{
    if (!impl())
        return;

    impl()->voidMethod();
}

void WebDOMTestObj::voidMethodWithArgs(int intArg, const WebDOMString& strArg, const WebDOMTestObj& objArg)
{
    if (!impl())
        return;

    impl()->voidMethodWithArgs(intArg, strArg, toWebCore(objArg));
}

int WebDOMTestObj::intMethod()
{
    if (!impl())
        return 0;

    return impl()->intMethod();
}

int WebDOMTestObj::intMethodWithArgs(int intArg, const WebDOMString& strArg, const WebDOMTestObj& objArg)
{
    if (!impl())
        return 0;

    return impl()->intMethodWithArgs(intArg, strArg, toWebCore(objArg));
}

WebDOMTestObj WebDOMTestObj::objMethod()
{
    if (!impl())
        return WebDOMTestObj();

    return toWebKit(WTF::getPtr(impl()->objMethod()));
}

WebDOMTestObj WebDOMTestObj::objMethodWithArgs(int intArg, const WebDOMString& strArg, const WebDOMTestObj& objArg)
{
    if (!impl())
        return WebDOMTestObj();

    return toWebKit(WTF::getPtr(impl()->objMethodWithArgs(intArg, strArg, toWebCore(objArg))));
}

WebDOMTestObj WebDOMTestObj::methodThatRequiresAllArgs(const WebDOMString& strArg, const WebDOMTestObj& objArg)
{
    if (!impl())
        return WebDOMTestObj();

    return toWebKit(WTF::getPtr(impl()->methodThatRequiresAllArgs(strArg, toWebCore(objArg))));
}

WebDOMTestObj WebDOMTestObj::methodThatRequiresAllArgsAndThrows(const WebDOMString& strArg, const WebDOMTestObj& objArg)
{
    if (!impl())
        return WebDOMTestObj();

    WebCore::ExceptionCode ec = 0;
    WebDOMTestObj result = toWebKit(WTF::getPtr(impl()->methodThatRequiresAllArgsAndThrows(strArg, toWebCore(objArg), ec)));
    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
    return result;
}

void WebDOMTestObj::serializedValue(const WebDOMString& serializedArg)
{
    if (!impl())
        return;

    impl()->serializedValue(WebCore::SerializedScriptValue::create(WebCore::String(serializedArg)));
}

void WebDOMTestObj::methodWithException()
{
    if (!impl())
        return;

    WebCore::ExceptionCode ec = 0;
    impl()->methodWithException(ec);
    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
}

void WebDOMTestObj::addEventListener(const WebDOMString& type, const WebDOMEventListener& listener, bool useCapture)
{
    if (!impl())
        return;

    impl()->addEventListener(type, toWebCore(listener), useCapture);
}

void WebDOMTestObj::removeEventListener(const WebDOMString& type, const WebDOMEventListener& listener, bool useCapture)
{
    if (!impl())
        return;

    impl()->removeEventListener(type, toWebCore(listener), useCapture);
}

void WebDOMTestObj::withDynamicFrame()
{
    if (!impl())
        return;

    impl()->withDynamicFrame();
}

void WebDOMTestObj::withDynamicFrameAndArg(int intArg)
{
    if (!impl())
        return;

    impl()->withDynamicFrameAndArg(intArg);
}

void WebDOMTestObj::withDynamicFrameAndOptionalArg(int intArg, int optionalArg)
{
    if (!impl())
        return;

    impl()->withDynamicFrameAndOptionalArg(intArg, optionalArg);
}

void WebDOMTestObj::withScriptStateVoid()
{
    if (!impl())
        return;

    impl()->withScriptStateVoid();
}

WebDOMTestObj WebDOMTestObj::withScriptStateObj()
{
    if (!impl())
        return WebDOMTestObj();

    return toWebKit(WTF::getPtr(impl()->withScriptStateObj()));
}

void WebDOMTestObj::withScriptStateVoidException()
{
    if (!impl())
        return;

    WebCore::ExceptionCode ec = 0;
    impl()->withScriptStateVoidException(ec);
    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
}

WebDOMTestObj WebDOMTestObj::withScriptStateObjException()
{
    if (!impl())
        return WebDOMTestObj();

    WebCore::ExceptionCode ec = 0;
    WebDOMTestObj result = toWebKit(WTF::getPtr(impl()->withScriptStateObjException(ec)));
    webDOMRaiseError(static_cast<WebDOMExceptionCode>(ec));
    return result;
}

void WebDOMTestObj::withScriptExecutionContext()
{
    if (!impl())
        return;

    impl()->withScriptExecutionContext();
}

void WebDOMTestObj::methodWithOptionalArg(int opt)
{
    if (!impl())
        return;

    impl()->methodWithOptionalArg(opt);
}

void WebDOMTestObj::methodWithNonOptionalArgAndOptionalArg(int nonOpt, int opt)
{
    if (!impl())
        return;

    impl()->methodWithNonOptionalArgAndOptionalArg(nonOpt, opt);
}

void WebDOMTestObj::methodWithNonOptionalArgAndTwoOptionalArgs(int nonOpt, int opt1, int opt2)
{
    if (!impl())
        return;

    impl()->methodWithNonOptionalArgAndTwoOptionalArgs(nonOpt, opt1, opt2);
}

WebCore::TestObj* toWebCore(const WebDOMTestObj& wrapper)
{
    return wrapper.impl();
}

WebDOMTestObj toWebKit(WebCore::TestObj* value)
{
    return WebDOMTestObj(value);
}
