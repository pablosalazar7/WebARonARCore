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

// FIXME: obviously many functions here can be made inline

#include <Foundation/Foundation.h>
#include <qstring.h>
#include <qregexp.h>
#include <stdio.h>

#ifndef USING_BORROWED_QSTRING

// QString class ===============================================================

// constants -------------------------------------------------------------------

const QString QString::null;

// static member functions -----------------------------------------------------

QString QString::number(int n)
{
    QString qs;
    qs.setNum(n);
    return qs;
}

QString QString::number(uint n)
{
    QString qs;
    qs.setNum(n);
    return qs;
}

QString QString::number(long n)
{
    QString qs;
    qs.setNum(n);
    return qs;
}

QString QString::number(ulong n)
{
    QString qs;
    qs.setNum(n);
    return qs;
}

QString QString::number(double n)
{
    QString qs;
    qs.setNum(n);
    return qs;
}

QString QString::fromLatin1(const char *chs)
{
    return QString(chs);
}

#ifdef USING_BORROWED_KURL
QString QString::fromLocal8Bit(const char *chs, int len)
{
    QString qs;
    if (chs && *chs) {
        qs.s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        if (qs.s) {
            if (len < 0) {
                // append null-terminated string
                // FIXME: is MacRoman the correct encoding?
                CFStringAppendCString(qs.s, chs, kCFStringEncodingMacRoman);
            } else {
                // append length-specified string
                // FIXME: this uses code similar to that in the
                // "QString(const QByteArray &)" constructor and could possibly
                // be refactored
                const int capacity = 64;
                UniChar buf[capacity];
                int fill = 0;
                for (uint i = 0; (i < len) && chs[i]; i++) {
                    buf[fill] = chs[i];
                    fill++;
                    if (fill == capacity) {
                        CFStringAppendCharacters(qs.s, buf, fill);
                        fill = 0;
                    }
                }
                // append any remainder in buffer
                if (fill) {
                    CFStringAppendCharacters(qs.s, buf, fill);
                }
            }
        }
    }
    return qs;
}
#endif // USING_BORROWED_KURL

// constructors, copy constructors, and destructors ----------------------------

