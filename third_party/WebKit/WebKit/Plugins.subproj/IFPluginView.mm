/*	
    IFPluginView.mm
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import "IFPluginView.h"
#include <Carbon/Carbon.h> 
#include "kwqdebug.h"

@implementation IFPluginViewNullEventSender

-(id)initializeWithNPP:(NPP)pluginInstance functionPointer:(NPP_HandleEventProcPtr)HandleEventFunction;
{
    instance = pluginInstance;
    NPP_HandleEvent = HandleEventFunction;
    return self;
}

-(void)sendNullEvents
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = nullEvent;
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event);
    //KWQDebug("NPP_HandleEvent(nullEvent): %d  when: %u\n", acceptedEvent, event.when);
    [self performSelector:@selector(sendNullEvents) withObject:nil afterDelay:.01];
}

-(void) stop
{
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(sendNullEvents) object:nil];
}

@end

@implementation IFPluginView

- initWithFrame: (NSRect) r widget: (QWidget *)w plugin: (WCPlugin *)plug url: (NSString *)location mime:(NSString *)mimeType  arguments:(NSDictionary *)arguments
{
    NPError npErr;
    char *cMime, *s;
    NPSavedData saved;
    NSArray *attributes, *values;
    NSString *attributeString;
    uint i;
        
    [super initWithFrame: r];
    instance = malloc(sizeof(NPP_t));
    instance->ndata = self;

    mime = mimeType;
    url = location;
    plugin = plug;
    [mime retain];
    [url retain];
    [plugin retain];
    
    NPP_New = 		[plugin NPP_New]; // copy function pointers
    NPP_Destroy = 	[plugin NPP_Destroy];
    NPP_SetWindow = 	[plugin NPP_SetWindow];
    NPP_NewStream = 	[plugin NPP_NewStream];
    NPP_WriteReady = 	[plugin NPP_WriteReady];
    NPP_Write = 	[plugin NPP_Write];
    NPP_StreamAsFile = 	[plugin NPP_StreamAsFile];
    NPP_DestroyStream = [plugin NPP_DestroyStream];
    NPP_HandleEvent = 	[plugin NPP_HandleEvent];
    NPP_URLNotify = 	[plugin NPP_URLNotify];
    
    attributes = [arguments allKeys];
    values = [arguments allValues];
    cAttributes = malloc(sizeof(char *) * [arguments count]);
    cValues = malloc(sizeof(char *) * [arguments count]);
    
    for(i=0; i<[arguments count]; i++){ // convert dictionary to 2 string arrays
        attributeString = [attributes objectAtIndex:i];
        s = malloc([attributeString length]+1);
        [attributeString getCString:s];
        cAttributes[i] = s;
        
        attributeString = [values objectAtIndex:i];
        s = malloc([attributeString length]+1);
        [attributeString getCString:s];
        cValues[i] = s;
    }
    cMime = malloc([mime length]+1);
    [mime getCString:cMime];
    npErr = NPP_New(cMime, instance, NP_EMBED, [arguments count], cAttributes, cValues, &saved);
    KWQDebug("NPP_New: %d\n", npErr);
    
    if([attributes containsObject:@"HIDDEN"]){
        hidden = TRUE;
    }else{
        hidden = FALSE;
    }
    transferred = FALSE;
    stopped = FALSE;
    filesToErase = [NSMutableArray arrayWithCapacity:2];
    [filesToErase retain];
    //trackingTag = [self addTrackingRect:r owner:self userData:nil assumeInside:NO];
    eventSender = [[IFPluginViewNullEventSender alloc] initializeWithNPP:instance functionPointer:NPP_HandleEvent];
    [eventSender sendNullEvents];
    return self;
}

- (void)drawRect:(NSRect)rect
{
    //MoveTo(0,0); // diagnol line test
    //LineTo((short)rect.size.width, (short)rect.size.height);
    [self setWindow:rect];
    if(!transferred){
        [self sendActivateEvent];
        [self newStream:url mimeType:mime notifyData:NULL];
        transferred = TRUE;
    }
    [self sendUpdateEvent];
}

- (void) setWindow:(NSRect)rect
{
    NPError npErr;
    NSRect frame, frameInWindow;
    
    frame = [self frame];
    frameInWindow = [self convertRect:rect toView:nil];
    
    nPort.port = [self qdPort];
    nPort.portx = 0;
    nPort.porty = 0;   
    window.window = &nPort;
    
    window.x = 0; 
    window.y = 0;

    window.width = (uint32)frame.size.width;
    window.height = (uint32)frame.size.height;

    window.clipRect.top = (uint16)rect.origin.y; // clip rect
    window.clipRect.left = (uint16)rect.origin.x;
    window.clipRect.bottom = (uint16)rect.size.height;
    window.clipRect.right = (uint16)rect.size.width;
    
    window.type = NPWindowTypeDrawable;
    
    npErr = NPP_SetWindow(instance, &window);
    KWQDebug("NPP_SetWindow: %d rect.size.height=%d rect.size.width=%d port=%d rect.origin.x=%f rect.origin.y=%f\n", npErr, (int)rect.size.height, (int)rect.size.width, (int)nPort.port, rect.origin.x, rect.origin.y);
    KWQDebug("frame.size.height=%d frame.size.width=%d frame.origin.x=%f frame.origin.y=%f\n", (int)frame.size.height, (int)frame.size.width, frame.origin.x, frame.origin.y);
    KWQDebug("frameInWindow.size.height=%d frameInWindow.size.width=%d frameInWindow.origin.x=%f frameInWindow.origin.y=%f\n", (int)frameInWindow.size.height, (int)frameInWindow.size.width, frameInWindow.origin.x, frameInWindow.origin.y);
}

- (void) newStream:(NSString *)streamURL mimeType:(NSString *)mimeType notifyData:(void *)notifyData
{
    char *cURL, *cMime;
    StreamData *streamData;
    NPStream *stream;
    NPError npErr;    
    uint16 transferMode;
    
    stream = malloc(sizeof(NPStream));
    cURL   = malloc([streamURL length]+1);
    cMime  = malloc([mime length]+1);
    [streamURL getCString:cURL];
    [mime getCString:cMime];
    stream->url = cURL;
    stream->end = 0;
    stream->lastmodified = 0;
    stream->notifyData = notifyData;
    
    streamData = malloc(sizeof(StreamData));
    streamData->stream = stream;
    streamData->offset = 0;
    streamData->mimeType = cMime;
    
    npErr = NPP_NewStream(instance, cMime, stream, FALSE, &transferMode);
    KWQDebug("NPP_NewStream: %d\n", npErr);
    streamData->transferMode = transferMode;
    
    if(transferMode == NP_NORMAL){
        KWQDebug("Stream type: NP_NORMAL\n");
        [WCURLHandleCreate([NSURL URLWithString:streamURL], self, streamData) loadInBackground];
    }else if(transferMode == NP_ASFILEONLY || transferMode == NP_ASFILE){
        if(transferMode == NP_ASFILEONLY) KWQDebug("Stream type: NP_ASFILEONLY\n");
        if(transferMode == NP_ASFILE) KWQDebug("Stream type: NP_ASFILE\n");
        streamData->filename  = [NSString stringWithString:[streamURL lastPathComponent]];
        [streamData->filename retain];
        streamData->data = [NSMutableData dataWithCapacity:0];
        [streamData->data retain];
        [WCURLHandleCreate([NSURL URLWithString:streamURL], self, streamData) loadInBackground];
    }else if(transferMode == NP_SEEK){
        KWQDebug("Stream type: NP_SEEK not yet supported\n");
    }
}

// cache methods

- (void)WCURLHandle:(id)sender resourceDataDidBecomeAvailable:(NSData *)data userData:(void *)userData
{
    int32 bytes;
    StreamData *streamData;
    
    streamData = userData;
    if(streamData->transferMode != NP_ASFILEONLY){
        bytes = NPP_WriteReady(instance, streamData->stream);
        //KWQDebug("NPP_WriteReady bytes=%u\n", bytes);
        bytes = NPP_Write(instance, streamData->stream, streamData->offset, [data length], (void *)[data bytes]);
        //KWQDebug("NPP_Write bytes=%u\n", bytes);
        streamData->offset += [data length];
    }
    if(streamData->transferMode == NP_ASFILE || streamData->transferMode == NP_ASFILEONLY){
        [streamData->data appendData:data];
    }
}

- (void)WCURLHandleResourceDidFinishLoading:(id)sender userData:(void *)userData
{
    NPError npErr;
    char *cFilename;
    NSMutableString *filenameClassic, *filename;
    StreamData *streamData;
    NSFileManager *fileManager;
        
    streamData = userData;
    if(streamData->transferMode == NP_ASFILE || streamData->transferMode == NP_ASFILEONLY){
        filenameClassic = [NSMutableString stringWithCapacity:200];
        filename = [NSMutableString stringWithCapacity:200];
        [filenameClassic appendString:startupVolumeName()];
        [filenameClassic appendString:@":private:tmp:"];  //FIXME: This should be the user's cache directory or somewhere else
        [filenameClassic appendString:streamData->filename];
        [filename appendString:@"/tmp/"];
        [filename appendString:streamData->filename];
        [filesToErase addObject:filename];
        fileManager = [NSFileManager defaultManager];
        KWQDebug("Writing plugin file out to: %s %s size: %u\n", [filenameClassic cString], [filename cString], [streamData->data length]);
        [fileManager removeFileAtPath:filename handler:nil];
        [fileManager createFileAtPath:filename contents:streamData->data attributes:nil];
        cFilename = malloc([filenameClassic length]+1);
        [filenameClassic getCString:cFilename];
        NPP_StreamAsFile(instance, streamData->stream, cFilename);
        [streamData->data release];
        [streamData->filename release];
    }
    npErr = NPP_DestroyStream(instance, streamData->stream, NPRES_DONE);
    KWQDebug("NPP_DestroyStream: %d\n", npErr);
    if(streamData->stream->notifyData){
        NPP_URLNotify(instance, streamData->stream->url, NPRES_DONE, streamData->stream->notifyData);
        KWQDebug("NPP_URLNotify\n");
    }
    [self setNeedsDisplay:YES];
    free(streamData);
}

- (void)WCURLHandleResourceDidBeginLoading:(id)sender userData:(void *)userData
{
}

- (void)WCURLHandleResourceDidCancelLoading:(id)sender userData:(void *)userData
{
}

- (void)WCURLHandle:(id)sender resourceDidFailLoadingWithResult:(int)result userData:(void *)userData
{
}

// event methods

-(BOOL)acceptsFirstResponder
{
    return YES;
}

- (BOOL)becomeFirstResponder
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = getFocusEvent;
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    KWQDebug("NPP_HandleEvent(getFocusEvent): %d  when: %u\n", acceptedEvent, event.when);
    return YES;
}

- (BOOL)resignFirstResponder
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = loseFocusEvent;
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    KWQDebug("NPP_HandleEvent(loseFocusEvent): %d  when: %u\n", acceptedEvent, event.when);
    return YES;
}

-(void)sendActivateEvent
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = activateEvt;
    event.message = (UInt32)[self qdPort];
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    KWQDebug("NPP_HandleEvent(activateEvent): %d  when: %u\n", acceptedEvent, event.when);
}

-(void)sendUpdateEvent
{
    EventRecord event;
    bool acceptedEvent;
    UnsignedWide msecs;
    
    event.what = updateEvt;
    event.message = (UInt32)[self qdPort];
    Microseconds(&msecs);
    event.when = (uint32)((double)UnsignedWideToUInt64(msecs) / 1000000 * 60); // microseconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event); 
    KWQDebug("NPP_HandleEvent(updateEvt): %d  when: %u\n", acceptedEvent, event.when);
}

-(void)mouseDown:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    Point pt;
    NSPoint viewPoint;
    NSRect frame;
    
    viewPoint = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    frame = [self frame];
    
    pt.v = (short)viewPoint.y; 
    pt.h = (short)viewPoint.x;
    event.what = mouseDown;
    event.where = pt;
    event.when = (uint32)([theEvent timestamp] * 60); // seconds to ticks
    acceptedEvent = NPP_HandleEvent(instance, &event);
    KWQDebug("NPP_HandleEvent(mouseDown): %d pt.v=%d, pt.h=%d ticks=%u\n", acceptedEvent, pt.v, pt.h, event.when);
}

-(void)mouseUp:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    Point pt;
    NSPoint viewPoint;
    NSRect frame;
    
    viewPoint = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    frame = [self frame];
    
    pt.v = (short)viewPoint.y; 
    pt.h = (short)viewPoint.x;
    event.what = mouseUp;
    event.where = pt;
    event.when = (uint32)([theEvent timestamp] * 60); 
    acceptedEvent = NPP_HandleEvent(instance, &event);
    KWQDebug("NPP_HandleEvent(mouseUp): %d pt.v=%d, pt.h=%d ticks=%u\n", acceptedEvent, pt.v, pt.h, event.when);
}

- (void)mouseDragged:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    Point pt;
    NSPoint viewPoint;
    NSRect frame;
    
    viewPoint = [self convertPoint:[theEvent locationInWindow] fromView:nil];
    frame = [self frame];
    
    pt.v = (short)viewPoint.y; 
    pt.h = (short)viewPoint.x;
    event.what = osEvt;
    event.where = pt;
    event.when = (uint32)([theEvent timestamp] * 60); // seconds to ticks
    event.message = mouseMovedMessage;
    acceptedEvent = NPP_HandleEvent(instance, &event);
    KWQDebug("NPP_HandleEvent(mouseDragged): %d pt.v=%d, pt.h=%d ticks=%u\n", acceptedEvent, pt.v, pt.h, event.when);
}

//FIXME: mouseEntered and mouseExited are not being called for some reason
- (void)mouseEntered:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    KWQDebug("NPP_HandleEvent(mouseEntered)\n");
    if([theEvent trackingNumber] != trackingTag)
        return;
    event.what = adjustCursorEvent;
    event.when = (uint32)([theEvent timestamp] * 60);
    acceptedEvent = NPP_HandleEvent(instance, &event);
    KWQDebug("NPP_HandleEvent(mouseEntered): %dn", acceptedEvent);
}

- (void)mouseExited:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    if([theEvent trackingNumber] != trackingTag)
        return;    
    event.what = adjustCursorEvent;
    event.when = (uint32)([theEvent timestamp] * 60);
    acceptedEvent = NPP_HandleEvent(instance, &event);
    KWQDebug("NPP_HandleEvent(mouseExited): %d\n", acceptedEvent);
}

- (void)keyUp:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    event.what = keyUp;
    event.message = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    event.when = (uint32)([theEvent timestamp] * 60);
    acceptedEvent = NPP_HandleEvent(instance, &event);
    KWQDebug("NPP_HandleEvent(keyUp): %d key:%c\n", acceptedEvent, (event.message & charCodeMask));
    //Note: QT Plug-in doesn't use keyUp's
}

- (void)keyDown:(NSEvent *)theEvent
{
    EventRecord event;
    bool acceptedEvent;
    
    event.what = keyDown;
    event.message = [[theEvent charactersIgnoringModifiers] characterAtIndex:0];
    event.when = (uint32)([theEvent timestamp] * 60);
    acceptedEvent = NPP_HandleEvent(instance, &event);
    KWQDebug("NPP_HandleEvent(keyDown): %d key:%c\n", acceptedEvent, (event.message & charCodeMask));
}

// plug-in to browser calls

-(NPError)getURLNotify:(const char *)url target:(const char *)target notifyData:(void *)notifyData
{
    KWQDebug("NPN_GetURLNotify: %s\n", url);
    if(target == NULL){ // send data to plug-in if target is null
        [self newStream:[NSString stringWithCString:url] mimeType:nil notifyData:(void *)notifyData];
    }
    
    return NPERR_NO_ERROR;
}

-(NPError)getURL:(const char *)url target:(const char *)target
{
    KWQDebug("getURL\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)postURLNotify:(const char *)url target:(const char *)target len:(UInt32)len buf:(const char *)buf file:(NPBool)file notifyData:(void *)notifyData
{
    KWQDebug("postURLNotify\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)postURL:(const char *)url target:(const char *)target len:(UInt32)len buf:(const char *)buf file:(NPBool)file
{
    KWQDebug("postURL\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)newStream:(NPMIMEType)type target:(const char *)target stream:(NPStream**)stream
{
    KWQDebug("newStream\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)write:(NPStream*)stream len:(SInt32)len buffer:(void *)buffer
{
    KWQDebug("write\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)destroyStream:(NPStream*)stream reason:(NPReason)reason
{
    KWQDebug("destroyStream\n");
    return NPERR_GENERIC_ERROR;
}

-(void)status:(const char *)message
{
    KWQDebug("status\n");
}

-(NPError)getValue:(NPNVariable)variable value:(void *)value
{
    KWQDebug("getValue\n");
    return NPERR_GENERIC_ERROR;
}

-(NPError)setValue:(NPPVariable)variable value:(void *)value
{
    KWQDebug("setValue\n");
    return NPERR_GENERIC_ERROR;
}

-(void)invalidateRect:(NPRect *)invalidRect
{
    KWQDebug("invalidateRect\n");
}

-(void)invalidateRegion:(NPRegion)invalidateRegion
{
    KWQDebug("invalidateRegion\n");
}

- (void)stop
{
    NPError npErr;
    
    if (!stopped){
        [eventSender stop];
        [eventSender release];
        npErr = NPP_Destroy(instance, NULL);
        KWQDebug("NPP_Destroy: %d\n", npErr);
        stopped = TRUE;
    }
}

-(void)forceRedraw
{
    KWQDebug("forceRedraw\n");
}

-(void)dealloc
{
    unsigned i;
    NSFileManager *fileManager;
    
    [self stop];
    fileManager = [NSFileManager defaultManager];
    for(i=0; i<[filesToErase count]; i++){
        [fileManager removeFileAtPath:[filesToErase objectAtIndex:i] handler:nil];
    }
    [filesToErase release];
    [super dealloc];
}

@end

NSString* startupVolumeName(void)
{
    NSString* rootName = nil;
    FSRef rootRef;
    if (FSPathMakeRef ((const UInt8 *) "/", & rootRef, NULL /*isDirectory*/) == noErr) {         
        HFSUniStr255  nameString;
        if (FSGetCatalogInfo (&rootRef, kFSCatInfoNone, NULL /*catalogInfo*/, &nameString, NULL /*fsSpec*/, NULL /*parentRef*/) == noErr) {
            rootName = [NSString stringWithCharacters:nameString.unicode length:nameString.length];
        }
    }
    return rootName;
}

