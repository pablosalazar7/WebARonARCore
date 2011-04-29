/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// FIXME: Avoid this source dependency on Chromium's base module.
#include <base/test/test_suite.h>

#include "WebKit.h"
#include "WebKitClient.h"
#include <webkit/support/webkit_support.h>

#if defined(WEBKIT_DLL_UNITTEST)
#include "WebUnitTests.h"
#endif

#include <gmock/gmock.h>

int main(int argc, char** argv)
{
    ::testing::InitGoogleMock(&argc, argv);
    TestSuite testSuite(argc, argv);
    // TestSuite must be created before SetUpTestEnvironment so it performs
    // initializations needed by WebKit support.
    webkit_support::SetUpTestEnvironmentForUnitTests();

#if defined(WEBKIT_DLL_UNITTEST)
    // For chromium multi-dll build, need to call webkit api to create a
    // TestSuite instance in webkit.dll and run all tests from there.
    int result = WebKit::RunAllUnitTests(argc, argv);
#else
    int result = testSuite.Run();
#endif

    webkit_support::TearDownTestEnvironment();
    return result;
}