QString::QString()
{
    s = NULL;
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(QChar qc)
{
    s = CFStringCreateMutable(kCFAllocatorDefault, 0);
    if (s) {
        // NOTE: this only works since our QChar implementation contains a
        // single UniChar data member
        CFStringAppendCharacters(s, &qc.c, 1);
    }
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(const QByteArray &qba)
{
    if (qba.size() && *qba.data()) {
        s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        if (s) {
            // FIXME: this uses code similar to that in the "fromLocal8Bit"
            // function and could possibly be refactored
            const int capacity = 64;
            UniChar buf[capacity];
            int fill = 0;
            for (uint len = 0; (len < qba.size()) && qba[len]; len++) {
                buf[fill] = qba[len];
                fill++;
                if (fill == capacity) {
                    CFStringAppendCharacters(s, buf, fill);
                    fill = 0;
                }
            }
            // append any remainder in buffer
            if (fill) {
                CFStringAppendCharacters(s, buf, fill);
            }
        }
    } else {
        s = NULL;
    }
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(const QChar *qcs, uint len)
{
    if (qcs || len) {
        s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        if (s) {
            CFStringAppendCharacters(s, &qcs->c, len);
        }
    } else {
        s = NULL;
    }
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(const char *chs)
{
    if (chs && *chs) {
        s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        if (s) {
            CFStringAppendCString(s, chs, kCFStringEncodingISOLatin1);
        }
    } else {
        s = NULL;
    }
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::QString(const QString &qs)
{
    // shared copy
    if (qs.s) {
        CFRetain(qs.s);
    }
    s = qs.s;
    cache = NULL;
    cacheType = CacheInvalid;
}

QString::~QString()
{
    if (s) {
        CFRelease(s);
    }
    if (cache) {
        CFAllocatorDeallocate(kCFAllocatorDefault, cache);
    }
}

// assignment operators --------------------------------------------------------

QString &QString::operator=(const QString &qs)
{
    // shared copy
    if (qs.s) {
        CFRetain(qs.s);
    }
    if (s) {
        CFRelease(s);
    }
    s = qs.s;
    cacheType = CacheInvalid;
    return *this;
}

QString &QString::operator=(const QCString &qcs)
{
    return *this = QString(qcs);
}

QString &QString::operator=(const char *chs)
{
    return *this = QString(chs);
}

QString &QString::operator=(QChar qc)
{
    return *this = QString(qc);
}

QString &QString::operator=(char ch)
{
    return *this = QString(QChar(ch));
}

// member functions ------------------------------------------------------------

uint QString::length() const
{
    return s ? CFStringGetLength(s) : 0;
}

const QChar *QString::unicode() const
{
    UniChar *ucs = NULL;
    uint len = length();
    if (len) {
        ucs = const_cast<UniChar *>(CFStringGetCharactersPtr(s));
        if (!ucs) {
#ifdef _KWQ_DEBUG_
            NSLog(@"WARNING %s:%s:%d (CFStringGetCharactersPtr failed)\n",
                    __FILE__, __FUNCTION__, __LINE__);
#endif
            if (cacheType != CacheUnicode) {
                flushCache();
                cache = CFAllocatorAllocate(kCFAllocatorDefault,
                        len * sizeof (UniChar), 0);
                if (cache) {
                    CFStringGetCharacters(s, CFRangeMake(0, len), cache);
                    cacheType = CacheUnicode;
                }
            }
            ucs = cache;
        }
    }
    // NOTE: this only works since our QChar implementation contains a single
    // UniChar data member
    return reinterpret_cast<const QChar *>(ucs); 
}

const char *QString::latin1() const
{
    char *chs = NULL;
    uint len = length();
    if (len) {
        chs = const_cast<char *>(CFStringGetCStringPtr(s,
                    kCFStringEncodingISOLatin1));
        if (!chs) {
#ifdef _KWQ_DEBUG_
            NSLog(@"WARNING %s:%s:%d (CFStringGetCharactersPtr failed)\n",
                    __FILE__, __FUNCTION__, __LINE__);
#endif
            if (cacheType != CacheLatin1) {
                flushCache();
                cache = CFAllocatorAllocate(kCFAllocatorDefault, len + 1, 0);
                if (cache) {
                    if (!CFStringGetCString(s, cache, len + 1,
                                kCFStringEncodingISOLatin1)) {
#ifdef _KWQ_DEBUG_
                        NSLog(@"WARNING %s:%s:%d (CFStringGetCString failed)\n",
                                __FILE__, __FUNCTION__, __LINE__);
#endif
                        *reinterpret_cast<char *>(cache) = '\0';
                    }
                    cacheType = CacheLatin1;
                }
            }
            chs = cache;
        }
    }
    // always return a valid pointer
    if (!chs) {
        static char emptyString[] = "";
        chs = emptyString;
    }
    return chs; 
}

const char *QString::ascii() const
{
    return latin1(); 
}

QCString QString::utf8() const
{
    return convertToQCString(kCFStringEncodingUTF8);
}

QCString QString::local8Bit() const
{
    return convertToQCString(kCFStringEncodingMacRoman);
}

bool QString::isNull() const
{
    // NOTE: do NOT use "unicode() == NULL"
    return s == NULL;
}

bool QString::isEmpty() const
{
    return length() == 0;
}

#ifdef USING_BORROWED_KURL
QChar QString::at(uint i) const
{
    return operator[](i);
}
#endif // USING_BORROWED_KURL

int QString::compare(const QString &qs) const
{
    if (s == qs.s) {
        return kCFCompareEqualTo;
    }
    if (!s) {
        return kCFCompareLessThan;
    }
    if (!qs.s) {
        return kCFCompareGreaterThan;
    }
    return CFStringCompare(s, qs.s, 0);
}

bool QString::startsWith(const QString &qs) const
{
    if (s && qs.s) {
        return CFStringHasPrefix(s, qs.s);
    }
    return FALSE;
}

int QString::find(QChar qc, int index) const
{
    if (s && qc.c) {
        CFIndex len = CFStringGetLength(s);
        if (index < 0) {
            index += len;
        }
        if (len && (index >= 0) && (index < len)) {
            CFStringInlineBuffer buf;
            CFStringInitInlineBuffer(s, &buf, CFRangeMake(0, len));
            for (CFIndex i = index; i < len; i++) {
                if (qc.c == CFStringGetCharacterFromInlineBuffer(&buf, i)) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int QString::find(char ch, int index) const
{
    return find(QChar(ch), index);
}

int QString::find(const QString &qs, int index) const
{
    if (s && qs) {
        CFIndex len = CFStringGetLength(s);
        if (index < 0) {
            index += len;
        }
        if (len && (index >= 0) && (index < len)) {
            CFRange r = CFStringFind(s, qs.s, 0);
            if (r.location != kCFNotFound) {
                return r.location;
            }
        }
    }
    return -1;
}

int QString::find(const char *chs, int index, bool cs) const
{
    int pos = -1;
    if (s && chs) {
        CFIndex len = CFStringGetLength(s);
        if (index < 0) {
            index += len;
        }
        if (len && (index >= 0) && (index < len)) {
            CFStringRef tmp = CFStringCreateWithCStringNoCopy(
                    kCFAllocatorDefault, chs, kCFStringEncodingISOLatin1,
                    kCFAllocatorNull);
            if (tmp) {
                CFRange r;
                if (CFStringFindWithOptions(s, tmp,
                        CFRangeMake(index, len - index),
                        cs ? 0 : kCFCompareCaseInsensitive, &r)) {
                    pos = r.location;
                }
                CFRelease(tmp);
            }
        }
    }
    return pos;
}

int QString::find(const QRegExp &qre, int index) const
{
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (index < 0) {
            index += len;
        }
        if (len && (index >= 0) && (index < len)) {
            return qre.match(*this, index);
        }
    }
    return -1;
}

int QString::findRev(char ch, int index) const
{
    if (s && ch) {
        CFIndex len = CFStringGetLength(s);
        if (index < 0) {
            index += len;
        }
        if (len && (index <= len)) {
            CFStringInlineBuffer buf;
            CFStringInitInlineBuffer(s, &buf, CFRangeMake(0, index));
            for (CFIndex i = index; i >= 0; i--) {
                if (ch == CFStringGetCharacterFromInlineBuffer(&buf, i)) {
                    return i;
                }
            }
        }
    }
    return -1;
}

int QString::findRev(const char *chs, int index) const
{
    int pos = -1;
    if (s && chs) {
        CFIndex len = CFStringGetLength(s);
        if (index < 0) {
            index += len;
        }
        if (len && (index <= len)) {
            CFStringRef tmp = CFStringCreateWithCStringNoCopy(
                    kCFAllocatorDefault, chs, kCFStringEncodingISOLatin1,
                    kCFAllocatorNull);
            if (tmp) {
                CFRange r;
                if (CFStringFindWithOptions(s, tmp, CFRangeMake(0, index),
                        kCFCompareBackwards, &r)) {
                    pos = r.location;
                }
                CFRelease(tmp);
            }
        }
    }
    return pos;
}

int QString::contains(char ch) const
{
    int c = 0;
    if (s && ch) {
        CFIndex len = CFStringGetLength(s);
        if (len) {
            CFStringInlineBuffer buf;
            CFStringInitInlineBuffer(s, &buf, CFRangeMake(0, len));
            for (CFIndex i = 0; i < len; i++) {
                if (ch == CFStringGetCharacterFromInlineBuffer(&buf, i)) {
                    c++;
                }
            }
        }
    }
    return c;
}

int QString::contains(const char *chs, bool cs) const
{
    int c = 0;
    if (s && chs) {
        CFStringRef tmp = CFStringCreateWithCStringNoCopy(
                kCFAllocatorDefault, chs, kCFStringEncodingISOLatin1,
                kCFAllocatorNull);
        if (tmp) {
            CFIndex pos = 0;
            CFIndex len = CFStringGetLength(s);
            while (pos < len) {
                CFRange r;
                if (!CFStringFindWithOptions(s, tmp,
                        CFRangeMake(pos, len - pos),
                        cs ? 0 : kCFCompareCaseInsensitive, &r)) {
                    break;
                }
                c++;
                // move to next possible overlapping match
                pos += r.location + 1;
            }
            CFRelease(tmp);
        }
    }
    return c;
}

#ifdef USING_BORROWED_KURL
ushort QString::toUShort() const
{
    return toUInt();
}
#endif // USING_BORROWED_KURL

int QString::toInt(bool *ok, int base) const
{
    return toLong(ok, base);
}

uint QString::toUInt(bool *ok) const
{
    // FIXME: this uses code similar to that in the "toLong" function and could
    // possibly be refactored
    uint n = 0;
    bool valid = FALSE;
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len) {
            CFStringInlineBuffer buf;
            UniChar uc;
            CFCharacterSetRef wscs =
                CFCharacterSetGetPredefined(
                        kCFCharacterSetWhitespaceAndNewline);
            CFStringInitInlineBuffer(s, &buf, CFRangeMake(0, len));
            CFIndex i;
            // ignore any leading whitespace
            for (i = 0; i < len; i++) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (!CFCharacterSetIsCharacterMember(wscs, uc)) {
                    break;
                }
            }
            // is there a number?
            while (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if ((uc >= '0') && (uc <= '9')) {
                    if (n > (UINT_MAX / 10)) {
                        valid = FALSE;
                        break;
                    }
                    n = (n * 10) + (uc - '0');
                } else {
                    break;
                }
                valid = TRUE;
                i++;
            }
            // ignore any trailing whitespace
            while (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (!CFCharacterSetIsCharacterMember(wscs, uc)) {
                    valid = FALSE;
                    break;
                }
                i++;
            }
        }
    }
    if (ok) {
        *ok = valid;
    }
    return valid ? n : 0;
}

long QString::toLong(bool *ok, int base) const
{
    // FIXME: this uses code similar to that in the "toUInt" function and could
    // possibly be refactored
    long n = 0;
    bool valid = FALSE;
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len) {
            CFStringInlineBuffer buf;
            UniChar uc;
            CFCharacterSetRef wscs =
                CFCharacterSetGetPredefined(
                        kCFCharacterSetWhitespaceAndNewline);
            CFStringInitInlineBuffer(s, &buf, CFRangeMake(0, len));
            CFIndex i;
            // ignore any leading whitespace
            for (i = 0; i < len; i++) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (!CFCharacterSetIsCharacterMember(wscs, uc)) {
                    break;
                }
            }
            // is there a sign?
            bool neg = FALSE;
            if (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (uc == '-') {
                    i++;
                    neg = TRUE;
                } else if (uc == '+') {
                    i++;
                }
            }
            // is there a number?
            while (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                // NOTE: ignore anything other than base 10 and base 16
                if ((uc >= '0') && (uc <= '9')) {
                    if (n > (INT_MAX / 10)) {
                        valid = FALSE;
                        break;
                    }
                    n = (n * 10) + (uc - '0');
                } else if (base == 16) {
                    if ((uc >= 'A') && (uc <= 'F')) {
                        if (n > (INT_MAX / 16)) {
                            valid = FALSE;
                            break;
                        }
                        n = (n * 16) + (10 + (uc - 'A'));
                    } else if ((uc >= 'a') && (uc <= 'f')) {
                        if (n > (INT_MAX / 16)) {
                            valid = FALSE;
                            break;
                        }
                        n = (n * 16) + (10 + (uc - 'a'));
                    } else {
                        break;
                    }
                } else {
                    break;
                }
                valid = TRUE;
                i++;
            }
            if (neg) {
                n = -n;
            }
            // ignore any trailing whitespace
            while (i < len) {
                uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if (!CFCharacterSetIsCharacterMember(wscs, uc)) {
                    valid = FALSE;
                    break;
                }
                i++;
            }
        }
    }
    if (ok) {
        *ok = valid;
    }
    return valid ? n : 0;
}

float QString::toFloat(bool *ok) const
{
    float n;
    if (s) {
        n = CFStringGetDoubleValue(s);
    } else {
        n = 0.0;
    }
    if (ok) {
        // NOTE: since CFStringGetDoubleValue returns 0.0 on error there is no
        // way to know if "n" is valid in that case
        if (n == 0.0) {
            *ok = FALSE;
        } else {
            *ok = TRUE;
        }
    }
    return n;
}

QString QString::arg(const QString &replacement, int width) const
{
    QString qs;
    if (s && CFStringGetLength(s)) {
        qs.s = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, s);
    }
    if (qs.s) {
        CFIndex pos = 0;
        UniChar found = 0;
        CFIndex len = CFStringGetLength(qs.s);
        if (len) {
            CFStringInlineBuffer buf;
            CFStringInitInlineBuffer(qs.s, &buf, CFRangeMake(0, len));
            // find position of lowest numerical position marker
            for (CFIndex i = 0; i < len; i++) {
                UniChar uc = CFStringGetCharacterFromInlineBuffer(&buf, i);
                if ((uc == '%') && ((i + 1) < len)) {
                    UniChar uc2 = CFStringGetCharacterFromInlineBuffer(&buf,
                            i + 1);
                    if ((uc2 >= '0') && (uc2 <= '9')) {
                        if (!found || (uc2 < found)) {
                            found = uc2;
                            pos = i;
                        }
                    }
                }
            }
        }
        CFIndex mlen;
        if (found) {
            mlen = 2;
        } else {
            // append space and then replacement text at end of string
            CFStringAppend(qs.s, CFSTR(" "));
            pos = len + 1;
            mlen = 0;
        }
        if (replacement.s) {
            CFStringReplace(qs.s, CFRangeMake(pos, mlen), replacement.s);
            if (width) {
                CFIndex rlen = CFStringGetLength(replacement.s);
                CFIndex padding;
                if (width < 0) {
                    padding = -width;
                    pos += rlen;
                } else {
                    padding = width;
                }
                padding -= rlen;
                if (padding > 0) {
                    CFMutableStringRef tmp =
                        CFStringCreateMutable(kCFAllocatorDefault, 0);
                    if (tmp) {
                        CFStringPad(tmp, CFSTR(" "), padding, 0);
                        CFStringInsert(qs.s, pos, tmp);
                        CFRelease(tmp);
                    }
                }
            }
        }
    }
    return qs;
}

QString QString::arg(short replacement, int width) const
{
    return arg(number((int)replacement), width);
}

QString QString::arg(ushort replacement, int width) const
{
    return arg(number((uint)replacement), width);
}

QString QString::arg(int replacement, int width) const
{
    return arg(number(replacement), width);
}

QString QString::arg(uint replacement, int width) const
{
    return arg(number(replacement), width);
}

QString QString::arg(long replacement, int width) const
{
    return arg(number(replacement), width);
}

QString QString::arg(ulong replacement, int width) const
{
    return arg(number(replacement), width);
}

QString QString::arg(double replacement, int width) const
{
    return arg(number(replacement), width);
}

QString QString::left(uint width) const
{
    return leftRight(width, TRUE);
}

QString QString::right(uint width) const
{
    return leftRight(width, FALSE);
}

QString QString::mid(int index, int width) const
{
    QString qs;
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len && (index >= 0) && (index < len) && width) {
            if (!((index == 0) && (width >= len))) {
                if (width > (len - index)) {
                    width = len - index;
                }
                CFStringRef tmp = CFStringCreateWithSubstring(
                        kCFAllocatorDefault, s, CFRangeMake(index, width));
                if (tmp) {
                    qs.s = CFStringCreateMutableCopy(kCFAllocatorDefault, 0,
                            tmp);
                    CFRelease(tmp);
                }
            } else {
                CFRetain(s);
                qs.s = s;
            }
        }
    }
    return qs;
}

