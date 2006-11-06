/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "FrameLoader.h"

#import "Cache.h"
#import "DOMElementInternal.h"
#import "Document.h"
#import "DocumentLoader.h"
#import "Element.h"
#import "FloatRect.h"
#import "FormDataStreamMac.h"
#import "FormState.h"
#import "FrameLoadRequest.h"
#import "FrameLoaderClient.h"
#import "FrameMac.h"
#import "FramePrivate.h"
#import "PageState.h"
#import "FrameTree.h"
#import "HTMLNames.h"
#import "LoaderNSURLExtras.h"
#import "LoaderNSURLRequestExtras.h"
#import "MainResourceLoader.h"
#import "NavigationAction.h"
#import "Page.h"
#import "Plugin.h"
#import "ResourceResponse.h"
#import "ResourceResponseMac.h"
#import "SubresourceLoader.h"
#import "TextResourceDecoder.h"
#import "WebCoreFrameBridge.h"
#import "WebCoreIconDatabaseBridge.h"
#import "WebCorePageBridge.h"
#import "WebCorePageState.h"
#import "WebCoreSystemInterface.h"
#import "WebDataProtocol.h"
#import "WindowFeatures.h"
#import <kjs/JSLock.h>
#import <wtf/Assertions.h>

namespace WebCore {

using namespace HTMLNames;

static double storedTimeOfLastCompletedLoad;

static void cancelAll(const ResourceLoaderSet& loaders)
{
    const ResourceLoaderSet copy = loaders;
    ResourceLoaderSet::const_iterator end = copy.end();
    for (ResourceLoaderSet::const_iterator it = copy.begin(); it != end; ++it)
        (*it)->cancel();
}

bool isBackForwardLoadType(FrameLoadType type)
{
    switch (type) {
        case FrameLoadTypeStandard:
        case FrameLoadTypeReload:
        case FrameLoadTypeReloadAllowingStaleData:
        case FrameLoadTypeSame:
        case FrameLoadTypeInternal:
        case FrameLoadTypeReplace:
            return false;
        case FrameLoadTypeBack:
        case FrameLoadTypeForward:
        case FrameLoadTypeIndexedBackForward:
            return true;
    }
    ASSERT_NOT_REACHED();
    return false;
}

static int numRequests(Document* document)
{
    if (document)
        return cache()->loader()->numRequests(document->docLoader());
    return 0;
}

void FrameLoader::prepareForLoadStart()
{
    m_client->progressStarted();
    m_client->dispatchDidStartProvisionalLoad();
}

void FrameLoader::setupForReplace()
{
    setState(FrameStateProvisional);
    m_provisionalDocumentLoader = m_documentLoader;
    m_documentLoader = nil;
    detachChildren();
}

void FrameLoader::setupForReplaceByMIMEType(const String& newMIMEType)
{
    activeDocumentLoader()->setupForReplaceByMIMEType(newMIMEType);
}

void FrameLoader::finalSetupForReplace(DocumentLoader* loader)
{
    m_client->clearUnarchivingState(loader);
}

void FrameLoader::load(const KURL& URL, Event* event)
{
    load(ResourceRequest(URL), true, event, 0, HashMap<String, String>());
}

void FrameLoader::load(const FrameLoadRequest& request, bool userGesture, Event* event,
    Element* submitForm, const HashMap<String, String>& formValues)
{
    String referrer;
    String argsReferrer = request.resourceRequest().httpReferrer();
    if (!argsReferrer.isEmpty())
        referrer = argsReferrer;
    else
        referrer = m_frame->referrer();
 
    bool hideReferrer;
    if (!canLoad(request.resourceRequest().url().getNSURL(), referrer, hideReferrer))
        return;
    if (hideReferrer)
        referrer = String();
    
    Frame* targetFrame = m_frame->tree()->find(request.frameName());
    if (!canTarget(targetFrame))
        return;
        
    if (request.resourceRequest().httpMethod() != "POST") {
        FrameLoadType loadType;
        if (request.resourceRequest().cachePolicy() == ReloadIgnoringCacheData)
            loadType = FrameLoadTypeReload;
        else if (!userGesture)
            loadType = FrameLoadTypeInternal;
        else
            loadType = FrameLoadTypeStandard;    
    
        load(request.resourceRequest().url(), referrer, loadType, 
            request.frameName(), event, submitForm, formValues);
    } else
        post(request.resourceRequest().url(), referrer, request.frameName(), 
            request.resourceRequest().httpBody(), request.resourceRequest().httpContentType(), event, submitForm, formValues);

    if (targetFrame && targetFrame != m_frame)
        [Mac(targetFrame)->bridge() activateWindow];
}

void FrameLoader::load(const KURL& URL, const String& referrer, FrameLoadType newLoadType,
    const String& frameName, Event* event, Element* form, const HashMap<String, String>& values)
{
    bool isFormSubmission = !values.isEmpty();
    
    NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:URL.getNSURL()];
    setHTTPReferrer(request, referrer);
    addExtraFieldsToRequest(request, true, event || isFormSubmission);
    if (newLoadType == FrameLoadTypeReload)
        [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];

    ASSERT(newLoadType != FrameLoadTypeSame);

    NavigationAction action(URL, newLoadType, isFormSubmission, event);

    RefPtr<FormState> formState;
    if (form && !values.isEmpty())
        formState = FormState::create(form, values, m_frame);
    
    if (!frameName.isEmpty()) {
        if (Frame* targetFrame = m_frame->tree()->find(frameName))
            targetFrame->loader()->load(URL, referrer, newLoadType, String(), event, form, values);
        else
            checkNewWindowPolicy(action, request, formState.release(), frameName);
        [request release];
        return;
    }

    RefPtr<DocumentLoader> oldDocumentLoader = m_documentLoader;

    bool sameURL = m_client->shouldTreatURLAsSameAsCurrent(URL);
    
    // Make sure to do scroll to anchor processing even if the URL is
    // exactly the same so pages with '#' links and DHTML side effects
    // work properly.
    if (!isFormSubmission
        && newLoadType != FrameLoadTypeReload
        && newLoadType != FrameLoadTypeSame
        && !shouldReload(URL, m_frame->url())
        // We don't want to just scroll if a link from within a
        // frameset is trying to reload the frameset into _top.
        && !m_frame->isFrameSet()) {

        // Just do anchor navigation within the existing content.
        
        // We don't do this if we are submitting a form, explicitly reloading,
        // currently displaying a frameset, or if the new URL does not have a fragment.
        // These rules are based on what KHTML was doing in KHTMLPart::openURL.
        
        // FIXME: What about load types other than Standard and Reload?
        
        oldDocumentLoader->setTriggeringAction(action);
        stopPolicyCheck();
        checkNavigationPolicy(request, oldDocumentLoader.get(), formState.release(),
            callContinueFragmentScrollAfterNavigationPolicy, this);
    } else {
        // must grab this now, since this load may stop the previous load and clear this flag
        bool isRedirect = m_quickRedirectComing;
        load(request, action, newLoadType, formState.release());
        if (isRedirect) {
            m_quickRedirectComing = false;
            if (m_provisionalDocumentLoader)
                m_provisionalDocumentLoader->setIsClientRedirect(true);
        } else if (sameURL)
            // Example of this case are sites that reload the same URL with a different cookie
            // driving the generated content, or a master frame with links that drive a target
            // frame, where the user has clicked on the same link repeatedly.
            m_loadType = FrameLoadTypeSame;
    }
    
    [request release];
}

void FrameLoader::load(NSURLRequest *request)
{
    // FIXME: is this the right place to reset loadType? Perhaps this should be done after loading is finished or aborted.
    m_loadType = FrameLoadTypeStandard;
    load(m_client->createDocumentLoader(request).get());
}

void FrameLoader::load(NSURLRequest *request, const String& frameName)
{
    if (frameName.isEmpty()) {
        load(request);
        return;
    }

    Frame* frame = m_frame->tree()->find(frameName);
    if (frame) {
        frame->loader()->load(request);
        return;
    }

    checkNewWindowPolicy(NavigationAction([request URL], NavigationTypeOther),
        request, 0, frameName);
}

void FrameLoader::load(NSURLRequest *request, const NavigationAction& action, FrameLoadType type, PassRefPtr<FormState> formState)
{
    RefPtr<DocumentLoader> loader = m_client->createDocumentLoader(request);
    setPolicyDocumentLoader(loader.get());

    loader->setTriggeringAction(action);
    if (m_documentLoader)
        loader->setOverrideEncoding(m_documentLoader->overrideEncoding());

    load(loader.get(), type, formState);
}

