/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
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

#import "config.h"
#import "LocalizedStrings.h"

#import "BlockExceptions.h"
#import "PlatformString.h"
#import "WebCoreViewFactory.h"

namespace WebCore {

String inputElementAltText()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [[WebCoreViewFactory sharedFactory] inputElementAltText];
    END_BLOCK_OBJC_EXCEPTIONS;
    return String();
}

String resetButtonDefaultLabel()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [[WebCoreViewFactory sharedFactory] resetButtonDefaultLabel];
    END_BLOCK_OBJC_EXCEPTIONS;
    return String();
}

String searchableIndexIntroduction()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [[WebCoreViewFactory sharedFactory] searchableIndexIntroduction];
    END_BLOCK_OBJC_EXCEPTIONS;
    return String();
}

String submitButtonDefaultLabel()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [[WebCoreViewFactory sharedFactory] submitButtonDefaultLabel];
    END_BLOCK_OBJC_EXCEPTIONS;
    return String();
}

String fileButtonChooseFileLabel()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [[WebCoreViewFactory sharedFactory] fileButtonChooseFileLabel];
    END_BLOCK_OBJC_EXCEPTIONS;
    return String();
}

String fileButtonNoFileSelectedLabel()
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    return [[WebCoreViewFactory sharedFactory] fileButtonNoFileSelectedLabel];
    END_BLOCK_OBJC_EXCEPTIONS;
    return String();
}

}