#ifdef USING_BORROWED_KURL
QString QString::copy() const
{
    return QString(*this);
}
#endif // USING_BORROWED_KURL

QString QString::lower() const
{
    QString qs;
    if (s && CFStringGetLength(s)) {
        qs.s = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, s);
    }
    if (qs.s) {
	CFStringLowercase(qs.s, NULL);
    }
    return qs;
}

QString QString::stripWhiteSpace() const
{
    QString qs;
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len) {
            CFCharacterSetRef wscs = CFCharacterSetGetPredefined(
                    kCFCharacterSetWhitespaceAndNewline);
            if (CFCharacterSetIsCharacterMember(wscs,
                    CFStringGetCharacterAtIndex(s, 0))
                    || CFCharacterSetIsCharacterMember(wscs,
                    CFStringGetCharacterAtIndex(s, len - 1))) {
                qs.s = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, s);
                if (qs.s) {
                    CFStringTrimWhitespace(qs.s);
                }
            }
            if (!qs.s) {
                CFRetain(s);
                qs.s = s;
            }
        }
    }
    return qs;
}

QString QString::simplifyWhiteSpace() const
{
    QString qs;
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len) {
            qs.s = CFStringCreateMutable(kCFAllocatorDefault, 0);
            if (qs.s) {
                CFCharacterSetRef wscs = CFCharacterSetGetPredefined(
                        kCFCharacterSetWhitespaceAndNewline);
                const UniChar *ucs = CFStringGetCharactersPtr(s);
                const int capacity = 64;
                UniChar buf[capacity];
                int fill = 0;
                bool chars = FALSE;
                bool space = FALSE;
                for (CFIndex i = 0; i < len; i++) {
                    UniChar uc;
                    if (ucs) {
                        uc = ucs[i];
                    } else {
                        uc = CFStringGetCharacterAtIndex(s, i);
                    }
                    if (CFCharacterSetIsCharacterMember(wscs, uc)) {
                        if (!chars) {
                            continue;
                        }
                        space = TRUE;
                    } else {
                        if (space) {
                            buf[fill] = ' ';
                            fill++;
                            if (fill == capacity) {
                                CFStringAppendCharacters(qs.s, buf, fill);
                                fill = 0;
                            }
                            space = FALSE;
                        }
                        buf[fill] = uc;
                        fill++;
                        if (fill == capacity) {
                            CFStringAppendCharacters(qs.s, buf, fill);
                            fill = 0;
                        }
                        chars = true;
                    }
                }
                if (fill) {
                    CFStringAppendCharacters(qs.s, buf, fill);
                }
            } else {
                CFRetain(s);
                qs.s = s;
            }
        }
    }
    return qs;
}

