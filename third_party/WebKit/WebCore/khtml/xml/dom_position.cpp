/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
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

#include "dom_position.h"

#include "css_computedstyle.h"
#include "css_valueimpl.h"
#include "dom2_viewsimpl.h"
#include "helper.h"
#include "htmltags.h"
#include "khtml_text_operations.h"
#include "qstring.h"
#include "rendering/render_block.h"
#include "rendering/render_flow.h"
#include "rendering/render_line.h"
#include "rendering/render_object.h"
#include "rendering/render_style.h"
#include "rendering/render_text.h"
#include "xml/dom_positioniterator.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_nodeimpl.h"

#if APPLE_CHANGES
#include "KWQAssertions.h"
#include "KWQLogging.h"
#else
#define ASSERT(assertion) assert(assertion)
#define LOG(channel, formatAndArgs...) ((void)0)
#endif

using khtml::CharacterIterator;
using khtml::findWordBoundary;
using khtml::InlineBox;
using khtml::InlineFlowBox;
using khtml::InlineTextBox;
using khtml::nextWordFromIndex;
using khtml::PRE;
using khtml::RenderBlock;
using khtml::RenderFlow;
using khtml::RenderObject;
using khtml::RenderStyle;
using khtml::RenderText;
using khtml::RootInlineBox;
using khtml::SimplifiedBackwardsTextIterator;
using khtml::TextIterator;
using khtml::VISIBLE;

namespace DOM {

static bool renderersOnDifferentLine(RenderObject *r1, long o1, RenderObject *r2, long o2)
{
    InlineBox *b1 = r1 ? r1->inlineBox(o1) : 0;
    InlineBox *b2 = r2 ? r2->inlineBox(o2) : 0;
    return (b1 && b2 && b1->root() != b2->root());
}

static NodeImpl *nextRenderedEditable(NodeImpl *node)
{
    while (1) {
        node = node->nextEditable();
        if (!node)
            return 0;
        if (!node->renderer())
            continue;
        if (node->renderer()->inlineBox(0))
            return node;
    }
    return 0;
}

static NodeImpl *previousRenderedEditable(NodeImpl *node)
{
    while (1) {
        node = node->previousEditable();
        if (!node)
            return 0;
        if (!node->renderer())
            continue;
        if (node->renderer()->inlineBox(0))
            return node;
    }
    return 0;
}


Position::Position(NodeImpl *node, long offset) 
    : m_node(node), m_offset(offset) 
{ 
    if (node) {
        node->ref();
    }
}

Position::Position(const Position &o)
    : m_node(o.m_node), m_offset(o.m_offset) 
{
    if (m_node) {
        m_node->ref();
    }
}

Position::~Position()
{
    if (m_node) {
        m_node->deref();
    }
}

Position &Position::operator=(const Position &o)
{
    if (m_node) {
        m_node->deref();
    }
    m_node = o.node();
    if (m_node) {
        m_node->ref();
    }

    m_offset = o.offset();
    
    return *this;
}

ElementImpl *Position::element() const
{
    if (isEmpty())
        return 0;
        
    NodeImpl *n = node();
    for (; n && !n->isElementNode(); n = n->parentNode()); //loop
        
    return static_cast<ElementImpl *>(n);
}

CSSComputedStyleDeclarationImpl *Position::computedStyle() const
{
    if (isEmpty())
        return 0;
    
    ElementImpl *elem = element();
    if (!elem)
        return 0;
        
    return new CSSComputedStyleDeclarationImpl(elem);
}

long Position::renderedOffset() const
{
    if (!node()->isTextNode())
        return offset();
   
    if (!node()->renderer())
        return offset();
                    
    long result = 0;
    RenderText *textRenderer = static_cast<RenderText *>(node()->renderer());
    for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
        long start = box->m_start;
        long end = box->m_start + box->m_len;
        if (offset() < start)
            return result;
        if (offset() <= end) {
            result += offset() - start;
            return result;
        }
        result += box->m_len;
    }
    return result;
}

