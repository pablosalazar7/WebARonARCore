//
//  WebSubresourceClient.h
//  WebKit
//
//  Created by Darin Adler on Sat Jun 15 2002.
//  Copyright (c) 2002 Apple Computer, Inc. All rights reserved.
//

#import <WebFoundation/WebResourceHandleDelegate.h>

@class WebDataSource;
@class WebResourceResponse;
@protocol WebCoreResourceHandle;
@protocol WebCoreResourceLoader;

@interface WebSubresourceClient : NSObject <WebResourceHandleDelegate, WebCoreResourceHandle>
{
    id <WebCoreResourceLoader> loader;
    WebDataSource *dataSource;
    NSURL *currentURL;
    WebResourceHandle *handle;

    // Both of these delegates are retained by the client.
    id <WebResourceLoadDelegate> resourceProgressDelegate;
    WebResourceRequest *request;
    WebResourceResponse *response;
    
    id identifier;
}

+ (WebSubresourceClient *)startLoadingResource:(id <WebCoreResourceLoader>)rLoader
    withURL:(NSURL *)URL referrer:(NSString *)referrer forDataSource:(WebDataSource *)source;

- (WebResourceHandle *)handle;

@end