QString &QString::setUnicode(const QChar *qcs, uint len)
{
    flushCache();
    if (qcs && len) {
        if (s) {
            CFStringRef tmp = CFStringCreateWithCharactersNoCopy(
                    kCFAllocatorDefault, &qcs->c, len, kCFAllocatorNull);
            if (tmp) {
                CFStringReplaceAll(s, tmp);
                CFRelease(tmp);
            }
        } else {
            s = CFStringCreateMutable(kCFAllocatorDefault, 0);
            if (s) {
                CFStringAppendCharacters(s, &qcs->c, len);
            }
        }
    } else if (s) {
        CFRelease(s);
        s = NULL;
    }
    return *this;
}

QString &QString::setLatin1(const char *chs)
{
    flushCache();
    if (chs && *chs) {
        if (s) {
            CFStringRef tmp = CFStringCreateWithCStringNoCopy(
                    kCFAllocatorDefault, chs, kCFStringEncodingISOLatin1,
                    kCFAllocatorNull);
            if (tmp) {
                CFStringReplaceAll(s, tmp);
                CFRelease(tmp);
            }
        } else {
            s = CFStringCreateMutable(kCFAllocatorDefault, 0);
            if (s) {
                CFStringAppendCString(s, chs, kCFStringEncodingISOLatin1);
            }
        }
    } else if (s) {
        CFRelease(s);
        s = NULL;
    }
    return *this;
}

