/*	
    IFWebFramePrivate.mm
	    
    Copyright 2001, Apple, Inc. All rights reserved.
*/
#import <WebKit/IFWebDataSource.h>
#import <WebKit/IFWebDataSourcePrivate.h>
#import <WebKit/IFWebViewPrivate.h>
#import <WebKit/IFWebFramePrivate.h>
#import <WebKit/IFPreferencesPrivate.h>

#import <WebKit/WebKitDebug.h>

#import <WebFoundation/IFError.h>

// includes from kde
#include <khtmlview.h>
#include <rendering/render_frames.h>

@implementation IFWebFramePrivate

- (void)dealloc
{
    [name autorelease];
    [view _setController: nil];
    [view autorelease];
    [dataSource _setController: nil];
    [dataSource autorelease];
    [provisionalDataSource _setController: nil];
    [provisionalDataSource autorelease];
    [errors release];
    [mainDocumentError release];
    if (renderFramePart)
        ((khtml::RenderPart *)renderFramePart)->deref();
    [super dealloc];
}

- (NSString *)name { return name; }
- (void)setName: (NSString *)n 
{
    [name autorelease];
    name = [n retain];
}


- view { return view; }
- (void)setView: v 
{ 
    [view autorelease];
    view = [v retain];
}

- (IFWebDataSource *)dataSource { return dataSource; }
- (void)setDataSource: (IFWebDataSource *)d
{
    if (dataSource != d){
        [dataSource _setController: nil];
        [dataSource autorelease];
        dataSource = [d retain];
    }
}


- (id <IFWebController>)controller { return controller; }
- (void)setController: (id <IFWebController>)c
{ 
    // Warning:  non-retained reference
    controller = c;
}


- (IFWebDataSource *)provisionalDataSource { return provisionalDataSource; }
- (void)setProvisionalDataSource: (IFWebDataSource *)d
{ 
    if (provisionalDataSource != d){
        [provisionalDataSource autorelease];
        provisionalDataSource = [d retain];
    }
}


- (void *)renderFramePart { return renderFramePart; }
- (void)setRenderFramePart: (void *)p 
{
    if (p)
        ((khtml::RenderPart *)p)->ref();
    if (renderFramePart)
        ((khtml::RenderPart *)renderFramePart)->deref();
    renderFramePart = p;
}


@end


@implementation IFWebFrame (IFPrivate)
- (void)_setController: (id <IFWebController>)controller
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    [data setController: controller];
}


// renderFramePart is a pointer to a RenderPart
- (void)_setRenderFramePart: (void *)p
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    [data setRenderFramePart: p];
}

- (void *)_renderFramePart
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    return [data renderFramePart];
}

- (void)_setDataSource: (IFWebDataSource *)ds
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    [data setDataSource: ds];
    [ds _setController: [self controller]];
}

char *stateNames[6] = {
    "zero state",
    "IFWEBFRAMESTATE_UNINITIALIZED",
    "IFWEBFRAMESTATE_PROVISIONAL",
    "IFWEBFRAMESTATE_COMMITTED_PAGE",
    "IFWEBFRAMESTATE_LAYOUT_ACCEPTABLE",
    "IFWEBFRAMESTATE_COMPLETE" };


- (void)_scheduleLayout: (NSTimeInterval)inSeconds
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    if (data->scheduledLayoutPending == NO){
        [NSTimer scheduledTimerWithTimeInterval:inSeconds target:self selector: @selector(_timedLayout:) userInfo: nil repeats:FALSE];
        data->scheduledLayoutPending = YES;
    }
}

- (void)_timedLayout: userInfo
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "%s:  state = %s\n", [[self name] cString], stateNames[data->state]);
    
    data->scheduledLayoutPending = NO;
    if (data->state == IFWEBFRAMESTATE_LAYOUT_ACCEPTABLE){
        if ([self controller])
            WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "%s:  performing timed layout, %f seconds since start of document load\n", [[self name] cString], CFAbsoluteTimeGetCurrent() - [[[[self controller] mainFrame] dataSource] _loadingStartedTime]);
        [[self view] setNeedsLayout: YES];
        [[self view] setNeedsDisplay: YES];
    }
    else {
        if ([self controller])
            WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "%s:  NOT performing timed layout (not needed), %f seconds since start of document load\n", [[self name] cString], CFAbsoluteTimeGetCurrent() - [[[[self controller] mainFrame] dataSource] _loadingStartedTime]);
    }
}