Position Position::equivalentLeafPosition() const
{
    if (isEmpty())
        return Position();

    if (!node()->renderer() || !node()->renderer()->firstChild())
        return *this;
    
    NodeImpl *n = node();
    int count = 0;
    while (1) {
        n = n->nextLeafNode();
        if (!n || !n->inSameContainingBlockFlowElement(node()))
            return *this;
        if (count + n->maxOffset() >= offset()) {
            count = offset() - count;
            break;
        }
        count += n->maxOffset();
    }
    return Position(n, count);
}

Position Position::previousRenderedEditablePosition() const
{
    if (isEmpty())
        return Position();

    if (node()->isContentEditable() && node()->hasChildNodes() == false && inRenderedContent())
        return *this;

    NodeImpl *n = node();
    while (1) {
        n = n->previousEditable();
        if (!n)
            return Position();
        if (n->renderer() && n->renderer()->style()->visibility() == VISIBLE)
            break;
    }
    
    return Position(n, 0);
}

Position Position::nextRenderedEditablePosition() const
{
    if (isEmpty())
        return Position();

    if (node()->isContentEditable() && node()->hasChildNodes() == false && inRenderedContent())
        return *this;

    NodeImpl *n = node();
    while (1) {
        n = n->nextEditable();
        if (!n)
            return Position();
        if (n->renderer() && n->renderer()->style()->visibility() == VISIBLE)
            break;
    }
    
    return Position(n, 0);
}

Position Position::previousCharacterPosition() const
{
    if (isEmpty())
        return Position();

    NodeImpl *fromRootEditableElement = node()->rootEditableElement();
    PositionIterator it(*this);

    bool atStartOfLine = isFirstRenderedPositionOnLine();
    bool rendered = inRenderedContent();
    
    while (!it.atStart()) {
        Position pos = it.previous();

        if (pos.node()->rootEditableElement() != fromRootEditableElement)
            return *this;

        if (atStartOfLine || !rendered) {
            if (pos.inRenderedContent())
                return pos;
        }
        else if (rendersInDifferentPosition(pos))
            return pos;
    }
    
    return *this;
}

Position Position::nextCharacterPosition() const
{
    if (isEmpty())
        return Position();

    NodeImpl *fromRootEditableElement = node()->rootEditableElement();
    PositionIterator it(*this);

    bool atEndOfLine = isLastRenderedPositionOnLine();
    bool rendered = inRenderedContent();
    
    while (!it.atEnd()) {
        Position pos = it.next();

        if (pos.node()->rootEditableElement() != fromRootEditableElement)
            return *this;

        if (atEndOfLine || !rendered) {
            if (pos.inRenderedContent())
                return pos;
        }
        else if (rendersInDifferentPosition(pos))
            return pos;
    }
    
    return *this;
}

Position Position::previousWordBoundary() const
{
    if (isEmpty())
        return Position();

    Position pos = *this;
    int tries = 0;
    while (tries < 2) {
        if (pos.node()->nodeType() == Node::TEXT_NODE || pos.node()->nodeType() == Node::CDATA_SECTION_NODE) {
            DOMString t = pos.node()->nodeValue();
            if (t.isEmpty())
                return *this;
            QChar *chars = t.unicode();
            uint len = t.length();
            int start, end;
            findWordBoundary(chars, len, pos.offset(), &start, &end);
            pos = Position(pos.node(), start);
            if (pos != *this)
                return pos;
            else {
                pos = previousCharacterPosition();
                if (pos == *this) {
                    // made no difference from the last try, might as well bail
                    break;
                }
            }
        }
        else {
            pos = Position(pos.node(), pos.node()->caretMinOffset());
            if (pos != *this)
                return pos;
        }
        tries++;
    }
    
    return *this;
}

