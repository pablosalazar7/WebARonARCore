/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
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
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef HTMLPreElement_h
#define HTMLPreElement_h

#include "HTMLElement.h"

namespace WebCore {

class HTMLPreElement : public HTMLElement {
public:
    HTMLPreElement(const QualifiedName&, Document*);

    virtual HTMLTagStatus endTagRequirement() const { return TagStatusRequired; }
    virtual int tagPriority() const { return 5; }

    bool mapToEntry(const QualifiedName&, MappedAttributeEntry&) const;
    void parseMappedAttribute(MappedAttribute*);

    int width() const;
    void setWidth(int w);

    bool wrap() const;
    void setWrap(bool b);
};

} // namespace WebCore

#endif // HTMLPreElement_h
