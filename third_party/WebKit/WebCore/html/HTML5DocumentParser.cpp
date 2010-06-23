/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTML5DocumentParser.h"

#include "DocumentFragment.h"
#include "Element.h"
#include "Frame.h"
#include "HTMLParserScheduler.h"
#include "HTMLTokenizer.h"
#include "HTML5PreloadScanner.h"
#include "HTMLScriptRunner.h"
#include "HTML5TreeBuilder.h"
#include "HTMLDocument.h"
#include "XSSAuditor.h"
#include <wtf/CurrentTime.h>

#if ENABLE(INSPECTOR)
#include "InspectorTimelineAgent.h"
#endif

namespace WebCore {

namespace {

class NestingLevelIncrementer : public Noncopyable {
public:
    explicit NestingLevelIncrementer(int& counter)
        : m_counter(&counter)
    {
        ++(*m_counter);
    }

    ~NestingLevelIncrementer()
    {
        --(*m_counter);
    }

private:
    int* m_counter;
};

} // namespace

HTML5DocumentParser::HTML5DocumentParser(HTMLDocument* document, bool reportErrors)
    : m_document(document)
    , m_tokenizer(new HTMLTokenizer)
    , m_scriptRunner(new HTMLScriptRunner(document, this))
    , m_treeConstructor(new HTML5TreeBuilder(m_tokenizer.get(), document, reportErrors))
    , m_parserScheduler(new HTMLParserScheduler(this))
    , m_endWasDelayed(false)
    , m_writeNestingLevel(0)
{
    begin();
}

// FIXME: Member variables should be grouped into self-initializing structs to
// minimize code duplication between these constructors.
HTML5DocumentParser::HTML5DocumentParser(DocumentFragment* fragment, FragmentScriptingPermission scriptingPermission)
    : m_document(fragment->document())
    , m_tokenizer(new HTMLTokenizer)
    , m_treeConstructor(new HTML5TreeBuilder(m_tokenizer.get(), fragment, scriptingPermission))
    , m_endWasDelayed(false)
    , m_writeNestingLevel(0)
{
    begin();
}

HTML5DocumentParser::~HTML5DocumentParser()
{
    // FIXME: We'd like to ASSERT that normal operation of this class clears
    // out any delayed actions, but we can't because we're unceremoniously
    // deleted.  If there were a required call to some sort of cancel function,
    // then we could ASSERT some invariants here.
}

void HTML5DocumentParser::begin()
{
    // FIXME: Should we reset the tokenizer?
}

void HTML5DocumentParser::stopParsing()
{
    DocumentParser::stopParsing();
    m_parserScheduler.clear(); // Deleting the scheduler will clear any timers.
}

bool HTML5DocumentParser::processingData() const
{
    return isScheduledForResume() || inWrite();
}

void HTML5DocumentParser::pumpTokenizerIfPossible(SynchronousMode mode)
{
    if (m_parserStopped || m_treeConstructor->isPaused())
        return;

    // Once a resume is scheduled, HTMLParserScheduler controls when we next pump.
    if (isScheduledForResume()) {
        ASSERT(mode == AllowYield);
        return;
    }

    pumpTokenizer(mode);
}

bool HTML5DocumentParser::isScheduledForResume() const
{
    return m_parserScheduler && m_parserScheduler->isScheduledForResume();
}

// Used by HTMLParserScheduler
void HTML5DocumentParser::resumeParsingAfterYield()
{
    // We should never be here unless we can pump immediately.  Call pumpTokenizer()
    // directly so that ASSERTS will fire if we're wrong.
    pumpTokenizer(AllowYield);
}

bool HTML5DocumentParser::runScriptsForPausedTreeConstructor()
{
    ASSERT(m_treeConstructor->isPaused());

    int scriptStartLine = 0;
    RefPtr<Element> scriptElement = m_treeConstructor->takeScriptToProcess(scriptStartLine);
    // We will not have a scriptRunner when parsing a DocumentFragment.
    if (!m_scriptRunner)
        return true;
    return m_scriptRunner->execute(scriptElement.release(), scriptStartLine);
}

void HTML5DocumentParser::pumpTokenizer(SynchronousMode mode)
{
    ASSERT(!m_parserStopped);
    ASSERT(!m_treeConstructor->isPaused());
    ASSERT(!isScheduledForResume());

    // We tell the InspectorTimelineAgent about every pump, even if we
    // end up pumping nothing.  It can filter out empty pumps itself.
    willPumpLexer();

    HTMLParserScheduler::PumpSession session;
    // FIXME: This loop body has is now too long and needs cleanup.
    while (mode == ForceSynchronous || (!m_parserStopped && m_parserScheduler->shouldContinueParsing(session))) {
        if (!m_tokenizer->nextToken(m_input.current(), m_token))
            break;

        m_treeConstructor->constructTreeFromToken(m_token);
        m_token.clear();

        // The parser will pause itself when waiting on a script to load or run.
        if (!m_treeConstructor->isPaused())
            continue;

        // If we're paused waiting for a script, we try to execute scripts before continuing.
        bool shouldContinueParsing = runScriptsForPausedTreeConstructor();
        m_treeConstructor->setPaused(!shouldContinueParsing);
        if (!shouldContinueParsing)
            break;
    }

    if (isWaitingForScripts()) {
        ASSERT(m_tokenizer->state() == HTMLTokenizer::DataState);
        if (!m_preloadScanner) {
            m_preloadScanner.set(new HTML5PreloadScanner(m_document));
            m_preloadScanner->appendToEnd(m_input.current());
        }
        m_preloadScanner->scan();
    }

    didPumpLexer();
}

void HTML5DocumentParser::willPumpLexer()
{
#if ENABLE(INSPECTOR)
    // FIXME: m_input.current().length() is only accurate if we
    // end up parsing the whole buffer in this pump.  We should pass how
    // much we parsed as part of didWriteHTML instead of willWriteHTML.
    if (InspectorTimelineAgent* timelineAgent = m_document->inspectorTimelineAgent())
        timelineAgent->willWriteHTML(m_input.current().length(), m_tokenizer->lineNumber());
#endif
}

void HTML5DocumentParser::didPumpLexer()
{
#if ENABLE(INSPECTOR)
    if (InspectorTimelineAgent* timelineAgent = m_document->inspectorTimelineAgent())
        timelineAgent->didWriteHTML(m_tokenizer->lineNumber());
#endif
}

void HTML5DocumentParser::write(const SegmentedString& source, bool isFromNetwork)
{
    if (m_parserStopped)
        return;

    NestingLevelIncrementer nestingLevelIncrementer(m_writeNestingLevel);

    if (isFromNetwork) {
        m_input.appendToEnd(source);
        if (m_preloadScanner)
            m_preloadScanner->appendToEnd(source);

        if (m_writeNestingLevel > 1) {
            // We've gotten data off the network in a nested call to write().
            // We don't want to consume any more of the input stream now.  Do
            // not worry.  We'll consume this data in a less-nested write().
            return;
        }
    } else
        m_input.insertAtCurrentInsertionPoint(source);

    pumpTokenizerIfPossible(isFromNetwork ? AllowYield : ForceSynchronous);
    endIfDelayed();
}

void HTML5DocumentParser::end()
{
    ASSERT(!isScheduledForResume());
    // NOTE: This pump should only ever emit buffered character tokens,
    // so ForceSynchronous vs. AllowYield should be meaningless.
    pumpTokenizerIfPossible(ForceSynchronous);

    // Informs the the rest of WebCore that parsing is really finished (and deletes this).
    m_treeConstructor->finished();
}

void HTML5DocumentParser::attemptToEnd()
{
    // finish() indicates we will not receive any more data. If we are waiting on
    // an external script to load, we can't finish parsing quite yet.

    if (inWrite() || isWaitingForScripts() || executingScript() || isScheduledForResume()) {
        m_endWasDelayed = true;
        return;
    }
    end();
}

void HTML5DocumentParser::endIfDelayed()
{
    // We don't check inWrite() here since inWrite() will be true if this was
    // called from write().
    if (!m_endWasDelayed || isWaitingForScripts() || executingScript() || isScheduledForResume())
        return;

    m_endWasDelayed = false;
    end();
}

void HTML5DocumentParser::finish()
{
    // We're not going to get any more data off the network, so we close the
    // input stream to indicate EOF.
    m_input.close();
    attemptToEnd();
}

bool HTML5DocumentParser::finishWasCalled()
{
    return m_input.isClosed();
}

// This function is virtual and just for the DocumentParser interface.
int HTML5DocumentParser::executingScript() const
{
    return inScriptExecution();
}

// This function is non-virtual and used throughout the implementation.
bool HTML5DocumentParser::inScriptExecution() const
{
    if (!m_scriptRunner)
        return false;
    return m_scriptRunner->inScriptExecution();
}

int HTML5DocumentParser::lineNumber() const
{
    return m_tokenizer->lineNumber();
}

int HTML5DocumentParser::columnNumber() const
{
    return m_tokenizer->columnNumber();
}

LegacyHTMLTreeConstructor* HTML5DocumentParser::htmlTreeConstructor() const
{
    return m_treeConstructor->legacyTreeConstructor();
}

bool HTML5DocumentParser::isWaitingForScripts() const
{
    return m_treeConstructor->isPaused();
}

void HTML5DocumentParser::resumeParsingAfterScriptExecution()
{
    ASSERT(!inScriptExecution());
    ASSERT(!m_treeConstructor->isPaused());

    pumpTokenizerIfPossible(AllowYield);

    // The document already finished parsing we were just waiting on scripts when finished() was called.
    endIfDelayed();
}

void HTML5DocumentParser::watchForLoad(CachedResource* cachedScript)
{
    cachedScript->addClient(this);
}

void HTML5DocumentParser::stopWatchingForLoad(CachedResource* cachedScript)
{
    cachedScript->removeClient(this);
}

bool HTML5DocumentParser::shouldLoadExternalScriptFromSrc(const AtomicString& srcValue)
{
    if (!m_XSSAuditor)
        return true;
    return m_XSSAuditor->canLoadExternalScriptFromSrc(srcValue);
}

void HTML5DocumentParser::notifyFinished(CachedResource* cachedResource)
{
    ASSERT(m_scriptRunner);
    // Ignore calls unless we have a script blocking the parser waiting
    // for its own load.  Otherwise this may be a load callback from
    // CachedResource::addClient because the script was already in the cache.
    // HTMLScriptRunner may not be ready to handle running that script yet.
    if (!m_scriptRunner->hasScriptsWaitingForLoad()) {
        ASSERT(m_scriptRunner->inScriptExecution());
        return;
    }
    ASSERT(!inScriptExecution());
    ASSERT(m_treeConstructor->isPaused());
    // Note: We only ever wait on one script at a time, so we always know this
    // is the one we were waiting on and can un-pause the tree builder.
    m_treeConstructor->setPaused(false);
    bool shouldContinueParsing = m_scriptRunner->executeScriptsWaitingForLoad(cachedResource);
    m_treeConstructor->setPaused(!shouldContinueParsing);
    if (shouldContinueParsing)
        resumeParsingAfterScriptExecution();
}

void HTML5DocumentParser::executeScriptsWaitingForStylesheets()
{
    // Document only calls this when the Document owns the DocumentParser
    // so this will not be called in the DocumentFragment case.
    ASSERT(m_scriptRunner);
    // Ignore calls unless we have a script blocking the parser waiting on a
    // stylesheet load.  Otherwise we are currently parsing and this
    // is a re-entrant call from encountering a </ style> tag.
    if (!m_scriptRunner->hasScriptsWaitingForStylesheets())
        return;
    ASSERT(!m_scriptRunner->inScriptExecution());
    ASSERT(m_treeConstructor->isPaused());
    // Note: We only ever wait on one script at a time, so we always know this
    // is the one we were waiting on and can un-pause the tree builder.
    m_treeConstructor->setPaused(false);
    bool shouldContinueParsing = m_scriptRunner->executeScriptsWaitingForStylesheets();
    m_treeConstructor->setPaused(!shouldContinueParsing);
    if (shouldContinueParsing)
        resumeParsingAfterScriptExecution();
}

ScriptController* HTML5DocumentParser::script() const
{
    return m_document->frame() ? m_document->frame()->script() : 0;
}

}
