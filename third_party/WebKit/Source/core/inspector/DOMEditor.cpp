/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "config.h"
#include "core/inspector/DOMEditor.h"

#include "bindings/v8/ExceptionState.h"
#include "bindings/v8/ExceptionStatePlaceholder.h"
#include "core/dom/DOMException.h"
#include "core/dom/Element.h"
#include "core/dom/Node.h"
#include "core/dom/Text.h"
#include "core/editing/markup.h"
#include "core/inspector/DOMPatchSupport.h"
#include "core/inspector/InspectorHistory.h"
#include "wtf/RefPtr.h"

using namespace std;

namespace WebCore {

class DOMEditor::RemoveChildAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(RemoveChildAction);
public:
    RemoveChildAction(Node* parentNode, Node* node)
        : InspectorHistory::Action("RemoveChild")
        , m_parentNode(parentNode)
        , m_node(node)
    {
    }

    virtual bool perform(ExceptionState& es)
    {
        m_anchorNode = m_node->nextSibling();
        return redo(es);
    }

    virtual bool undo(ExceptionState& es)
    {
        m_parentNode->insertBefore(m_node.get(), m_anchorNode.get(), es);
        return !es.hadException();
    }

    virtual bool redo(ExceptionState& es)
    {
        m_parentNode->removeChild(m_node.get(), es);
        return !es.hadException();
    }

private:
    RefPtr<Node> m_parentNode;
    RefPtr<Node> m_node;
    RefPtr<Node> m_anchorNode;
};

class DOMEditor::InsertBeforeAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(InsertBeforeAction);
public:
    InsertBeforeAction(Node* parentNode, PassRefPtr<Node> node, Node* anchorNode)
        : InspectorHistory::Action("InsertBefore")
        , m_parentNode(parentNode)
        , m_node(node)
        , m_anchorNode(anchorNode)
    {
    }

    virtual bool perform(ExceptionState& es)
    {
        if (m_node->parentNode()) {
            m_removeChildAction = adoptPtr(new RemoveChildAction(m_node->parentNode(), m_node.get()));
            if (!m_removeChildAction->perform(es))
                return false;
        }
        m_parentNode->insertBefore(m_node.get(), m_anchorNode.get(), es);
        return !es.hadException();
    }

    virtual bool undo(ExceptionState& es)
    {
        m_parentNode->removeChild(m_node.get(), es);
        if (es.hadException())
            return false;
        if (m_removeChildAction)
            return m_removeChildAction->undo(es);
        return true;
    }

    virtual bool redo(ExceptionState& es)
    {
        if (m_removeChildAction && !m_removeChildAction->redo(es))
            return false;
        m_parentNode->insertBefore(m_node.get(), m_anchorNode.get(), es);
        return !es.hadException();
    }

private:
    RefPtr<Node> m_parentNode;
    RefPtr<Node> m_node;
    RefPtr<Node> m_anchorNode;
    OwnPtr<RemoveChildAction> m_removeChildAction;
};

class DOMEditor::RemoveAttributeAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(RemoveAttributeAction);
public:
    RemoveAttributeAction(Element* element, const String& name)
        : InspectorHistory::Action("RemoveAttribute")
        , m_element(element)
        , m_name(name)
    {
    }

    virtual bool perform(ExceptionState& es)
    {
        m_value = m_element->getAttribute(m_name);
        return redo(es);
    }

    virtual bool undo(ExceptionState& es)
    {
        m_element->setAttribute(m_name, m_value, es);
        return true;
    }

    virtual bool redo(ExceptionState&)
    {
        m_element->removeAttribute(m_name);
        return true;
    }

private:
    RefPtr<Element> m_element;
    String m_name;
    String m_value;
};

