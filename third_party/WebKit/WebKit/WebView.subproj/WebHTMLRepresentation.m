/*	
        WebHTMLRepresentation.m
	Copyright 2002, Apple, Inc. All rights reserved.
*/

#import <WebKit/WebHTMLRepresentation.h>

#import <WebKit/WebDataSource.h>
#import <WebKit/WebBridge.h>
#import <WebKit/WebKitStatisticsPrivate.h>
#import <WebKit/WebFramePrivate.h>
#import <WebFoundation/WebResourceResponse.h>

@interface WebHTMLRepresentationPrivate : NSObject
{
@public
    WebBridge *bridge;
}
@end

@implementation WebHTMLRepresentationPrivate
@end

@implementation WebHTMLRepresentation

- init
{
    [super init];
    
    _private = [[WebHTMLRepresentationPrivate alloc] init];
    
    ++WebHTMLRepresentationCount;
    
    return self;
}

- (void)dealloc
{
    --WebHTMLRepresentationCount;
    
    [_private release];

    [super dealloc];
}

- (WebBridge *)_bridge
{
    return _private->bridge;
}

- (void)setDataSource:(WebDataSource *)dataSource
{
    _private->bridge = [[dataSource webFrame] _bridge];
    [_private->bridge setContentType: [[dataSource response] contentType]];
}

- (void)receivedData:(NSData *)data withDataSource:(WebDataSource *)dataSource
{
    if ([dataSource webFrame])
        [_private->bridge receivedData:data withDataSource:dataSource];
}

- (void)receivedError:(WebError *)error withDataSource:(WebDataSource *)dataSource
{
}

- (void)finishedLoadingWithDataSource:(WebDataSource *)dataSource
{
}

- (NSString *)documentSource
{
    return [WebBridge stringWithData:[[_private->bridge dataSource] data] textEncoding:[_private->bridge textEncoding]];
}


- (id<WebDOMDocument>)DOMDocument
{
    return [_private->bridge DOMDocument];
}

- (void)setSelectionFrom:(id<WebDOMNode>)start startOffset:(int)startOffset to:(id<WebDOMNode>)end endOffset:(int) endOffset
{
}

- (NSString *)reconstructedDocumentSource
{
    // FIXME implement
    return @"";
}

- (NSAttributedString *)attributedText
{
    // FIXME:  Implement
    return nil;
}

- (NSAttributedString *)attributedStringFrom: (id<WebDOMNode>)startNode startOffset: (int)startOffset to: (id<WebDOMNode>)endNode endOffset: (int)endOffset
{
    return [_private->bridge attributedStringFrom: startNode startOffset: startOffset to: endNode endOffset: endOffset];
}

@end
