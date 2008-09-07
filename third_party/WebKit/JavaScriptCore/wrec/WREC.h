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

#ifndef WREC_h
#define WREC_h

#if ENABLE(WREC)

#include "ustring.h"
#include <masm/MacroAssembler.h>
#include <wtf/ASCIICType.h>
#include <wtf/Vector.h>

namespace KJS {


    typedef int (*WRECFunction)(const UChar* input, unsigned start, unsigned length, int* output) WREC_CALL;

    class ExecState;
    class GenerateAtomFunctor;
    struct CharacterClassRange;
    struct CharacterClass;

    struct Quantifier {
        static const unsigned NoMaxSpecified = UINT_MAX;

        enum Type {
            None,
            Greedy,
            NonGreedy,
            Error,
        } m_type;

        unsigned m_min;
        unsigned m_max;

        Quantifier()
            : m_type(None)
        {
        }

        Quantifier(Type type, unsigned min = 0, unsigned max = NoMaxSpecified)
            : m_type(type)
            , m_min(min)
            , m_max(max)
        {
        }
    };


    class WRECompiler;
    class WRECParser;

    typedef Vector<MacroAssembler::JmpSrc> JmpSrcVector;

    class WRECGenerator {
    public:
        WRECGenerator(WRECParser& parser, MacroAssembler& jit)
            : m_parser(parser)
            , m_jit(jit)
        {
        }

        typedef MacroAssembler::JmpSrc JmpSrc;
        typedef MacroAssembler::JmpDst JmpDst;

        // these regs setup by the params
        static const IA32MacroAssembler::RegisterID INPUT_REG = IA32MacroAssembler::eax;
        static const IA32MacroAssembler::RegisterID CURR_POS_REG = IA32MacroAssembler::edx;
        static const IA32MacroAssembler::RegisterID LENGTH_REG = IA32MacroAssembler::ecx;
        // CURR_VAL_REG used as a temporary, DISJUNCTION_BEGIN_POS_REG holds the start of the current disjunction - which is the start of the whole match, for the top--level dijunction.
        static const IA32MacroAssembler::RegisterID CURR_VAL_REG = IA32MacroAssembler::esi;
        static const IA32MacroAssembler::RegisterID OUTPUT_REG = IA32MacroAssembler::edi;
        static const IA32MacroAssembler::RegisterID QUANTIFIER_COUNT_REG = IA32MacroAssembler::ebx;

        friend class GenerateAtomFunctor;
        friend class GeneratePatternCharacterFunctor;
        friend class GenerateCharacterClassFunctor;
        friend class GenerateBackreferenceFunctor;
        friend class GenerateParenthesesNonGreedyFunctor;

        void generateGreedyQuantifier(JmpSrcVector& failures, GenerateAtomFunctor& functor, unsigned min, unsigned max);
        void generateNonGreedyQuantifier(JmpSrcVector& failures, GenerateAtomFunctor& functor, unsigned min, unsigned max);
        void generateBacktrack1();
        void generateBacktrackBackreference(unsigned subpatternId);
        void generateCharacterClass(JmpSrcVector& failures, CharacterClass& charClass, bool invert);
        void generateCharacterClassInverted(JmpSrcVector& failures, CharacterClass& charClass);
        void generateCharacterClassInvertedRange(JmpSrcVector& failures, JmpSrcVector& matchDest, const CharacterClassRange* ranges, unsigned count, unsigned* matchIndex, const UChar* matches, unsigned matchCount);
        void generatePatternCharacter(JmpSrcVector& failures, int ch);
        void generateAssertionWordBoundary(JmpSrcVector& failures, bool invert);
        void generateAssertionBOL(JmpSrcVector& failures);
        void generateAssertionEOL(JmpSrcVector& failures);
        void generateBackreference(JmpSrcVector& failures, unsigned subpatternID);
        void generateBackreferenceQuantifier(JmpSrcVector& failures, Quantifier::Type quantifierType, unsigned subpatternId, unsigned min, unsigned max);
        enum ParenthesesType { capturing, non_capturing, assertion, inverted_assertion }; // order is relied on in generateParentheses()
        JmpSrc generateParentheses(ParenthesesType type);
        JmpSrc gererateParenthesesResetTrampoline(JmpSrcVector& newFailures, unsigned subpatternIdBefore, unsigned subpatternIdAfter);
        void generateParenthesesNonGreedy(JmpSrcVector& failures, JmpDst start, JmpSrc success, JmpSrc fail);

