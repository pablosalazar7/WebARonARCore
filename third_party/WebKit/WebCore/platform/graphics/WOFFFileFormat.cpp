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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WOFFFileFormat.h"

#if !ENABLE(OPENTYPE_SANITIZER)

#include "SharedBuffer.h"

#if !PLATFORM(WIN)
#include <zlib.h>
#else
#include "SoftLinking.h"

SOFT_LINK_LIBRARY(zlib1);
 (zlib1, uncompress, int, __cdecl, (Bytef *dest, uLongf *destLen, const Bytef *source, uLong sourceLen), (dest, destLen, source, sourceLen));

#if CPU(BIG_ENDIAN)
#define ntohs(x) ((uint16_t)(x))
#define htons(x) ((uint16_t)(x))
#define ntohl(x) ((uint32_t)(x))
#define htonl(x) ((uint32_t)(x))
#elif CPU(MIDDLE_ENDIAN)
#define ntohs(x) ((unit16_t)(x))
#define htons(x) ((uint16_t)(x))
#define ntohl(x) ((uint32_t)((((uint32_t)(x) & 0xffff0000) >> 16) | (((uint32_t)(x) & 0xffff) << 16))
#define htonl(x) ntohl(x)
#else
#define ntohs(x) ((uint16_t)((((uint16_t)(x) & 0xff00) >> 8) | (((uint16_t)(x) & 0x00ff) << 8)))
#define htons(x) ntohs(x)
#define ntohl(x) ((uint32_t)((((uint32_t)(x) & 0xff000000) >> 24) | (((uint32_t)(x) & 0x00ff0000) >>  8) | \
                 (((uint32_t)(x) & 0x0000ff00) <<  8) | (((uint32_t)(x) & 0x000000ff) << 24)))
#define htonl(x) ntohl(x)
#endif

#endif // PLATFORM(WIN)

namespace WebCore {

static bool readUInt32(SharedBuffer* buffer, size_t& offset, uint32_t& value)
{
    ASSERT_ARG(offset, offset <= buffer->size());
    if (buffer->size() - offset < sizeof(value))
        return false;

    value = ntohl(*reinterpret_cast<const uint32_t*>(buffer->data() + offset));
    offset += sizeof(value);

    return true;
}

static bool readUInt16(SharedBuffer* buffer, size_t& offset, uint16_t& value)
{
    ASSERT_ARG(offset, offset <= buffer->size());
    if (buffer->size() - offset < sizeof(value))
        return false;

    value = ntohs(*reinterpret_cast<const uint16_t*>(buffer->data() + offset));
    offset += sizeof(value);

    return true;
}

static bool writeUInt32(Vector<char>& vector, uint32_t value)
{
    uint32_t bigEndianValue = htonl(value);
    return vector.tryAppend(reinterpret_cast<char*>(&bigEndianValue), sizeof(bigEndianValue));
}

static bool writeUInt16(Vector<char>& vector, uint16_t value)
{
    uint16_t bigEndianValue = htons(value);
    return vector.tryAppend(reinterpret_cast<char*>(&bigEndianValue), sizeof(bigEndianValue));
}

static const uint32_t woffSignature = 0x774f4646; /* 'wOFF' */

bool isWOFF(SharedBuffer* buffer)
{
    size_t offset = 0;
    uint32_t signature;

    return readUInt32(buffer, offset, signature) && signature == woffSignature;
}

bool convertWOFFToSfnt(SharedBuffer* woff, Vector<char>& sfnt)
{
#if PLATFORM(WINDOWS)
    if (!zlib1Library())
        return false;
#endif
    ASSERT_ARG(sfnt, sfnt.isEmpty());

    size_t offset = 0;

    // Read the WOFF header.
    uint32_t signature;
    if (!readUInt32(woff, offset, signature) || signature != woffSignature) {
        ASSERT_NOT_REACHED();
        return false;
    }

    uint32_t flavor;
    if (!readUInt32(woff, offset, flavor))
        return false;

    uint32_t length;
    if (!readUInt32(woff, offset, length) || length != woff->size())
        return false;

    uint16_t numTables;
    if (!readUInt16(woff, offset, numTables))
        return false;

    if (!numTables || numTables > 0x0fff)
        return false;

    uint16_t reserved;
    if (!readUInt16(woff, offset, reserved) || reserved)
        return false;

    uint32_t totalSfntSize;
    if (!readUInt32(woff, offset, totalSfntSize))
        return false;

    if (woff->size() - offset < sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uint32_t))
        return false;

