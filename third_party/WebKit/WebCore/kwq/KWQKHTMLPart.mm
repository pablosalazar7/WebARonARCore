/*
 * Copyright (C) 2001, 2002 Apple Computer, Inc.  All rights reserved.
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

#import "KWQKHTMLPart.h"

#import "htmltokenizer.h"
#import "html_documentimpl.h"
#import "render_root.h"
#import "render_frames.h"
#import "render_text.h"
#import "khtmlpart_p.h"
#import "khtmlview.h"
#import "kjs_window.h"

#import "WebCoreBridge.h"
#import "WebCoreViewFactory.h"

#import "KWQDummyView.h"
#import "KWQKJobClasses.h"
#import "KWQLogging.h"

#import "xml/dom2_eventsimpl.h"

#import <JavaScriptCore/property_map.h>

#undef _KWQ_TIMING

using DOM::DocumentImpl;
using DOM::DOMString;
using DOM::EventImpl;
using DOM::Node;

using khtml::Cache;
using khtml::ChildFrame;
using khtml::Decoder;
using khtml::MouseDoubleClickEvent;
using khtml::MouseMoveEvent;
using khtml::MousePressEvent;
using khtml::MouseReleaseEvent;
using khtml::RenderObject;
using khtml::RenderPart;
using khtml::RenderStyle;
using khtml::RenderText;
using khtml::RenderWidget;

using KIO::Job;

using KJS::SavedProperties;
using KJS::Window;

using KParts::ReadOnlyPart;
using KParts::URLArgs;

NSEvent *KWQKHTMLPart::_currentEvent = nil;
NSResponder *KWQKHTMLPart::_firstResponderAtMouseDownTime = nil;

void KHTMLPart::completed()
{
    KWQ(this)->_completed.call();
}

void KHTMLPart::completed(bool arg)
{
    KWQ(this)->_completed.call(arg);
}

void KHTMLPart::nodeActivated(const Node &)
{
}

bool KHTMLPart::openURL(const KURL &URL)
{
    ASSERT_NOT_REACHED();
    return true;
}

void KHTMLPart::onURL(const QString &)
{
}

void KHTMLPart::setStatusBarText(const QString &status)
{
    KWQ(this)->setStatusBarText(status);
}

void KHTMLPart::started(Job *j)
{
    KWQ(this)->_started.call(j);
}

static void redirectionTimerMonitor(void *context)
{
    KWQKHTMLPart *kwq = static_cast<KWQKHTMLPart *>(context);
    kwq->redirectionTimerStartedOrStopped();
}

KWQKHTMLPart::KWQKHTMLPart()
    : _started(this, SIGNAL(started(KIO::Job *)))
    , _completed(this, SIGNAL(completed()))
    , _completedWithBool(this, SIGNAL(completed(bool)))
    , _ownsView(false)
    , _mouseDownView(nil)
    , _sendingEventToSubview(false)
    , _formSubmittedFlag(false)
{
    // Must init the cache before connecting to any signals
    Cache::init();

    // The widget is made outside this class in our case.
    KHTMLPart::init( 0, DefaultGUI );

    mutableInstances().prepend(this);
    d->m_redirectionTimer.setMonitor(redirectionTimerMonitor, this);
}

KWQKHTMLPart::~KWQKHTMLPart()
{
    mutableInstances().remove(this);
    if (_ownsView) {
        delete d->m_view;
    }
}

WebCoreBridge *KWQKHTMLPart::bridgeForFrameName(const QString &frameName)
{
    WebCoreBridge *frame;
    if (frameName.isEmpty()) {
        // If we're the only frame in a frameset then pop the frame.
        KHTMLPart *parent = parentPart();
        frame = parent ? KWQ(parent)->_bridge : nil;
        if ([[frame childFrames] count] != 1) {
            frame = _bridge;
        }
    } else {
        frame = [_bridge findOrCreateFramedNamed:frameName.getNSString()];
    }
    
    return frame;
}

QString KWQKHTMLPart::generateFrameName()
{
    return QString::fromNSString([_bridge generateFrameName]);
}

bool KWQKHTMLPart::openURL(const KURL &url)
{
    // FIXME: The lack of args here to get the reload flag from
    // indicates a problem in how we use KHTMLPart::processObjectRequest,
    // where we are opening the URL before the args are set up.
    [_bridge loadURL:url.url().getNSString() reload:NO triggeringEvent:nil isFormSubmission:NO];
    return true;
}

void KWQKHTMLPart::openURLRequest(const KURL &url, const URLArgs &args)
{
    [bridgeForFrameName(args.frameName) loadURL:url.url().getNSString() reload:args.reload triggeringEvent:nil isFormSubmission:NO];
}

void KWQKHTMLPart::submitForm(const KURL &url, const URLArgs &args)
{
    // we do not want to submit more than one form, nor do we want to submit a single form more than once 
    // this flag prevents these from happening
    // note that the flag is reset in setView()
    // since this part may get reused if it is pulled from the b/f cache
    if (_formSubmittedFlag) {
        return;
    }
    _formSubmittedFlag = true;
    
    if (!args.doPost()) {
        [bridgeForFrameName(args.frameName) loadURL:url.url().getNSString() reload:args.reload
            triggeringEvent:_currentEvent isFormSubmission:YES];
    } else {
        QString contentType = args.contentType();
        ASSERT(contentType.startsWith("Content-Type: "));
        [bridgeForFrameName(args.frameName) postWithURL:url.url().getNSString()
            data:[NSData dataWithBytes:args.postData.data() length:args.postData.size()]
            contentType:contentType.mid(14).getNSString() triggeringEvent:_currentEvent];
    }
}

void KWQKHTMLPart::slotData(NSString *encoding, bool forceEncoding, const char *bytes, int length, bool complete)
{
    if (!d->m_workingURL.isEmpty()) {
        receivedFirstData();
    }
    
    ASSERT(d->m_doc);
    ASSERT(d->m_doc->parsing());
    
    if (encoding) {
        setEncoding(QString::fromNSString(encoding), forceEncoding);
    } else {
        setEncoding(QString::null, false);
    }
    
    write(bytes, length);
}

void KWQKHTMLPart::urlSelected(const KURL &url, int button, int state, const URLArgs &args)
{
    [bridgeForFrameName(args.frameName) loadURL:url.url().getNSString() reload:args.reload
        triggeringEvent:_currentEvent isFormSubmission:NO];
}

class KWQPluginPart : public ReadOnlyPart
{
    virtual bool openURL(const KURL &) { return true; }
    virtual bool closeURL() { return true; }
};

ReadOnlyPart *KWQKHTMLPart::createPart(const ChildFrame &child, const KURL &url, const QString &mimeType)
{
    if (child.m_type == ChildFrame::Object) {
        NSMutableArray *attributesArray = [NSMutableArray arrayWithCapacity:child.m_params.count()];
        for (uint i = 0; i < child.m_params.count(); i++) {
            [attributesArray addObject:child.m_params[i].getNSString()];
        }
        
        KWQPluginPart *newPart = new KWQPluginPart;
        newPart->setWidget(new QWidget([_bridge viewForPluginWithURL:url.url().getNSString()
                                                          attributes:attributesArray
                                                             baseURL:d->m_doc->baseURL().getNSString()
                                                            MIMEType:child.m_args.serviceType.getNSString()]));
        return newPart;
    } else {
        LOG(Frames, "name %s", child.m_name.ascii());
        HTMLIFrameElementImpl *o = static_cast<HTMLIFrameElementImpl *>(child.m_frame->element());
        WebCoreBridge *childBridge = [_bridge createChildFrameNamed:child.m_name.getNSString()
                                                            withURL:url.url().getNSString()
                                                         renderPart:child.m_frame
                                                    allowsScrolling:o->scrollingMode() != QScrollView::AlwaysOff
                                                        marginWidth:o->getMarginWidth()
                                                       marginHeight:o->getMarginHeight()];
        return [childBridge part];
    }
}
    
void KWQKHTMLPart::setView(KHTMLView *view, bool weOwnIt)
{
    if (_ownsView) {
        if (!(d->m_doc && d->m_doc->inPageCache()))
            delete d->m_view;
    }
    d->m_view = view;
    setWidget(view);
    _ownsView = weOwnIt;
    
    // only one form submission is allowed per view of a part
    // since this part may be getting reused as a result of being
    // pulled from the back/forward cache, reset this flag
    _formSubmittedFlag = false;
}

KHTMLView *KWQKHTMLPart::view() const
{
    return d->m_view;
}

void KWQKHTMLPart::setTitle(const DOMString &title)
{
    [_bridge setTitle:title.string().getNSString()];
}

void KWQKHTMLPart::setStatusBarText(const QString &status)
{
    [_bridge setStatusText:status.getNSString()];
}

void KWQKHTMLPart::scheduleClose()
{
    [[_bridge window] performSelector:@selector(close) withObject:nil afterDelay:0.0];
}

void KWQKHTMLPart::unfocusWindow()
{
    [_bridge unfocusWindow];
}

void KWQKHTMLPart::jumpToSelection()
{
    // Assumes that selection will only ever be text nodes. This is currently
    // true, but will it always be so?
    if (!d->m_selectionStart.isNull()) {
        RenderText *rt = dynamic_cast<RenderText *>(d->m_selectionStart.handle()->renderer());
        if (rt) {
            int x = 0, y = 0;
            rt->posOfChar(d->m_startOffset, x, y);
            // The -50 offset is copied from KHTMLPart::findTextNext, which sets the contents position
            // after finding a matched text string.
            d->m_view->setContentsPos(x - 50, y - 50);
        }
    }
}

void KWQKHTMLPart::redirectionTimerStartedOrStopped()
{
    if (d->m_redirectionTimer.isActive()) {
        [_bridge reportClientRedirectToURL:d->m_redirectURL.getNSString()
                                     delay:d->m_delayRedirect
                                  fireDate:[d->m_redirectionTimer.getNSTimer() fireDate]
                               lockHistory:d->m_redirectLockHistory];
    } else {
        [_bridge reportClientRedirectCancelled];
    }
}

void KWQKHTMLPart::paint(QPainter *p, const QRect &rect)
{
#ifndef NDEBUG
    p->fillRect(rect.x(), rect.y(), rect.width(), rect.height(), QColor(0xFF, 0, 0));
#endif

    if (renderer()) {
        renderer()->layer()->paint(p, rect.x(), rect.y(), rect.width(), rect.height());
    } else {
        ERROR("called KWQKHTMLPart::paint with nil renderer");
    }
}

RenderObject *KWQKHTMLPart::renderer()
{
    DocumentImpl *doc = xmlDocImpl();
    return doc ? doc->renderer() : 0;
}

QString KWQKHTMLPart::userAgent() const
{
    NSString *us = [_bridge userAgentForURL:m_url.url().getNSString()];
         
    if (us)
        return QString::fromNSString(us);
    return QString();
}

NSView *KWQKHTMLPart::nextKeyViewInFrame(NodeImpl *node, KWQSelectionDirection direction)
{
    DocumentImpl *doc = xmlDocImpl();
    if (!doc) {
        return nil;
    }
    for (;;) {
        node = direction == KWQSelectingNext
            ? doc->nextFocusNode(node) : doc->previousFocusNode(node);
        if (!node) {
            return nil;
        }
        RenderWidget *renderWidget = dynamic_cast<RenderWidget *>(node->renderer());
        if (renderWidget) {
            QWidget *widget = renderWidget->widget();
            KHTMLView *childFrameWidget = dynamic_cast<KHTMLView *>(widget);
            if (childFrameWidget) {
                NSView *view = KWQ(childFrameWidget->part())->nextKeyViewInFrame(0, direction);
                if (view) {
                    return view;
                }
            } else if (widget) {
                NSView *view = widget->getView();
                // AppKit won't be able to handle scrolling and making us the first responder
                // well unless we are actually installed in the correct place. KHTML only does
                // that for visible widgets, so we need to do it explicitly here.
                int x, y;
                if (view && renderWidget->absolutePosition(x, y)) {
                    renderWidget->view()->addChild(widget, x, y);
                    return view;
                }
            }
        }
    }
}

NSView *KWQKHTMLPart::nextKeyViewInFrameHierarchy(NodeImpl *node, KWQSelectionDirection direction)
{
    NSView *next = nextKeyViewInFrame(node, direction);
    if (next) {
        return next;
    }
    
    KHTMLPart *parent = parentPart();
    if (parent) {
        next = KWQ(parent)->nextKeyView(parent->frame(this)->m_frame->element(), direction);
        if (next) {
            return next;
        }
    }
    
    return nil;
}

NSView *KWQKHTMLPart::nextKeyView(NodeImpl *node, KWQSelectionDirection direction)
{
    NSView *next = nextKeyViewInFrameHierarchy(node, direction);
    if (next) {
        return next;
    }

    // Look at views from the top level part up, looking for a next key view that we can use.
    next = direction == KWQSelectingNext
        ? [_bridge nextKeyViewOutsideWebViews]
        : [_bridge previousKeyViewOutsideWebViews];
    if (next) {
        return next;
    }
    
    // If all else fails, make a loop by starting from 0.
    return nextKeyViewInFrameHierarchy(0, direction);
}

NSView *KWQKHTMLPart::nextKeyViewForWidget(QWidget *startingWidget, KWQSelectionDirection direction)
{
    // Use the event filter object to figure out which RenderWidget owns this QWidget and get to the DOM.
    // Then get the next key view in the order determined by the DOM.
    NodeImpl *node = nodeForWidget(startingWidget);
    return partForNode(node)->nextKeyView(node, direction);
}

bool KWQKHTMLPart::canCachePage()
{
    // Only save page state if:
    // 1.  We're not a frame or frameset.
    // 2.  The page has no javascript timers.
    // 3.  The page has no unload handler.
    // 4.  The page has no plugins.
    if (d->m_doc &&
        (d->m_frames.count() ||
        parentPart() ||
        d->m_objects.count() ||
        d->m_doc->getWindowEventListener (EventImpl::UNLOAD_EVENT) ||
        (d->m_jscript && Window::retrieveWindow(this)->hasTimeouts()))){
        return false;
    }
    return true;
}

void KWQKHTMLPart::saveWindowProperties(SavedProperties *windowProperties)
{
    KJS::Window *window;
    window = Window::retrieveWindow(this);
    if (window)
        window->saveProperties(*windowProperties);
}

void KWQKHTMLPart::saveLocationProperties(SavedProperties *locationProperties)
{
    KJS::Window *window;
    window = Window::retrieveWindow(this);
    if (window)
        window->saveProperties(*locationProperties);
}

void KWQKHTMLPart::restoreWindowProperties(SavedProperties *windowProperties)
{
    KJS::Window *window;
    window = Window::retrieveWindow(this);
    if (window)
        window->restoreProperties(*windowProperties);
}

void KWQKHTMLPart::restoreLocationProperties(SavedProperties *locationProperties)
{
    KJS::Window *window;
    window = Window::retrieveWindow(this);
    if (window)
        window->location()->restoreProperties(*locationProperties);
}

void KWQKHTMLPart::openURLFromPageCache(DocumentImpl *doc, RenderObject *renderer, KURL *url, SavedProperties *windowProperties, SavedProperties *locationProperties)
{
    d->m_redirectionTimer.stop();

    // We still have to close the previous part page.
    if (!d->m_restored){
        closeURL();
    }
            
    d->m_bComplete = false;
    
    // Don't re-emit the load event.
    d->m_bLoadEventEmitted = true;
    
    // delete old status bar msg's from kjs (if it _was_ activated on last URL)
    if( d->m_bJScriptEnabled )
    {
        d->m_kjsStatusBarText = QString::null;
        d->m_kjsDefaultStatusBarText = QString::null;
    }

    m_url = *url;
    
    // set the javascript flags according to the current url
    d->m_bJScriptEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaScriptEnabled(m_url.host());
    d->m_bJScriptDebugEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaScriptDebugEnabled();
    d->m_bJavaEnabled = KHTMLFactory::defaultHTMLSettings()->isJavaEnabled(m_url.host());
    d->m_bPluginsEnabled = KHTMLFactory::defaultHTMLSettings()->isPluginsEnabled(m_url.host());
    
    // initializing m_url to the new url breaks relative links when opening such a link after this call and _before_ begin() is called (when the first
    // data arrives) (Simon)
    if(m_url.protocol().startsWith( "http" ) && !m_url.host().isEmpty() && m_url.path().isEmpty()) {
        m_url.setPath("/");
        emit d->m_extension->setLocationBarURL( m_url.prettyURL() );
    }
    
    // copy to m_workingURL after fixing m_url above
    d->m_workingURL = m_url;
        
    emit started( 0L );
    
    // -----------begin-----------
    clear();

    doc->restoreRenderer(renderer);
    
    d->m_bCleared = false;
    d->m_cacheId = 0;
    d->m_bComplete = false;
    d->m_bLoadEventEmitted = false;
    d->m_referrer = m_url.url();
    
    d->m_doc = doc;
    d->m_doc->ref();

    setView (doc->view(), true);
    
    updatePolicyBaseURL();
        
    restoreWindowProperties (windowProperties);
    restoreLocationProperties (locationProperties);
}

WebCoreBridge *KWQKHTMLPart::bridgeForWidget(QWidget *widget)
{
    return partForNode(nodeForWidget(widget))->bridge();
}

KWQKHTMLPart *KWQKHTMLPart::partForNode(NodeImpl *node)
{
    return KWQ(node->getDocument()->view()->part());
}

NodeImpl *KWQKHTMLPart::nodeForWidget(QWidget *widget)
{
    return static_cast<const RenderWidget *>(widget->eventFilterObject())->element();
}

void KWQKHTMLPart::setDocumentFocus(QWidget *widget)
{
    NodeImpl *node = nodeForWidget(widget);
    node->getDocument()->setFocusNode(node);
}

void KWQKHTMLPart::clearDocumentFocus(QWidget *widget)
{
    nodeForWidget(widget)->getDocument()->setFocusNode(0);
}

void KWQKHTMLPart::saveDocumentState()
{
    [_bridge saveDocumentState];
}

void KWQKHTMLPart::restoreDocumentState()
{
    [_bridge restoreDocumentState];
}

QPtrList<KWQKHTMLPart> &KWQKHTMLPart::mutableInstances()
{
    static QPtrList<KWQKHTMLPart> instancesList;
    return instancesList;
}

void KWQKHTMLPart::updatePolicyBaseURL()
{
    if (parentPart()) {
        setPolicyBaseURL(parentPart()->docImpl()->policyBaseURL());
    } else {
        setPolicyBaseURL(m_url.url());
    }
}

void KWQKHTMLPart::setPolicyBaseURL(const DOMString &s)
{
    // XML documents will cause this to return null.  docImpl() is
    // an HTMLdocument only. -dwh
    if (docImpl())
        docImpl()->setPolicyBaseURL(s);
    ConstFrameIt end = d->m_frames.end();
    for (ConstFrameIt it = d->m_frames.begin(); it != end; ++it) {
        ReadOnlyPart *subpart = (*it).m_part;
        static_cast<KWQKHTMLPart *>(subpart)->setPolicyBaseURL(s);
    }
}

QString KWQKHTMLPart::requestedURLString() const
{
    return QString::fromNSString([_bridge requestedURL]);
}

void KWQKHTMLPart::forceLayout()
{
    KHTMLView *v = d->m_view;
    if (v) {
        v->layout();
        // We cannot unschedule a pending relayout, since the force can be called with
        // a tiny rectangle from a drawRect update.  By unscheduling we in effect
        // "validate" and stop the necessary full repaint from occurring.  Basically any basic
        // append/remove DHTML is broken by this call.  For now, I have removed the optimization
        // until we have a better invalidation stategy. -dwh
        //v->unscheduleRelayout();
    }
}

QString KWQKHTMLPart::referrer() const
{
    return d->m_referrer;
}

void KWQKHTMLPart::runJavaScriptAlert(const QString &message)
{
    [[WebCoreViewFactory sharedFactory] runJavaScriptAlertPanelWithMessage:message.getNSString()];
}

bool KWQKHTMLPart::runJavaScriptConfirm(const QString &message)
{
    return [[WebCoreViewFactory sharedFactory] runJavaScriptConfirmPanelWithMessage:message.getNSString()];
}

bool KWQKHTMLPart::runJavaScriptPrompt(const QString &prompt, const QString &defaultValue, QString &result)
{
    NSString *returnedText;
    bool ok = [[WebCoreViewFactory sharedFactory] runJavaScriptTextInputPanelWithPrompt:prompt.getNSString()
        defaultText:defaultValue.getNSString() returningText:&returnedText];
    if (ok)
        result = QString::fromNSString(returnedText);
    return ok;
}

void KWQKHTMLPart::createEmptyDocument()
{
    [_bridge loadEmptyDocumentSynchronously];
}

void KWQKHTMLPart::addMetaData(const QString &key, const QString &value)
{
    d->m_job->addMetaData(key, value);
}

bool KWQKHTMLPart::keyEvent(NSEvent *event)
{
    ASSERT([event type] == NSKeyDown || [event type] == NSKeyUp);

    // Check for cases where we are too early for events -- possible unmatched key up
    // from pressing return in the location bar.
    DocumentImpl *doc = xmlDocImpl();
    if (!doc) {
        return false;
    }
    NodeImpl *node = doc->focusNode();
    if (!node) {
	return false;
    }
    
    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = [event retain];

    const char *characters = [[event characters] lossyCString];
    int ascii = (characters != nil && strlen(characters) == 1) ? characters[0] : 0;

    QKeyEvent qEvent([event type] == NSKeyDown ? QEvent::KeyPress : QEvent::KeyRelease,
		     [event keyCode],
		     ascii,
		     stateForCurrentEvent(),
		     QString::fromNSString([event characters]),
		     [event isARepeat]);
    bool result = node->dispatchKeyEvent(&qEvent);

    // We want to send both a down and a press for the initial key event.
    // This is a temporary hack; we need to do this a better way.
    if ([event type] == NSKeyDown && ![event isARepeat]) {
	QKeyEvent qEvent(QEvent::KeyPress,
			 [event keyCode],
			 ascii,
			 stateForCurrentEvent(),
			 QString::fromNSString([event characters]),
			 true);
        node->dispatchKeyEvent(&qEvent);
    }

    ASSERT(_currentEvent == event);
    [event release];
    _currentEvent = oldCurrentEvent;

    return result;
}

// This does the same kind of work that KHTMLPart::openURL does, except it relies on the fact
// that a higher level already checked that the URLs match and the scrolling is the right thing to do.
void KWQKHTMLPart::scrollToAnchor(const KURL &URL)
{
    m_url = URL;
    started(0);

    if (!gotoAnchor(URL.encodedHtmlRef()))
        gotoAnchor(URL.htmlRef());

    d->m_bComplete = true;
    d->m_doc->setParsing(false);

    completed();
}

bool KWQKHTMLPart::closeURL()
{
    saveDocumentState();
    return KHTMLPart::closeURL();
}

void KWQKHTMLPart::khtmlMousePressEvent(MousePressEvent *event)
{
    if (!passWidgetMouseDownEventToWidget(event)) {
        // We don't do this at the start of mouse down handling (before calling into WebCore),
        // because we don't want to do it until we know we didn't hit a widget.
        NSView *view = d->m_view->getDocumentView();
        NSWindow *window = [view window];
        if ([_currentEvent clickCount] <= 1 && [window firstResponder] != view) {
            [window makeFirstResponder:view];
        }
        
        KHTMLPart::khtmlMousePressEvent(event);
    }
}

void KWQKHTMLPart::khtmlMouseDoubleClickEvent(MouseDoubleClickEvent *event)
{
    if (!passWidgetMouseDownEventToWidget(event)) {
        KHTMLPart::khtmlMouseDoubleClickEvent(event);
    }
}

bool KWQKHTMLPart::passWidgetMouseDownEventToWidget(khtml::MouseEvent *event)
{
    // Figure out which view to send the event to.
    RenderObject *target = event->innerNode().handle()->renderer();
    if (!target || !target->isWidget()) {
        return false;
    }
    return passWidgetMouseDownEventToWidget(static_cast<RenderWidget *>(target));
}

bool KWQKHTMLPart::passWidgetMouseDownEventToWidget(RenderWidget *renderWidget)
{
    QWidget *widget = renderWidget->widget();
    if (!widget) {
        ERROR("hit a RenderWidget without a corresponding QWidget, means a frame is half-constructed");
        return true;
    }
    NSView *nodeView = widget->getView();
    ASSERT(nodeView);
    ASSERT([nodeView superview]);
    NSView *topView = nodeView;
    NSView *superview;
    while ((superview = [topView superview])) {
        topView = superview;
    }
    NSView *view = [nodeView hitTest:[[nodeView superview] convertPoint:[_currentEvent locationInWindow] fromView:topView]];
    if (view == nil) {
        ERROR("KHTML says we hit a RenderWidget, but AppKit doesn't agree we hit the corresponding NSView");
        return true;
    }
    
    NSWindow *window = [view window];
    if ([window firstResponder] == view) {
        // In the case where we just became first responder, we should send the mouseDown:
        // to the NSTextField, not the NSTextField's editor. This code makes sure that happens.
        // If we don't do this, we see a flash of selected text when clicking in a text field.
        if (_firstResponderAtMouseDownTime != view && [view isKindOfClass:[NSTextView class]]) {
            NSView *superview = view;
            while (superview != nodeView) {
                superview = [superview superview];
                ASSERT(superview);
                if ([superview isKindOfClass:[NSControl class]]) {
                    if ([(NSControl *)superview currentEditor] == view) {
                        view = superview;
                    }
                    break;
                }
            }
        }
    } else {
        // Normally [NSWindow sendEvent:] handles setting the first responder.
        // But in our case, the event was sent to the view representing the entire web page.
        if ([_currentEvent clickCount] <= 1 && [view acceptsFirstResponder] && [view needsPanelToBecomeKey]) {
            [window makeFirstResponder:view];
        }
    }

    ASSERT(!_sendingEventToSubview);
    _sendingEventToSubview = true;
    [view mouseDown:_currentEvent];
    _sendingEventToSubview = false;
    
    // Remember which view we sent the event to, so we can direct the release event properly.
    _mouseDownView = view;
    _mouseDownWasInSubframe = false;
    
    return true;
}

// Note that this does the same kind of check as [target isDescendantOf:superview].
// There are two differences: This is a lot slower because it has to walk the whole
// tree, and this works in cases where the target has already been deallocated.
static bool findViewInSubviews(NSView *superview, NSView *target)
{
    NSEnumerator *e = [[superview subviews] objectEnumerator];
    NSView *subview;
    while ((subview = [e nextObject])) {
        if (subview == target || findViewInSubviews(subview, target))
            return true;
    }
    return false;
}

NSView *KWQKHTMLPart::mouseDownViewIfStillGood()
{
    // Since we have no way of tracking the lifetime of _mouseDownView, we have to assume that
    // it could be deallocated already. We search for it in our subview tree; if we don't find
    // it, we set it to nil.
    NSView *mouseDownView = _mouseDownView;
    if (!mouseDownView) {
        return nil;
    }
    KHTMLView *topKHTMLView = d->m_view;
    NSView *topView = topKHTMLView ? topKHTMLView->getView() : nil;
    if (!topView || !findViewInSubviews(topView, mouseDownView)) {
        _mouseDownView = nil;
        return nil;
    }
    return mouseDownView;
}

void KWQKHTMLPart::khtmlMouseMoveEvent(MouseMoveEvent *event)
{
    NSView *view;
    if ([_currentEvent type] != NSLeftMouseDragged) {
        view = nil;
    } else {
    	view = mouseDownViewIfStillGood();
    }
    if (!view) {
        KHTMLPart::khtmlMouseMoveEvent(event);
        return;
    }
    
    _sendingEventToSubview = true;
    [view mouseDragged:_currentEvent];
    _sendingEventToSubview = false;
}

void KWQKHTMLPart::khtmlMouseReleaseEvent(MouseReleaseEvent *event)
{
    NSView *view = mouseDownViewIfStillGood();
    if (!view) {
        KHTMLPart::khtmlMouseReleaseEvent(event);
        return;
    }
    
    _sendingEventToSubview = true;
    [view mouseUp:_currentEvent];
    _sendingEventToSubview = false;
}

void KWQKHTMLPart::clearTimers(KHTMLView *view)
{
    if (view) {
        view->unscheduleRelayout();
        view->unscheduleRepaint();
    }
}

void KWQKHTMLPart::clearTimers()
{
    clearTimers(d->m_view);
}

bool KWQKHTMLPart::passSubframeEventToSubframe(DOM::NodeImpl::MouseEvent &event)
{
    switch ([_currentEvent type]) {
    	case NSLeftMouseDown: {
            NodeImpl *node = event.innerNode.handle();
            if (!node) {
                return false;
            }
            RenderPart *renderPart = dynamic_cast<RenderPart *>(node->renderer());
            if (!renderPart) {
                return false;
            }
            if (!passWidgetMouseDownEventToWidget(renderPart)) {
                return false;
            }
            _mouseDownWasInSubframe = true;
            return true;
        }
        case NSLeftMouseUp: {
            if (!_mouseDownWasInSubframe) {
                return false;
            }
            NSView *view = mouseDownViewIfStillGood();
            if (!view) {
                return false;
            }
            ASSERT(!_sendingEventToSubview);
            _sendingEventToSubview = true;
            [view mouseUp:_currentEvent];
            _sendingEventToSubview = false;
            return true;
        }
        case NSLeftMouseDragged: {
            if (!_mouseDownWasInSubframe) {
                return false;
            }
            NSView *view = mouseDownViewIfStillGood();
            if (!view) {
                return false;
            }
            ASSERT(!_sendingEventToSubview);
            _sendingEventToSubview = true;
            [view mouseDragged:_currentEvent];
            _sendingEventToSubview = false;
            return true;
        }
        default:
            return false;
    }
}

int KWQKHTMLPart::buttonForCurrentEvent()
{
    switch ([_currentEvent type]) {
    case NSLeftMouseDown:
    case NSLeftMouseUp:
        return Qt::LeftButton;
    case NSRightMouseDown:
    case NSRightMouseUp:
        return Qt::RightButton;
    case NSOtherMouseDown:
    case NSOtherMouseUp:
        return Qt::MidButton;
    default:
        return 0;
    }
}

int KWQKHTMLPart::stateForCurrentEvent()
{
    int state = buttonForCurrentEvent();
    
    unsigned modifiers = [_currentEvent modifierFlags];

    if (modifiers & NSControlKeyMask)
        state |= Qt::ControlButton;
    if (modifiers & NSShiftKeyMask)
        state |= Qt::ShiftButton;
    if (modifiers & NSAlternateKeyMask)
        state |= Qt::AltButton;
    if (modifiers & NSCommandKeyMask)
        state |= Qt::MetaButton;
    
    return state;
}

void KWQKHTMLPart::mouseDown(NSEvent *event)
{
    if (!d->m_view || _sendingEventToSubview) {
        return;
    }

    _mouseDownView = nil;

    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = [event retain];
    
    NSResponder *oldFirstResponderAtMouseDownTime = _firstResponderAtMouseDownTime;
    _firstResponderAtMouseDownTime = [[[d->m_view->getView() window] firstResponder] retain];

    QMouseEvent kEvent(QEvent::MouseButtonPress, QPoint([event locationInWindow]),
        buttonForCurrentEvent(), stateForCurrentEvent(), [event clickCount]);
    d->m_view->viewportMousePressEvent(&kEvent);
    
    [_firstResponderAtMouseDownTime release];
    _firstResponderAtMouseDownTime = oldFirstResponderAtMouseDownTime;

    ASSERT(_currentEvent == event);
    [event release];
    _currentEvent = oldCurrentEvent;
}

void KWQKHTMLPart::mouseDragged(NSEvent *event)
{
    if (!d->m_view || _sendingEventToSubview) {
        return;
    }

    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = [event retain];

    QMouseEvent kEvent(QEvent::MouseMove, QPoint([event locationInWindow]), Qt::LeftButton, Qt::LeftButton);
    d->m_view->viewportMouseMoveEvent(&kEvent);
    
    ASSERT(_currentEvent == event);
    [event release];
    _currentEvent = oldCurrentEvent;
}

void KWQKHTMLPart::mouseUp(NSEvent *event)
{
    if (!d->m_view || _sendingEventToSubview) {
        return;
    }

    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = [event retain];

    // Our behavior here is a little different that Qt. Qt always sends
    // a mouse release event, even for a double click. To correct problems
    // in khtml's DOM click event handling we do not send a release here
    // for a double click. Instead we send that event from KHTMLView's
    // viewportMouseDoubleClickEvent.
    int clickCount = [event clickCount];
    if (clickCount > 0 && clickCount % 2 == 0) {
        QMouseEvent doubleClickEvent(QEvent::MouseButtonDblClick, QPoint([event locationInWindow]),
            buttonForCurrentEvent(), stateForCurrentEvent(), clickCount);
        d->m_view->viewportMouseDoubleClickEvent(&doubleClickEvent);
    } else {
        QMouseEvent releaseEvent(QEvent::MouseButtonRelease, QPoint([event locationInWindow]),
            buttonForCurrentEvent(), stateForCurrentEvent(), clickCount);
        d->m_view->viewportMouseReleaseEvent(&releaseEvent);
    }
    
    ASSERT(_currentEvent == event);
    [event release];
    _currentEvent = oldCurrentEvent;
    
    _mouseDownView = nil;
}

void KWQKHTMLPart::mouseMoved(NSEvent *event)
{
    if (!d->m_view) {
        return;
    }
    
    NSEvent *oldCurrentEvent = _currentEvent;
    _currentEvent = [event retain];
    
    QMouseEvent kEvent(QEvent::MouseMove, QPoint([event locationInWindow]), 0, stateForCurrentEvent());
    d->m_view->viewportMouseMoveEvent(&kEvent);
    
    ASSERT(_currentEvent == event);
    [event release];
    _currentEvent = oldCurrentEvent;
}

NSAttributedString *KWQKHTMLPart::attributedString(NodeImpl *_startNode, int startOffset, NodeImpl *endNode, int endOffset)
{
    NSMutableAttributedString *result = [[[NSMutableAttributedString alloc] init] autorelease];

    bool hasNewLine = true;
    bool hasParagraphBreak = true;

    Node n = _startNode;
    while (!n.isNull()) {
        RenderObject *renderer = n.handle()->renderer();
        if (renderer) {
            if (n.nodeType() == Node::TEXT_NODE) {
                NSFont *font = nil;
                RenderStyle *style = renderer->style();
                if (style) {
                    font = style->font().getNSFont();
                }
    
                QString str = n.nodeValue().string();
    
                QString text;
                if(n == _startNode && n == endNode && startOffset >=0 && endOffset >= 0)
                    text = str.mid(startOffset, endOffset - startOffset);
                else if(n == _startNode && startOffset >= 0)
                    text = str.mid(startOffset);
                else if(n == endNode && endOffset >= 0)
                    text = str.left(endOffset);
                else
                    text = str;
    
                text = text.stripWhiteSpace();
                if (text.length() > 1)
                    text += ' ';
    
                if (text.length() > 0) {
                    hasNewLine = false;
                    hasParagraphBreak = false;
                    NSMutableDictionary *attrs = nil;
                    if (font) {
                        attrs = [[NSMutableDictionary alloc] init];
                        [attrs setObject:font forKey:NSFontAttributeName];
                        if (style && style->color().isValid())
                            [attrs setObject:style->color().getNSColor() forKey:NSForegroundColorAttributeName];
                        if (style && style->backgroundColor().isValid())
                            [attrs setObject:style->backgroundColor().getNSColor() forKey:NSBackgroundColorAttributeName];
                    }
                    NSAttributedString *partialString = [[NSAttributedString alloc] initWithString:text.getNSString() attributes:attrs];
                    [attrs release];
                    [result appendAttributedString: partialString];                
                    [partialString release];
                }
            } else {
                // This is our simple HTML -> ASCII transformation:
                QString text;
                unsigned short _id = n.elementId();
                switch(_id) {
                    case ID_BR:
                        text += "\n";
                        hasNewLine = true;
                        break;
    
                    case ID_TD:
                    case ID_TH:
                    case ID_HR:
                    case ID_OL:
                    case ID_UL:
                    case ID_LI:
                    case ID_DD:
                    case ID_DL:
                    case ID_DT:
                    case ID_PRE:
                    case ID_BLOCKQUOTE:
                    case ID_DIV:
                        if (!hasNewLine)
                            text += "\n";
                        hasNewLine = true;
                        break;
                    case ID_P:
                    case ID_TR:
                    case ID_H1:
                    case ID_H2:
                    case ID_H3:
                    case ID_H4:
                    case ID_H5:
                    case ID_H6:
                        if (!hasNewLine)
                            text += "\n";
                        if (!hasParagraphBreak)
                            text += "\n";
                            hasParagraphBreak = true;
                        hasNewLine = true;
                        break;
                }
                NSAttributedString *partialString = [[NSAttributedString alloc] initWithString:text.getNSString()];
                [result appendAttributedString: partialString];
                [partialString release];
            }
        }

        if (n == endNode)
            break;

        Node next = n.firstChild();
        if (next.isNull())
            next = n.nextSibling();

        while (next.isNull() && !n.parentNode().isNull()) {
            QString text;
            n = n.parentNode();
            next = n.nextSibling();

            unsigned short _id = n.elementId();
            switch(_id) {
                case ID_TD:
                case ID_TH:
                case ID_HR:
                case ID_OL:
                case ID_UL:
                case ID_LI:
                case ID_DD:
                case ID_DL:
                case ID_DT:
                case ID_PRE:
                case ID_BLOCKQUOTE:
                case ID_DIV:
                    if (!hasNewLine)
                        text += "\n";
                    hasNewLine = true;
                    break;
                case ID_P:
                case ID_TR:
                case ID_H1:
                case ID_H2:
                case ID_H3:
                case ID_H4:
                case ID_H5:
                case ID_H6:
                    if (!hasNewLine)
                        text += "\n";
                    // An extra newline is needed at the start, not the end, of these types of tags,
                    // so don't add another here.
                    hasNewLine = true;
                    break;
            }
            
            NSAttributedString *partialString = [[NSAttributedString alloc] initWithString:text.getNSString()];
            [result appendAttributedString:partialString];
            [partialString release];
        }

        n = next;
    }

    return result;
}