Position Position::nextWordBoundary() const
{
    if (isEmpty())
        return Position();

    Position pos = *this;
    int tries = 0;
    while (tries < 2) {
        if (pos.node()->nodeType() == Node::TEXT_NODE || pos.node()->nodeType() == Node::CDATA_SECTION_NODE) {
            DOMString t = pos.node()->nodeValue();
            if (t.isEmpty())
                return *this;
            QChar *chars = t.unicode();
            uint len = t.length();
            int start, end;
            findWordBoundary(chars, len, pos.offset(), &start, &end);
            pos = Position(pos.node(), end);
            if (pos != *this)
                return pos;
            else {
                pos = nextCharacterPosition();
                if (pos == *this) {
                    // made no difference from the last try, might as well bail
                    break;
                }
            }
        } 
        else {
            pos = Position(pos.node(), pos.node()->caretMaxOffset());
            if (pos != *this)
                return pos;
        }
        tries++;
    }
    
    return *this;
}

Position Position::previousWordPosition() const
{
    NodeImpl *n = node();
    if (!n)
        return Position();
    DocumentImpl *d = n->getDocument();
    if (!d)
        return Position();
    NodeImpl *de = d->documentElement();
    if (!de)
        return Position();

    Range searchRange(d);
    searchRange.setStartBefore(de);
    Position end(equivalentRangeCompliantPosition());
    searchRange.setEnd(end.node(), end.offset());
    SimplifiedBackwardsTextIterator it(searchRange);
    QString string;
    unsigned next = 0;
    while (!it.atEnd() && it.length() > 0) {
        // Keep asking the iterator for chunks until the nextWordFromIndex() function
        // returns a non-zero value.
        string.prepend(it.characters(), it.length());
        next = nextWordFromIndex(const_cast<QChar *>(string.unicode()), string.length(), string.length(), false);
        if (next != 0)
            break;
        it.advance();
    }
    
    Position pos(*this);
    if (it.atEnd() && next == 0) {
        Range range(it.range());
        pos = Position(range.startContainer().handle(), range.startOffset());
    }
    else if (!it.atEnd() && it.length() == 0) {
        // Got a zero-length chunk.
        // This means we have hit a replaced element.
        // Make a check to see if the position should be before or after the replaced element
        // by performing an additional check with a modified string which uses an "X" 
        // character to stand in for the replaced element.
        QChar chars[2];
        chars[0] = 'X';
        chars[1] = ' ';
        string.prepend(chars, 2);
        unsigned pastImage = nextWordFromIndex(const_cast<QChar *>(string.unicode()), string.length(), string.length(), false);
        Range range(it.range());
        if (pastImage == 0)
            pos = Position(range.startContainer().handle(), range.startOffset());
        else
            pos = Position(range.endContainer().handle(), range.endOffset());
    }
    else if (next != 0) {
        // The simpler iterator used in this function, as compared to the one used in 
        // nextWordPosition(), gives us results we can use directly without having to 
        // iterate again to translate the next value into a DOM position. 
        NodeImpl *node = it.range().startContainer().handle();
        if (node->isTextNode()) {
            // The next variable contains a usable index into a text node
            pos = Position(node, next);
        }
        else {
            // If we are not in a text node, we ended on a node boundary, so the
            // range start offset should be used.
            pos = Position(node, it.range().startOffset());
        }
    }
    // Use DOWNSTREAM here so that we don't jump past words at the start of lines.
    // <rdar://problem/3765519> REGRESSION (Mail): word movement goes too far upstream at start of line
    pos = pos.equivalentDeepPosition().closestRenderedPosition(DOWNSTREAM);
    return pos;
}