QString &QString::setNum(short n)
{
    return setNum((long)n);
}

QString &QString::setNum(ushort n)
{
    return setNum((ulong)n);
}

QString &QString::setNum(int n)
{
    const int capacity = 64;
    char buf[capacity];
    buf[snprintf(buf, capacity - 1, "%d", n)] = '\0';
    return setLatin1(buf);
}

QString &QString::setNum(uint n)
{
    const int capacity = 64;
    char buf[capacity];
    buf[snprintf(buf, capacity - 1, "%u", n)] = '\0';
    return setLatin1(buf);
}

QString &QString::setNum(long n)
{
    const int capacity = 64;
    char buf[capacity];
    buf[snprintf(buf, capacity - 1, "%D", n)] = '\0';
    return setLatin1(buf);
}

QString &QString::setNum(ulong n)
{
    const int capacity = 64;
    char buf[capacity];
    buf[snprintf(buf, capacity - 1, "%U", n)] = '\0';
    return setLatin1(buf);
}

QString &QString::setNum(double n)
{
    const int capacity = 64;
    char buf[capacity];
    buf[snprintf(buf, capacity - 1, "%.6lg", n)] = '\0';
    return setLatin1(buf);
}

QString &QString::sprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    flushCache();
    if (format && *format) {
        if (!s) {
            s = CFStringCreateMutable(kCFAllocatorDefault, 0);
        }
        if (s) {
            CFStringRef f = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault,
                    format, kCFStringEncodingISOLatin1, kCFAllocatorNull);
            if (f) {
                CFStringRef tmp = CFStringCreateWithFormatAndArguments(
                        kCFAllocatorDefault, NULL, f, args);
                if (tmp) {
                    CFStringReplaceAll(s, tmp);
                    CFRelease(tmp);
                }
                CFRelease(f);
            }
        }
    } else if (s) {
        CFRelease(s);
        s = NULL;
    }
    va_end(args);
    return *this;
}

