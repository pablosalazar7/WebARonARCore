/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef StringConcatenate_h
#define StringConcatenate_h

#include "UString.h"

namespace JSC {

template<typename StringType>
class StringTypeAdapter {
};

template<>
class StringTypeAdapter<char*> {
public:
    StringTypeAdapter<char*>(char* buffer)
        : m_buffer((unsigned char*)buffer)
        , m_length(strlen(buffer))
    {
    }

    unsigned length() { return m_length; }

    void writeTo(UChar* destination)
    {
        for (unsigned i = 0; i < m_length; ++i)
            destination[i] = m_buffer[i];
    }

private:
    const unsigned char* m_buffer;
    unsigned m_length;
};

template<>
class StringTypeAdapter<const char*> {
public:
    StringTypeAdapter<const char*>(const char* buffer)
        : m_buffer((unsigned char*)buffer)
        , m_length(strlen(buffer))
    {
    }

    unsigned length() { return m_length; }

    void writeTo(UChar* destination)
    {
        for (unsigned i = 0; i < m_length; ++i)
            destination[i] = m_buffer[i];
    }

private:
    const unsigned char* m_buffer;
    unsigned m_length;
};

template<>
class StringTypeAdapter<UString> {
public:
    StringTypeAdapter<UString>(UString& string)
        : m_data(string.characters())
        , m_length(string.length())
    {
    }

    unsigned length() { return m_length; }