class DOMEditor::SetAttributeAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(SetAttributeAction);
public:
    SetAttributeAction(Element* element, const String& name, const String& value)
        : InspectorHistory::Action("SetAttribute")
        , m_element(element)
        , m_name(name)
        , m_value(value)
        , m_hadAttribute(false)
    {
    }

    virtual bool perform(ExceptionState& es)
    {
        m_hadAttribute = m_element->hasAttribute(m_name);
        if (m_hadAttribute)
            m_oldValue = m_element->getAttribute(m_name);
        return redo(es);
    }

    virtual bool undo(ExceptionState& es)
    {
        if (m_hadAttribute)
            m_element->setAttribute(m_name, m_oldValue, es);
        else
            m_element->removeAttribute(m_name);
        return true;
    }

    virtual bool redo(ExceptionState& es)
    {
        m_element->setAttribute(m_name, m_value, es);
        return true;
    }

private:
    RefPtr<Element> m_element;
    String m_name;
    String m_value;
    bool m_hadAttribute;
    String m_oldValue;
};

class DOMEditor::SetOuterHTMLAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(SetOuterHTMLAction);
public:
    SetOuterHTMLAction(Node* node, const String& html)
        : InspectorHistory::Action("SetOuterHTML")
        , m_node(node)
        , m_nextSibling(node->nextSibling())
        , m_html(html)
        , m_newNode(0)
        , m_history(adoptPtr(new InspectorHistory()))
        , m_domEditor(adoptPtr(new DOMEditor(m_history.get())))
    {
    }

    virtual bool perform(ExceptionState& es)
    {
        m_oldHTML = createMarkup(m_node.get());
        DOMPatchSupport domPatchSupport(m_domEditor.get(), m_node->ownerDocument());
        m_newNode = domPatchSupport.patchNode(m_node.get(), m_html, es);
        return !es.hadException();
    }

    virtual bool undo(ExceptionState& es)
    {
        return m_history->undo(es);
    }

    virtual bool redo(ExceptionState& es)
    {
        return m_history->redo(es);
    }

    Node* newNode()
    {
        return m_newNode;
    }

private:
    RefPtr<Node> m_node;
    RefPtr<Node> m_nextSibling;
    String m_html;
    String m_oldHTML;
    Node* m_newNode;
    OwnPtr<InspectorHistory> m_history;
    OwnPtr<DOMEditor> m_domEditor;
};

class DOMEditor::ReplaceWholeTextAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(ReplaceWholeTextAction);
public:
    ReplaceWholeTextAction(Text* textNode, const String& text)
        : InspectorHistory::Action("ReplaceWholeText")
        , m_textNode(textNode)
        , m_text(text)
    {
    }

    virtual bool perform(ExceptionState& es)
    {
        m_oldText = m_textNode->wholeText();
        return redo(es);
    }

    virtual bool undo(ExceptionState&)
    {
        m_textNode->replaceWholeText(m_oldText);
        return true;
    }

    virtual bool redo(ExceptionState&)
    {
        m_textNode->replaceWholeText(m_text);
        return true;
    }

private:
    RefPtr<Text> m_textNode;
    String m_text;
    String m_oldText;
};

class DOMEditor::ReplaceChildNodeAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(ReplaceChildNodeAction);
public:
    ReplaceChildNodeAction(Node* parentNode, PassRefPtr<Node> newNode, Node* oldNode)
        : InspectorHistory::Action("ReplaceChildNode")
        , m_parentNode(parentNode)
        , m_newNode(newNode)
        , m_oldNode(oldNode)
    {
    }

    virtual bool perform(ExceptionState& es)
    {
        return redo(es);
    }

    virtual bool undo(ExceptionState& es)
    {
        m_parentNode->replaceChild(m_oldNode, m_newNode.get(), es);
        return !es.hadException();
    }

    virtual bool redo(ExceptionState& es)
    {
        m_parentNode->replaceChild(m_newNode, m_oldNode.get(), es);
        return !es.hadException();
    }

private:
    RefPtr<Node> m_parentNode;
    RefPtr<Node> m_newNode;
    RefPtr<Node> m_oldNode;
};

class DOMEditor::SetNodeValueAction : public InspectorHistory::Action {
    WTF_MAKE_NONCOPYABLE(SetNodeValueAction);
public:
    SetNodeValueAction(Node* node, const String& value)
        : InspectorHistory::Action("SetNodeValue")
        , m_node(node)
        , m_value(value)
    {
    }

    virtual bool perform(ExceptionState&)
    {
        m_oldValue = m_node->nodeValue();
        return redo(IGNORE_EXCEPTION);
    }

