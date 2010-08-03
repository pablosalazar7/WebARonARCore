/*
 * Copyright (C) 2006 Apple Inc. All rights reserved.
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

#ifndef SVGForeignObjectElement_h
#define SVGForeignObjectElement_h

#if ENABLE(SVG) && ENABLE(SVG_FOREIGN_OBJECT)
#include "SVGTests.h"
#include "SVGLangSpace.h"
#include "SVGURIReference.h"
#include "SVGStyledTransformableElement.h"
#include "SVGExternalResourcesRequired.h"

namespace WebCore {
    class SVGLength;

    class SVGForeignObjectElement : public SVGStyledTransformableElement,
                                    public SVGTests,
                                    public SVGLangSpace,
                                    public SVGExternalResourcesRequired {
    public:
        SVGForeignObjectElement(const QualifiedName&, Document*);
        virtual ~SVGForeignObjectElement();

        virtual bool isValid() const { return SVGTests::isValid(); }
        virtual void parseMappedAttribute(Attribute*);
        virtual void svgAttributeChanged(const QualifiedName&);
        virtual void synchronizeProperty(const QualifiedName&);

        bool childShouldCreateRenderer(Node*) const;
        virtual RenderObject* createRenderer(RenderArena* arena, RenderStyle* style);

    private:
        virtual bool selfHasRelativeLengths() const;

        DECLARE_ANIMATED_PROPERTY(SVGForeignObjectElement, SVGNames::xAttr, SVGLength, X, x)
        DECLARE_ANIMATED_PROPERTY(SVGForeignObjectElement, SVGNames::yAttr, SVGLength, Y, y)
        DECLARE_ANIMATED_PROPERTY(SVGForeignObjectElement, SVGNames::widthAttr, SVGLength, Width, width)
        DECLARE_ANIMATED_PROPERTY(SVGForeignObjectElement, SVGNames::heightAttr, SVGLength, Height, height)

        // SVGURIReference
        DECLARE_ANIMATED_PROPERTY(SVGForeignObjectElement, XLinkNames::hrefAttr, String, Href, href)

        // SVGExternalResourcesRequired
        DECLARE_ANIMATED_PROPERTY(SVGForeignObjectElement, SVGNames::externalResourcesRequiredAttr, bool, ExternalResourcesRequired, externalResourcesRequired)
    };

} // namespace WebCore

#endif // ENABLE(SVG) && ENABLE(SVG_FOREIGN_OBJECT)
#endif