- (void)_transitionProvisionalToLayoutAcceptable
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    switch ([self _state]){
    	case IFWEBFRAMESTATE_COMMITTED_PAGE:
        {
            [self _setState: IFWEBFRAMESTATE_LAYOUT_ACCEPTABLE];
                    
            // Start a timer to guarantee that we get an initial layout after
            // X interval, even if the document and resources are not completely
            // loaded.
            BOOL timedDelayEnabled = [[IFPreferences standardPreferences] _initialTimedLayoutEnabled];
            if (timedDelayEnabled){
                NSTimeInterval defaultTimedDelay = [[IFPreferences standardPreferences] _initialTimedLayoutDelay];
                double timeSinceStart;

                // If the delay getting to the commited state exceeds the initial layout delay, go
                // ahead and schedule a layout.
                timeSinceStart = (CFAbsoluteTimeGetCurrent() - [[self dataSource] _loadingStartedTime]);
                if (timeSinceStart > (double)defaultTimedDelay){
                    WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "performing early layout because commit time, %f, exceeded initial layout interval %f\n", timeSinceStart, defaultTimedDelay);
                    [self _timedLayout: nil];
                }
                else {
                    NSTimeInterval timedDelay = defaultTimedDelay - timeSinceStart;
                    
                    WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "registering delayed layout after %f seconds, time since start %f\n", timedDelay, timeSinceStart);
                    [self _scheduleLayout: timedDelay];
                }
            }
            break;
        }

        case IFWEBFRAMESTATE_PROVISIONAL:
        case IFWEBFRAMESTATE_COMPLETE:
        case IFWEBFRAMESTATE_LAYOUT_ACCEPTABLE:
        {
            break;
        }
        
        case IFWEBFRAMESTATE_UNINITIALIZED:
        default:
        {
            [[NSException exceptionWithName:NSGenericException reason: [NSString stringWithFormat: @"invalid state attempting to transition to IFWEBFRAMESTATE_LAYOUT_ACCEPTABLE from %s", stateNames[data->state]] userInfo: nil] raise];
            return;
        }
    }
}


- (void)_transitionProvisionalToCommitted
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    WEBKIT_ASSERT ([self controller] != nil);

    switch ([self _state]){
    	case IFWEBFRAMESTATE_PROVISIONAL:
        {
            id view = [self view];

            WEBKIT_ASSERT (view != nil);

            // Make sure any plugsin are shut down cleanly.
            [view _stopPlugins];
            
            // Remove any widgets.
            [view _removeSubviews];
            
            // Set the committed data source on the frame.
            [self _setDataSource: data->provisionalDataSource];
            
            // If we're a frame (not the main frame) hookup the kde internals.  This introduces a nasty dependency 
            // in kde on the view.
            khtml::RenderPart *renderPartFrame = [self _renderFramePart];
            if (renderPartFrame && [view isKindOfClass: NSClassFromString(@"IFWebView")]){
                // Setting the widget will delete the previous KHTMLView associated with the frame.
                renderPartFrame->setWidget ([view _provisionalWidget]);
            }
        
            // dataSourceChanged: will reset the view and begin trying to
            // display the new new datasource.
            [view dataSourceChanged: data->provisionalDataSource];
        
            
            // Now that the provisional data source is committed, release it.
            [data setProvisionalDataSource: nil];
        
            [self _setState: IFWEBFRAMESTATE_COMMITTED_PAGE];
        
            [[[self dataSource] _locationChangeHandler] locationChangeCommitted];
            
            // If we have a title let the controller know about it.
            if ([[self dataSource] pageTitle])
                [[[self dataSource] _locationChangeHandler] receivedPageTitle:[[self dataSource] pageTitle] forDataSource:[self dataSource]];

            break;
        }
        
        case IFWEBFRAMESTATE_UNINITIALIZED:
        case IFWEBFRAMESTATE_COMMITTED_PAGE:
        case IFWEBFRAMESTATE_LAYOUT_ACCEPTABLE:
        case IFWEBFRAMESTATE_COMPLETE:
        default:
        {
            [[NSException exceptionWithName:NSGenericException reason:[NSString stringWithFormat: @"invalid state attempting to transition to IFWEBFRAMESTATE_COMMITTED from %s", stateNames[data->state]] userInfo: nil] raise];
            return;
        }
    }
}


- (IFWebFrameState)_state
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    
    return data->state;
}

- (void)_setState: (IFWebFrameState)newState
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  transition from %s to %s\n", [[self name] cString], stateNames[data->state], stateNames[newState]);
    if ([self controller])
        WEBKITDEBUGLEVEL (WEBKIT_LOG_TIMING, "%s:  transition from %s to %s, %f seconds since start of document load\n", [[self name] cString], stateNames[data->state], stateNames[newState], CFAbsoluteTimeGetCurrent() - [[[[self controller] mainFrame] dataSource] _loadingStartedTime]);
    
    if (newState == IFWEBFRAMESTATE_COMPLETE && self == [[self controller] mainFrame])
        WEBKITDEBUGLEVEL (WEBKIT_LOG_DOCUMENTLOAD, "completed %s (%f seconds)", [[[[self dataSource] inputURL] absoluteString] cString], CFAbsoluteTimeGetCurrent() - [[self dataSource] _loadingStartedTime]);

    data->state = newState;
}

- (void)_addError: (IFError *)error forResource: (NSString *)resourceDescription
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    if (data->errors == 0)
        data->errors = [[NSMutableDictionary alloc] init];
        
    [data->errors setObject: error forKey: resourceDescription];
}