Position Position::nextWordPosition() const
{
    NodeImpl *n = node();
    if (!n)
        return Position();
    DocumentImpl *d = n->getDocument();
    if (!d)
        return Position();
    NodeImpl *de = d->documentElement();
    if (!de)
        return Position();

    Range searchRange(d);
    Position start(equivalentRangeCompliantPosition());
    searchRange.setStart(start.node(), start.offset());
    searchRange.setEndAfter(de);
    TextIterator it(searchRange);
    QString string;
    unsigned next = 0;
    while (!it.atEnd() && it.length() > 0) {
        // Keep asking the iterator for chunks until the nextWordFromIndex() function
        // returns a value not equal to the length of the string passed to it.
        string.append(it.characters(), it.length());
        next = nextWordFromIndex(const_cast<QChar *>(string.unicode()), string.length(), 0, true);
        if (next != string.length())
            break;
        it.advance();
    }
    
    Position pos(*this);
    if (it.atEnd() && next == string.length()) {
        Range range(it.range());
        pos = Position(range.startContainer().handle(), range.startOffset());
    }
    else if (!it.atEnd() && it.length() == 0) {
        // Got a zero-length chunk.
        // This means we have hit a replaced element.
        // Make a check to see if the position should be before or after the replaced element
        // by performing an additional check with a modified string which uses an "X" 
        // character to stand in for the replaced element.
        QChar chars[2];
        chars[0] = ' ';
        chars[1] = 'X';
        string.append(chars, 2);
        unsigned pastImage = nextWordFromIndex(const_cast<QChar *>(string.unicode()), string.length(), 0, true);
        Range range(it.range());
        if (next != pastImage)
            pos = Position(range.endContainer().handle(), range.endOffset());
        else
            pos = Position(range.startContainer().handle(), range.startOffset());
    }
    else if (next != 0) {
        // Use the character iterator to translate the next value into a DOM position.
        CharacterIterator charIt(searchRange);
        charIt.advance(next - 1);
        pos = Position(charIt.range().endContainer().handle(), charIt.range().endOffset());
    }
    pos = pos.equivalentDeepPosition().closestRenderedPosition(UPSTREAM);
    return pos;
}

Position Position::previousLinePosition(int x) const
{
    if (!node())
        return Position();

    if (!node()->renderer())
        return *this;

    RenderBlock *containingBlock = 0;
    RootInlineBox *root = 0;
    InlineBox *box = node()->renderer()->inlineBox(offset());
    if (box) {
        root = box->root()->prevRootBox();
        if (root)
            containingBlock = node()->renderer()->containingBlock();
    }

    if (!root) {
        // This containing editable block does not have a previous line.
        // Need to move back to previous containing editable block in this root editable
        // block and find the last root line box in that block.
        NodeImpl *startBlock = node()->enclosingBlockFlowElement();
        NodeImpl *n = node()->previousEditable();
        while (n && startBlock == n->enclosingBlockFlowElement())
            n = n->previousEditable();
        while (n) {
            if (!n->inSameRootEditableElement(node()))
                break;
            Position pos(n, n->caretMinOffset());
            if (pos.inRenderedContent()) {
                ASSERT(n->renderer());
                box = n->renderer()->inlineBox(n->caretMaxOffset());
                if (box) {
                    // previous root line box found
                    root = box->root();
                    containingBlock = n->renderer()->containingBlock();
                    break;
                }
                else {
                    return pos;
                }
            }
            n = n->previousEditable();
        }
    }
    
    if (root) {
        int absx, absy;
        containingBlock->absolutePosition(absx, absy);
        RenderObject *renderer = root->closestLeafChildForXPos(x, absx)->object();
        return renderer->positionForCoordinates(x, absy + root->topOverflow());
    }
    
    // Could not find a previous line. This means we must already be on the first line.
    // Move to the start of the content in this block, which effectively moves us
    // to the start of the line we're on.
    return Position(node()->rootEditableElement(), 0);
}

Position Position::nextLinePosition(int x) const
{
    if (!node())
        return Position();

    if (!node()->renderer())
        return *this;

    RenderBlock *containingBlock = 0;
    RootInlineBox *root = 0;
    InlineBox *box = node()->renderer()->inlineBox(offset());
    if (box) {
        root = box->root()->nextRootBox();
        if (root)
            containingBlock = node()->renderer()->containingBlock();
    }

    if (!root) {
        // This containing editable block does not have a next line.
        // Need to move forward to next containing editable block in this root editable
        // block and find the first root line box in that block.
        NodeImpl *startBlock = node()->enclosingBlockFlowElement();
        NodeImpl *n = node()->nextEditable();
        while (n && startBlock == n->enclosingBlockFlowElement())
            n = n->nextEditable();
        while (n) {
            if (!n->inSameRootEditableElement(node()))
                break;
            Position pos(n, n->caretMinOffset());
            if (pos.inRenderedContent()) {
                ASSERT(n->renderer());
                box = n->renderer()->inlineBox(n->caretMinOffset());
                if (box) {
                    // next root line box found
                    root = box->root();
                    containingBlock = n->renderer()->containingBlock();
                    break;
                }
                else {
                    return pos;
                }
            }
            n = n->nextEditable();
        }
    }
    
    if (root) {
        int absx, absy;
        containingBlock->absolutePosition(absx, absy);
        RenderObject *renderer = root->closestLeafChildForXPos(x, absx)->object();
        return renderer->positionForCoordinates(x, absy + root->topOverflow());
    }    

    // Could not find a next line. This means we must already be on the last line.
    // Move to the end of the content in this block, which effectively moves us
    // to the end of the line we're on.
    ElementImpl *rootElement = node()->rootEditableElement();
    return Position(rootElement, rootElement ? rootElement->childNodeCount() : 0);
}

