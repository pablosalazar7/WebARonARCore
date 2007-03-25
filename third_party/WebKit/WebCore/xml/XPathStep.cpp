/*
 * Copyright 2005 Frerich Raabe <raabe@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "XPathStep.h"

#if ENABLE(XPATH)

#include "Document.h"
#include "NamedAttrMap.h"
#include "XPathNSResolver.h"
#include "XPathParser.h"
#include "XPathUtil.h"

namespace WebCore {
namespace XPath {

Step::Step(Axis axis, const NodeTest& nodeTest, const Vector<Predicate*>& predicates)
    : m_axis(axis)
    , m_nodeTest(nodeTest)
    , m_predicates(predicates)
{
}

Step::Step(Axis axis, const NodeTest& nodeTest, const String& namespaceURI, const Vector<Predicate*>& predicates)
    : m_axis(axis)
    , m_nodeTest(nodeTest)
    , m_namespaceURI(namespaceURI)
    , m_predicates(predicates)
{
}

Step::~Step()
{
    deleteAllValues(m_predicates);
}

NodeSet Step::evaluate(Node* context) const
{
    NodeSet nodes = nodesInAxis(context);
    
    EvaluationContext& evaluationContext = Expression::evaluationContext();
    
    for (unsigned i = 0; i < m_predicates.size(); i++) {
        Predicate* predicate = m_predicates[i];

        NodeSet newNodes;
        if (!nodes.isSorted())
            newNodes.markSorted(false);

        evaluationContext.size = nodes.size();
        evaluationContext.position = 1;
        for (unsigned j = 0; j < nodes.size(); j++) {
            Node* node = nodes[j];

            Expression::evaluationContext().node = node;
            EvaluationContext backupCtx = evaluationContext;
            if (predicate->evaluate())
                newNodes.append(node);

            evaluationContext = backupCtx;
            ++evaluationContext.position;
        }

        nodes.swap(newNodes);
    }
    return nodes;
}

NodeSet Step::nodesInAxis(Node* context) const
{
    NodeSet nodes;
    switch (m_axis) {
        case ChildAxis:
            if (context->isAttributeNode()) // In XPath model, attribute nodes do not have children.
                return nodes;

            for (Node* n = context->firstChild(); n; n = n->nextSibling())
                if (nodeMatches(n))
                    nodes.append(n);
            return nodes;
        case DescendantAxis:
            if (context->isAttributeNode()) // In XPath model, attribute nodes do not have children.
                return nodes;

            for (Node* n = context->firstChild(); n; n = n->traverseNextNode(context))
                if (nodeMatches(n))
                    nodes.append(n);
            return nodes;
        case ParentAxis:
            if (context->isAttributeNode()) {
                Node* n = static_cast<Attr*>(context)->ownerElement();
                if (nodeMatches(n))
                    nodes.append(n);
            } else {
                Node* n = context->parentNode();
                if (n && nodeMatches(n))
                    nodes.append(n);
            }
            return nodes;
        case AncestorAxis: {
            Node* n = context;
            if (context->isAttributeNode()) {
                n = static_cast<Attr*>(context)->ownerElement();
                if (nodeMatches(n))
                    nodes.append(n);
            }
            for (n = n->parentNode(); n; n = n->parentNode())
                if (nodeMatches(n))
                    nodes.append(n);
            nodes.reverse();
            return nodes;
        }
        case FollowingSiblingAxis:
            if (context->nodeType() == Node::ATTRIBUTE_NODE ||
                 context->nodeType() == Node::XPATH_NAMESPACE_NODE) 
                return nodes;
            
            for (Node* n = context->nextSibling(); n; n = n->nextSibling())
                if (nodeMatches(n))
                    nodes.append(n);
            return nodes;
        case PrecedingSiblingAxis:
            if (context->nodeType() == Node::ATTRIBUTE_NODE ||
                 context->nodeType() == Node::XPATH_NAMESPACE_NODE)
                return nodes;
            
            for (Node* n = context->previousSibling(); n; n = n->previousSibling())
                if (nodeMatches(n))
                    nodes.append(n);

            nodes.reverse();
            return nodes;
        case FollowingAxis:
            if (context->isAttributeNode()) {
                Node* p = static_cast<Attr*>(context)->ownerElement();
                while ((p = p->traverseNextNode()))
                    if (nodeMatches(p))
                        nodes.append(p);
            } else {
                for (Node* p = context; !isRootDomNode(p); p = p->parentNode()) {
                    for (Node* n = p->nextSibling(); n; n = n->nextSibling()) {
                        if (nodeMatches(n))
                            nodes.append(n);
                        for (Node* c = n->firstChild(); c; c = c->traverseNextNode(n))
                            if (nodeMatches(c))
                                nodes.append(c);
                    }
                }
            }
            return nodes;
        case PrecedingAxis:
            if (context->isAttributeNode())
                context = static_cast<Attr*>(context)->ownerElement();

            for (Node* p = context; !isRootDomNode(p); p = p->parentNode()) {
                for (Node* n = p->previousSibling(); n ; n = n->previousSibling()) {
                    if (nodeMatches(n))
                        nodes.append(n);
                    for (Node* c = n->firstChild(); c; c = c->traverseNextNode(n))
                        if (nodeMatches(c))
                            nodes.append(c);
                }
            }
            nodes.markSorted(false);
            return nodes;
        case AttributeAxis: {
            if (context->nodeType() != Node::ELEMENT_NODE)
                return nodes;

            NamedAttrMap* attrs = context->attributes();
            if (!attrs)
                return nodes;

            for (unsigned long i = 0; i < attrs->length(); ++i) {
                RefPtr<Node> n = attrs->item(i);
                if (nodeMatches(n.get()))
                    nodes.append(n.release());
            }
            return nodes;
        }
        case NamespaceAxis:
            // XPath namespace nodes are not implemented yet.
            return nodes;
        case SelfAxis:
            if (nodeMatches(context))
                nodes.append(context);
            return nodes;
        case DescendantOrSelfAxis:
            if (nodeMatches(context))
                nodes.append(context);
            if (context->isAttributeNode()) // In XPath model, attribute nodes do not have children.
                return nodes;

            for (Node* n = context->firstChild(); n; n = n->traverseNextNode(context))
            if (nodeMatches(n))
                nodes.append(n);
            return nodes;
        case AncestorOrSelfAxis: {
            if (nodeMatches(context))
                nodes.append(context);
            Node* n = context;
            if (context->isAttributeNode()) {
                n = static_cast<Attr*>(context)->ownerElement();
                if (nodeMatches(n))
                    nodes.append(n);
            }
            for (n = n->parentNode(); n; n = n->parentNode())
                if (nodeMatches(n))
                    nodes.append(n);

            nodes.reverse();
            return nodes;
        }
    }
    ASSERT_NOT_REACHED();
    return nodes;
}


bool Step::nodeMatches(Node* node) const
{
    switch (m_nodeTest.kind()) {
        case NodeTest::TextNodeTest:
            return node->nodeType() == Node::TEXT_NODE || node->nodeType() == Node::CDATA_SECTION_NODE;
        case NodeTest::CommentNodeTest:
            return node->nodeType() == Node::COMMENT_NODE;
        case NodeTest::ProcessingInstructionNodeTest: {
            const String& name = m_nodeTest.data();
            return node->nodeType() == Node::PROCESSING_INSTRUCTION_NODE && (name.isEmpty() || node->nodeName() == name);
        }
        case NodeTest::ElementNodeTest:
            return node->isElementNode();
        case NodeTest::AnyNodeTest:
            return true;
        case NodeTest::NameTest: {
            const String& name = m_nodeTest.data();
            if (name == "*")
                return node->nodeType() == primaryNodeType(m_axis) && (m_namespaceURI.isEmpty() || m_namespaceURI == node->namespaceURI());

            if (m_axis == AttributeAxis) {
                // In XPath land, namespace nodes are not accessible on the attribute axis.
                if (name == "xmlns")
                    return false;

                // FIXME: check the namespace!
                return node->nodeName() == name;
            } else if (m_axis == NamespaceAxis) {
                // Node test on the namespace axis is not implemented yet
            } else {
                // We use tagQName here because we don't want the element name in uppercase 
                // like we get with HTML elements.
                // Paths without namespaces should match HTML elements in HTML documents despite those having an XHTML namespace.
                return node->nodeType() == Node::ELEMENT_NODE
                        && static_cast<Element*>(node)->tagQName().localName() == name
                        && ((node->isHTMLElement() && node->document()->isHTMLDocument() && m_namespaceURI.isNull()) || m_namespaceURI == node->namespaceURI());
            }
        }
    }
    ASSERT_NOT_REACHED();
    return false;
}

Node::NodeType Step::primaryNodeType(Axis axis) const
{
    switch (axis) {
        case AttributeAxis:
            return Node::ATTRIBUTE_NODE;
        case NamespaceAxis:
            return Node::XPATH_NAMESPACE_NODE;
        default:
            return Node::ELEMENT_NODE;
    }
}

}
}

#endif // ENABLE(XPATH)
