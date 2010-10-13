/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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

#include "config.h"
#include "Element.h"
#include "FormatBlockCommand.h"
#include "Document.h"
#include "htmlediting.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "visible_units.h"

namespace WebCore {

using namespace HTMLNames;

FormatBlockCommand::FormatBlockCommand(Document* document, const QualifiedName& tagName) 
    : ApplyBlockElementCommand(document, tagName)
{
}

void FormatBlockCommand::formatRange(const Position&, const Position& end, RefPtr<Element>&)
{
    setEndingSelection(VisiblePosition(end));

    Node* refNode = enclosingBlockFlowElement(endingSelection().visibleStart());
    if (refNode->hasTagName(tagName()))
        // We're already in a block with the format we want, so we don't have to do anything
        return;

    VisiblePosition paragraphStart = startOfParagraph(end);
    VisiblePosition paragraphEnd = endOfParagraph(end);
    VisiblePosition blockStart = startOfBlock(endingSelection().visibleStart());
    VisiblePosition blockEnd = endOfBlock(endingSelection().visibleStart());
    RefPtr<Element> blockNode = createBlockElement();
    RefPtr<Element> placeholder = createBreakElement(document());

    Node* root = endingSelection().start().node()->rootEditableElement();
    if (validBlockTag(refNode->nodeName().lower()) && 
        paragraphStart == blockStart && paragraphEnd == blockEnd && 
        refNode != root && !root->isDescendantOf(refNode))
        // Already in a valid block tag that only contains the current paragraph, so we can swap with the new tag
        insertNodeBefore(blockNode, refNode);
    else {
        // Avoid inserting inside inline elements that surround paragraphStart with upstream().
        // This is only to avoid creating bloated markup.
        insertNodeAt(blockNode, paragraphStart.deepEquivalent().upstream());
    }
    appendNode(placeholder, blockNode);

    VisiblePosition destination(Position(placeholder.get(), 0));
    if (paragraphStart == paragraphEnd && !lineBreakExistsAtVisiblePosition(paragraphStart)) {
        setEndingSelection(destination);
        return;
    }
    moveParagraph(paragraphStart, paragraphEnd, destination, true, false);
}

}