Position Position::upstream(EStayInBlock stayInBlock) const
{
    if (!node())
        return Position();

    NodeImpl *block = node()->isBlockFlow() ? node() : node()->enclosingBlockFlowElement();
    
    PositionIterator it(equivalentDeepPosition());            
    for (; !it.atStart(); it.previous()) {
        if (stayInBlock) {
            NodeImpl *currentBlock = it.current().node()->isBlockFlow() ? it.current().node() : it.current().node()->enclosingBlockFlowElement();
            if (block != currentBlock)
                return it.next();
        }

        RenderObject *renderer = it.current().node()->renderer();
        if (!renderer)
            continue;

        if (renderer->style()->visibility() != VISIBLE)
            continue;

        if ((it.current().node() != node() && renderer->isBlockFlow()) || renderer->isReplaced() || renderer->isBR()) {
            if (it.current().offset() >= renderer->caretMaxOffset())
                return Position(it.current().node(), renderer->caretMaxOffset());
            else
                continue;
        }

        if (renderer->isText() && static_cast<RenderText *>(renderer)->firstTextBox()) {
            if (it.current().node() != node())
                return Position(it.current().node(), renderer->caretMaxOffset());

            if (it.current().offset() < 0)
                continue;
            uint textOffset = it.current().offset();

            RenderText *textRenderer = static_cast<RenderText *>(renderer);
            for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
                if (textOffset > box->start() && textOffset <= box->start() + box->len())
                    return it.current();
                else if (box != textRenderer->lastTextBox() && 
                         !box->nextOnLine() && 
                         textOffset == box->start() + box->len() + 1)
                    return it.current();
            }
        }
    }
    
    return it.current();
}

Position Position::downstream(EStayInBlock stayInBlock) const
{
    if (!node())
        return Position();

    NodeImpl *block = node()->isBlockFlow() ? node() : node()->enclosingBlockFlowElement();
    
    PositionIterator it(equivalentDeepPosition());            
    for (; !it.atEnd(); it.next()) {   
        if (stayInBlock) {
            NodeImpl *currentBlock = it.current().node()->isBlockFlow() ? it.current().node() : it.current().node()->enclosingBlockFlowElement();
            if (block != currentBlock)
                return it.previous();
        }

        RenderObject *renderer = it.current().node()->renderer();
        if (!renderer)
            continue;

        if (renderer->style()->visibility() != VISIBLE)
            continue;

        if ((it.current().node() != node() && renderer->isBlockFlow()) || renderer->isReplaced() || renderer->isBR()) {
            if (it.current().offset() <= renderer->caretMinOffset())
                return Position(it.current().node(), renderer->caretMinOffset());
            else
                continue;
        }

        if (renderer->isText() && static_cast<RenderText *>(renderer)->firstTextBox()) {
            if (it.current().node() != node())
                return Position(it.current().node(), renderer->caretMinOffset());

            if (it.current().offset() < 0)
                continue;
            uint textOffset = it.current().offset();

            RenderText *textRenderer = static_cast<RenderText *>(renderer);
            for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
                if (textOffset >= box->start() && textOffset <= box->end())
                    return it.current();
                else if (box != textRenderer->lastTextBox() && 
                         !box->nextOnLine() && 
                         textOffset == box->start() + box->len())
                    return it.current();
            }
        }
    }
    
    return it.current();
}