- (void)_isLoadComplete
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    
    WEBKIT_ASSERT ([self controller] != nil);

    switch ([self _state]){
        // Shouldn't ever be in this state.
        case IFWEBFRAMESTATE_UNINITIALIZED:
        {
            [[NSException exceptionWithName:NSGenericException reason:@"invalid state IFWEBFRAMESTATE_UNINITIALIZED during completion check with error" userInfo: nil] raise];
            return;
        }
        
        case IFWEBFRAMESTATE_PROVISIONAL:
        {
            WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete in IFWEBFRAMESTATE_PROVISIONAL\n", [[self name] cString]);
            // If we've received any errors we may be stuck in the provisional state and actually
            // complete.
            if ([[self errors] count] != 0){
                // Check all children first.
                WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete, current state IFWEBFRAMESTATE_PROVISIONAL, %d errors\n", [[self name] cString], [[self errors] count]);
                if (![[self provisionalDataSource] isLoading]){
                    WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete in IFWEBFRAMESTATE_PROVISIONAL, load done\n", [[self name] cString]);
                    // We now the provisional data source didn't cut the mustard, release it.
                    [data setProvisionalDataSource: nil];
                    
                    [self _setState: IFWEBFRAMESTATE_COMPLETE];
                    [[[self provisionalDataSource] _locationChangeHandler] locationChangeDone: [self mainDocumentError]];
                    return;
                }
            }
            return;
        }
        
        case IFWEBFRAMESTATE_COMMITTED_PAGE:
        case IFWEBFRAMESTATE_LAYOUT_ACCEPTABLE:
        {
            WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete, current state IFWEBFRAMESTATE_COMMITTED\n", [[self name] cString]);
            if (![[self dataSource] isLoading]){
                id mainView = [[[self controller] mainFrame] view];
                id thisView = [self view];

                WEBKIT_ASSERT ([self dataSource] != nil);

                [self _setState: IFWEBFRAMESTATE_COMPLETE];
                
                [[self dataSource] _part]->end();
                
                // May need to relayout each time a frame is completely
                // loaded.
                [mainView setNeedsLayout: YES];
                
                // Layout this view (eventually).
                [thisView setNeedsLayout: YES];
                
                // Draw this view (eventually), and it's scroll view
                // (eventually).
                [thisView setNeedsDisplay: YES];
                if ([thisView _frameScrollView])
                    [[thisView _frameScrollView] setNeedsDisplay: YES];

                // Force a relayout and draw NOW if we are complete are the top level.
                if ([[self controller] mainFrame] == self){
                    [mainView layout];
                    [mainView display];
                }
 
                // Jump to anchor point, if necessary.
                [[self dataSource] _part]->gotoBaseAnchor();
                   
                [[[self dataSource] _locationChangeHandler] locationChangeDone: [self mainDocumentError]];
                
                return;
            }
            // A resource was loaded, but the entire frame isn't complete.  Schedule a
            // layout.
            else {
                if ([self _state] == IFWEBFRAMESTATE_LAYOUT_ACCEPTABLE){
                    BOOL resourceTimedDelayEnabled = [[IFPreferences standardPreferences] _resourceTimedLayoutEnabled];
                    if (resourceTimedDelayEnabled){
                        NSTimeInterval timedDelay = [[IFPreferences standardPreferences] _resourceTimedLayoutDelay];
                        [self _scheduleLayout: timedDelay];
                    }
                }
            }
            return;
        }
        
        case IFWEBFRAMESTATE_COMPLETE:
        {
            WEBKITDEBUGLEVEL (WEBKIT_LOG_LOADING, "%s:  checking complete, current state IFWEBFRAMESTATE_COMPLETE\n", [[self name] cString]);
            return;
        }
        
        // Yikes!  Serious horkage.
        default:
        {
            [[NSException exceptionWithName:NSGenericException reason:@"invalid state during completion check with error" userInfo: nil] raise];
            break;
        }
    }
}

+ (void)_recursiveCheckCompleteFromFrame: (IFWebFrame *)fromFrame
{
    int i, count;
    NSArray *childFrames;
    
    childFrames = [[fromFrame dataSource] children];
    count = [childFrames count];
    for (i = 0; i < count; i++){
        IFWebFrame *childFrame;
        
        childFrame = [childFrames objectAtIndex: i];
        [IFWebFrame _recursiveCheckCompleteFromFrame: childFrame];
        [childFrame _isLoadComplete];
    }
    [fromFrame _isLoadComplete];
}

// Called every time a resource is completely loaded, or an error is received.
- (void)_checkLoadCompleteResource: (NSString *)resourceDescription error: (IFError *)error isMainDocument: (BOOL)mainDocument
{

    WEBKIT_ASSERT ([self controller] != nil);

    if (error) {
        if (mainDocument)
            [self _setMainDocumentError: error];
        [self _addError: error forResource: resourceDescription];
    }

    // Now walk the frame tree to see if any frame that may have initiated a load is done.
    [IFWebFrame _recursiveCheckCompleteFromFrame: [[self controller] mainFrame]];
}


- (void)_setMainDocumentError: (IFError *)error
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;
    data->mainDocumentError = [error retain];
}

- (void)_clearErrors
{
    IFWebFramePrivate *data = (IFWebFramePrivate *)_framePrivate;

    [data->errors release];
    data->errors = nil;
    [data->mainDocumentError release];
    data->mainDocumentError = nil;
}

@end