void FrameLoader::load(DocumentLoader* newDocumentLoader)
{
    stopPolicyCheck();
    setPolicyDocumentLoader(newDocumentLoader);

    NSMutableURLRequest *r = newDocumentLoader->request();
    addExtraFieldsToRequest(r, true, false);
    FrameLoadType type;
    if (m_client->shouldTreatURLAsSameAsCurrent([newDocumentLoader->originalRequest() URL])) {
        [r setCachePolicy:NSURLRequestReloadIgnoringCacheData];
        type = FrameLoadTypeSame;
    } else
        type = FrameLoadTypeStandard;

    if (m_documentLoader)
        newDocumentLoader->setOverrideEncoding(m_documentLoader->overrideEncoding());
    
    // When we loading alternate content for an unreachable URL that we're
    // visiting in the b/f list, we treat it as a reload so the b/f list 
    // is appropriately maintained.
    if (shouldReloadToHandleUnreachableURL(newDocumentLoader->originalRequest())) {
        ASSERT(type == FrameLoadTypeStandard);
        type = FrameLoadTypeReload;
    }

    load(newDocumentLoader, type, 0);
}

void FrameLoader::load(DocumentLoader* loader, FrameLoadType type, PassRefPtr<FormState> formState)
{
    ASSERT(m_client->hasWebView());

    // Unfortunately the view must be non-nil, this is ultimately due
    // to parser requiring a FrameView.  We should fix this dependency.

    ASSERT(m_client->hasFrameView());

    m_policyLoadType = type;

    if (Frame* parent = m_frame->tree()->parent())
        loader->setOverrideEncoding(parent->loader()->documentLoader()->overrideEncoding());

    stopPolicyCheck();
    setPolicyDocumentLoader(loader);

    checkNavigationPolicy(loader->request(), loader, formState,
        callContinueLoadAfterNavigationPolicy, this);
}

bool FrameLoader::canLoad(NSURL *URL, const String& referrer, bool& hideReferrer)
{
    bool referrerIsWebURL = referrer.startsWith("http:", false) || referrer.startsWith("https:", false);
    bool referrerIsLocalURL = referrer.startsWith("file:", false) || referrer.startsWith("applewebdata:");
    bool URLIsFileURL = [URL scheme] && [[URL scheme] compare:@"file" options:(NSCaseInsensitiveSearch|NSLiteralSearch)] == NSOrderedSame;
    bool referrerIsSecureURL = referrer.startsWith("https:", false);
    bool URLIsSecureURL = [URL scheme] && [[URL scheme] compare:@"https" options:(NSCaseInsensitiveSearch|NSLiteralSearch)] == NSOrderedSame;
    
    hideReferrer = !referrerIsWebURL || (referrerIsSecureURL && !URLIsSecureURL);
    return !URLIsFileURL || referrerIsLocalURL;
}

bool FrameLoader::canTarget(Frame* target) const
{
    // This method prevents this exploit:
    // <rdar://problem/3715785> multiple frame injection vulnerability reported by Secunia, affects almost all browsers

    if (!target)
        return true;

    // Allow with navigation within the same page/frameset.
    if (m_frame->page() == target->page())
        return true;

    String domain;
    if (Document* document = m_frame->document())
        domain = document->domain();
    // Allow if the request is made from a local file.
    if (domain.isEmpty())
        return true;
    
    Frame* parent = target->tree()->parent();
    // Allow if target is an entire window.
    if (!parent)
        return true;
    
    String parentDomain;
    if (Document* parentDocument = parent->document())
        domain = parentDocument->domain();
    // Allow if the domain of the parent of the targeted frame equals this domain.
    return equalIgnoringCase(parentDomain, domain);
}

Frame* FrameLoader::createWindow(const FrameLoadRequest& request, const WindowFeatures& features)
{ 
    ASSERT(!features.dialog || request.frameName().isEmpty());

    if (!request.frameName().isEmpty())
        if (Frame* frame = m_frame->tree()->find(request.frameName())) {
            if (!request.resourceRequest().url().isEmpty())
                frame->loader()->load(request, true, nil, 0, HashMap<String, String>());
            [Mac(frame)->bridge() activateWindow];
            return frame;
        }

    WebCorePageBridge *pageBridge;
    if (features.dialog)
        pageBridge = [m_frame->page()->bridge() createModalDialogWithURL:request.resourceRequest().url().getNSURL() referrer:m_frame->referrer()];
    else
        pageBridge = [Mac(m_frame)->bridge() createWindowWithURL:request.resourceRequest().url().getNSURL()];
    if (!pageBridge)
        return 0;

    Page* page = [pageBridge impl];
    Frame* frame = page->mainFrame();
    frame->tree()->setName(request.frameName());

    [Mac(frame)->bridge() setToolbarsVisible:features.toolBarVisible || features.locationBarVisible];
    [Mac(frame)->bridge() setStatusbarVisible:features.statusBarVisible];
    [Mac(frame)->bridge() setScrollbarsVisible:features.scrollbarsVisible];
    [Mac(frame)->bridge() setWindowIsResizable:features.resizable];

    // The width and 'height parameters specify the dimensions of the view, but we can only resize
    // the window, so adjust for the difference between the window size and the view size.

    FloatRect windowRect = page->windowRect();
    
    // FIXME: We'd like to get frameViewSize from the frame's view, but that doesn't
    // get created until the frame loads its document.
    IntSize frameViewSize = IntSize([[pageBridge outerView] frame].size);
    if (features.xSet)
        windowRect.setX(features.x);
    if (features.ySet)
        windowRect.setY(features.y);
    if (features.widthSet)
        windowRect.setWidth(features.width + (windowRect.width() - frameViewSize.width()));
    if (features.heightSet)
        windowRect.setHeight(features.height + (windowRect.height() - frameViewSize.height()));
    page->setWindowRect(windowRect);

    [Mac(frame)->bridge() showWindow];

    return frame;
}

bool FrameLoader::startLoadingMainResource(NSMutableURLRequest *request, id identifier)
{
    ASSERT(!m_mainResourceLoader);
    m_mainResourceLoader = MainResourceLoader::create(m_frame);
    m_mainResourceLoader->setIdentifier(identifier);
    addExtraFieldsToRequest(request, true, false);
    if (!m_mainResourceLoader->load(request)) {
        // FIXME: If this should really be caught, we should just ASSERT this doesn't happen;
        // should it be caught by other parts of WebKit or other parts of the app?
        LOG_ERROR("could not create WebResourceHandle for URL %@ -- should be caught by policy handler level", [request URL]);
        m_mainResourceLoader = 0;
        return false;
    }
    return true;
}

// FIXME: Poor method name; also, why is this not part of startProvisionalLoad:?
void FrameLoader::startLoading()
{
    m_provisionalDocumentLoader->prepareForLoadStart();

    if (isLoadingMainResource())
        return;

    m_client->clearLoadingFromPageCache(m_provisionalDocumentLoader.get());

    id identifier = m_client->dispatchIdentifierForInitialRequest
        (m_provisionalDocumentLoader.get(), m_provisionalDocumentLoader->originalRequest());
        
    if (!startLoadingMainResource(m_provisionalDocumentLoader->actualRequest(), identifier))
        m_provisionalDocumentLoader->updateLoading();
}

void FrameLoader::stopLoadingPlugIns()
{
    cancelAll(m_plugInStreamLoaders);
}

void FrameLoader::stopLoadingSubresources()
{
    cancelAll(m_subresourceLoaders);
}

void FrameLoader::stopLoading(NSError *error)
{
    m_mainResourceLoader->cancel(error);
}

void FrameLoader::stopLoadingSubframes()
{
    for (Frame* child = m_frame->tree()->firstChild(); child; child = child->tree()->nextSibling())
        child->loader()->stopLoading();
}

void FrameLoader::stopLoading()
{
    // If this method is called from within this method, infinite recursion can occur (3442218). Avoid this.
    if (m_isStoppingLoad)
        return;

    m_isStoppingLoad = true;

    stopPolicyCheck();

    stopLoadingSubframes();
    if (m_provisionalDocumentLoader)
        m_provisionalDocumentLoader->stopLoading();
    if (m_documentLoader)
        m_documentLoader->stopLoading();
    setProvisionalDocumentLoader(0);
    m_client->clearArchivedResources();

    m_isStoppingLoad = false;    
}

void FrameLoader::cancelMainResourceLoad()
{
    if (m_mainResourceLoader)
        m_mainResourceLoader->cancel();
}

void FrameLoader::cancelPendingArchiveLoad(ResourceLoader* loader)
{
    m_client->cancelPendingArchiveLoad(loader);
}

