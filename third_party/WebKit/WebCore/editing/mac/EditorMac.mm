/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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

#import "config.h"
#import "Editor.h"

#import "ClipboardAccessPolicy.h"
#import "Clipboard.h"
#import "ClipboardMac.h"
#import "Document.h"
#import "EditorClient.h"
#import "Element.h"
#import "ExceptionHandlers.h"
#import "Frame.h"
#import "PlatformString.h"
#import "Selection.h"
#import "SelectionController.h"
#import "TypingCommand.h"
#import "TextIterator.h"
#import "htmlediting.h"
#import "visible_units.h"

namespace WebCore {

extern "C" {

// Kill ring calls. Would be better to use NSKillRing.h, but that's not available in SPI.

void _NSAppendToKillRing(NSString *);
void _NSPrependToKillRing(NSString *);
void _NSNewKillRingSequence(void);
}

PassRefPtr<Clipboard> Editor::newGeneralClipboard(ClipboardAccessPolicy policy)
{
    return new ClipboardMac(false, [NSPasteboard generalPasteboard], policy);
}

NSString* Editor::userVisibleString(NSURL* nsURL)
{
    if (client())
        return client()->userVisibleString(nsURL);
    return nil;
}

void Editor::addToKillRing(Range* range, bool prepend)
{
    if (m_startNewKillRingSequence)
        _NSNewKillRingSequence();

    String text = plainText(range);
    text.replace('\\', m_frame->backslashAsCurrencySymbol());
    if (prepend)
        _NSPrependToKillRing((NSString*)text);
    else
        _NSAppendToKillRing((NSString*)text);
    m_startNewKillRingSequence = false;
}

void Editor::showFontPanel()
{
    [[NSFontManager sharedFontManager] orderFrontFontPanel:nil];
}

void Editor::showStylesPanel()
{
    [[NSFontManager sharedFontManager] orderFrontStylesPanel:nil];
}

void Editor::showColorPanel()
{
    [[NSApplication sharedApplication] orderFrontColorPanel:nil];
}

} // namespace WebCore