QString &QString::prepend(const QString &qs)
{
    return insert(0, qs);
}

QString &QString::append(const QString &qs)
{
    return insert(length(), qs);
}

QString &QString::insert(uint index, const QString &qs)
{
    flushCache();
    if (qs.s) {
        CFIndex len = CFStringGetLength(qs.s);
        if (len) {
            if (!s) {
                s = CFStringCreateMutable(kCFAllocatorDefault, 0);
            }
            if (s) {
                if (index < CFStringGetLength(s)) {
                    CFStringInsert(s, index, qs.s);
                } else {
                    CFStringAppend(s, qs.s);
                }
            }
        }
    }
    return *this;
}

QString &QString::insert(uint index, QChar qc)
{
    return insert(index, QString(qc));
}

QString &QString::insert(uint index, char ch)
{
    return insert(index, QString(QChar(ch)));
}

QString &QString::remove(uint index, uint width)
{
    flushCache();
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len && (index < len) && width) {
            if (width > (len - index)) {
                width = len - index;
            }
            CFStringDelete(s, CFRangeMake(index, width));
        }
    }
    return *this;
}

QString &QString::replace(const QRegExp &qre, const QString &qs)
{
    flushCache();
    if (s) {
        int len = qs.length();
        for (int i = 0; i < CFStringGetLength(s); i += len) {
            int width = 0;
            i = qre.match(*this, i, &width, FALSE);
            if ((i < 0) || !width) {
                break;
            }
            CFRange r = CFRangeMake(i, width);
            if (len) {
                CFStringReplace(s, r, qs.s);
            } else {
                CFStringDelete(s, r);
            }
        }
    }
    return *this;
}