DocumentLoader* FrameLoader::activeDocumentLoader() const
{
    if (m_state == FrameStateProvisional)
        return m_provisionalDocumentLoader.get();
    return m_documentLoader.get();
}

void FrameLoader::addPlugInStreamLoader(ResourceLoader* loader)
{
    m_plugInStreamLoaders.add(loader);
    activeDocumentLoader()->setLoading(true);
}

void FrameLoader::removePlugInStreamLoader(ResourceLoader* loader)
{
    m_plugInStreamLoaders.remove(loader);
    activeDocumentLoader()->updateLoading();
}

bool FrameLoader::isLoadingMainResource() const
{
    return m_mainResourceLoader;
}

bool FrameLoader::isLoadingSubresources() const
{
    return !m_subresourceLoaders.isEmpty();
}

bool FrameLoader::isLoadingPlugIns() const
{
    return !m_plugInStreamLoaders.isEmpty();
}

bool FrameLoader::isLoading() const
{
    return isLoadingMainResource() || isLoadingSubresources() || isLoadingPlugIns();
}

void FrameLoader::addSubresourceLoader(ResourceLoader* loader)
{
    ASSERT(!m_provisionalDocumentLoader);
    m_subresourceLoaders.add(loader);
    activeDocumentLoader()->setLoading(true);
}

void FrameLoader::removeSubresourceLoader(ResourceLoader* loader)
{
    m_subresourceLoaders.remove(loader);
    activeDocumentLoader()->updateLoading();
    checkLoadComplete();
}

NSData *FrameLoader::mainResourceData() const
{
    if (!m_mainResourceLoader)
        return nil;
    return m_mainResourceLoader->resourceData();
}

void FrameLoader::releaseMainResourceLoader()
{
    m_mainResourceLoader = 0;
}

void FrameLoader::setDocumentLoader(DocumentLoader* loader)
{
    if (!loader && !m_documentLoader)
        return;
    
    ASSERT(loader != m_documentLoader);
    ASSERT(!loader || loader->frameLoader() == this);

    m_client->prepareForDataSourceReplacement();
    if (m_documentLoader)
        m_documentLoader->detachFromFrame();

    m_documentLoader = loader;
}

DocumentLoader* FrameLoader::documentLoader() const
{
    return m_documentLoader.get();
}

void FrameLoader::setPolicyDocumentLoader(DocumentLoader* loader)
{
    if (m_policyDocumentLoader == loader)
        return;

    ASSERT(m_frame);
    if (loader)
        loader->setFrame(m_frame);
    if (m_policyDocumentLoader
            && m_policyDocumentLoader != m_provisionalDocumentLoader
            && m_policyDocumentLoader != m_documentLoader)
        m_policyDocumentLoader->detachFromFrame();

    m_policyDocumentLoader = loader;
}
   
DocumentLoader* FrameLoader::provisionalDocumentLoader()
{
    return m_provisionalDocumentLoader.get();
}

void FrameLoader::setProvisionalDocumentLoader(DocumentLoader* loader)
{
    ASSERT(!loader || !m_provisionalDocumentLoader);
    ASSERT(!loader || loader->frameLoader() == this);

    if (m_provisionalDocumentLoader && m_provisionalDocumentLoader != m_documentLoader)
        m_provisionalDocumentLoader->detachFromFrame();

    m_provisionalDocumentLoader = loader;
}

FrameState FrameLoader::state() const
{
    return m_state;
}

double FrameLoader::timeOfLastCompletedLoad()
{
    return storedTimeOfLastCompletedLoad;
}

void FrameLoader::provisionalLoadStarted()
{
    m_firstLayoutDone = false;
    m_frame->provisionalLoadStarted();
    m_client->provisionalLoadStarted();
}

void FrameLoader::setState(FrameState newState)
{    
    m_state = newState;
    
    if (newState == FrameStateProvisional)
        provisionalLoadStarted();
    else if (newState == FrameStateComplete) {
        frameLoadCompleted();
        storedTimeOfLastCompletedLoad = CFAbsoluteTimeGetCurrent();
        if (m_documentLoader)
            m_documentLoader->stopRecordingResponses();
    }
}

void FrameLoader::clearProvisionalLoad()
{
    setProvisionalDocumentLoader(0);
    m_client->progressCompleted();
    setState(FrameStateComplete);
}

void FrameLoader::markLoadComplete()
{
    setState(FrameStateComplete);
}

void FrameLoader::commitProvisionalLoad()
{
    stopLoadingSubresources();
    stopLoadingPlugIns();

    setDocumentLoader(m_provisionalDocumentLoader.get());
    setProvisionalDocumentLoader(0);
    setState(FrameStateCommittedPage);
}

id FrameLoader::identifierForInitialRequest(NSURLRequest *clientRequest)
{
    return m_client->dispatchIdentifierForInitialRequest(activeDocumentLoader(), clientRequest);
}

void FrameLoader::applyUserAgent(NSMutableURLRequest *request)
{
    static String lastUserAgent;
    static RetainPtr<NSString> lastUserAgentNSString;

    String userAgent = client()->userAgent();
    ASSERT(!userAgent.isNull());
    if (userAgent != lastUserAgent) {
        lastUserAgent = userAgent;
        lastUserAgentNSString = userAgent;
    }

    [request setValue:lastUserAgentNSString.get() forHTTPHeaderField:@"User-Agent"];
}

NSURLRequest *FrameLoader::willSendRequest(ResourceLoader* loader, NSMutableURLRequest *clientRequest, NSURLResponse *redirectResponse)
{
    applyUserAgent(clientRequest);
    return m_client->dispatchWillSendRequest(activeDocumentLoader(), loader->identifier(), clientRequest, redirectResponse);
}

void FrameLoader::didReceiveAuthenticationChallenge(ResourceLoader* loader, NSURLAuthenticationChallenge *currentWebChallenge)
{
    m_client->dispatchDidReceiveAuthenticationChallenge(activeDocumentLoader(), loader->identifier(), currentWebChallenge);
}

void FrameLoader::didCancelAuthenticationChallenge(ResourceLoader* loader, NSURLAuthenticationChallenge *currentWebChallenge)
{
    m_client->dispatchDidCancelAuthenticationChallenge(activeDocumentLoader(), loader->identifier(), currentWebChallenge);
}

void FrameLoader::didReceiveResponse(ResourceLoader* loader, NSURLResponse *r)
{
    activeDocumentLoader()->addResponse(r);
    
    m_client->incrementProgress(loader->identifier(), r);
    m_client->dispatchDidReceiveResponse(activeDocumentLoader(), loader->identifier(), r);
}

void FrameLoader::didReceiveData(ResourceLoader* loader, NSData *data, int lengthReceived)
{
    m_client->incrementProgress(loader->identifier(), data);
    m_client->dispatchDidReceiveContentLength(activeDocumentLoader(), loader->identifier(), lengthReceived);
}

void FrameLoader::didFinishLoad(ResourceLoader* loader)
{    
    m_client->completeProgress(loader->identifier());
    m_client->dispatchDidFinishLoading(activeDocumentLoader(), loader->identifier());
}

void FrameLoader::didFailToLoad(ResourceLoader* loader, NSError *error)
{
    m_client->completeProgress(loader->identifier());
    if (error)
        m_client->dispatchDidFailLoading(activeDocumentLoader(), loader->identifier(), error);
}

bool FrameLoader::privateBrowsingEnabled() const
{
    return m_client->privateBrowsingEnabled();
}

NSURLRequest *FrameLoader::originalRequest() const
{
    return activeDocumentLoader()->originalRequestCopy();
}

void FrameLoader::receivedMainResourceError(NSError *error, bool isComplete)
{
    // Retain because the stop may release the last reference to it.
    RefPtr<Frame> protect(m_frame);

    RefPtr<DocumentLoader> loader = activeDocumentLoader();
    
    if (isComplete) {
        // FIXME: Don't want to do this if an entirely new load is going, so should check
        // that both data sources on the frame are either this or nil.
        m_frame->stop();
        if (m_client->shouldFallBack(error))
            m_frame->handleFallbackContent();
    }
    
    if (m_state == FrameStateProvisional) {
        NSURL *failedURL = [m_provisionalDocumentLoader->originalRequestCopy() URL];
        m_frame->didNotOpenURL(failedURL);

        // We might have made a page cache item, but now we're bailing out due to an error before we ever
        // transitioned to the new page (before WebFrameState == commit).  The goal here is to restore any state
        // so that the existing view (that wenever got far enough to replace) can continue being used.
        Document* document = m_frame->document();
        if (document)
            document->setInPageCache(false);
        m_client->invalidateCurrentItemPageCache();
        
        // Call clientRedirectCancelledOrFinished here so that the frame load delegate is notified that the redirect's
        // status has changed, if there was a redirect. The frame load delegate may have saved some state about
        // the redirect in its -webView:willPerformClientRedirectToURL:delay:fireDate:forFrame:. Since we are definitely
        // not going to use this provisional resource, as it was cancelled, notify the frame load delegate that the redirect
        // has ended.
        if (m_sentRedirectNotification)
            clientRedirectCancelledOrFinished(false);
    }
    
    
    loader->mainReceivedError(error, isComplete);
}