Position Position::equivalentRangeCompliantPosition() const
{
    if (isEmpty())
        return *this;

    if (!node()->parentNode())
        return *this;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return *this;
        
    if (!renderer->isReplaced() && !renderer->isBR())
        return *this;
    
    int o = 0;
    const NodeImpl *n = node();
    while ((n = n->previousSibling()))
        o++;
    
    return Position(node()->parentNode(), o + offset());
}

Position Position::equivalentShallowPosition() const
{
    if (isEmpty())
        return *this;

    Position pos(*this);
    while (pos.offset() == pos.node()->caretMinOffset() && pos.node()->parentNode() && pos.node() == pos.node()->parentNode()->firstChild())
        pos = Position(pos.node()->parentNode(), 0);
    return pos;
}

Position Position::equivalentDeepPosition() const
{
    if (isEmpty() || node()->isAtomicNode())
        return *this;

    NodeImpl *child = 0;
    Position pos(*this);
    if (offset() >= (int)node()->childNodeCount()) {
        child = node()->lastChild();
        pos = Position(child, child->caretMaxOffset());
        ASSERT(child);
        while (!child->isAtomicNode() && pos.node()->hasChildNodes()) {
            child = pos.node()->lastChild();
            ASSERT(child);
            pos = Position(child, child->caretMaxOffset());
        }
    }
    else {
        child = node()->childNode(offset());
        ASSERT(child);
        pos = Position(child, 0);
        while (!child->isAtomicNode() && pos.node()->hasChildNodes()) {
            child = pos.node()->firstChild();
            ASSERT(child);
            pos = Position(child, 0);
        }
    }
    return pos;
}

Position Position::closestRenderedPosition(EAffinity affinity) const
{
    if (isEmpty() || inRenderedContent())
        return *this;

    Position pos;
    PositionIterator it(*this);
    
    switch (affinity) {
        case UPSTREAM:
            // look upstream first
            it.setPosition(*this);
            while (!it.atStart()) {
                it.previous();
                if (it.current().inRenderedContent())
                    return it.current();
            }
            // if this does not find something rendered, look downstream
            it.setPosition(*this);
            while (!it.atEnd()) {
                it.next();
                if (it.current().inRenderedContent())
                    return it.current();
            }
            break;
        case DOWNSTREAM:
            // look downstream first
            it.setPosition(*this);
            while (!it.atEnd()) {
                it.next();
                if (it.current().inRenderedContent())
                    return it.current();
            }
            // if this does not find something rendered, look upstream
            it.setPosition(*this);
            while (!it.atStart()) {
                it.previous();
                if (it.current().inRenderedContent())
                    return it.current();
            }
            break;
    }
    
    return Position();
}

bool Position::inRenderedContent() const
{
    if (isEmpty())
        return false;
        
    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;
    
    if (renderer->style()->visibility() != VISIBLE)
        return false;

    // FIXME: This check returns false for a <br> at the end of a line!
    if (renderer->isBR() && static_cast<RenderText *>(renderer)->firstTextBox()) {
        return offset() == 0;
    }
    else if (renderer->isText()) {
        RenderText *textRenderer = static_cast<RenderText *>(renderer);
        for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
            if (offset() >= box->m_start && offset() <= box->m_start + box->m_len) {
                return true;
            }
            else if (offset() < box->m_start) {
                // The offset we're looking for is before this node
                // this means the offset must be in content that is
                // not rendered. Return false.
                return false;
            }
        }
    }
    else if (offset() >= renderer->caretMinOffset() && offset() <= renderer->caretMaxOffset()) {
        // return true for replaced elements, for inline flows if they have a line box
        // and for blocks if they are empty
        if (renderer->isReplaced() ||
            (renderer->isInlineFlow() && static_cast<RenderFlow *>(renderer)->firstLineBox()) ||
            (node()->isBlockFlow() && !node()->firstChild()))
            return true;
    }
    
    return false;
}

