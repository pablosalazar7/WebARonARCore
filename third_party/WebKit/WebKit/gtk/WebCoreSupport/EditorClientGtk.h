/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Apple Computer, Inc.
 *
 * All rights reserved.
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

#ifndef EditorClientGtk_h
#define EditorClientGtk_h

#include "EditorClient.h"

#include <wtf/Deque.h>
#include <wtf/Forward.h>

typedef struct _WebKitWebView WebKitWebView;

namespace WebCore {
    class Page;
}

namespace WebKit {

    class EditorClient : public WebCore::EditorClient {
    protected:
        bool m_isInRedo;

        WTF::Deque<WTF::RefPtr<WebCore::EditCommand> > undoStack;
        WTF::Deque<WTF::RefPtr<WebCore::EditCommand> > redoStack;

    public:
        EditorClient(WebKitWebView*);
        ~EditorClient();

        // from EditorClient
        virtual void pageDestroyed();

        virtual bool shouldDeleteRange(WebCore::Range*);
        virtual bool shouldShowDeleteInterface(WebCore::HTMLElement*);
        virtual bool smartInsertDeleteEnabled();
        virtual bool isSelectTrailingWhitespaceEnabled();
        virtual bool isContinuousSpellCheckingEnabled();
        virtual void toggleContinuousSpellChecking();
        virtual bool isGrammarCheckingEnabled();
        virtual void toggleGrammarChecking();
        virtual int spellCheckerDocumentTag();

        virtual bool isEditable();

        virtual bool shouldBeginEditing(WebCore::Range*);
        virtual bool shouldEndEditing(WebCore::Range*);
        virtual bool shouldInsertNode(WebCore::Node*, WebCore::Range*, WebCore::EditorInsertAction);
        virtual bool shouldInsertText(const WebCore::String&, WebCore::Range*, WebCore::EditorInsertAction);
        virtual bool shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity, bool stillSelecting);

        virtual bool shouldApplyStyle(WebCore::CSSStyleDeclaration*, WebCore::Range*);

        virtual bool shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*);

        virtual void didBeginEditing();
        virtual void respondToChangedContents();
        virtual void respondToChangedSelection();
        virtual void didEndEditing();
        virtual void didWriteSelectionToPasteboard();
        virtual void didSetSelectionTypesForPasteboard();

        virtual void registerCommandForUndo(WTF::PassRefPtr<WebCore::EditCommand>);
        virtual void registerCommandForRedo(WTF::PassRefPtr<WebCore::EditCommand>);
        virtual void clearUndoRedoOperations();

        virtual bool canUndo() const;
        virtual bool canRedo() const;

        virtual void undo();
        virtual void redo();

        virtual void handleKeyboardEvent(WebCore::KeyboardEvent*);
        virtual void handleInputMethodKeydown(WebCore::KeyboardEvent*);

        virtual void textFieldDidBeginEditing(WebCore::Element*);
        virtual void textFieldDidEndEditing(WebCore::Element*);
        virtual void textDidChangeInTextField(WebCore::Element*);
        virtual bool doTextFieldCommandFromEvent(WebCore::Element*, WebCore::KeyboardEvent*);
        virtual void textWillBeDeletedInTextField(WebCore::Element*);
        virtual void textDidChangeInTextArea(WebCore::Element*);

        virtual void ignoreWordInSpellDocument(const WebCore::String&);
        virtual void learnWord(const WebCore::String&);
        virtual void checkSpellingOfString(const UChar*, int length, int* misspellingLocation, int* misspellingLength);
        virtual WebCore::String getAutoCorrectSuggestionForMisspelledWord(const WebCore::String&);
        virtual void checkGrammarOfString(const UChar*, int length, WTF::Vector<WebCore::GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength);
        virtual void updateSpellingUIWithGrammarString(const WebCore::String&, const WebCore::GrammarDetail&);
        virtual void updateSpellingUIWithMisspelledWord(const WebCore::String&);
        virtual void showSpellingUI(bool show);
        virtual bool spellingUIIsShowing();
        virtual void getGuessesForWord(const WebCore::String&, WTF::Vector<WebCore::String>& guesses);
        virtual void setInputMethodState(bool enabled);

        WebKitWebView* m_webView;
    };
}

#endif

// vim: ts=4 sw=4 et