void FrameLoader::clientRedirectCancelledOrFinished(bool cancelWithLoadInProgress)
{
    // Note that -webView:didCancelClientRedirectForFrame: is called on the frame load delegate even if
    // the redirect succeeded.  We should either rename this API, or add a new method, like
    // -webView:didFinishClientRedirectForFrame:
    m_client->dispatchDidCancelClientRedirect();

    if (!cancelWithLoadInProgress)
        m_quickRedirectComing = false;

    m_sentRedirectNotification = false;
}

void FrameLoader::clientRedirected(const KURL& URL, double seconds, double fireDate, bool lockHistory, bool isJavaScriptFormAction)
{
    m_client->dispatchWillPerformClientRedirect(URL, seconds, fireDate);
    
    // Remember that we sent a redirect notification to the frame load delegate so that when we commit
    // the next provisional load, we can send a corresponding -webView:didCancelClientRedirectForFrame:
    m_sentRedirectNotification = true;
    
    // If a "quick" redirect comes in an, we set a special mode so we treat the next
    // load as part of the same navigation. If we don't have a document loader, we have
    // no "original" load on which to base a redirect, so we treat the redirect as a normal load.
    m_quickRedirectComing = lockHistory && m_documentLoader && !isJavaScriptFormAction;
}

bool FrameLoader::shouldReload(const KURL& currentURL, const KURL& destinationURL)
{
    // This function implements the rule: "Don't reload if navigating by fragment within
    // the same URL, but do reload if going to a new URL or to the same URL with no
    // fragment identifier at all."
    if (!currentURL.hasRef() && !destinationURL.hasRef())
        return true;
    return !equalIgnoringRef(currentURL, destinationURL);
}

void FrameLoader::callContinueFragmentScrollAfterNavigationPolicy(void* argument,
    NSURLRequest *request, PassRefPtr<FormState>)
{
    FrameLoader* loader = static_cast<FrameLoader*>(argument);
    loader->continueFragmentScrollAfterNavigationPolicy(request);
}

void FrameLoader::continueFragmentScrollAfterNavigationPolicy(NSURLRequest *request)
{
    if (!request)
        return;
    
    NSURL *URL = [request URL];
    
    bool isRedirect = m_quickRedirectComing;
    m_quickRedirectComing = false;
    
    m_documentLoader->replaceRequestURLForAnchorScroll(URL);
    if (!isRedirect && !m_client->shouldTreatURLAsSameAsCurrent(URL)) {
        // NB: must happen after _setURL, since we add based on the current request.
        // Must also happen before we openURL and displace the scroll position, since
        // adding the BF item will save away scroll state.
        
        // NB2:  If we were loading a long, slow doc, and the user anchor nav'ed before
        // it was done, currItem is now set the that slow doc, and prevItem is whatever was
        // before it.  Adding the b/f item will bump the slow doc down to prevItem, even
        // though its load is not yet done.  I think this all works out OK, for one because
        // we have already saved away the scroll and doc state for the long slow load,
        // but it's not an obvious case.

        m_client->addHistoryItemForFragmentScroll();
    }
    
    m_frame->scrollToAnchor(URL);
    
    if (!isRedirect)
        // This will clear previousItem from the rest of the frame tree that didn't
        // doing any loading. We need to make a pass on this now, since for anchor nav
        // we'll not go through a real load and reach Completed state.
        checkLoadComplete();
 
    m_client->dispatchDidChangeLocationWithinPage();
    m_client->didFinishLoad();
}

void FrameLoader::closeOldDataSources()
{
    // FIXME: Is it important for this traversal to be postorder instead of preorder?
    // If so, add helpers for postorder traversal, and use them. If not, then lets not
    // use a recursive algorithm here.
    for (Frame* child = m_frame->tree()->firstChild(); child; child = child->tree()->nextSibling())
        child->loader()->closeOldDataSources();
    
    if (m_documentLoader)
        m_client->dispatchWillClose();

    m_client->setMainFrameDocumentReady(false); // stop giving out the actual DOMDocument to observers
}

void FrameLoader::open(PageState& state)
{
    ASSERT(m_frame->page()->mainFrame() == m_frame);

    FramePrivate* d = m_frame->d;

    m_frame->cancelRedirection();

    // We still have to close the previous part page.
    m_frame->closeURL();

    d->m_bComplete = false;
    
    // Don't re-emit the load event.
    d->m_bLoadEventEmitted = true;
    
    // Delete old status bar messages (if it _was_ activated on last URL).
    if (m_frame->javaScriptEnabled()) {
        d->m_kjsStatusBarText = String();
        d->m_kjsDefaultStatusBarText = String();
    }

    KURL URL = state.URL();

    if (URL.protocol().startsWith("http") && !URL.host().isEmpty() && URL.path().isEmpty())
        URL.setPath("/");
    
    d->m_url = URL;
    d->m_workingURL = URL;

    m_frame->started();

    m_frame->clear();

    Document* document = state.document();
    document->setInPageCache(false);

    d->m_bCleared = false;
    d->m_bComplete = false;
    d->m_bLoadEventEmitted = false;
    d->m_referrer = URL.url();
    
    m_frame->setView(document->view());
    
    d->m_doc = document;
    d->m_mousePressNode = state.mousePressNode();
    d->m_decoder = document->decoder();

    m_frame->updatePolicyBaseURL();

    state.restoreJavaScriptState(m_frame->page());

    m_frame->checkCompleted();
}

void FrameLoader::opened()
{
    if (m_loadType == FrameLoadTypeStandard && m_documentLoader->isClientRedirect())
        m_client->updateHistoryAfterClientRedirect();

    if (m_client->isLoadingFromPageCache(m_documentLoader.get())) {
        // Force a layout to update view size and thereby update scrollbars.
        m_client->forceLayout();

        const ResponseVector& responses = m_documentLoader->responses();
        size_t count = responses.size();
        for (size_t i = 0; i < count; i++) {
            NSURLResponse *response = responses[i].get();
            // FIXME: If the WebKit client changes or cancels the request, this is not respected.
            NSError *error;
            id identifier;
            NSURLRequest *request = [[NSURLRequest alloc] initWithURL:[response URL]];
            requestFromDelegate(request, identifier, error);
            sendRemainingDelegateMessages(identifier, response, (unsigned)[response expectedContentLength], error);
            [request release];
        }
        
        m_client->loadedFromPageCache();

        m_documentLoader->setPrimaryLoadComplete(true);

        // FIXME: Why only this frame and not parent frames?
        checkLoadCompleteForThisFrame();
    }
}

void FrameLoader::commitProvisionalLoad(NSDictionary *pageCache)
{
    RefPtr<DocumentLoader> pdl = m_provisionalDocumentLoader;
    
    if (m_loadType != FrameLoadTypeReplace)
        closeOldDataSources();
    
    if (!pageCache)
        m_client->makeRepresentation(pdl.get());
    
    transitionToCommitted(pageCache);
    
    // Call -_clientRedirectCancelledOrFinished: here so that the frame load delegate is notified that the redirect's
    // status has changed, if there was a redirect.  The frame load delegate may have saved some state about
    // the redirect in its -webView:willPerformClientRedirectToURL:delay:fireDate:forFrame:.  Since we are
    // just about to commit a new page, there cannot possibly be a pending redirect at this point.
    if (m_sentRedirectNotification)
        clientRedirectCancelledOrFinished(false);
    
    WebCorePageState *pageState = [pageCache objectForKey:WebCorePageCacheStateKey];
    if (PageState* frameState = [pageState impl]) {
        open(*frameState);
        frameState->clear();
    } else {
        NSURLResponse *response = pdl->response();
    
        NSURL *URL = [pdl->request() _webDataRequestBaseURL];
        if (!URL)
            URL = [response URL];
        if (!URL || urlIsEmpty(URL))
            URL = [NSURL URLWithString:@"about:blank"];    

        m_frame->setResponseMIMEType([response MIMEType]);
        if (m_frame->didOpenURL(URL)) {
            if ([response isKindOfClass:[NSHTTPURLResponse class]])
                if (NSString *refresh = [[(NSHTTPURLResponse *)response allHeaderFields] objectForKey:@"Refresh"])
                    m_frame->addMetaData("http-refresh", refresh);
            m_frame->addMetaData("modified", [wkGetNSURLResponseLastModifiedDate(response)
                descriptionWithCalendarFormat:@"%a %b %d %Y %H:%M:%S" timeZone:nil locale:nil]);
        }
    }
    opened();
}