        void gernerateDisjunction(JmpSrcVector& successes, JmpSrcVector& failures);
        void terminateDisjunction(JmpSrcVector& successes);

    private:
        WRECParser& m_parser;
        MacroAssembler& m_jit;
    };

    class WRECParser {
    public:
        bool m_ignoreCase;
        bool m_multiline;
        unsigned m_numSubpatterns;
        enum WRECError {
            NoError,
            Error_malformedCharacterClass,
            Error_malformedParentheses,
            Error_malformedPattern,
            Error_malformedQuantifier,
            Error_malformedEscape,
            TempError_unsupportedQuantifier,
            TempError_unsupportedParentheses,
        } m_err;

        WRECParser(const UString& pattern, bool ignoreCase, bool multiline, MacroAssembler& jit)
            : m_ignoreCase(ignoreCase)
            , m_multiline(multiline)
            , m_numSubpatterns(0)
            , m_err(NoError)
            , m_generator(*this, jit)
            , m_data(pattern.data())
            , m_size(pattern.size())
            , m_index(0)
        {
        }

        void parseAlternative(JmpSrcVector& failures)
        {
            while (parseTerm(failures)) { }
        }

        void parseDisjunction(JmpSrcVector& failures);

        bool parseTerm(JmpSrcVector& failures);
        bool parseEscape(JmpSrcVector& failures);
        bool parseOctalEscape(JmpSrcVector& failures);
        bool parseParentheses(JmpSrcVector& failures);
        bool parseCharacterClass(JmpSrcVector& failures);
        bool parseCharacterClassQuantifier(JmpSrcVector& failures, CharacterClass& charClass, bool invert);
        bool parsePatternCharacterQualifier(JmpSrcVector& failures, int ch);
        bool parseBackreferenceQuantifier(JmpSrcVector& failures, unsigned subpatternId);

        ALWAYS_INLINE Quantifier parseGreedyQuantifier();
        Quantifier parseQuantifier();

        static const int EndOfPattern = -1;

        int peek()
        {
            if (m_index >= m_size)
                return EndOfPattern;
            return m_data[m_index];
        }

        int consume()
        {
            if (m_index >= m_size)
                return EndOfPattern;
            return m_data[m_index++];
        }

        bool peekIsDigit()
        {
            return WTF::isASCIIDigit(peek());
        }

        unsigned peekDigit()
        {
            ASSERT(peekIsDigit());
            return peek() - '0';
        }

        unsigned consumeDigit()
        {
            ASSERT(peekIsDigit());
            return consume() - '0';
        }

        unsigned consumeNumber()
        {
            int n = consumeDigit();
            while (peekIsDigit()) {
                n *= 10;
                n += consumeDigit();
            }
            return n;
        }

        int consumeHex(int count)
        {
            int n = 0;
            while (count--) {
                if (!WTF::isASCIIHexDigit(peek()))
                    return -1;
                n = (n<<4) | WTF::toASCIIHexValue(consume());
            }
            return n;
        }

        unsigned consumeOctal()
        {
            unsigned n = 0;
            while (n < 32 && WTF::isASCIIOctalDigit(peek()))
                n = n * 8 + (consume() - '0');
            return n;
        }

        bool isEndOfPattern()
        {
            return peek() != EndOfPattern;
        }
        
    private:
        WRECGenerator m_generator;
        const UChar* m_data;
        unsigned m_size;
        unsigned m_index;
    };

} // namespace KJS

#endif // ENABLE(WREC)

#endif // WREC_h
