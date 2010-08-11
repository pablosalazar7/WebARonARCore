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

#if ENABLE(DATABASE)

#include "WebDOMTestCallback.h"

#include "Class1.h"
#include "Class2.h"
#include "Class3.h"
#include "KURL.h"
#include "TestCallback.h"
#include "WebDOMClass1.h"
#include "WebDOMClass2.h"
#include "WebDOMClass3.h"
#include "WebDOMString.h"
#include "WebExceptionHandler.h"
#include <wtf/GetPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/AtomicString.h>

struct WebDOMTestCallback::WebDOMTestCallbackPrivate {
    WebDOMTestCallbackPrivate(WebCore::TestCallback* object = 0)
        : impl(object)
    {
    }

    RefPtr<WebCore::TestCallback> impl;
};

WebDOMTestCallback::WebDOMTestCallback()
    : WebDOMObject()
    , m_impl(0)
{
}

WebDOMTestCallback::WebDOMTestCallback(WebCore::TestCallback* impl)
    : WebDOMObject()
    , m_impl(new WebDOMTestCallbackPrivate(impl))
{
}

WebDOMTestCallback::WebDOMTestCallback(const WebDOMTestCallback& copy)
    : WebDOMObject()
{
    m_impl = copy.impl() ? new WebDOMTestCallbackPrivate(copy.impl()) : 0;
}

WebDOMTestCallback& WebDOMTestCallback::operator=(const WebDOMTestCallback& copy)
{
    delete m_impl;
    m_impl = copy.impl() ? new WebDOMTestCallbackPrivate(copy.impl()) : 0;
    return *this;
}

WebCore::TestCallback* WebDOMTestCallback::impl() const
{
    return m_impl ? m_impl->impl.get() : 0;
}

WebDOMTestCallback::~WebDOMTestCallback()
{
    delete m_impl;
    m_impl = 0;
}

bool WebDOMTestCallback::callbackWithClass1Param(const WebDOMClass1& class1Param)
{
    if (!impl())
        return false;

    return impl()->callbackWithClass1Param(0, toWebCore(class1Param));
}

bool WebDOMTestCallback::callbackWithClass2Param(const WebDOMClass2& class2Param, const WebDOMString& strArg)
{
    if (!impl())
        return false;

    return impl()->callbackWithClass2Param(0, toWebCore(class2Param), strArg);
}

int WebDOMTestCallback::callbackWithNonBoolReturnType(const WebDOMClass3& class3Param)
{
    if (!impl())
        return 0;

    return impl()->callbackWithNonBoolReturnType(0, toWebCore(class3Param));
}

WebCore::TestCallback* toWebCore(const WebDOMTestCallback& wrapper)
{
    return wrapper.impl();
}

WebDOMTestCallback toWebKit(WebCore::TestCallback* value)
{
    return WebDOMTestCallback(value);
}

#endif // ENABLE(DATABASE)