NSURLRequest *FrameLoader::initialRequest() const
{
    return activeDocumentLoader()->initialRequest();
}

void FrameLoader::receivedData(NSData *data)
{
    activeDocumentLoader()->receivedData(data);
}

void FrameLoader::setRequest(NSURLRequest *request)
{
    activeDocumentLoader()->setRequest(request);
}

void FrameLoader::handleFallbackContent()
{
    m_frame->handleFallbackContent();
}

bool FrameLoader::isStopping() const
{
    return activeDocumentLoader()->isStopping();
}

void FrameLoader::setResponse(NSURLResponse *response)
{
    activeDocumentLoader()->setResponse(response);
}

void FrameLoader::mainReceivedError(NSError *error, bool isComplete)
{
    activeDocumentLoader()->mainReceivedError(error, isComplete);
}

void FrameLoader::finishedLoading()
{
    // Retain because the stop may release the last reference to it.
    RefPtr<Frame> protect(m_frame);

    RefPtr<DocumentLoader> dl = activeDocumentLoader();
    dl->finishedLoading();
    if (dl->mainDocumentError() || !dl->frameLoader())
        return;
    dl->setPrimaryLoadComplete(true);
    m_client->dispatchDidLoadMainResource(dl.get());
    checkLoadComplete();
}

void FrameLoader::notifyIconChanged()
{
    ASSERT([[WebCoreIconDatabaseBridge sharedInstance] _isEnabled]);
    NSImage *icon = [[WebCoreIconDatabaseBridge sharedInstance]
        iconForPageURL:urlOriginalDataAsString(activeDocumentLoader()->URL().getNSURL())
        withSize:NSMakeSize(16, 16)];
    m_client->dispatchDidReceiveIcon(icon);
}

KURL FrameLoader::URL() const
{
    return activeDocumentLoader()->URL();
}

NSError *FrameLoader::cancelledError(NSURLRequest *request) const
{
    return m_client->cancelledError(request);
}

NSError *FrameLoader::fileDoesNotExistError(NSURLResponse *response) const
{
    return m_client->fileDoesNotExistError(response);    
}

bool FrameLoader::willUseArchive(ResourceLoader* loader, NSURLRequest *request, NSURL *originalURL) const
{
    return m_client->willUseArchive(loader, request, originalURL);
}

bool FrameLoader::isArchiveLoadPending(ResourceLoader* loader) const
{
    return m_client->isArchiveLoadPending(loader);
}

void FrameLoader::handleUnimplementablePolicy(NSError *error)
{
    m_delegateIsHandlingUnimplementablePolicy = true;
    m_client->dispatchUnableToImplementPolicy(error);
    m_delegateIsHandlingUnimplementablePolicy = false;
}

void FrameLoader::cannotShowMIMEType(NSURLResponse *response)
{
    handleUnimplementablePolicy(m_client->cannotShowMIMETypeError(response));
}

NSError *FrameLoader::interruptionForPolicyChangeError(NSURLRequest *request)
{
    return m_client->interruptForPolicyChangeError(request);
}

bool FrameLoader::isHostedByObjectElement() const
{
    Element* owner = m_frame->ownerElement();
    return owner && owner->hasTagName(objectTag);
}

bool FrameLoader::isLoadingMainFrame() const
{
    Page* page = m_frame->page();
    return page && m_frame == page->mainFrame();
}

bool FrameLoader::canShowMIMEType(const String& MIMEType) const
{
    return m_client->canShowMIMEType(MIMEType);
}

bool FrameLoader::representationExistsForURLScheme(const String& URLScheme)
{
    return m_client->representationExistsForURLScheme(URLScheme);
}

String FrameLoader::generatedMIMETypeForURLScheme(const String& URLScheme)
{
    return m_client->generatedMIMETypeForURLScheme(URLScheme);
}

void FrameLoader::checkNavigationPolicy(NSURLRequest *newRequest,
    NavigationPolicyDecisionFunction function, void* argument)
{
    checkNavigationPolicy(newRequest, activeDocumentLoader(), 0, function, argument);
}

void FrameLoader::checkContentPolicy(const String& MIMEType, ContentPolicyDecisionFunction function, void* argument)
{
    m_policyCheck.set(function, argument);
    m_client->dispatchDecidePolicyForMIMEType(&FrameLoader::continueAfterContentPolicy,
        MIMEType, activeDocumentLoader()->request());
}

void FrameLoader::cancelContentPolicyCheck()
{
    m_client->cancelPolicyCheck();
    m_policyCheck.clear();
}

bool FrameLoader::shouldReloadToHandleUnreachableURL(NSURLRequest *request)
{
    NSURL *unreachableURL = [request _webDataRequestUnreachableURL];
    if (unreachableURL == nil)
        return false;

    if (!isBackForwardLoadType(m_policyLoadType))
        return false;

    // We only treat unreachableURLs specially during the delegate callbacks
    // for provisional load errors and navigation policy decisions. The former
    // case handles well-formed URLs that can't be loaded, and the latter
    // case handles malformed URLs and unknown schemes. Loading alternate content
    // at other times behaves like a standard load.
    DocumentLoader* compareDocumentLoader = 0;
    if (m_delegateIsDecidingNavigationPolicy || m_delegateIsHandlingUnimplementablePolicy)
        compareDocumentLoader = m_policyDocumentLoader.get();
    else if (m_delegateIsHandlingProvisionalLoadError)
        compareDocumentLoader = m_provisionalDocumentLoader.get();

    return compareDocumentLoader && [unreachableURL isEqual:[compareDocumentLoader->request() URL]];
}

void FrameLoader::reloadAllowingStaleData(const String& encoding)
{
    if (!m_documentLoader)
        return;

    NSMutableURLRequest *request = [m_documentLoader->request() mutableCopy];
    KURL unreachableURL = m_documentLoader->unreachableURL();
    if (!unreachableURL.isEmpty())
        [request setURL:unreachableURL.getNSURL()];

    [request setCachePolicy:NSURLRequestReturnCacheDataElseLoad];

    RefPtr<DocumentLoader> loader = m_client->createDocumentLoader(request);
    setPolicyDocumentLoader(loader.get());

    [request release];

    loader->setOverrideEncoding(encoding);

    load(loader.get(), FrameLoadTypeReloadAllowingStaleData, 0);
}

void FrameLoader::reload()
{
    if (!m_documentLoader)
        return;

    NSMutableURLRequest *initialRequest = m_documentLoader->request();
    
    // If a window is created by javascript, its main frame can have an empty but non-nil URL.
    // Reloading in this case will lose the current contents (see 4151001).
    if ([[[initialRequest URL] absoluteString] length] == 0)
        return;

    // Replace error-page URL with the URL we were trying to reach.
    NSURL *unreachableURL = [initialRequest _webDataRequestUnreachableURL];
    if (unreachableURL != nil)
        initialRequest = [NSURLRequest requestWithURL:unreachableURL];
    
    RefPtr<DocumentLoader> loader = m_client->createDocumentLoader(initialRequest);
    setPolicyDocumentLoader(loader.get());

    NSMutableURLRequest *request = loader->request();

    [request setCachePolicy:NSURLRequestReloadIgnoringCacheData];

    // If we're about to re-post, set up action so the application can warn the user.
    if ([[request HTTPMethod] isEqualToString:@"POST"])
        loader->setTriggeringAction(NavigationAction([request URL], NavigationTypeFormResubmitted));

    loader->setOverrideEncoding(m_documentLoader->overrideEncoding());
    
    load(loader.get(), FrameLoadTypeReload, 0);
}

void FrameLoader::didReceiveServerRedirectForProvisionalLoadForFrame()
{
    m_client->dispatchDidReceiveServerRedirectForProvisionalLoad();
}

void FrameLoader::finishedLoadingDocument(DocumentLoader* loader)
{
    m_client->finishedLoading(loader);
}

void FrameLoader::committedLoad(DocumentLoader* loader, NSData *data)
{
    m_client->committedLoad(loader, data);
}

bool FrameLoader::isReplacing() const
{
    return m_loadType == FrameLoadTypeReplace;
}

