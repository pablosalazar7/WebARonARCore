/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebEditorClient_h
#define WebEditorClient_h

#include <WebCore/EditorClient.h>

namespace WebKit {

class WebPage;

class WebEditorClient : public WebCore::EditorClient {
public:
    WebEditorClient(WebPage* page)
        : m_page(page)
    {
    }

private:
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
    virtual bool shouldInsertText(const WTF::String&, WebCore::Range*, WebCore::EditorInsertAction);
    virtual bool shouldChangeSelectedRange(WebCore::Range* fromRange, WebCore::Range* toRange, WebCore::EAffinity, bool stillSelecting);
    
    virtual bool shouldApplyStyle(WebCore::CSSStyleDeclaration*, WebCore::Range*);
    virtual bool shouldMoveRangeAfterDelete(WebCore::Range*, WebCore::Range*);

    virtual void didBeginEditing();
    virtual void respondToChangedContents();
    virtual void respondToChangedSelection();
    virtual void didEndEditing();
    virtual void didWriteSelectionToPasteboard();
    virtual void didSetSelectionTypesForPasteboard();
    
    virtual void registerCommandForUndo(PassRefPtr<WebCore::EditCommand>);
    virtual void registerCommandForRedo(PassRefPtr<WebCore::EditCommand>);
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

#if PLATFORM(MAC)
    virtual NSString* userVisibleString(NSURL*);
    virtual WebCore::DocumentFragment* documentFragmentFromAttributedString(NSAttributedString*, Vector<WebCore::ArchiveResource*>&);
    virtual void setInsertionPasteboard(NSPasteboard*);
#ifdef BUILDING_ON_TIGER
    virtual NSArray* pasteboardTypesForSelection(WebCore::Frame*);
#endif
#endif

#if PLATFORM(MAC) && !defined(BUILDING_ON_TIGER) && !defined(BUILDING_ON_LEOPARD)
    virtual void uppercaseWord();
    virtual void lowercaseWord();
    virtual void capitalizeWord();
    virtual void showSubstitutionsPanel(bool show);
    virtual bool substitutionsPanelIsShowing();
    virtual void toggleSmartInsertDelete();
    virtual bool isAutomaticQuoteSubstitutionEnabled();
    virtual void toggleAutomaticQuoteSubstitution();
    virtual bool isAutomaticLinkDetectionEnabled();
    virtual void toggleAutomaticLinkDetection();
    virtual bool isAutomaticDashSubstitutionEnabled();
    virtual void toggleAutomaticDashSubstitution();
    virtual bool isAutomaticTextReplacementEnabled();
    virtual void toggleAutomaticTextReplacement();
    virtual bool isAutomaticSpellingCorrectionEnabled();
    virtual void toggleAutomaticSpellingCorrection();
#endif

    virtual void ignoreWordInSpellDocument(const WTF::String&);
    virtual void learnWord(const WTF::String&);
    virtual void checkSpellingOfString(const UChar*, int length, int* misspellingLocation, int* misspellingLength);
    virtual WTF::String getAutoCorrectSuggestionForMisspelledWord(const WTF::String& misspelledWord);
    virtual void checkGrammarOfString(const UChar*, int length, Vector<WebCore::GrammarDetail>&, int* badGrammarLocation, int* badGrammarLength);
#if PLATFORM(MAC) && !defined(BUILDING_ON_TIGER) && !defined(BUILDING_ON_LEOPARD)
    virtual void checkTextOfParagraph(const UChar* text, int length, uint64_t checkingTypes, Vector<WebCore::TextCheckingResult>& results);
#endif
    virtual void updateSpellingUIWithGrammarString(const WTF::String&, const WebCore::GrammarDetail& detail);
    virtual void updateSpellingUIWithMisspelledWord(const WTF::String&);
    virtual void showSpellingUI(bool show);
    virtual bool spellingUIIsShowing();
    virtual void getGuessesForWord(const WTF::String&, Vector<WTF::String>& guesses);
    virtual void willSetInputMethodState();
    virtual void setInputMethodState(bool enabled);
#if !defined(BUILDING_ON_TIGER) && !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    virtual void showCorrectionPanel(const WebCore::FloatRect& boundingBoxOfReplacedString, const WTF::String& replacedString, const WTF::String& replacementString, WebCore::Editor*);
    virtual void dismissCorrectionPanel(bool correctionAccepted);
#endif
    WebPage* m_page;
};

} // namespace WebKit

#endif // WebEditorClient_h
