/*
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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
 */

#ifndef SVGPathStringSource_h
#define SVGPathStringSource_h

#if ENABLE(SVG)
#include "PlatformString.h"
#include "SVGPathSource.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class SVGPathStringSource : public SVGPathSource {
public:
    static PassOwnPtr<SVGPathStringSource> create(const String& string)
    {
        return adoptPtr(new SVGPathStringSource(string));
    }

    virtual ~SVGPathStringSource();

    virtual bool hasMoreData() const;
    virtual bool moveToNextToken();
    virtual bool parseFloat(float& result);
    virtual bool parseFlag(bool& result);
    virtual bool parseSVGSegmentType(SVGPathSegType&);
    virtual SVGPathSegType nextCommand(SVGPathSegType previousCommand);

private:
    SVGPathStringSource(const String&);
    String m_string;

    const UChar* m_current;
    const UChar* m_end;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGPathStringSource_h