void FrameLoader::setReplacing()
{
    m_loadType = FrameLoadTypeReplace;
}

void FrameLoader::revertToProvisional(DocumentLoader* loader)
{
    m_client->revertToProvisionalState(loader);
}

void FrameLoader::setMainDocumentError(DocumentLoader* loader, NSError *error)
{
    m_client->setMainDocumentError(loader, error);
}

void FrameLoader::mainReceivedCompleteError(DocumentLoader* loader, NSError *error)
{
    loader->setPrimaryLoadComplete(true);
    m_client->dispatchDidLoadMainResource(activeDocumentLoader());
    checkLoadComplete();
}

bool FrameLoader::subframeIsLoading() const
{
    // It's most likely that the last added frame is the last to load so we walk backwards.
    for (Frame* child = m_frame->tree()->lastChild(); child; child = child->tree()->previousSibling()) {
        FrameLoader* childLoader = child->loader();
        DocumentLoader* documentLoader = childLoader->documentLoader();
        if (documentLoader && documentLoader->isLoadingInAPISense())
            return true;
        documentLoader = childLoader->provisionalDocumentLoader();
        if (documentLoader && documentLoader->isLoadingInAPISense())
            return true;
    }
    return false;
}

void FrameLoader::willChangeTitle(DocumentLoader* loader)
{
    m_client->willChangeTitle(loader);
}

void FrameLoader::didChangeTitle(DocumentLoader* loader)
{
    m_client->didChangeTitle(loader);

    // The title doesn't get communicated to the WebView until we are committed.
    if (loader->isCommitted())
        if (NSURL *URLForHistory = canonicalURL(loader->URLForHistory().getNSURL())) {
            // Must update the entries in the back-forward list too.
            // This must go through the WebFrame because it has the right notion of the current b/f item.
            m_client->setTitle(loader->title(), URLForHistory);
            m_client->setMainFrameDocumentReady(true); // update observers with new DOMDocument
            m_client->dispatchDidReceiveTitle(loader->title());
        }
}

FrameLoadType FrameLoader::loadType() const
{
    return m_loadType;
}

void FrameLoader::stopPolicyCheck()
{
    m_client->cancelPolicyCheck();
    PolicyCheck check = m_policyCheck;
    m_policyCheck.clear();
    check.clearRequest();
    check.call();
}

void FrameLoader::checkNewWindowPolicy(const NavigationAction& action, NSURLRequest *request,
    PassRefPtr<FormState> formState, const String& frameName)
{
    m_policyCheck.set(request, formState, frameName,
        callContinueLoadAfterNewWindowPolicy, this);
    m_client->dispatchDecidePolicyForNewWindowAction(&FrameLoader::continueAfterNewWindowPolicy,
        action, request, frameName);
}

void FrameLoader::continueAfterNewWindowPolicy(PolicyAction policy)
{
    PolicyCheck check = m_policyCheck;
    m_policyCheck.clear();

    switch (policy) {
        case PolicyIgnore:
            check.clearRequest();
            break;
        case PolicyDownload:
            m_client->startDownload(check.request());
            check.clearRequest();
            break;
        case PolicyUse:
            break;
    }

    check.call();
}

void FrameLoader::checkNavigationPolicy(NSURLRequest *request, DocumentLoader* loader,
    PassRefPtr<FormState> formState, NavigationPolicyDecisionFunction function, void* argument)
{
    NavigationAction action = loader->triggeringAction();
    if (action.isEmpty()) {
        action = NavigationAction([request URL], NavigationTypeOther);
        loader->setTriggeringAction(action);
    }
        
    // Don't ask more than once for the same request or if we are loading an empty URL.
    // This avoids confusion on the part of the client.
    if ([request isEqual:loader->lastCheckedRequest()] || urlIsEmpty([request URL])) {
        function(argument, request, 0);
        return;
    }
    
    // We are always willing to show alternate content for unreachable URLs;
    // treat it like a reload so it maintains the right state for b/f list.
    if ([request _webDataRequestUnreachableURL] != nil) {
        if (isBackForwardLoadType(m_policyLoadType))
            m_policyLoadType = FrameLoadTypeReload;
        function(argument, request, 0);
        return;
    }
    
    loader->setLastCheckedRequest(request);

    m_policyCheck.set(request, formState, function, argument);

    m_delegateIsDecidingNavigationPolicy = true;
    m_client->dispatchDecidePolicyForNavigationAction(&FrameLoader::continueAfterNavigationPolicy,
        action, request);
    m_delegateIsDecidingNavigationPolicy = false;
}

void FrameLoader::continueAfterNavigationPolicy(PolicyAction policy)
{
    PolicyCheck check = m_policyCheck;
    m_policyCheck.clear();

    switch (policy) {
        case PolicyIgnore:
            check.clearRequest();
            break;
        case PolicyDownload:
            m_client->startDownload(check.request());
            check.clearRequest();
            break;
        case PolicyUse:
            if (!m_client->canHandleRequest(check.request())) {
                handleUnimplementablePolicy(m_client->cannotShowURLError(check.request()));
                check.clearRequest();
            }
            break;
    }

    check.call();
}

void FrameLoader::continueAfterContentPolicy(PolicyAction policy)
{
    PolicyCheck check = m_policyCheck;
    m_policyCheck.clear();
    check.call(policy);
}

void FrameLoader::continueAfterWillSubmitForm(PolicyAction)
{
    startLoading();
}

void FrameLoader::callContinueLoadAfterNavigationPolicy(void* argument,
    NSURLRequest *request, PassRefPtr<FormState> formState)
{
    FrameLoader* loader = static_cast<FrameLoader*>(argument);
    loader->continueLoadAfterNavigationPolicy(request, formState);
}

void FrameLoader::continueLoadAfterNavigationPolicy(NSURLRequest *request,
    PassRefPtr<FormState> formState)
{
    // If we loaded an alternate page to replace an unreachableURL, we'll get in here with a
    // nil policyDataSource because loading the alternate page will have passed
    // through this method already, nested; otherwise, policyDataSource should still be set.
    ASSERT(m_policyDocumentLoader || !m_provisionalDocumentLoader->unreachableURL().isEmpty());

    BOOL isTargetItem = m_client->provisionalItemIsTarget();

    // Two reasons we can't continue:
    //    1) Navigation policy delegate said we can't so request is nil. A primary case of this 
    //       is the user responding Cancel to the form repost nag sheet.
    //    2) User responded Cancel to an alert popped up by the before unload event handler.
    // The "before unload" event handler runs only for the main frame.
    bool canContinue = request && (!isLoadingMainFrame() || Mac(m_frame)->shouldClose());

    if (!canContinue) {
        // If we were waiting for a quick redirect, but the policy delegate decided to ignore it, then we 
        // need to report that the client redirect was cancelled.
        if (m_quickRedirectComing)
            clientRedirectCancelledOrFinished(false);

        setPolicyDocumentLoader(0);

        // If the navigation request came from the back/forward menu, and we punt on it, we have the 
        // problem that we have optimistically moved the b/f cursor already, so move it back.  For sanity, 
        // we only do this when punting a navigation for the target frame or top-level frame.  
        if ((isTargetItem || isLoadingMainFrame()) && isBackForwardLoadType(m_policyLoadType))
            m_client->resetBackForwardList();

        return;
    }

    FrameLoadType type = m_policyLoadType;
    stopLoading();
    setProvisionalDocumentLoader(m_policyDocumentLoader.get());
    m_loadType = type;
    setState(FrameStateProvisional);

    setPolicyDocumentLoader(0);

    if (isBackForwardLoadType(type) && m_client->loadProvisionalItemFromPageCache())
        return;

    if (formState)
        m_client->dispatchWillSubmitForm(&FrameLoader::continueAfterWillSubmitForm, formState);
    else
        continueAfterWillSubmitForm();
}

void FrameLoader::didFirstLayout()
{
    if (isBackForwardLoadType(m_loadType) && m_client->hasBackForwardList())
        m_client->restoreScrollPositionAndViewState();

    m_firstLayoutDone = true;
    m_client->dispatchDidFirstLayout();
}

void FrameLoader::frameLoadCompleted()
{
    m_client->frameLoadCompleted();

    // After a canceled provisional load, firstLayoutDone is false.
    // Reset it to true if we're displaying a page.
    if (m_documentLoader)
        m_firstLayoutDone = true;
}

bool FrameLoader::firstLayoutDone() const
{
    return m_firstLayoutDone;
}

bool FrameLoader::isQuickRedirectComing() const
{
    return m_quickRedirectComing;
}

void FrameLoader::closeDocument()
{
    m_client->willCloseDocument();
    m_frame->closeURL();
}