    offset += sizeof(uint16_t); // majorVersion
    offset += sizeof(uint16_t); // minorVersion
    offset += sizeof(uint32_t); // metaOffset
    offset += sizeof(uint32_t); // metaLength
    offset += sizeof(uint32_t); // metaOrigLength
    offset += sizeof(uint32_t); // privOffset
    offset += sizeof(uint32_t); // privLength

    // Check if the WOFF can supply as many tables as it claims it has.
    if (woff->size() - offset < numTables * 5 * sizeof(uint32_t))
        return false;

    // Write the sfnt offset subtable.
    uint16_t entrySelector = 0;
    uint16_t searchRange = 1;
    while (searchRange < numTables >> 1) {
        entrySelector++;
        searchRange <<= 1;
    }
    searchRange <<= 4;
    uint16_t rangeShift = (numTables << 4) - searchRange;

    if (!writeUInt32(sfnt, flavor)
        || !writeUInt16(sfnt, numTables)
        || !writeUInt16(sfnt, searchRange)
        || !writeUInt16(sfnt, entrySelector)
        || !writeUInt16(sfnt, rangeShift))
        return false;

    if (sfnt.size() > totalSfntSize)
        return false;

    if (totalSfntSize - sfnt.size() < numTables * 4 * sizeof(uint32_t))
        return false;

    size_t sfntTableDirectoryCursor = sfnt.size();
    sfnt.grow(sfnt.size() + numTables * 4 * sizeof(uint32_t));

    // Process tables.
    for (uint16_t i = 0; i < numTables; ++i) {
        // Read a WOFF table directory entry.
        uint32_t tableTag;
        if (!readUInt32(woff, offset, tableTag))
            return false;

        uint32_t tableOffset;
        if (!readUInt32(woff, offset, tableOffset))
            return false;

        uint32_t tableCompLength;
        if (!readUInt32(woff, offset, tableCompLength))
            return false;

        if (tableOffset > woff->size() || tableCompLength > woff->size() - tableOffset)
            return false;

        uint32_t tableOrigLength;
        if (!readUInt32(woff, offset, tableOrigLength) || tableCompLength > tableOrigLength)
            return false;

        if (tableOrigLength > totalSfntSize || sfnt.size() > totalSfntSize - tableOrigLength)
            return false;

        uint32_t tableOrigChecksum;
        if (!readUInt32(woff, offset, tableOrigChecksum))
            return false;

        // Write an sfnt table directory entry.
        uint32_t* sfntTableDirectoryPtr = reinterpret_cast<uint32_t*>(sfnt.data() + sfntTableDirectoryCursor);
        *sfntTableDirectoryPtr++ = htonl(tableTag);
        *sfntTableDirectoryPtr++ = htonl(tableOrigChecksum);
        *sfntTableDirectoryPtr++ = htonl(sfnt.size());
        *sfntTableDirectoryPtr++ = htonl(tableOrigLength);
        sfntTableDirectoryCursor += 4 * sizeof(uint32_t);

        if (tableCompLength == tableOrigLength) {
            // The table is not compressed.
            if (!sfnt.tryAppend(woff->data() + tableOffset, tableCompLength))
                return false;
        } else {
            uLongf destLen = tableOrigLength;
            if (!sfnt.tryReserveCapacity(sfnt.size() + tableOrigLength))
                return false;
            Bytef* dest = reinterpret_cast<Bytef*>(sfnt.end());
            sfnt.grow(sfnt.size() + tableOrigLength);
            if (uncompress(dest, &destLen, reinterpret_cast<const Bytef*>(woff->data() + tableOffset), tableCompLength) != Z_OK)
                return false;
            if (destLen != tableOrigLength)
                return false;
        }

        // Pad to a multiple of 4 bytes.
        while (sfnt.size() % 4)
            sfnt.append(0);
    }

    return sfnt.size() == totalSfntSize;
}

#endif // !ENABLE(OPENTYPE_SANITIZER)

} // namespace WebCore
