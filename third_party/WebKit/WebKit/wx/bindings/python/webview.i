/*
 * Copyright (C) 2007 Kevin Ollivier  All rights reserved.
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
 
%module webview

%{
#include "wx/wxPython/wxPython.h"
#include "wx/wxPython/pyclasses.h"
#include "WebBrowserShell.h"
#include "WebDOMSelection.h"
#include "WebEdit.h"
#include "WebFrame.h"
#include "WebDOMDefines.h"
#include "WebSettings.h"
#include "WebView.h"

#include "WebDOMAttr.h"
#include "WebDOMCSSStyleDeclaration.h"
#include "WebDOMDocument.h"
#include "WebDOMDocumentFragment.h"
#include "WebDOMDOMSelection.h"
#include "WebDOMElement.h"
#include "WebDOMEventListener.h"
#include "WebDOMNamedNodeMap.h"
#include "WebDOMNode.h"
#include "WebDOMNodeList.h"
#include "WebDOMObject.h"
#include "WebDOMRange.h"

PyObject* createDOMNodeSubtype(WebDOMNode* ptr, bool setThisOwn)
{
    //static wxPyTypeInfoHashMap* typeInfoCache = NULL;

    //if (typeInfoCache == NULL)
    //    typeInfoCache = new wxPyTypeInfoHashMap;

    swig_type_info* swigType = 0; //(*typeInfoCache)[name];
    char* name = 0;
    if (ptr) {
        // it wasn't in the cache, so look it up from SWIG
        switch (ptr->nodeType()) {
            case WebDOMNode::WEBDOM_ELEMENT_NODE:
                name = "WebDOMElement*";
                break;
            case WebDOMNode::WEBDOM_ATTRIBUTE_NODE:
                name = "WebDOMAttr*";
                break;
            default:
                name = "WebDOMNode*";
        }
        swigType = SWIG_TypeQuery(name);
        if (swigType)
            return SWIG_Python_NewPointerObj(ptr, swigType, setThisOwn);
        
        // if it still wasn't found, try looking for a mapped name
        //if (swigType) {
            // and add it to the map if found
        //    (*typeInfoCache)[className] = swigType;
        //}
    }
    
    Py_INCREF(Py_None);
    
    return Py_None;
}

%}
//---------------------------------------------------------------------------

%import core.i
%import windows.i

%typemap(out) WebDOMNode*             { $result = createDOMNodeSubtype($1, (bool)$owner); }
%typemap(out) WebDOMElement*             { $result = createDOMNodeSubtype($1, (bool)$owner); }

MAKE_CONST_WXSTRING(WebViewNameStr);

MustHaveApp(wxWebBrowserShell);
MustHaveApp(wxWebFrame);
MustHaveApp(wxWebView);

%include WebDOMDefines.h
%include WebKitDefines.h

%include WebDOMObject.h
%include WebDOMNode.h

%include WebDOMAttr.h
%include WebDOMDOMSelection.h
%include WebDOMElement.h
%include WebDOMNodeList.h
%include WebDOMRange.h

%include WebBrowserShell.h
%include WebDOMSelection.h
%include WebEdit.h
%include WebFrame.h
%include WebSettings.h
%include WebView.h

%constant wxEventType wxEVT_WEBVIEW_BEFORE_LOAD;
%constant wxEventType wxEVT_WEBVIEW_LOAD;
%constant wxEventType wxEVT_WEBVIEW_NEW_WINDOW;
%constant wxEventType wxEVT_WEBVIEW_RIGHT_CLICK;
%constant wxEventType wxEVT_WEBVIEW_CONSOLE_MESSAGE;
%constant wxEventType wxEVT_WEBVIEW_RECEIVED_TITLE;

%pythoncode {
EVT_WEBVIEW_BEFORE_LOAD = wx.PyEventBinder( wxEVT_WEBVIEW_BEFORE_LOAD, 1 )
EVT_WEBVIEW_LOAD = wx.PyEventBinder( wxEVT_WEBVIEW_LOAD, 1 )
EVT_WEBVIEW_NEW_WINDOW = wx.PyEventBinder( wxEVT_WEBVIEW_NEW_WINDOW, 1 )
EVT_WEBVIEW_RIGHT_CLICK = wx.PyEventBinder( wxEVT_WEBVIEW_RIGHT_CLICK, 1 )
EVT_WEBVIEW_CONSOLE_MESSAGE = wx.PyEventBinder( wxEVT_WEBVIEW_CONSOLE_MESSAGE, 1 )
EVT_WEBVIEW_RECEIVED_TITLE = wx.PyEventBinder( wxEVT_WEBVIEW_RECEIVED_TITLE, 1 )    
}
