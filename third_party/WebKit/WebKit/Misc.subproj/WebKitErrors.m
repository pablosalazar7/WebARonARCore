/*
 *  WebKitErrors.m
 *  WebKit
 *
 *  Created by Chris Blumenberg on Wed Feb 12 2003.
 *  Copyright (c) 2003 Apple Computer, Inc. All rights reserved.
 *
 */

#import <WebKit/WebKitErrors.h>

#import <WebFoundation/WebLocalizableStrings.h>
#import <WebFoundation/WebNSErrorExtras.h>

#import <pthread.h>

NSString *WebKitErrorDomain = @"WebKitErrorDomain";

// Download and file I/O errors
#define WebKitErrorDescriptionCannotCreateFile UI_STRING("Cannot create file", "WebKitErrorCannotCreateFile description")
#define WebKitErrorDescriptionCannotOpenFile UI_STRING("Cannot open file", "WebKitErrorCannotOpenFile description")
#define WebKitErrorDescriptionCannotCloseFile UI_STRING("Failure occurred while closing file", "WebKitErrorCannotCloseFile description")
#define WebKitErrorDescriptionCannotWriteToFile UI_STRING("Cannot write file", "WebKitErrorCannotWriteToFile description")
#define WebKitErrorDescriptionCannotRemoveFile UI_STRING("Cannot remove file", "WebKitErrorCannotRemoveFile description")
#define WebKitErrorDescriptionCannotMoveFile UI_STRING("Cannot move file", "WebKitErrorCannotMoveFile description")
#define WebKitErrorDescriptionDownloadDecodingFailedToComplete UI_STRING_KEY("Download decoding failed", "Download decoding failed (at end)", "WebKitErrorDownloadDecodingFailedToComplete description")
#define WebKitErrorDescriptionDownloadDecodingFailedMidStream UI_STRING_KEY("Download decoding failed", "Download decoding failed (midstream)", "WebKitErrorDownloadDecodingFailedMidStream description")

// Policy errors
#define WebKitErrorDescriptionCannotShowMIMEType UI_STRING("Cannot show content with specified mime type", "WebKitErrorCannotShowMIMEType description")
#define WebKitErrorDescriptionCannotShowURL UI_STRING("Cannot show URL", "WebKitErrorCannotShowURL description")
#define WebKitErrorDescriptionLocationChangeInterruptedByPolicyChange UI_STRING("Location change interrupted", "WebKitErrorLocationChangeInterruptedByPolicyChange description")

// FIXME: Delete this error once localization is unfrozen. It isn't used.
#define WebKitErrorDescriptionResourceLoadInterruptedByPolicyChange UI_STRING("Resource load interrupted", "WebKitErrorResourceLoadInterruptedByPolicyChange description")

// Plug-in and java errors
#define WebKitErrorDescriptionCannotFindPlugin UI_STRING("Cannot find plug-in", "WebKitErrorCannotFindPlugin description")
#define WebKitErrorDescriptionCannotLoadPlugin UI_STRING("Cannot load plug-in", "WebKitErrorCannotLoadPlugin description")
#define WebKitErrorDescriptionJavaUnavailable UI_STRING("Java is unavailable", "WebKitErrorJavaUnavailable description")


static pthread_once_t registerErrorsControl = PTHREAD_ONCE_INIT;
static void registerErrors(void);

@implementation NSError (WebKitExtras)

+ (void)_registerWebKitErrors
{
    pthread_once(&registerErrorsControl, registerErrors);
}

+ (NSError *)_webKitErrorWithCode:(int)code failingURL:(NSString *)URL
{
    [self _registerWebKitErrors];

    return [self _web_errorWithDomain:WebKitErrorDomain code:code failingURL:URL];
}


@end

static void registerErrors()
{
    NSAutoreleasePool *pool;

    pool = [[NSAutoreleasePool alloc] init];

    NSDictionary *dict = [NSDictionary dictionaryWithObjectsAndKeys:
        // Download and file I/O errors
        WebKitErrorDescriptionCannotCreateFile, 		[NSNumber numberWithInt: WebKitErrorCannotCreateFile],
        WebKitErrorDescriptionCannotOpenFile, 			[NSNumber numberWithInt: WebKitErrorCannotOpenFile],
        WebKitErrorDescriptionCannotCloseFile, 			[NSNumber numberWithInt: WebKitErrorCannotCloseFile],
        WebKitErrorDescriptionCannotWriteToFile, 		[NSNumber numberWithInt: WebKitErrorCannotWriteToFile],
        WebKitErrorDescriptionCannotRemoveFile, 		[NSNumber numberWithInt: WebKitErrorCannotRemoveFile],
        WebKitErrorDescriptionCannotMoveFile, 			[NSNumber numberWithInt: WebKitErrorCannotMoveFile],
        WebKitErrorDescriptionDownloadDecodingFailedToComplete,	[NSNumber numberWithInt: WebKitErrorDownloadDecodingFailedToComplete],
        WebKitErrorDescriptionDownloadDecodingFailedMidStream, 	[NSNumber numberWithInt: WebKitErrorDownloadDecodingFailedMidStream],

        // Policy errors
        WebKitErrorDescriptionCannotShowMIMEType, 		[NSNumber numberWithInt: WebKitErrorCannotShowMIMEType],
        WebKitErrorDescriptionCannotShowURL, 			[NSNumber numberWithInt: WebKitErrorCannotShowURL],
        WebKitErrorDescriptionLocationChangeInterruptedByPolicyChange, 	[NSNumber numberWithInt: WebKitErrorLocationChangeInterruptedByPolicyChange],

        // Plug-in and java errors
        WebKitErrorDescriptionCannotFindPlugin,		[NSNumber numberWithInt: WebKitErrorCannotFindPlugin],
        WebKitErrorDescriptionCannotLoadPlugin,		[NSNumber numberWithInt: WebKitErrorCannotLoadPlugin],
        WebKitErrorDescriptionJavaUnavailable,		[NSNumber numberWithInt: WebKitErrorJavaUnavailable],
        nil];

    [NSError _web_addErrorsWithCodesAndDescriptions:dict inDomain:WebKitErrorDomain];

    [pool release];
}
