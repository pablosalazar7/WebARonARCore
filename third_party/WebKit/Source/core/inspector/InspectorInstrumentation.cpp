/*
* Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "core/inspector/InspectorInstrumentation.h"

#include "bindings/v8/DOMWrapperWorld.h"
#include "bindings/v8/ScriptController.h"
#include "core/css/CSSRule.h"
#include "core/css/CSSStyleRule.h"
#include "core/css/StyleRule.h"
#include "core/css/resolver/StyleResolver.h"
#include "core/dom/DeviceOrientationData.h"
#include "core/dom/EventContext.h"
#include "core/inspector/ConsoleAPITypes.h"
#include "core/inspector/InspectorAgent.h"
#include "core/inspector/InspectorApplicationCacheAgent.h"
#include "core/inspector/InspectorCSSAgent.h"
#include "core/inspector/InspectorCanvasAgent.h"
#include "core/inspector/InspectorConsoleAgent.h"
#include "core/inspector/InspectorController.h"
#include "core/inspector/InspectorDOMAgent.h"
#include "core/inspector/InspectorDOMDebuggerAgent.h"
#include "core/inspector/InspectorDOMStorageAgent.h"
#include "core/inspector/InspectorDatabaseAgent.h"
#include "core/inspector/InspectorDebuggerAgent.h"
#include "core/inspector/InspectorHeapProfilerAgent.h"
#include "core/inspector/InspectorLayerTreeAgent.h"
#include "core/inspector/InspectorPageAgent.h"
#include "core/inspector/InspectorProfilerAgent.h"
#include "core/inspector/InspectorResourceAgent.h"
#include "core/inspector/InspectorTimelineAgent.h"
#include "core/inspector/InspectorWorkerAgent.h"
#include "core/inspector/InstrumentingAgents.h"
#include "core/inspector/PageDebuggerAgent.h"
#include "core/inspector/PageRuntimeAgent.h"
#include "core/inspector/ScriptArguments.h"
#include "core/inspector/ScriptCallStack.h"
#include "core/inspector/ScriptProfile.h"
#include "core/inspector/WorkerInspectorController.h"
#include "core/inspector/WorkerRuntimeAgent.h"
#include "core/loader/DocumentLoader.h"
#include "core/page/ConsoleTypes.h"
#include "core/page/DOMWindow.h"
#include "core/rendering/RenderObject.h"
#include "core/workers/WorkerContext.h"
#include "core/workers/WorkerThread.h"
#include "core/xml/XMLHttpRequest.h"
#include "modules/webdatabase/Database.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

namespace WebCore {

namespace {
static HashSet<InstrumentingAgents*>* instrumentingAgentsSet = 0;
}

namespace InspectorInstrumentation {
int FrontendCounter::s_frontendCounter = 0;
}

InspectorInstrumentationCookie::InspectorInstrumentationCookie()
    : m_instrumentingAgents(0)
    , m_timelineAgentId(0)
{
}

InspectorInstrumentationCookie::InspectorInstrumentationCookie(InstrumentingAgents* agents, int timelineAgentId)
    : m_instrumentingAgents(agents)
    , m_timelineAgentId(timelineAgentId)
{
}

InspectorInstrumentationCookie::InspectorInstrumentationCookie(const InspectorInstrumentationCookie& other)
    : m_instrumentingAgents(other.m_instrumentingAgents)
    , m_timelineAgentId(other.m_timelineAgentId)
{
}

InspectorInstrumentationCookie& InspectorInstrumentationCookie::operator=(const InspectorInstrumentationCookie& other)
{
    if (this != &other) {
        m_instrumentingAgents = other.m_instrumentingAgents;
        m_timelineAgentId = other.m_timelineAgentId;
    }
    return *this;
}

InspectorInstrumentationCookie::~InspectorInstrumentationCookie()
{
}

namespace InspectorInstrumentation {

InspectorTimelineAgent* retrieveTimelineAgent(const InspectorInstrumentationCookie&);

void didClearWindowObjectInWorldImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, DOMWrapperWorld* world)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didClearWindowObjectInWorld(frame, world);
    if (InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent())
        inspectorAgent->didClearWindowObjectInWorld(frame, world);
    if (PageDebuggerAgent* debuggerAgent = instrumentingAgents->pageDebuggerAgent())
        debuggerAgent->didClearWindowObjectInWorld(frame, world);
    if (PageRuntimeAgent* pageRuntimeAgent = instrumentingAgents->pageRuntimeAgent())
        pageRuntimeAgent->didClearWindowObjectInWorld(frame, world);
}

bool isDebuggerPausedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        return debuggerAgent->isPaused();
    return false;
}

void willInsertDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* parent)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willInsertDOMNode(parent);
}

void didInsertDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didInsertDOMNode(node);
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didInsertDOMNode(node);
}

void willRemoveDOMNodeImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willRemoveDOMNode(node);
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->willRemoveDOMNode(node);
}

void willModifyDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& oldValue, const AtomicString& newValue)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willModifyDOMAttr(element, oldValue, newValue);
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->willModifyDOMAttr(element, oldValue, newValue);
}

void didModifyDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& name, const AtomicString& value)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didModifyDOMAttr(element, name, value);
}

void didRemoveDOMAttrImpl(InstrumentingAgents* instrumentingAgents, Element* element, const AtomicString& name)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didRemoveDOMAttr(element, name);
}

void characterDataModifiedImpl(InstrumentingAgents* instrumentingAgents, CharacterData* characterData)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->characterDataModified(characterData);
}

void didInvalidateStyleAttrImpl(InstrumentingAgents* instrumentingAgents, Node* node)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didInvalidateStyleAttr(node);
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didInvalidateStyleAttr(node);
}

void activeStyleSheetsUpdatedImpl(InstrumentingAgents* instrumentingAgents, Document* document, const Vector<RefPtr<StyleSheet> >& newSheets)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->activeStyleSheetsUpdated(document, newSheets);
}

void frameWindowDiscardedImpl(InstrumentingAgents* instrumentingAgents, DOMWindow* domWindow)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->frameWindowDiscarded(domWindow);
}

void mediaQueryResultChangedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->mediaQueryResultChanged();
}

void didPushShadowRootImpl(InstrumentingAgents* instrumentingAgents, Element* host, ShadowRoot* root)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didPushShadowRoot(host, root);
}

void willPopShadowRootImpl(InstrumentingAgents* instrumentingAgents, Element* host, ShadowRoot* root)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->willPopShadowRoot(host, root);
}

void didCreateNamedFlowImpl(InstrumentingAgents* instrumentingAgents, Document* document, NamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didCreateNamedFlow(document, namedFlow);
}

void willRemoveNamedFlowImpl(InstrumentingAgents* instrumentingAgents, Document* document, NamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->willRemoveNamedFlow(document, namedFlow);
}

void didUpdateRegionLayoutImpl(InstrumentingAgents* instrumentingAgents, Document* document, NamedFlow* namedFlow)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didUpdateRegionLayout(document, namedFlow);
}

bool forcePseudoStateImpl(InstrumentingAgents* instrumentingAgents, Element* element, CSSSelector::PseudoType pseudoState)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        return cssAgent->forcePseudoState(element, pseudoState);
    return false;
}

void willSendXMLHttpRequestImpl(InstrumentingAgents* instrumentingAgents, const String& url)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willSendXMLHttpRequest(url);
}

void didScheduleResourceRequestImpl(InstrumentingAgents* instrumentingAgents, Document* document, const String& url)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScheduleResourceRequest(document, url);
}

void didInstallTimerImpl(InstrumentingAgents* instrumentingAgents, ScriptExecutionContext* context, int timerId, int timeout, bool singleShot)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didInstallTimer(context, timerId, timeout, singleShot);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didInstallTimer(context, timerId, timeout, singleShot);
}

void didRemoveTimerImpl(InstrumentingAgents* instrumentingAgents, ScriptExecutionContext* context, int timerId)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didRemoveTimer(context, timerId);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didRemoveTimer(context, timerId);
}

InspectorInstrumentationCookie willCallFunctionImpl(InstrumentingAgents* instrumentingAgents, ScriptExecutionContext* context, const String& scriptName, int scriptLine)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willCallFunction(context, scriptName, scriptLine))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didCallFunctionImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didCallFunction();
}

InspectorInstrumentationCookie willDispatchXHRReadyStateChangeEventImpl(InstrumentingAgents* instrumentingAgents, ScriptExecutionContext* context, XMLHttpRequest* request)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willDispatchXHRReadyStateChangeEvent(context, request))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didDispatchXHRReadyStateChangeEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchXHRReadyStateChangeEvent();
}

InspectorInstrumentationCookie willDispatchEventImpl(InstrumentingAgents* instrumentingAgents, Document* document, const Event& event, DOMWindow* window, Node* node, const EventPath& eventPath)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willDispatchEvent(document, event, window, node, eventPath))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didDispatchEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchEvent();
}

InspectorInstrumentationCookie willHandleEventImpl(InstrumentingAgents* instrumentingAgents, Event* event)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willHandleEvent(event);
    return InspectorInstrumentationCookie(instrumentingAgents, 0);
}

void didHandleEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorDebuggerAgent* debuggerAgent = cookie.instrumentingAgents()->inspectorDebuggerAgent())
        debuggerAgent->didHandleEvent();
}

InspectorInstrumentationCookie willDispatchEventOnWindowImpl(InstrumentingAgents* instrumentingAgents, const Event& event, DOMWindow* window)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willDispatchEventOnWindow(event, window))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didDispatchEventOnWindowImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchEventOnWindow();
}

InspectorInstrumentationCookie willEvaluateScriptImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, const String& url, int lineNumber)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willEvaluateScript(frame, url, lineNumber))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didEvaluateScriptImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didEvaluateScript();
}

void scriptsEnabledImpl(InstrumentingAgents* instrumentingAgents, bool isEnabled)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->scriptsEnabled(isEnabled);
}

void didCreateIsolatedContextImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, ScriptState* scriptState, SecurityOrigin* origin)
{
    if (PageRuntimeAgent* runtimeAgent = instrumentingAgents->pageRuntimeAgent())
        runtimeAgent->didCreateIsolatedContext(frame, scriptState, origin);
}

InspectorInstrumentationCookie willFireTimerImpl(InstrumentingAgents* instrumentingAgents, ScriptExecutionContext* context, int timerId)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willFireTimer(context, timerId);
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willFireTimer(context, timerId))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didFireTimerImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorDebuggerAgent* debuggerAgent = cookie.instrumentingAgents()->inspectorDebuggerAgent())
        debuggerAgent->didFireTimer();
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didFireTimer();
}

void didInvalidateLayoutImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didInvalidateLayout(frame);
}

InspectorInstrumentationCookie willLayoutImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willLayout(frame))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didLayoutImpl(const InspectorInstrumentationCookie& cookie, RenderObject* root)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didLayout(root);
    if (InspectorPageAgent* pageAgent = cookie.instrumentingAgents()->inspectorPageAgent())
        pageAgent->didLayout(root);
}

void didScrollImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didScroll();
}

void didResizeMainFrameImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didResizeMainFrame();
}

InspectorInstrumentationCookie willDispatchXHRLoadEventImpl(InstrumentingAgents* instrumentingAgents, ScriptExecutionContext* context, XMLHttpRequest* request)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willDispatchXHRLoadEvent(context, request))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didDispatchXHRLoadEventImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didDispatchXHRLoadEvent();
}

void willScrollLayerImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willScrollLayer(frame);
}

void didScrollLayerImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScrollLayer();
}

void willPaintImpl(InstrumentingAgents* instrumentingAgents, RenderObject* renderer)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willPaint(renderer);
}

void didPaintImpl(InstrumentingAgents* instrumentingAgents, RenderObject* renderer, GraphicsContext* context, const LayoutRect& rect)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didPaint(renderer, context, rect);
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didPaint(renderer, context, rect);
}

InspectorInstrumentationCookie willRecalculateStyleImpl(InstrumentingAgents* instrumentingAgents, Document* document)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willRecalculateStyle(document);
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willRecalculateStyle(document))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didRecalculateStyleImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didRecalculateStyle();
    if (InspectorResourceAgent* resourceAgent = cookie.instrumentingAgents()->inspectorResourceAgent())
        resourceAgent->didRecalculateStyle();
    if (InspectorPageAgent* pageAgent = cookie.instrumentingAgents()->inspectorPageAgent())
        pageAgent->didRecalculateStyle();
}

void didRecalculateStyleForElementImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didRecalculateStyleForElement();
}

void didScheduleStyleRecalculationImpl(InstrumentingAgents* instrumentingAgents, Document* document)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didScheduleStyleRecalculation(document);
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didScheduleStyleRecalculation(document);
}

InspectorInstrumentationCookie willMatchRuleImpl(InstrumentingAgents* instrumentingAgents, StyleRule* rule, InspectorCSSOMWrappers& inspectorCSSOMWrappers, DocumentStyleSheetCollection* sheetCollection)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->willMatchRule(rule, inspectorCSSOMWrappers, sheetCollection);
    return InspectorInstrumentationCookie(instrumentingAgents, 0);
}

void didMatchRuleImpl(const InspectorInstrumentationCookie& cookie, bool matched)
{
    if (InspectorCSSAgent* cssAgent = cookie.instrumentingAgents()->inspectorCSSAgent())
        cssAgent->didMatchRule(matched);
}

InspectorInstrumentationCookie willProcessRuleImpl(InstrumentingAgents* instrumentingAgents, StyleRule* rule, StyleResolver* styleResolver)
{
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->willProcessRule(rule, styleResolver);
    return InspectorInstrumentationCookie(instrumentingAgents, 0);
}

void didProcessRuleImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorCSSAgent* cssAgent = cookie.instrumentingAgents()->inspectorCSSAgent())
        cssAgent->didProcessRule();
}

void applyUserAgentOverrideImpl(InstrumentingAgents* instrumentingAgents, String* userAgent)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->applyUserAgentOverride(userAgent);
}

void applyScreenWidthOverrideImpl(InstrumentingAgents* instrumentingAgents, long* width)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->applyScreenWidthOverride(width);
}

void applyScreenHeightOverrideImpl(InstrumentingAgents* instrumentingAgents, long* height)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->applyScreenHeightOverride(height);
}

bool shouldApplyScreenWidthOverrideImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent()) {
        long width = 0;
        pageAgent->applyScreenWidthOverride(&width);
        return !!width;
    }
    return false;
}

bool shouldApplyScreenHeightOverrideImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent()) {
        long height = 0;
        pageAgent->applyScreenHeightOverride(&height);
        return !!height;
    }
    return false;
}

void applyEmulatedMediaImpl(InstrumentingAgents* instrumentingAgents, String* media)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->applyEmulatedMedia(media);
}

void willSendRequestImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willSendRequest(identifier, loader, request, redirectResponse);
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willSendRequest(identifier, loader, request, redirectResponse);
}

void continueAfterPingLoaderImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& response)
{
    willSendRequestImpl(instrumentingAgents, identifier, loader, request, response);
}

void markResourceAsCachedImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->markResourceAsCached(identifier);
}

void didLoadResourceFromMemoryCacheImpl(InstrumentingAgents* instrumentingAgents, DocumentLoader* loader, CachedResource* resource)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didLoadResourceFromMemoryCache(loader, resource);
}

InspectorInstrumentationCookie willReceiveResourceDataImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, unsigned long identifier, int length)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willReceiveResourceData(frame, identifier, length))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didReceiveResourceDataImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didReceiveResourceData();
}

InspectorInstrumentationCookie willReceiveResourceResponseImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, unsigned long identifier, const ResourceResponse& response)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willReceiveResourceResponse(frame, identifier, response))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didReceiveResourceResponseImpl(const InspectorInstrumentationCookie& cookie, unsigned long identifier, DocumentLoader* loader, const ResourceResponse& response, ResourceLoader* resourceLoader)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didReceiveResourceResponse(identifier, loader, response, resourceLoader);
    if (InspectorResourceAgent* resourceAgent = cookie.instrumentingAgents()->inspectorResourceAgent())
        resourceAgent->didReceiveResourceResponse(identifier, loader, response, resourceLoader);
    if (InspectorConsoleAgent* consoleAgent = cookie.instrumentingAgents()->inspectorConsoleAgent())
        consoleAgent->didReceiveResourceResponse(identifier, loader, response, resourceLoader); // This should come AFTER resource notification, front-end relies on this.
}

void didReceiveResourceResponseButCanceledImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    InspectorInstrumentationCookie cookie = willReceiveResourceResponse(frame, identifier, r);
    didReceiveResourceResponse(cookie, identifier, loader, r, 0);
}

void continueAfterXFrameOptionsDeniedImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void continueWithPolicyDownloadImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void continueWithPolicyIgnoreImpl(Frame* frame, DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    didReceiveResourceResponseButCanceledImpl(frame, loader, identifier, r);
}

void didReceiveDataImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const char* data, int dataLength, int encodedDataLength)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveData(identifier, data, dataLength, encodedDataLength);
}

void didFinishLoadingImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, double monotonicFinishTime)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didFinishLoading(identifier, loader, monotonicFinishTime);
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFinishLoading(identifier, loader, monotonicFinishTime);
}

void didFailLoadingImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, DocumentLoader* loader, const ResourceError& error)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didFailLoading(identifier, loader, error);
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFailLoading(identifier, loader, error);
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->didFailLoading(identifier, loader, error); // This should come AFTER resource notification, front-end relies on this.
}

void documentThreadableLoaderStartedLoadingForClientImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, ThreadableLoaderClient* client)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->documentThreadableLoaderStartedLoadingForClient(identifier, client);
}

void willLoadXHRImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client, const String& method, const KURL& url, bool async, PassRefPtr<FormData> formData, const HTTPHeaderMap& headers, bool includeCredentials)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willLoadXHR(client, method, url, async, formData, headers, includeCredentials);
}

void didFailXHRLoadingImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFailXHRLoading(client);
}

void didFinishXHRLoadingImpl(InstrumentingAgents* instrumentingAgents, ThreadableLoaderClient* client, unsigned long identifier, const String& sourceString, const String& url, const String& sendURL, unsigned sendLineNumber)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->didFinishXHRLoading(client, identifier, sourceString, url, sendURL, sendLineNumber);
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didFinishXHRLoading(client, identifier, sourceString, url, sendURL, sendLineNumber);
}

void didReceiveXHRResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveXHRResponse(identifier);
}

void willLoadXHRSynchronouslyImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willLoadXHRSynchronously();
}

void didLoadXHRSynchronouslyImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didLoadXHRSynchronously();
}

void scriptImportedImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const String& sourceString)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->scriptImported(identifier, sourceString);
}

void scriptExecutionBlockedByCSPImpl(InstrumentingAgents* instrumentingAgents, const String& directiveText)
{
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->scriptExecutionBlockedByCSP(directiveText);
}

void didReceiveScriptResponseImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveScriptResponse(identifier);
}

void domContentLoadedEventFiredImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->domContentLoadedEventFired(frame);
    if (InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent())
        inspectorAgent->domContentLoadedEventFired(frame);
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->domContentLoadedEventFired(frame);
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->domContentLoadedEventFired(frame);
}

void loadEventFiredImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->loadEventFired(frame);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->loadEventFired(frame);
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->loadEventFired(frame);
}

void frameDetachedFromParentImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorCanvasAgent* canvasAgent = instrumentingAgents->inspectorCanvasAgent())
        canvasAgent->frameDetachedFromParent(frame);
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameDetachedFromParent(frame);
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->frameDetachedFromParent(frame);
}

void didCommitLoadImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, DocumentLoader* loader)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->didCommitLoad(frame, loader);
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didCommitLoad(frame, loader);
    if (InspectorCSSAgent* cssAgent = instrumentingAgents->inspectorCSSAgent())
        cssAgent->didCommitLoad(frame, loader);
    if (InspectorDatabaseAgent* databaseAgent = instrumentingAgents->inspectorDatabaseAgent())
        databaseAgent->didCommitLoad(frame, loader);
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->didCommitLoad(frame, loader);
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->didCommitLoad(frame, loader);
    if (InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent())
        inspectorAgent->didCommitLoad(frame, loader);
    if (InspectorCanvasAgent* canvasAgent = instrumentingAgents->inspectorCanvasAgent())
        canvasAgent->didCommitLoad(frame, loader);
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->didCommitLoad(frame, loader);
}

void frameDocumentUpdatedImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorDOMAgent* domAgent = instrumentingAgents->inspectorDOMAgent())
        domAgent->frameDocumentUpdated(frame);
}

void loaderDetachedFromFrameImpl(InstrumentingAgents* instrumentingAgents, DocumentLoader* loader)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->loaderDetachedFromFrame(loader);
}

void frameStartedLoadingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameStartedLoading(frame);
}

void frameStoppedLoadingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameStoppedLoading(frame);
}

void frameScheduledNavigationImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, double delay)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameScheduledNavigation(frame, delay);
}

void frameClearedScheduledNavigationImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->frameClearedScheduledNavigation(frame);
}

InspectorInstrumentationCookie willRunJavaScriptDialogImpl(InstrumentingAgents* instrumentingAgents, const String& message)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        pageAgent->willRunJavaScriptDialog(message);
    return InspectorInstrumentationCookie(instrumentingAgents, 0);
}

void didRunJavaScriptDialogImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorPageAgent* pageAgent = cookie.instrumentingAgents()->inspectorPageAgent())
        pageAgent->didRunJavaScriptDialog();
}

void willDestroyCachedResourceImpl(CachedResource* cachedResource)
{
    if (!instrumentingAgentsSet)
        return;
    HashSet<InstrumentingAgents*>::iterator end = instrumentingAgentsSet->end();
    for (HashSet<InstrumentingAgents*>::iterator it = instrumentingAgentsSet->begin(); it != end; ++it) {
        InstrumentingAgents* instrumentingAgents = *it;
        if (InspectorResourceAgent* inspectorResourceAgent = instrumentingAgents->inspectorResourceAgent())
            inspectorResourceAgent->willDestroyCachedResource(cachedResource);
    }
}

InspectorInstrumentationCookie willWriteHTMLImpl(InstrumentingAgents* instrumentingAgents, Document* document, unsigned startLine)
{
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willWriteHTML(document, startLine))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didWriteHTMLImpl(const InspectorInstrumentationCookie& cookie, unsigned endLine)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didWriteHTML(endLine);
}

void didRequestAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, Document* document, int callbackId)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didRequestAnimationFrame(document, callbackId);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didRequestAnimationFrame(document, callbackId);
}

void didCancelAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, Document* document, int callbackId)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->didCancelAnimationFrame(document, callbackId);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didCancelAnimationFrame(document, callbackId);
}

InspectorInstrumentationCookie willFireAnimationFrameImpl(InstrumentingAgents* instrumentingAgents, Document* document, int callbackId)
{
    if (InspectorDOMDebuggerAgent* domDebuggerAgent = instrumentingAgents->inspectorDOMDebuggerAgent())
        domDebuggerAgent->willFireAnimationFrame(document, callbackId);
    int timelineAgentId = 0;
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        if (timelineAgent->willFireAnimationFrame(document, callbackId))
            timelineAgentId = timelineAgent->id();
    }
    return InspectorInstrumentationCookie(instrumentingAgents, timelineAgentId);
}

void didFireAnimationFrameImpl(const InspectorInstrumentationCookie& cookie)
{
    if (InspectorTimelineAgent* timelineAgent = retrieveTimelineAgent(cookie))
        timelineAgent->didFireAnimationFrame();
}

// FIXME: Drop this once we no longer generate stacks outside of Inspector.
void addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, PassRefPtr<ScriptCallStack> callStack, unsigned long requestIdentifier)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, callStack, requestIdentifier);
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->addMessageToConsole(source, type);
}

void addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, ScriptState* state, PassRefPtr<ScriptArguments> arguments, unsigned long requestIdentifier)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, state, arguments, requestIdentifier);
    if (InspectorDebuggerAgent* debuggerAgent = instrumentingAgents->inspectorDebuggerAgent())
        debuggerAgent->addMessageToConsole(source, type);
}

void addMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, MessageSource source, MessageType type, MessageLevel level, const String& message, const String& scriptId, unsigned lineNumber, ScriptState* state, unsigned long requestIdentifier)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->addMessageToConsole(source, type, level, message, scriptId, lineNumber, state, requestIdentifier);
}

void consoleCountImpl(InstrumentingAgents* instrumentingAgents, ScriptState* state, PassRefPtr<ScriptArguments> arguments)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->count(state, arguments);
}

void startConsoleTimingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, const String& title)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->time(frame, title);
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->startTiming(title);
}

void stopConsoleTimingImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, const String& title, PassRefPtr<ScriptCallStack> stack)
{
    if (InspectorConsoleAgent* consoleAgent = instrumentingAgents->inspectorConsoleAgent())
        consoleAgent->stopTiming(title, stack);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->timeEnd(frame, title);
}

void consoleTimeStampImpl(InstrumentingAgents* instrumentingAgents, Frame* frame, PassRefPtr<ScriptArguments> arguments)
{
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent()) {
        String message;
        arguments->getFirstArgumentAsString(message);
        timelineAgent->didTimeStamp(frame, message);
     }
}

void addStartProfilingMessageToConsoleImpl(InstrumentingAgents* instrumentingAgents, const String& title, unsigned lineNumber, const String& sourceURL)
{
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent())
        profilerAgent->addStartProfilingMessageToConsole(title, lineNumber, sourceURL);
}

void addProfileImpl(InstrumentingAgents* instrumentingAgents, RefPtr<ScriptProfile> profile, PassRefPtr<ScriptCallStack> callStack)
{
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent()) {
        const ScriptCallFrame& lastCaller = callStack->at(0);
        profilerAgent->addProfile(profile, lastCaller.lineNumber(), lastCaller.sourceURL());
    }
}

String getCurrentUserInitiatedProfileNameImpl(InstrumentingAgents* instrumentingAgents, bool incrementProfileNumber)
{
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent())
        return profilerAgent->getCurrentUserInitiatedProfileName(incrementProfileNumber);
    return "";
}

bool profilerEnabledImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorProfilerAgent* profilerAgent = instrumentingAgents->inspectorProfilerAgent())
        return profilerAgent->enabled();
    return false;
}

void didOpenDatabaseImpl(InstrumentingAgents* instrumentingAgents, PassRefPtr<Database> database, const String& domain, const String& name, const String& version)
{
    if (InspectorDatabaseAgent* databaseAgent = instrumentingAgents->inspectorDatabaseAgent())
        databaseAgent->didOpenDatabase(database, domain, name, version);
}

void didDispatchDOMStorageEventImpl(InstrumentingAgents* instrumentingAgents, const String& key, const String& oldValue, const String& newValue, StorageType storageType, SecurityOrigin* securityOrigin)
{
    if (InspectorDOMStorageAgent* domStorageAgent = instrumentingAgents->inspectorDOMStorageAgent())
        domStorageAgent->didDispatchDOMStorageEvent(key, oldValue, newValue, storageType, securityOrigin);
}

bool shouldPauseDedicatedWorkerOnStartImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        return workerAgent->shouldPauseDedicatedWorkerOnStart();
    return false;
}

void didStartWorkerContextImpl(InstrumentingAgents* instrumentingAgents, WorkerContextProxy* workerContextProxy, const KURL& url)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        workerAgent->didStartWorkerContext(workerContextProxy, url);
}

void willEvaluateWorkerScriptImpl(InstrumentingAgents* instrumentingAgents, WorkerContext* workerContext, int workerThreadStartMode)
{
    if (WorkerRuntimeAgent* runtimeAgent = instrumentingAgents->workerRuntimeAgent())
        runtimeAgent->willEvaluateWorkerScript(workerContext, workerThreadStartMode);
}

void workerContextTerminatedImpl(InstrumentingAgents* instrumentingAgents, WorkerContextProxy* proxy)
{
    if (InspectorWorkerAgent* workerAgent = instrumentingAgents->inspectorWorkerAgent())
        workerAgent->workerContextTerminated(proxy);
}

void didCreateWebSocketImpl(InstrumentingAgents* instrumentingAgents, Document* document, unsigned long identifier, const KURL& requestURL, const String& protocol)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didCreateWebSocket(document, identifier, requestURL, protocol);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didCreateWebSocket(document, identifier, requestURL, protocol);
}

void willSendWebSocketHandshakeRequestImpl(InstrumentingAgents* instrumentingAgents, Document* document, unsigned long identifier, const WebSocketHandshakeRequest& request)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->willSendWebSocketHandshakeRequest(document, identifier, request);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->willSendWebSocketHandshakeRequest(document, identifier, request);
}

void didReceiveWebSocketHandshakeResponseImpl(InstrumentingAgents* instrumentingAgents, Document* document, unsigned long identifier, const WebSocketHandshakeResponse& response)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketHandshakeResponse(document, identifier, response);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didReceiveWebSocketHandshakeResponse(document, identifier, response);
}

void didCloseWebSocketImpl(InstrumentingAgents* instrumentingAgents, Document* document, unsigned long identifier)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didCloseWebSocket(document, identifier);
    if (InspectorTimelineAgent* timelineAgent = instrumentingAgents->inspectorTimelineAgent())
        timelineAgent->didCloseWebSocket(document, identifier);
}

void didReceiveWebSocketFrameImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const WebSocketFrame& frame)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketFrame(identifier, frame);
}

void didSendWebSocketFrameImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const WebSocketFrame& frame)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didSendWebSocketFrame(identifier, frame);
}

void didReceiveWebSocketFrameErrorImpl(InstrumentingAgents* instrumentingAgents, unsigned long identifier, const String& errorMessage)
{
    if (InspectorResourceAgent* resourceAgent = instrumentingAgents->inspectorResourceAgent())
        resourceAgent->didReceiveWebSocketFrameError(identifier, errorMessage);
}

void networkStateChangedImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorApplicationCacheAgent* applicationCacheAgent = instrumentingAgents->inspectorApplicationCacheAgent())
        applicationCacheAgent->networkStateChanged();
}

void updateApplicationCacheStatusImpl(InstrumentingAgents* instrumentingAgents, Frame* frame)
{
    if (InspectorApplicationCacheAgent* applicationCacheAgent = instrumentingAgents->inspectorApplicationCacheAgent())
        applicationCacheAgent->updateApplicationCacheStatus(frame);
}

bool collectingHTMLParseErrorsImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorAgent* inspectorAgent = instrumentingAgents->inspectorAgent())
        return inspectorAgent->hasFrontend();
    return false;
}

bool canvasAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForScriptExecutionContext(scriptExecutionContext);
    return instrumentingAgents && instrumentingAgents->inspectorCanvasAgent();
}

bool consoleAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForScriptExecutionContext(scriptExecutionContext);
    InspectorConsoleAgent* consoleAgent = instrumentingAgents ? instrumentingAgents->inspectorConsoleAgent() : 0;
    return consoleAgent && consoleAgent->enabled();
}

bool timelineAgentEnabled(ScriptExecutionContext* scriptExecutionContext)
{
    InstrumentingAgents* instrumentingAgents = instrumentingAgentsForScriptExecutionContext(scriptExecutionContext);
    return instrumentingAgents && instrumentingAgents->inspectorTimelineAgent();
}

void registerInstrumentingAgents(InstrumentingAgents* instrumentingAgents)
{
    if (!instrumentingAgentsSet)
        instrumentingAgentsSet = new HashSet<InstrumentingAgents*>();
    instrumentingAgentsSet->add(instrumentingAgents);
}

void unregisterInstrumentingAgents(InstrumentingAgents* instrumentingAgents)
{
    if (!instrumentingAgentsSet)
        return;
    instrumentingAgentsSet->remove(instrumentingAgents);
    if (instrumentingAgentsSet->isEmpty()) {
        delete instrumentingAgentsSet;
        instrumentingAgentsSet = 0;
    }
}

InspectorTimelineAgent* retrieveTimelineAgent(const InspectorInstrumentationCookie& cookie)
{
    if (!cookie.instrumentingAgents())
        return 0;
    InspectorTimelineAgent* timelineAgent = cookie.instrumentingAgents()->inspectorTimelineAgent();
    if (timelineAgent && cookie.hasMatchingTimelineAgentId(timelineAgent->id()))
        return timelineAgent;
    return 0;
}

InstrumentingAgents* instrumentingAgentsForPage(Page* page)
{
    if (!page)
        return 0;
    return instrumentationForPage(page);
}

InstrumentingAgents* instrumentingAgentsForRenderObject(RenderObject* renderer)
{
    return instrumentingAgentsForFrame(renderer->frame());
}

InstrumentingAgents* instrumentingAgentsForWorkerContext(WorkerContext* workerContext)
{
    if (!workerContext)
        return 0;
    return instrumentationForWorkerContext(workerContext);
}

InstrumentingAgents* instrumentingAgentsForNonDocumentContext(ScriptExecutionContext* context)
{
    if (context->isWorkerContext())
        return instrumentationForWorkerContext(static_cast<WorkerContext*>(context));
    return 0;
}

GeolocationPosition* overrideGeolocationPositionImpl(InstrumentingAgents* instrumentingAgents, GeolocationPosition* position)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        position = pageAgent->overrideGeolocationPosition(position);
    return position;
}

DeviceOrientationData* overrideDeviceOrientationImpl(InstrumentingAgents* instrumentingAgents, DeviceOrientationData* deviceOrientation)
{
    if (InspectorPageAgent* pageAgent = instrumentingAgents->inspectorPageAgent())
        deviceOrientation = pageAgent->overrideDeviceOrientation(deviceOrientation);
    return deviceOrientation;
}

void layerTreeDidChangeImpl(InstrumentingAgents* instrumentingAgents)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->layerTreeDidChange();
}

void renderLayerDestroyedImpl(InstrumentingAgents* instrumentingAgents, const RenderLayer* renderLayer)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->renderLayerDestroyed(renderLayer);
}

void pseudoElementDestroyedImpl(InstrumentingAgents* instrumentingAgents, PseudoElement* pseudoElement)
{
    if (InspectorLayerTreeAgent* layerTreeAgent = instrumentingAgents->inspectorLayerTreeAgent())
        layerTreeAgent->pseudoElementDestroyed(pseudoElement);
}

bool cssErrorFilter(const CSSParserString& content, int propertyId, int errorType)
{
    return InspectorCSSAgent::cssErrorFilter(content, propertyId, errorType);
}

} // namespace InspectorInstrumentation

namespace InstrumentationEvents {
const char PaintSetup[] = "PaintSetup";
const char PaintLayer[] = "PaintLayer";
const char RasterTask[] = "RasterTask";
const char ImageDecodeTask[] = "ImageDecodeTask";
const char Paint[] = "Paint";
const char Layer[] = "Layer";
const char BeginFrame[] = "BeginFrame";
};

namespace InstrumentationEventArguments {
const char LayerId[] = "layerId";
const char PageId[] = "pageId";
const char NodeId[] = "nodeId";
};

} // namespace WebCore