    void writeTo(UChar* destination)
    {
        for (unsigned i = 0; i < m_length; ++i)
            destination[i] = m_data[i];
    }

private:
    const UChar* m_data;
    unsigned m_length;
};

inline void sumWithOverflow(unsigned& total, unsigned addend, bool& overflow)
{
    unsigned oldTotal = total;
    total = oldTotal + addend;
    if (total < oldTotal)
        overflow = true;
}

template<typename StringType1, typename StringType2>
PassRefPtr<StringImpl> tryMakeString(StringType1 string1, StringType2 string2)
{
    StringTypeAdapter<StringType1> adapter1(string1);
    StringTypeAdapter<StringType2> adapter2(string2);

    UChar* buffer;
    bool overflow = false;
    unsigned length = adapter1.length();
    sumWithOverflow(length, adapter2.length(), overflow);
    if (overflow)
        return 0;
    PassRefPtr<StringImpl> resultImpl = StringImpl::tryCreateUninitialized(length, buffer);
    if (!resultImpl)
        return 0;

    UChar* result = buffer;
    adapter1.writeTo(result);
    result += adapter1.length();
    adapter2.writeTo(result);

    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3>
PassRefPtr<StringImpl> tryMakeString(StringType1 string1, StringType2 string2, StringType3 string3)
{
    StringTypeAdapter<StringType1> adapter1(string1);
    StringTypeAdapter<StringType2> adapter2(string2);
    StringTypeAdapter<StringType3> adapter3(string3);

    UChar* buffer = 0;
    bool overflow = false;
    unsigned length = adapter1.length();
    sumWithOverflow(length, adapter2.length(), overflow);
    sumWithOverflow(length, adapter3.length(), overflow);
    if (overflow)
        return 0;
    PassRefPtr<StringImpl> resultImpl = StringImpl::tryCreateUninitialized(length, buffer);
    if (!resultImpl)
        return 0;

    UChar* result = buffer;
    adapter1.writeTo(result);
    result += adapter1.length();
    adapter2.writeTo(result);
    result += adapter2.length();
    adapter3.writeTo(result);

    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4>
PassRefPtr<StringImpl> tryMakeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4)
{
    StringTypeAdapter<StringType1> adapter1(string1);
    StringTypeAdapter<StringType2> adapter2(string2);
    StringTypeAdapter<StringType3> adapter3(string3);
    StringTypeAdapter<StringType4> adapter4(string4);

    UChar* buffer;
    bool overflow = false;
    unsigned length = adapter1.length();
    sumWithOverflow(length, adapter2.length(), overflow);
    sumWithOverflow(length, adapter3.length(), overflow);
    sumWithOverflow(length, adapter4.length(), overflow);
    if (overflow)
        return 0;
    PassRefPtr<StringImpl> resultImpl = StringImpl::tryCreateUninitialized(length, buffer);
    if (!resultImpl)
        return 0;

    UChar* result = buffer;
    adapter1.writeTo(result);
    result += adapter1.length();
    adapter2.writeTo(result);
    result += adapter2.length();
    adapter3.writeTo(result);
    result += adapter3.length();
    adapter4.writeTo(result);

    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5>
PassRefPtr<StringImpl> tryMakeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5)
{
    StringTypeAdapter<StringType1> adapter1(string1);
    StringTypeAdapter<StringType2> adapter2(string2);
    StringTypeAdapter<StringType3> adapter3(string3);
    StringTypeAdapter<StringType4> adapter4(string4);
    StringTypeAdapter<StringType5> adapter5(string5);

    UChar* buffer;
    bool overflow = false;
    unsigned length = adapter1.length();
    sumWithOverflow(length, adapter2.length(), overflow);
    sumWithOverflow(length, adapter3.length(), overflow);
    sumWithOverflow(length, adapter4.length(), overflow);
    sumWithOverflow(length, adapter5.length(), overflow);
    if (overflow)
        return 0;
    PassRefPtr<StringImpl> resultImpl = StringImpl::tryCreateUninitialized(length, buffer);
    if (!resultImpl)
        return 0;

    UChar* result = buffer;
    adapter1.writeTo(result);
    result += adapter1.length();
    adapter2.writeTo(result);
    result += adapter2.length();
    adapter3.writeTo(result);
    result += adapter3.length();
    adapter4.writeTo(result);
    result += adapter4.length();
    adapter5.writeTo(result);

    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6>
PassRefPtr<StringImpl> tryMakeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6)
{
    StringTypeAdapter<StringType1> adapter1(string1);
    StringTypeAdapter<StringType2> adapter2(string2);
    StringTypeAdapter<StringType3> adapter3(string3);
    StringTypeAdapter<StringType4> adapter4(string4);
    StringTypeAdapter<StringType5> adapter5(string5);
    StringTypeAdapter<StringType6> adapter6(string6);

    UChar* buffer;
    bool overflow = false;
    unsigned length = adapter1.length();
    sumWithOverflow(length, adapter2.length(), overflow);
    sumWithOverflow(length, adapter3.length(), overflow);
    sumWithOverflow(length, adapter4.length(), overflow);
    sumWithOverflow(length, adapter5.length(), overflow);
    sumWithOverflow(length, adapter6.length(), overflow);
    if (overflow)
        return 0;
    PassRefPtr<StringImpl> resultImpl = StringImpl::tryCreateUninitialized(length, buffer);
    if (!resultImpl)
        return 0;

    UChar* result = buffer;
    adapter1.writeTo(result);
    result += adapter1.length();
    adapter2.writeTo(result);
    result += adapter2.length();
    adapter3.writeTo(result);
    result += adapter3.length();
    adapter4.writeTo(result);
    result += adapter4.length();
    adapter5.writeTo(result);
    result += adapter5.length();
    adapter6.writeTo(result);

    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6, typename StringType7>
PassRefPtr<StringImpl> tryMakeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6, StringType7 string7)
{
    StringTypeAdapter<StringType1> adapter1(string1);
    StringTypeAdapter<StringType2> adapter2(string2);
    StringTypeAdapter<StringType3> adapter3(string3);
    StringTypeAdapter<StringType4> adapter4(string4);
    StringTypeAdapter<StringType5> adapter5(string5);
    StringTypeAdapter<StringType6> adapter6(string6);
    StringTypeAdapter<StringType7> adapter7(string7);

    UChar* buffer;
    bool overflow = false;
    unsigned length = adapter1.length();
    sumWithOverflow(length, adapter2.length(), overflow);
    sumWithOverflow(length, adapter3.length(), overflow);
    sumWithOverflow(length, adapter4.length(), overflow);
    sumWithOverflow(length, adapter5.length(), overflow);
    sumWithOverflow(length, adapter6.length(), overflow);
    sumWithOverflow(length, adapter7.length(), overflow);
    if (overflow)
        return 0;
    PassRefPtr<StringImpl> resultImpl = StringImpl::tryCreateUninitialized(length, buffer);
    if (!resultImpl)
        return 0;

    UChar* result = buffer;
    adapter1.writeTo(result);
    result += adapter1.length();
    adapter2.writeTo(result);
    result += adapter2.length();
    adapter3.writeTo(result);
    result += adapter3.length();
    adapter4.writeTo(result);
    result += adapter4.length();
    adapter5.writeTo(result);
    result += adapter5.length();
    adapter6.writeTo(result);
    result += adapter6.length();
    adapter7.writeTo(result);

    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6, typename StringType7, typename StringType8>
PassRefPtr<StringImpl> tryMakeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6, StringType7 string7, StringType8 string8)
{
    StringTypeAdapter<StringType1> adapter1(string1);
    StringTypeAdapter<StringType2> adapter2(string2);
    StringTypeAdapter<StringType3> adapter3(string3);
    StringTypeAdapter<StringType4> adapter4(string4);
    StringTypeAdapter<StringType5> adapter5(string5);
    StringTypeAdapter<StringType6> adapter6(string6);
    StringTypeAdapter<StringType7> adapter7(string7);
    StringTypeAdapter<StringType8> adapter8(string8);

    UChar* buffer;
    bool overflow = false;
    unsigned length = adapter1.length();
    sumWithOverflow(length, adapter2.length(), overflow);
    sumWithOverflow(length, adapter3.length(), overflow);
    sumWithOverflow(length, adapter4.length(), overflow);
    sumWithOverflow(length, adapter5.length(), overflow);
    sumWithOverflow(length, adapter6.length(), overflow);
    sumWithOverflow(length, adapter7.length(), overflow);
    sumWithOverflow(length, adapter8.length(), overflow);
    if (overflow)
        return 0;
    PassRefPtr<StringImpl> resultImpl = StringImpl::tryCreateUninitialized(length, buffer);
    if (!resultImpl)
        return 0;

    UChar* result = buffer;
    adapter1.writeTo(result);
    result += adapter1.length();
    adapter2.writeTo(result);
    result += adapter2.length();
    adapter3.writeTo(result);
    result += adapter3.length();
    adapter4.writeTo(result);
    result += adapter4.length();
    adapter5.writeTo(result);
    result += adapter5.length();
    adapter6.writeTo(result);
    result += adapter6.length();
    adapter7.writeTo(result);
    result += adapter7.length();
    adapter8.writeTo(result);

    return resultImpl;
}

template<typename StringType1, typename StringType2>
UString makeString(StringType1 string1, StringType2 string2)
{
    PassRefPtr<StringImpl> resultImpl = tryMakeString(string1, string2);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3>
UString makeString(StringType1 string1, StringType2 string2, StringType3 string3)
{
    PassRefPtr<StringImpl> resultImpl = tryMakeString(string1, string2, string3);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4>
UString makeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4)
{
    PassRefPtr<StringImpl> resultImpl = tryMakeString(string1, string2, string3, string4);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5>
UString makeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5)
{
    PassRefPtr<StringImpl> resultImpl = tryMakeString(string1, string2, string3, string4, string5);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6>
UString makeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6)
{
    PassRefPtr<StringImpl> resultImpl = tryMakeString(string1, string2, string3, string4, string5, string6);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6, typename StringType7>
UString makeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6, StringType7 string7)
{
    PassRefPtr<StringImpl> resultImpl = tryMakeString(string1, string2, string3, string4, string5, string6, string7);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6, typename StringType7, typename StringType8>
UString makeString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6, StringType7 string7, StringType8 string8)
{
    PassRefPtr<StringImpl> resultImpl = tryMakeString(string1, string2, string3, string4, string5, string6, string7, string8);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

} // namespace JSC

#endif