bool Position::inRenderedText() const
{
    if (isEmpty() || !node()->isTextNode())
        return false;
        
    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;
    
    RenderText *textRenderer = static_cast<RenderText *>(renderer);
    for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
        if (offset() < box->m_start) {
            // The offset we're looking for is before this node
            // this means the offset must be in content that is
            // not rendered. Return false.
            return false;
        }
        if (offset() >= box->m_start && offset() <= box->m_start + box->m_len)
            return true;
    }
    
    return false;
}

bool Position::isRenderedCharacter() const
{
    if (isEmpty() || !node()->isTextNode())
        return false;
        
    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;
    
    RenderText *textRenderer = static_cast<RenderText *>(renderer);
    for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
        if (offset() < box->m_start) {
            // The offset we're looking for is before this node
            // this means the offset must be in content that is
            // not rendered. Return false.
            return false;
        }
        if (offset() >= box->m_start && offset() < box->m_start + box->m_len)
            return true;
    }
    
    return false;
}

bool Position::rendersInDifferentPosition(const Position &pos) const
{
    if (isEmpty() || pos.isEmpty())
        return false;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;
    
    RenderObject *posRenderer = pos.node()->renderer();
    if (!posRenderer)
        return false;

    if (renderer->style()->visibility() != VISIBLE ||
        posRenderer->style()->visibility() != VISIBLE)
        return false;
    
    if (node() == pos.node()) {
        if (node()->id() == ID_BR)
            return false;

        if (offset() == pos.offset())
            return false;
            
        if (!node()->isTextNode() && !pos.node()->isTextNode()) {
            if (offset() != pos.offset())
                return true;
        }
    }
    
    if (node()->id() == ID_BR && pos.inRenderedContent())
        return true;
                
    if (pos.node()->id() == ID_BR && inRenderedContent())
        return true;
                
    if (node()->enclosingBlockFlowElement() != pos.node()->enclosingBlockFlowElement())
        return true;

    if (node()->isTextNode() && !inRenderedText())
        return false;

    if (pos.node()->isTextNode() && !pos.inRenderedText())
        return false;

    long thisRenderedOffset = renderedOffset();
    long posRenderedOffset = pos.renderedOffset();

    if (renderer == posRenderer && thisRenderedOffset == posRenderedOffset)
        return false;

    LOG(Editing, "onDifferentLine:        %s\n", renderersOnDifferentLine(renderer, offset(), posRenderer, pos.offset()) ? "YES" : "NO");
    LOG(Editing, "renderer:               %p [%p]\n", renderer, renderer ? renderer->inlineBox(offset()) : 0);
    LOG(Editing, "thisRenderedOffset:         %d\n", thisRenderedOffset);
    LOG(Editing, "posRenderer:            %p [%p]\n", posRenderer, posRenderer ? posRenderer->inlineBox(offset()) : 0);
    LOG(Editing, "posRenderedOffset:      %d\n", posRenderedOffset);
    LOG(Editing, "node min/max:           %d:%d\n", node()->caretMinOffset(), node()->caretMaxRenderedOffset());
    LOG(Editing, "pos node min/max:       %d:%d\n", pos.node()->caretMinOffset(), pos.node()->caretMaxRenderedOffset());
    LOG(Editing, "----------------------------------------------------------------------\n");

    InlineBox *b1 = renderer ? renderer->inlineBox(offset()) : 0;
    InlineBox *b2 = posRenderer ? posRenderer->inlineBox(pos.offset()) : 0;

    if (!b1 || !b2) {
        return false;
    }

    if (b1->root() != b2->root()) {
        return true;
    }

    if (nextRenderedEditable(node()) == pos.node() && 
        thisRenderedOffset == (long)node()->caretMaxRenderedOffset() && posRenderedOffset == 0) {
        return false;
    }
    
    if (previousRenderedEditable(node()) == pos.node() && 
        thisRenderedOffset == 0 && posRenderedOffset == (long)pos.node()->caretMaxRenderedOffset()) {
        return false;
    }

    return true;
}