    virtual bool undo(ExceptionState&)
    {
        m_node->setNodeValue(m_oldValue);
        return true;
    }

    virtual bool redo(ExceptionState&)
    {
        m_node->setNodeValue(m_value);
        return true;
    }

private:
    RefPtr<Node> m_node;
    String m_value;
    String m_oldValue;
};

DOMEditor::DOMEditor(InspectorHistory* history) : m_history(history) { }

DOMEditor::~DOMEditor() { }

bool DOMEditor::insertBefore(Node* parentNode, PassRefPtr<Node> node, Node* anchorNode, ExceptionState& es)
{
    return m_history->perform(adoptPtr(new InsertBeforeAction(parentNode, node, anchorNode)), es);
}

bool DOMEditor::removeChild(Node* parentNode, Node* node, ExceptionState& es)
{
    return m_history->perform(adoptPtr(new RemoveChildAction(parentNode, node)), es);
}

bool DOMEditor::setAttribute(Element* element, const String& name, const String& value, ExceptionState& es)
{
    return m_history->perform(adoptPtr(new SetAttributeAction(element, name, value)), es);
}

bool DOMEditor::removeAttribute(Element* element, const String& name, ExceptionState& es)
{
    return m_history->perform(adoptPtr(new RemoveAttributeAction(element, name)), es);
}

bool DOMEditor::setOuterHTML(Node* node, const String& html, Node** newNode, ExceptionState& es)
{
    OwnPtr<SetOuterHTMLAction> action = adoptPtr(new SetOuterHTMLAction(node, html));
    SetOuterHTMLAction* rawAction = action.get();
    bool result = m_history->perform(action.release(), es);
    if (result)
        *newNode = rawAction->newNode();
    return result;
}

bool DOMEditor::replaceWholeText(Text* textNode, const String& text, ExceptionState& es)
{
    return m_history->perform(adoptPtr(new ReplaceWholeTextAction(textNode, text)), es);
}

bool DOMEditor::replaceChild(Node* parentNode, PassRefPtr<Node> newNode, Node* oldNode, ExceptionState& es)
{
    return m_history->perform(adoptPtr(new ReplaceChildNodeAction(parentNode, newNode, oldNode)), es);
}

bool DOMEditor::setNodeValue(Node* node, const String& value, ExceptionState& es)
{
    return m_history->perform(adoptPtr(new SetNodeValueAction(node, value)), es);
}

static void populateErrorString(ExceptionState& es, ErrorString* errorString)
{
    if (es.hadException())
        *errorString = DOMException::getErrorName(es.code());
}

bool DOMEditor::insertBefore(Node* parentNode, PassRefPtr<Node> node, Node* anchorNode, ErrorString* errorString)
{
    TrackExceptionState es;
    bool result = insertBefore(parentNode, node, anchorNode, es);
    populateErrorString(es, errorString);
    return result;
}

bool DOMEditor::removeChild(Node* parentNode, Node* node, ErrorString* errorString)
{
    TrackExceptionState es;
    bool result = removeChild(parentNode, node, es);
    populateErrorString(es, errorString);
    return result;
}

bool DOMEditor::setAttribute(Element* element, const String& name, const String& value, ErrorString* errorString)
{
    TrackExceptionState es;
    bool result = setAttribute(element, name, value, es);
    populateErrorString(es, errorString);
    return result;
}

bool DOMEditor::removeAttribute(Element* element, const String& name, ErrorString* errorString)
{
    TrackExceptionState es;
    bool result = removeAttribute(element, name, es);
    populateErrorString(es, errorString);
    return result;
}

bool DOMEditor::setOuterHTML(Node* node, const String& html, Node** newNode, ErrorString* errorString)
{
    TrackExceptionState es;
    bool result = setOuterHTML(node, html, newNode, es);
    populateErrorString(es, errorString);
    return result;
}

bool DOMEditor::replaceWholeText(Text* textNode, const String& text, ErrorString* errorString)
{
    TrackExceptionState es;
    bool result = replaceWholeText(textNode, text, es);
    populateErrorString(es, errorString);
    return result;
}

} // namespace WebCore

