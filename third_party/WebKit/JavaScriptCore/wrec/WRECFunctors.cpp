/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WRECFunctors.h"

#if ENABLE(WREC)

#include "WRECGenerator.h"

using namespace WTF;

namespace JSC { namespace WREC {

void GeneratePatternCharacterFunctor::generateAtom(Generator* wrec, JmpSrcVector& failures)
{
    wrec->generatePatternCharacter(failures, m_ch);
}

void GeneratePatternCharacterFunctor::backtrack(Generator* wrec)
{
    wrec->generateBacktrack1();
}

void GenerateCharacterClassFunctor::generateAtom(Generator* wrec, JmpSrcVector& failures)
{
    wrec->generateCharacterClass(failures, *m_charClass, m_invert);
}

void GenerateCharacterClassFunctor::backtrack(Generator* wrec)
{
    wrec->generateBacktrack1();
}

void GenerateBackreferenceFunctor::generateAtom(Generator* wrec, JmpSrcVector& failures)
{
    wrec->generateBackreference(failures, m_subpatternId);
}

void GenerateBackreferenceFunctor::backtrack(Generator* wrec)
{
    wrec->generateBacktrackBackreference(m_subpatternId);
}

void GenerateParenthesesNonGreedyFunctor::generateAtom(Generator* wrec, JmpSrcVector& failures)
{
    wrec->generateParenthesesNonGreedy(failures, m_start, m_success, m_fail);
}

void GenerateParenthesesNonGreedyFunctor::backtrack(Generator*)
{
    // FIXME: do something about this.
    CRASH();
}

} } // namespace JSC::WREC

#endif // ENABLE(WREC)