bool Position::isFirstRenderedPositionOnLine() const
{
    if (isEmpty())
        return false;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;

    if (renderer->style()->visibility() != VISIBLE)
        return false;
    
    if (!inRenderedContent())
        return false;

    PositionIterator it(*this);
    while (!it.atStart()) {
        it.previous();
        RenderObject *currentRenderer = it.current().node()->renderer();
        if (!currentRenderer || currentRenderer->firstChild())
            // we want a leaf for this check
            continue;
        if (it.current().inRenderedContent())
            return renderersOnDifferentLine(renderer, offset(), currentRenderer, it.current().offset());
    }
    
    return true;
}

bool Position::isLastRenderedPositionOnLine() const
{
    if (isEmpty())
        return false;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;

    if (renderer->style()->visibility() != VISIBLE)
        return false;
    
    if (!inRenderedContent())
        return false;
    
    if (node()->id() == ID_BR)
        return true;
    
    PositionIterator it(*this);
    while (!it.atEnd()) {
        it.next();
        RenderObject *currentRenderer = it.current().node()->renderer();
        if (!currentRenderer || currentRenderer->firstChild())
            // we want a leaf for this check
            continue;
        if (it.current().inRenderedContent())
            return renderersOnDifferentLine(renderer, offset(), currentRenderer, it.current().offset());
    }
    
    return true;
}

bool Position::inFirstEditableInRootEditableElement() const
{
    if (isEmpty() || !inRenderedContent())
        return false;

    PositionIterator it(*this);
    while (!it.atStart()) {
        it.previous();
        RenderObject *currentRenderer = it.current().node()->renderer();
        if (!currentRenderer || currentRenderer->firstChild())
            // we want a leaf for this check
            continue;
        if (it.current().inRenderedContent())
            return false;
    }

    return true;
}

static inline bool isWS(const QChar &c)
{
    const char nonBreakingSpace = 0xA0;
    return c.isSpace() && c != nonBreakingSpace;
}

Position Position::leadingWhitespacePosition() const
{
    if (isEmpty())
        return Position();
    
    if (upstream(StayInBlock).node()->id() == ID_BR)
        return Position();
    
    Position prev = previousCharacterPosition();
    if (prev != *this && prev.node()->inSameContainingBlockFlowElement(node()) && prev.node()->isTextNode()) {
        DOMString string = static_cast<TextImpl *>(prev.node())->data();
        if (isWS(string[prev.offset()]))
            return prev;
    }

    return Position();
}

Position Position::trailingWhitespacePosition() const
{
    if (isEmpty())
        return Position();

    if (node()->isTextNode()) {
        TextImpl *textNode = static_cast<TextImpl *>(node());
        if (offset() < (long)textNode->length()) {
            DOMString string = static_cast<TextImpl *>(node())->data();
            if (isWS(string[offset()]))
                return *this;
            return Position();
        }
    }

    if (downstream(StayInBlock).node()->id() == ID_BR)
        return Position();

    Position next = nextCharacterPosition();
    if (next != *this && next.node()->inSameContainingBlockFlowElement(node()) && next.node()->isTextNode()) {
        DOMString string = static_cast<TextImpl *>(next.node())->data();
        if (isWS(string[0]))
            return next;
    }

    return Position();
}

void Position::debugPosition(const char *msg) const
{
    if (isEmpty())
        fprintf(stderr, "Position [%s]: empty\n", msg);
    else
        fprintf(stderr, "Position [%s]: %s [%p] at %d\n", msg, getTagName(node()->id()).string().latin1(), node(), offset());
}

#ifndef NDEBUG
#define FormatBufferSize 1024
void Position::formatForDebugger(char *buffer, unsigned length) const
{
    DOMString result;
    DOMString s;
    
    if (isEmpty()) {
        result = "<empty>";
    }
    else {
        char s[FormatBufferSize];
        result += "offset ";
        result += QString::number(m_offset);
        result += " of ";
        m_node->formatForDebugger(s, FormatBufferSize);
        result += s;
    }
          
    strncpy(buffer, result.string().latin1(), length - 1);
}
#undef FormatBufferSize
#endif

} // namespace DOM