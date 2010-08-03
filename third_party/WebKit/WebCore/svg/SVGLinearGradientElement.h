/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGLinearGradientElement_h
#define SVGLinearGradientElement_h

#if ENABLE(SVG)
#include "SVGGradientElement.h"

namespace WebCore {

    struct LinearGradientAttributes;
    class SVGLength;

    class SVGLinearGradientElement : public SVGGradientElement {
    public:
        SVGLinearGradientElement(const QualifiedName&, Document*);
        virtual ~SVGLinearGradientElement();

        virtual void parseMappedAttribute(Attribute*);
        virtual void svgAttributeChanged(const QualifiedName&);
        virtual void synchronizeProperty(const QualifiedName&);

        virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

        LinearGradientAttributes collectGradientProperties();
        void calculateStartEndPoints(const LinearGradientAttributes&, FloatPoint& startPoint, FloatPoint& endPoint);

    private:
        virtual bool selfHasRelativeLengths() const;

        DECLARE_ANIMATED_PROPERTY(SVGLinearGradientElement, SVGNames::x1Attr, SVGLength, X1, x1)
        DECLARE_ANIMATED_PROPERTY(SVGLinearGradientElement, SVGNames::y1Attr, SVGLength, Y1, y1)
        DECLARE_ANIMATED_PROPERTY(SVGLinearGradientElement, SVGNames::x2Attr, SVGLength, X2, x2)
        DECLARE_ANIMATED_PROPERTY(SVGLinearGradientElement, SVGNames::y2Attr, SVGLength, Y2, y2)
    };

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