void FrameLoader::transitionToCommitted(NSDictionary *pageCache)
{
    ASSERT(m_client->hasWebView());
    ASSERT(m_state == FrameStateProvisional);

    if (m_state != FrameStateProvisional)
        return;

    m_client->setCopiesOnScroll();
    m_client->updateHistoryForCommit();

    // The call to closeURL invokes the unload event handler, which can execute arbitrary
    // JavaScript. If the script initiates a new load, we need to abandon the current load,
    // or the two will stomp each other.
    DocumentLoader* pdl = m_provisionalDocumentLoader.get();
    closeDocument();
    if (pdl != m_provisionalDocumentLoader)
        return;

    commitProvisionalLoad();

    // Handle adding the URL to the back/forward list.
    DocumentLoader* dl = m_documentLoader.get();
    String ptitle = dl->title();

    switch (m_loadType) {
    case FrameLoadTypeForward:
    case FrameLoadTypeBack:
    case FrameLoadTypeIndexedBackForward:
        if (m_client->hasBackForwardList()) {
            m_client->updateHistoryForBackForwardNavigation();

            // Create a document view for this document, or used the cached view.
            if (pageCache)
                m_client->setDocumentViewFromPageCache(pageCache);
            else
                m_client->makeDocumentView();
        }
        break;

    case FrameLoadTypeReload:
    case FrameLoadTypeSame:
    case FrameLoadTypeReplace:
        m_client->updateHistoryForReload();
        m_client->makeDocumentView();
        break;

    // FIXME - just get rid of this case, and merge FrameLoadTypeReloadAllowingStaleData with the above case
    case FrameLoadTypeReloadAllowingStaleData:
        m_client->makeDocumentView();
        break;

    case FrameLoadTypeStandard:
        m_client->updateHistoryForStandardLoad();
        m_client->makeDocumentView();
        break;

    case FrameLoadTypeInternal:
        m_client->updateHistoryForInternalLoad();
        m_client->makeDocumentView();
        break;

    // FIXME Remove this check when dummy ds is removed (whatever "dummy ds" is).
    // An exception should be thrown if we're in the FrameLoadTypeUninitialized state.
    default:
        ASSERT_NOT_REACHED();
    }

    // Tell the client we've committed this URL.
    ASSERT(m_client->hasFrameView());
    m_client->dispatchDidCommitLoad();
    
    // If we have a title let the WebView know about it.
    if (!ptitle.isNull())
        m_client->dispatchDidReceiveTitle(ptitle);
}

void FrameLoader::checkLoadCompleteForThisFrame()
{
    ASSERT(m_client->hasWebView());

    switch (m_state) {
        case FrameStateProvisional: {
            if (m_delegateIsHandlingProvisionalLoadError)
                return;

            RefPtr<DocumentLoader> pdl = m_provisionalDocumentLoader;

            // If we've received any errors we may be stuck in the provisional state and actually complete.
            NSError *error = pdl->mainDocumentError();
            if (!error)
                return;

            // Check all children first.
            LoadErrorResetToken *resetToken = m_client->tokenForLoadErrorReset();
            bool shouldReset = true;
            if (!pdl->isLoadingInAPISense()) {
                m_delegateIsHandlingProvisionalLoadError = true;
                m_client->dispatchDidFailProvisionalLoad(error);
                m_delegateIsHandlingProvisionalLoadError = false;

                // FIXME: can stopping loading here possibly have any effect, if isLoading is false,
                // which it must be to be in this branch of the if? And is it OK to just do a full-on
                // stopLoading instead of stopLoadingSubframes?
                stopLoadingSubframes();
                pdl->stopLoading();

                // Finish resetting the load state, but only if another load hasn't been started by the
                // delegate callback.
                if (pdl == m_provisionalDocumentLoader)
                    clearProvisionalLoad();
                else if (m_documentLoader) {
                    KURL unreachableURL = m_documentLoader->unreachableURL();
                    if (!unreachableURL.isEmpty() && unreachableURL == [pdl->request() URL])
                        shouldReset = false;
                }
            }
            if (shouldReset)
                m_client->resetAfterLoadError(resetToken);
            else
                m_client->doNotResetAfterLoadError(resetToken);
            return;
        }
        
        case FrameStateCommittedPage: {
            DocumentLoader* dl = m_documentLoader.get();            
            if (dl->isLoadingInAPISense())
                return;

            markLoadComplete();

            // FIXME: Is this subsequent work important if we already navigated away?
            // Maybe there are bugs because of that, or extra work we can skip because
            // the new page is ready.

            m_client->forceLayoutForNonHTML();
             
            // If the user had a scroll point, scroll to it, overriding the anchor point if any.
            if ((isBackForwardLoadType(m_loadType) || m_loadType == FrameLoadTypeReload)
                    && m_client->hasBackForwardList())
                m_client->restoreScrollPositionAndViewState();

            NSError *error = dl->mainDocumentError();
            if (error)
                m_client->dispatchDidFailLoad(error);
            else
                m_client->dispatchDidFinishLoad();

            m_client->progressCompleted();
            return;
        }
        
        case FrameStateComplete:
            // Even if already complete, we might have set a previous item on a frame that
            // didn't do any data loading on the past transaction. Make sure to clear these out.
            m_client->frameLoadCompleted();
            return;
    }

    ASSERT_NOT_REACHED();
}

void FrameLoader::callContinueLoadAfterNewWindowPolicy(void* argument,
    NSURLRequest *request, PassRefPtr<FormState> formState, const String& frameName)
{
    FrameLoader* loader = static_cast<FrameLoader*>(argument);
    loader->continueLoadAfterNewWindowPolicy(request, formState, frameName);
}

void FrameLoader::continueLoadAfterNewWindowPolicy(NSURLRequest *request,
    PassRefPtr<FormState> formState, const String& frameName)
{
    if (!request)
        return;

    RefPtr<Frame> frame = m_frame;
    RefPtr<Frame> mainFrame = m_client->dispatchCreatePage(nil);
    if (!mainFrame)
        return;

    mainFrame->tree()->setName(frameName);
    mainFrame->loader()->client()->dispatchShow();
    mainFrame->setOpener(frame.get());
    mainFrame->loader()->load(request, NavigationAction(), FrameLoadTypeStandard, formState);
}

void FrameLoader::sendRemainingDelegateMessages(id identifier, NSURLResponse *response, unsigned length, NSError *error)
{    
    if (response)
        m_client->dispatchDidReceiveResponse(m_documentLoader.get(), identifier, response);
    
    if (length > 0)
        m_client->dispatchDidReceiveContentLength(m_documentLoader.get(), identifier, length);
    
    if (!error)
        m_client->dispatchDidFinishLoading(m_documentLoader.get(), identifier);
    else
        m_client->dispatchDidFailLoading(m_documentLoader.get(), identifier, error);
}

NSURLRequest *FrameLoader::requestFromDelegate(NSURLRequest *request, id& identifier, NSError *& error)
{
    ASSERT(request != nil);

    identifier = m_client->dispatchIdentifierForInitialRequest(m_documentLoader.get(), request); 
    NSURLRequest *newRequest = m_client->dispatchWillSendRequest(m_documentLoader.get(), identifier, request, nil);

    if (newRequest == nil)
        error = m_client->cancelledError(request);
    else
        error = nil;

    return newRequest;
}

void FrameLoader::loadedResourceFromMemoryCache(NSURLRequest *request, NSURLResponse *response, int length)
{
    if (m_client->dispatchDidLoadResourceFromMemoryCache(m_documentLoader.get(), request, response, length))
        return;

    id identifier;
    NSError *error;
    requestFromDelegate(request, identifier, error);
    sendRemainingDelegateMessages(identifier, response, length, error);
}

void FrameLoader::post(const KURL& URL, const String& referrer, const String& frameName, const FormData& formData, 
    const String& contentType, Event* event, Element* form, const HashMap<String, String>& formValues)
{
    // When posting, use the NSURLRequestReloadIgnoringCacheData load flag.
    // This prevents a potential bug which may cause a page with a form that uses itself
    // as an action to be returned from the cache without submitting.

    // FIXME: Where's the code that implements what the comment above says?

    NSMutableURLRequest *request = [[NSMutableURLRequest alloc] initWithURL:URL.getNSURL()];
    addExtraFieldsToRequest(request, true, true);

    setHTTPReferrer(request, referrer);
    [request setHTTPMethod:@"POST"];
    setHTTPBody(request, formData);
    [request setValue:contentType forHTTPHeaderField:@"Content-Type"];

    NavigationAction action(URL, FrameLoadTypeStandard, true, event);

    RefPtr<FormState> formState;
    if (form && !formValues.isEmpty())
        formState = FormState::create(form, formValues, m_frame);

    if (!frameName.isEmpty()) {
        if (Frame* targetFrame = m_frame->tree()->find(frameName))
            targetFrame->loader()->load(request, action, FrameLoadTypeStandard, formState.release());
        else
            checkNewWindowPolicy(action, request, formState.release(), frameName);
    } else
        load(request, action, FrameLoadTypeStandard, formState.release());

    [request release];
}