void QString::truncate(uint newLen)
{
    flushCache();
    if (s) {
        if (newLen) {
            CFIndex len = CFStringGetLength(s);
            if (len && (newLen < len)) {
                CFStringDelete(s, CFRangeMake(newLen, len - newLen));
            }
        } else {
            CFRelease(s);
            s = NULL;
        }
    }
}

void QString::fill(QChar qc, int len)
{
    flushCache();
    if (s) {
        if (len < 0) {
            len = CFStringGetLength(s);
        }
        CFRelease(s);
        s = NULL;
    }
    if (len > 0) {
        UniChar *ucs = CFAllocatorAllocate(kCFAllocatorDefault,
                len * sizeof (UniChar), 0);
        if (ucs) {
            for (int i = 0; i < len; i++) {
                ucs[i] = qc.c;
            }
            s = CFStringCreateMutableWithExternalCharactersNoCopy(
                    kCFAllocatorDefault, ucs, len, 0, kCFAllocatorDefault);
            if (!s) {
                CFAllocatorDeallocate(kCFAllocatorDefault, ucs);
            }
        }
    }
}

void QString::compose()
{
    // FIXME: unimplemented because we don't do ligatures yet
    NSLog(@"WARNING %s:%s:%d (NOT YET IMPLEMENTED)\n", __FILE__, __FUNCTION__,
            __LINE__);
}

QString QString::visual()
{
    // FIXME: unimplemented because we don't do BIDI yet
    NSLog(@"WARNING %s:%s:%d (NOT YET IMPLEMENTED)\n", __FILE__, __FUNCTION__,
            __LINE__);
    return QString(*this);
}

// operators -------------------------------------------------------------------

bool QString::operator!() const
{ 
    return isNull(); 
}

QString::operator const char *() const
{
    return latin1();
}

QChar QString::operator[](int index) const
{
    if (s && (index >= 0)) {
        CFIndex len = CFStringGetLength(s);
        if (index < len) {
            return QChar(CFStringGetCharacterAtIndex(s, index));
        }
    }
    return QChar(0);
}

QString &QString::operator+=(const QString &qs)
{
    return insert(length(), qs);
}

QString &QString::operator+=(QChar qc)
{
    return insert(length(), QString(qc));
}

QString &QString::operator+=(char ch)
{
    return insert(length(), QString(QChar(ch)));
}

// private member functions ----------------------------------------------------

void QString::flushCache() const
{
    if (cache) {
        CFAllocatorDeallocate(kCFAllocatorDefault, cache);
        cache = NULL;
        cacheType = CacheInvalid;
    }
}

