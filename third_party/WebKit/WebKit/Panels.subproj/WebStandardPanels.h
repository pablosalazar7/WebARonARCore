/*	
        WebStandardPanels.h
        Copyright 2002 Apple, Inc. All rights reserved.
        
        Public header file.
*/

#import <Cocoa/Cocoa.h>

@class WebStandardPanelsPrivate;

/*!
    @class WebStandardPanels
    @discussion This class allows the use of standard, WebKit-provided
    panels for particular UI that requires panel interaction. The only
    one available currently is authentication.
*/
@interface WebStandardPanels : NSObject
{
@private
    WebStandardPanelsPrivate *_privatePanels;
}

/*!
    @method sharedStandardPanels
    @abstract Get the shared singleton standard panels object
    @result The shared standard panels instance
*/
+(WebStandardPanels *)sharedStandardPanels;

/*!
    @method setUsesStandardAuthenticationPanel:
    @abstract Request that the standard authentication panel be used or not used.
    @param use YES if the standard authentication panel should be used, NO otherwise
*/
-(void)setUsesStandardAuthenticationPanel:(BOOL)use;

/*!
    @method usesStandardAuthenticationPanel
    @abstract Determine whether the standard authentication panel will be used or not used.
    @result YES if the standard authentication panel should be used, NO otherwise
*/
-(BOOL)usesStandardAuthenticationPanel;

/*!
    @method didStartLoadingURL:inWindow:
    @abstract Notify the standard panel mechanism of the start of a resource load
    @param URL The URL that is starting to load
    @param window The window associated with the load.
    @discussion This is only needed for resources that are not being
    loaded via WebKit. WebKit takes care of this bookkeeping
    automatically for resources that it loads.
*/
-(void)didStartLoadingURL:(NSURL *)URL inWindow:(NSWindow *)window;

/*!
    @method didStopLoadingURL:inWindow:
    @abstract Notify the standard panel mechanism of the end of a resource load
    @param URL The URL that finished loading
    @param window The window associated with the load.
    @discussion This is only needed for resources that are not being
    loaded via WebKit. WebKit takes care of this bookkeeping
    automatically for resources that it loads.
*/
-(void)didStopLoadingURL:(NSURL *)URL inWindow:(NSWindow *)window;

/*!
    @method frontmostWindowLoadingURL:
    @abstract Get the frontmost window loading a URL
    @param URL The URL in question
    @result The frontmost window loading the given URL
    @discussion This may be useful for clients that want to implement their
    own custom panels for special purposes.
*/
-(NSWindow *)frontmostWindowLoadingURL:(NSURL *)URL;

@end
