/*	
        WebFrame.h
	    
	    Copyright 2001, Apple, Inc. All rights reserved.

        Public header file.
*/

#import <Foundation/Foundation.h>

@class WebController;
@class WebDataSource;
@class WebError;
@class WebFramePrivate;
@class WebView;
@class WebResourceRequest;

/*!
    @class WebFrame
    @discussion Every web page is represented by at least one WebFrame.  A WebFrame
    has a WebView and a WebDataSource.
*/
@interface WebFrame : NSObject
{
@private
    WebFramePrivate *_private;
}

/*!
    @method initWithName:webView:controller:
    @abstract The designated initializer of WebFrame.
    @param name The name of the frame.
    @param view The view for the frame.
    @param controller The controller that manages the frame.
    @result Returns an initialized WebFrame.
*/
- initWithName: (NSString *)name webView: (WebView *)view controller: (WebController *)controller;

/*!
    @method name
    @result The frame name.
*/
- (NSString *)name;

/*!
    @method setController:
    @param controller The controller to assign to this frame.
*/
- (void)setController: (WebController *)controller;

/*!
    @method controller
    @result Returns the controller of this frame.
*/
- (WebController *)controller;

/*!
    @method setWebView:
    @param view The view to assign to this frame.
*/
- (void)setWebView: (WebView *)view;

/*!
    @method webView
    @result The WebView for this frame.
*/
- (WebView *)webView;

/*!
    @method loadRequest:
    @param request The web request to load.
*/
-(void)loadRequest:(WebResourceRequest *)request;

/*!
    @method dataSource
    @discussion Returns the committed data source.  Will return nil if the
    provisional data source hasn't yet been loaded.
    @result The datasource for this frame.
*/
- (WebDataSource *)dataSource;

/*!
    @method provisionalDataSource
    @discussion Will return the provisional data source.  The provisional data source will
    be nil if no data source has been set on the frame, or the data source
    has successfully transitioned to the committed data source.
    @result The provisional datasource of this frame.
*/
- (WebDataSource *)provisionalDataSource;

/*!
    @method stopLoading
    @discussion Stop any pending loads on the frame's data source,
    and its children.
*/
- (void)stopLoading;

/*!
    @method reload
*/
- (void)reload;

/*!
    @method findFrameNamed:
    @discussion This method returns a frame with the given name. findFrameNamed returns self 
    for _self and _current, the parent frame for _parent and the main frame for _top. 
    findFrameNamed returns self for _parent and _top if the receiver is the mainFrame.
    findFrameNamed first searches from the current frame to all descending frames then the
    rest of the frames in the controller. If still not found, findFrameNamed searches the
    frames of the other controllers.
    @param name The name of the frame to find.
    @result The frame matching the provided name. nil if the frame is not found.
*/
- (WebFrame *)findFrameNamed:(NSString *)name;

/*!
    @method findOrCreateFrameNamed:
    @discussion This method calls findFrameNamed but if no frame is found, a new window is opened
    and the main frame of the new window is named with the given name.
    @param name The name of the frame to find.
    @result The frame matching the provided name. Won't be nil.
*/
- (WebFrame *)findOrCreateFrameNamed:(NSString *)name;

/*!
    @method parent
    @result The frame containing this frame, or nil if this is a top level frame.
*/
- (WebFrame *)parent;

/*!
    @method children
    @discussion The frames in the array are associated with a frame set or iframe.
    @result Returns an array of WebFrame.
*/
- (NSArray *)children;


@end
