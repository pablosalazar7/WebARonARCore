/*
 * Copyright (C) 2002 Apple Computer, Inc.  All rights reserved.
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

#import "KWQPageState.h"

#import <JavaScriptCore/property_map.h>

#import "dom_docimpl.h"
#import "khtmlview.h"

#import "KWQAssertions.h"
#import "KWQKHTMLPart.h"

using DOM::DocumentImpl;

using KJS::SavedProperties;

@implementation KWQPageState

- initWithDocument:(DocumentImpl *)doc URL:(const KURL &)u windowProperties:(SavedProperties *)wp locationProperties:(SavedProperties *)lp
{
    [super init];
    doc->ref();
    document = doc;
    docRenderer = doc->renderer();
    document->setInPageCache(YES);
    URL = new KURL(u);
    windowProperties = wp;
    locationProperties = lp;
    return self;
}

// Called when the KWQPageState is restored.  It should relinquish ownership
// of objects to core.
- (void)invalidate
{
    // Should only ever invalidate once.
    ASSERT(document);
    
    document->setInPageCache(NO);
    document->deref();
    document = 0;

    delete URL;
    URL = 0;
    
    delete windowProperties;
    windowProperties = 0;
    delete locationProperties;
    locationProperties = 0;
}

- (void)dealloc
{
    if (document) {
        KHTMLView *view = document->view();

        // FIXME: We see evidence that view is nil in backtraces
        // from the field. So we know this assert would sometimes fire.
        // We check for nil below anyway until we figure out the cause.
        ASSERT(view);
        
        KWQKHTMLPart::clearTimers(view);

        document->setInPageCache(NO);
        document->detach();
        document->deref();
        
        if (view) {
            view->clearPart();
            delete view;
        }
    }
    
    delete URL;
    delete windowProperties;
    delete locationProperties;

    [super dealloc];
}

- (DocumentImpl *)document
{
    return document;
}

- (KURL *)URL
{
    return URL;
}

- (SavedProperties *)windowProperties
{
    return windowProperties;
}

- (SavedProperties *)locationProperties
{
    return locationProperties;
}

- (RenderObject *)renderer
{
    return docRenderer;
}

@end
