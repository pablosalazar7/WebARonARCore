/*
 * Copyright (C) 2001 Apple Computer, Inc.  All rights reserved.
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
#import "KWQView.h"

#import "kwqdebug.h"

#include <khtmlview.h>
#include <qwidget.h>
#include <qpainter.h>
#include <html/html_documentimpl.h>



@implementation KWQView

- initWithFrame: (NSRect) r widget: (QWidget *)w 
{
    [super initWithFrame: r];
    widget = w;
    isFlipped = YES;
    return self;
}


// This should eventually be removed.
- (void)drawRect:(NSRect)rect {
    widget->paint((void *)0);
}

- (void)setIsFlipped: (bool)flag
{
    isFlipped = flag;
}


- (BOOL)isFlipped 
{
    return isFlipped;
}

@end


@implementation KWQNSButton

- initWithFrame: (NSRect) r widget: (QWidget *)w 
{
    [super initWithFrame: r];
    widget = w;
    return self;
}

@end


@implementation KWQNSComboBox

- initWithFrame: (NSRect) r widget: (QWidget *)w 
{
    [super initWithFrame: r];
    widget = w;
    return self;
}

@end


@implementation KWQHTMLView

- initWithFrame: (NSRect) r widget: (QWidget *)w 
{
    NSNotificationCenter *notificationCenter;
    SEL notificationReceivedSEL;

    [super initWithFrame: r];
    widget = w;
    isFlipped = YES;
    needsLayout = YES;
    notificationCenter = [NSNotificationCenter defaultCenter];

    notificationReceivedSEL = @selector(notificationReceived:);
    [notificationCenter addObserver:self
            selector:notificationReceivedSEL name:nil object:nil];
            
    return self;
}

#ifdef DELAY_LAYOUT
- delayLayout: sender
{
    [NSObject cancelPreviousPerformRequestsWithTarget: self selector: @selector(delayLayout:) object: self];
    KWQDEBUG("KWQHTMLView:  delayLayout called\n");
    [self setNeedsLayout: YES];
    [self setNeedsDisplay: YES];
}

-(void)notificationReceived:(NSNotification *)notification
{
    if ([[notification name] rangeOfString: @"uri-fin-"].location == 0){
        KWQDEBUG1("KWQHTMLView: Received notification, %s\n", DEBUG_OBJECT([notification name]));
        [self performSelector:@selector(delayLayout:) withObject:self afterDelay:(NSTimeInterval)0.5];
    }
}
#else
-(void)notificationReceived:(NSNotification *)notification
{
    if ([[notification name] rangeOfString: @"uri-fin-"].location == 0){
        [self setNeedsLayout: YES];
        [self setNeedsDisplay: YES];
    }
}
#endif

- (void)layout
{
    if (((KHTMLView *)widget)->part()->xmlDocImpl() && 
        ((KHTMLView *)widget)->part()->xmlDocImpl()->renderer()){
        if (needsLayout){
            long start = _GetMillisecondsSinceEpoch();
            ((KHTMLView *)widget)->layout(TRUE);
            KWQDEBUGLEVEL1 (0x200, "layout time %d\n", _GetMillisecondsSinceEpoch() - start);
            needsLayout = NO;
        }
    }
}


- (void)setNeedsLayout: (bool)flag
{
    needsLayout = flag;
}

// This should eventually be removed.
- (void)drawRect:(NSRect)rect {
    if (widget != 0l){
        //widget->paint((void *)0);
        
        [self layout];

        QPainter p(widget);    
        
        [self lockFocus];

        long start = _GetMillisecondsSinceEpoch();
        ((KHTMLView *)widget)->drawContents( &p, (int)rect.origin.x, 
                    (int)rect.origin.y, 
                    (int)rect.size.width, 
                    (int)rect.size.height );
        KWQDEBUGLEVEL1 (0x200, "draw time %d\n", _GetMillisecondsSinceEpoch() - start);
        [self unlockFocus];
    }
}

- (void)setIsFlipped: (bool)flag
{
    isFlipped = flag;
}


- (BOOL)isFlipped 
{
    return isFlipped;
}


- (void)resetView 
{
    NSArray *views = [self subviews];
    int count;
    
    count = [views count];
    while (count--){
        KWQDEBUG2("Removing %p %s\n", [views objectAtIndex: 0], DEBUG_OBJECT([[[views objectAtIndex: 0] class] className]));
        [[views objectAtIndex: 0] removeFromSuperviewWithoutNeedingDisplay]; 
    }
    [self setFrameSize: NSMakeSize (0,0)];
}

- (void)setFrame:(NSRect)frameRect
{
    [super setFrame:frameRect];
    [self setNeedsLayout: YES];
}


// FIXME.  This should be replaced.  Ultimately we will use something like:
// [[webView dataSource] setURL: url];
- (void)setURL: (NSString *)urlString
{
    KURL url = [urlString cString];
 
    [self resetView];
    part->openURL (url);
}

- (void)mouseUp: (NSEvent *)event
{
    KWQDEBUGLEVEL1 (0x100, "mouseUp %s\n", DEBUG_OBJECT(event));
}

- (void)mouseDown: (NSEvent *)event
{
    KWQDEBUGLEVEL1 (0x100, "mouseDown %s\n", DEBUG_OBJECT(event));
}

- (void)mouseDragged: (NSEvent *)event
{
    NSPoint p = [event locationInWindow];
    KWQDEBUG2 ("mouseDragged %f, %f\n", p.x, p.y);
}


@end