QCString QString::convertToQCString(CFStringEncoding enc) const
{
    uint len = length();
    if (len) {
        char *chs = CFAllocatorAllocate(kCFAllocatorDefault, len + 1, 0);
        if (chs) {
            if (!CFStringGetCString(s, chs, len + 1, enc)) {
#ifdef _KWQ_DEBUG_
                NSLog(@"WARNING %s:%s:%d (CFStringGetCString failed)\n",
                        __FILE__, __FUNCTION__, __LINE__);
#endif
                *reinterpret_cast<char *>(chs) = '\0';
            }
            QCString qcs = QCString(chs);
            CFAllocatorDeallocate(kCFAllocatorDefault, chs);
            return qcs;
        }
    }
    return QCString();
}

QString QString::leftRight(uint width, bool left) const
{
    QString qs;
    if (s) {
        CFIndex len = CFStringGetLength(s);
        if (len && width) {
            if (len > width) {
                CFStringRef tmp = CFStringCreateWithSubstring(
                        kCFAllocatorDefault, s, left ? CFRangeMake(0, width)
                        : CFRangeMake(len - width, len));
                if (tmp) {
                    qs.s = CFStringCreateMutableCopy(kCFAllocatorDefault, 0,
                            tmp);
                    CFRelease(tmp);
                }
            } else {
                CFRetain(s);
                qs.s = s;
            }
        }
    }
    return qs;
}


// operators associated with QString ===========================================

bool operator==(const QString &qs1, const QString &qs2)
{
    if (qs1.s == qs2.s) {
        return TRUE;
    }
    if (qs1.s && qs2.s) {
        return CFStringCompare(qs1.s, qs2.s, 0) == kCFCompareEqualTo;
    }
    return FALSE;
}

bool operator==(const QString &qs, const char *chs)
{
    bool result = FALSE;
    if (qs.s && chs) {
        CFStringRef tmp = CFStringCreateWithCStringNoCopy(
                kCFAllocatorDefault, chs, kCFStringEncodingISOLatin1,
                kCFAllocatorNull);
        if (tmp) {
            result = CFStringCompare(qs.s, tmp, 0) == kCFCompareEqualTo;
            CFRelease(tmp);
        }
    }
    return result;
}

bool operator==(const char *chs, const QString &qs)
{
    return qs == chs;
}

bool operator!=(const QString &qs1, const QString &qs2)
{
    return !(qs1 == qs2);
}

bool operator!=(const QString &qs, const char *chs)
{
    return !(qs == chs);
}

bool operator!=(const char *chs, const QString &qs)
{
    return !(qs == chs);
}

QString operator+(const QString &qs1, const QString &qs2)
{
    QString tmp(qs1);
    tmp += qs2;
    return tmp;
}

QString operator+(const QString &qs, const char *chs)
{
    QString tmp(qs);
    tmp += chs;
    return tmp;
}

QString operator+(const QString &qs, QChar qc)
{
    QString tmp(qs);
    QString tmp2 = QString(qc);
    tmp += tmp2;
    return tmp;
}

QString operator+(const QString &qs, char ch)
{
    QString tmp(qs);
    QString tmp2 = QString(QChar(ch));
    tmp += tmp2;
    return tmp;
}

QString operator+(const char *chs, const QString &qs)
{
    QString tmp(chs);
    tmp += qs;
    return tmp;
}

QString operator+(QChar qc, const QString &qs)
{
    QString tmp = QString(qc);
    tmp += qs;
    return tmp;
}

QString operator+(char ch, const QString &qs)
{
    QString tmp = QString(QChar(ch));
    tmp += qs;
    return tmp;
}


// class QConstString ==========================================================

// constructors, copy constructors, and destructors ----------------------------

QConstString::QConstString(QChar *qcs, uint len)
{
    if (qcs || len) {
        // NOTE: use instead of CFStringCreateWithCharactersNoCopy function to
        // guarantee backing store is not copied even though string is mutable
        s = CFStringCreateMutableWithExternalCharactersNoCopy(
                kCFAllocatorDefault, &qcs->c, len, len, kCFAllocatorNull);
    } else {
        s = NULL;
    }
}

// member functions ------------------------------------------------------------

const QString &QConstString::string() const
{
    return *this;
}

#else // USING_BORROWED_QSTRING
// This will help to keep the linker from complaining about empty archives
void KWQString_Dummy() {}
#endif // USING_BORROWED_QSTRING
