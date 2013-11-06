/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#include "config.h"
#include "public/web/WebSelector.h"

#include "../platform/WebString.h"
#include "public/platform/WebUnitTestSupport.h"
#include <gtest/gtest.h>

using namespace blink;

namespace {

TEST(WebSelectorTest, Canonicalizes)
{
    EXPECT_EQ("h1, h2[style=\"foobar\"] span", canonicalizeSelector("h1,h2[style='foobar']    span").utf8());
    EXPECT_EQ("h1, h2[style=\"foobar\"] span", canonicalizeSelector("h1, h2[style=\"foobar\"] span").utf8());
}

TEST(WebSelectorTest, Checks)
{
    EXPECT_EQ("", canonicalizeSelector("h1..h2").utf8());
    EXPECT_EQ("", canonicalizeSelector("h1..h2", WebSelectorTypeCompound).utf8());
}

TEST(WebSelectorTest, Restricts)
{
    EXPECT_EQ("", canonicalizeSelector("h1 span,h2", WebSelectorTypeCompound).utf8());
    EXPECT_EQ("h1, h2[style=\"foobar\"].cls", canonicalizeSelector("h1,h2[style=\"foobar\"].cls", WebSelectorTypeCompound).utf8());
}

} // namespace
