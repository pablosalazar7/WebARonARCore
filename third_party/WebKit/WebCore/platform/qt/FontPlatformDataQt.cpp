/*
 * This file is part of the internal font implementation.  It should not be included by anyone other than
 * FontMac.cpp, FontWin.cpp and Font.cpp.
 *
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"
#include "FontPlatformData.h"

#include "DeprecatedString.h"
#include "FontDescription.h"

#include <QDebug>
#include <QFontInfo>

namespace WebCore {

FontPlatformData::FontPlatformData()
{
}

FontPlatformData::FontPlatformData(Deleted)
{
}

FontPlatformData::FontPlatformData(const FontDescription& fontDescription, const AtomicString& familyName)
{
    QFont font("Times New Roman", 12);
    font.setFamily(familyName.domString());
    font.setPixelSize(fontDescription.computedSize());
    font.setItalic(fontDescription.italic());
    font.setWeight(fontDescription.weight());
    setFont(font);
}

FontPlatformData::FontPlatformData(const FontPlatformData& other)
    : m_font(other.m_font)
{
}

FontPlatformData& FontPlatformData::operator=(const FontPlatformData& other)
{
    m_font = other.m_font;
    return *this;
}

FontPlatformData::~FontPlatformData()
{
}

bool FontPlatformData::isFixedPitch()
{
    return QFontInfo(m_font).fixedPitch();
}

void FontPlatformData::setFont(const QFont& other)
{
    m_font = other;
}

QFont FontPlatformData::font() const
{
    return m_font;
}

unsigned FontPlatformData::hash() const
{
    return qHash(&m_font);
}

bool FontPlatformData::operator==(const FontPlatformData& other) const
{
    return (other.m_font == m_font);
}

}

// vim: ts=4 sw=4 et