void FrameLoader::detachChildren()
{
    // FIXME: Is it really necessary to do this in reverse order?
    Frame* previous;
    for (Frame* child = m_frame->tree()->lastChild(); child; child = previous) {
        previous = child->tree()->previousSibling();
        child->loader()->detachFromParent();
    }
}

void FrameLoader::detachFromParent()
{
    RefPtr<Frame> protect(m_frame);

    closeDocument();
    stopLoading();
    m_client->detachedFromParent1();
    detachChildren();
    m_client->detachedFromParent2();
    setDocumentLoader(0);
    m_client->detachedFromParent3();
    if (Frame* parent = m_frame->tree()->parent())
        parent->tree()->removeChild(m_frame);
    [Mac(m_frame)->bridge() close];
    m_client->detachedFromParent4();
}

void FrameLoader::addExtraFieldsToRequest(NSMutableURLRequest *request, bool mainResource, bool alwaysFromRequest)
{
    applyUserAgent(request);
    
    if (m_loadType == FrameLoadTypeReload)
        [request setValue:@"max-age=0" forHTTPHeaderField:@"Cache-Control"];
    
    // Don't set the cookie policy URL if it's already been set.
    if (![request mainDocumentURL]) {
        if (mainResource && (isLoadingMainFrame() || alwaysFromRequest))
            [request setMainDocumentURL:[request URL]];
        else
            [request setMainDocumentURL:m_frame->page()->mainFrame()->url().getNSURL()];
    }
    
    if (mainResource)
        [request setValue:@"text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5" forHTTPHeaderField:@"Accept"];
}

// Called every time a resource is completely loaded, or an error is received.
void FrameLoader::checkLoadComplete()
{
    ASSERT(m_client->hasWebView());

    for (RefPtr<Frame> frame = m_frame; frame; frame = frame->tree()->parent())
        frame->loader()->checkLoadCompleteForThisFrame();
}

int FrameLoader::numPendingOrLoadingRequests(bool recurse) const
{
    int count = 0;
    const Frame* frame = m_frame;

    count += numRequests(frame->document());

    if (recurse)
        while ((frame = frame->tree()->traverseNext(frame)))
            count += numRequests(frame->document());

    return count;
}

bool FrameLoader::isReloading() const
{
    return [documentLoader()->request() cachePolicy] == NSURLRequestReloadIgnoringCacheData;
}

String FrameLoader::referrer() const
{
    return [documentLoader()->request() valueForHTTPHeaderField:@"Referer"];
}

void FrameLoader::loadEmptyDocumentSynchronously()
{
    NSURL *url = [[NSURL alloc] initWithString:@""];
    NSURLRequest *request = [[NSURLRequest alloc] initWithURL:url];
    load(request);
    [request release];
    [url release];
}

void FrameLoader::loadResourceSynchronously(const ResourceRequest& request, Vector<char>& data, ResourceResponse& r)
{
#if 0
    NSDictionary *headerDict = nil;
    const HTTPHeaderMap& requestHeaders = request.httpHeaderFields();

    if (!requestHeaders.isEmpty())
        headerDict = [NSDictionary _webcore_dictionaryWithHeaderMap:requestHeaders];
    
    FormData postData;
    if (!request.httpBody().elements().isEmpty())
        postData = request.httpBody();
    Vector<char> results([resultData length]);

    memcpy(results.data(), [resultData bytes], [resultData length]);
#endif

    NSURL *URL = request.url().getNSURL();

    // Since this is a subresource, we can load any URL (we ignore the return value).
    // But we still want to know whether we should hide the referrer or not, so we call the canLoadURL method.
    String referrer = m_frame->referrer();
    bool hideReferrer;
    canLoad(URL, referrer, hideReferrer);
    if (hideReferrer)
        referrer = String();
    
    NSMutableURLRequest *initialRequest = [[NSMutableURLRequest alloc] initWithURL:URL];
    [initialRequest setTimeoutInterval:10];
    
    [initialRequest setHTTPMethod:request.httpMethod()];
    
    if (!request.httpBody().isEmpty())        
        setHTTPBody(initialRequest, request.httpBody());
    
    HTTPHeaderMap::const_iterator end = request.httpHeaderFields().end();
    for (HTTPHeaderMap::const_iterator it = request.httpHeaderFields().begin(); it != end; ++it)
        [initialRequest addValue:it->second forHTTPHeaderField:it->first];
    
    if (isConditionalRequest(initialRequest))
        [initialRequest setCachePolicy:NSURLRequestReloadIgnoringCacheData];
    else
        [initialRequest setCachePolicy:[documentLoader()->request() cachePolicy]];
    
    if (!referrer.isEmpty())
        setHTTPReferrer(initialRequest, referrer);
    
    [initialRequest setMainDocumentURL:[m_frame->page()->mainFrame()->loader()->documentLoader()->request() URL]];
    applyUserAgent(initialRequest);
    
    NSError *error = nil;
    id identifier = nil;    
    NSURLRequest *newRequest = requestFromDelegate(initialRequest, identifier, error);
    
    NSURLResponse *response = nil;
    NSData *result = nil;
    if (error == nil) {
        ASSERT(newRequest != nil);
        result = [NSURLConnection sendSynchronousRequest:newRequest returningResponse:&response error:&error];
    }
    
    [initialRequest release];

    if (error == nil)
        getResourceResponse(r, response);
    else {
        r = ResourceResponse(URL, String(), 0, String(), String());
        if ([error domain] == NSURLErrorDomain)
            r.setHTTPStatusCode([error code]);
        else
            r.setHTTPStatusCode(404);
    }
    
    sendRemainingDelegateMessages(identifier, response, [result length], error);
    
    data.resize([result length]);
    memcpy(data.data(), [result bytes], [result length]);
}

void FrameLoader::setClient(FrameLoaderClient* client)
{
    ASSERT(client);
    ASSERT(!m_client);
    m_client = client;
}

FrameLoaderClient* FrameLoader::client() const
{
    return m_client;
}

PolicyCheck::PolicyCheck()
    : m_navigationFunction(0)
    , m_newWindowFunction(0)
    , m_contentFunction(0)
{
}

void PolicyCheck::clear()
{
    clearRequest();
    m_navigationFunction = 0;
    m_newWindowFunction = 0;
    m_contentFunction = 0;
}

void PolicyCheck::set(NSURLRequest *request, PassRefPtr<FormState> formState,
    NavigationPolicyDecisionFunction function, void* argument)
{
    m_request = request;
    m_formState = formState;
    m_frameName = String();

    m_navigationFunction = function;
    m_newWindowFunction = 0;
    m_contentFunction = 0;
    m_argument = argument;
}

void PolicyCheck::set(NSURLRequest *request, PassRefPtr<FormState> formState,
    const String& frameName, NewWindowPolicyDecisionFunction function, void* argument)
{
    m_request = request;
    m_formState = formState;
    m_frameName = frameName;

    m_navigationFunction = 0;
    m_newWindowFunction = function;
    m_contentFunction = 0;
    m_argument = argument;
}

void PolicyCheck::set(ContentPolicyDecisionFunction function, void* argument)
{
    m_request = nil;
    m_formState = 0;
    m_frameName = String();

    m_navigationFunction = 0;
    m_newWindowFunction = 0;
    m_contentFunction = function;
    m_argument = argument;
}

void PolicyCheck::call()
{
    if (m_navigationFunction)
        m_navigationFunction(m_argument, m_request.get(), m_formState.get());
    if (m_newWindowFunction)
        m_newWindowFunction(m_argument, m_request.get(), m_formState.get(), m_frameName);
    ASSERT(!m_contentFunction);
}

void PolicyCheck::call(PolicyAction action)
{
    ASSERT(!m_navigationFunction);
    ASSERT(!m_newWindowFunction);
    ASSERT(m_contentFunction);
    m_contentFunction(m_argument, action);
}

void PolicyCheck::clearRequest()
{
    m_request = nil;
    m_formState = 0;
    m_frameName = String();
}

FrameLoaderClient::~FrameLoaderClient()
{
}

}
