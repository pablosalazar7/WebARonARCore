/*
    Copyright (C) 2004, 2005 Nikolas Zimmermann <wildfox@kde.org>
                  2004, 2005 Rob Buis <buis@kde.org>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef KSVG_SVGRadialGradientElementImpl_H
#define KSVG_SVGRadialGradientElementImpl_H
#if SVG_SUPPORT

#include <SVGGradientElementImpl.h>

namespace KSVG
{
    class SVGAnimatedLengthImpl;
    class SVGRadialGradientElementImpl : public SVGGradientElementImpl
    {
    public:
        SVGRadialGradientElementImpl(const KDOM::QualifiedName& tagName, KDOM::DocumentImpl *doc);
        virtual ~SVGRadialGradientElementImpl();

        // 'SVGRadialGradientElement' functions
        SVGAnimatedLengthImpl *cx() const;
        SVGAnimatedLengthImpl *cy() const;
        SVGAnimatedLengthImpl *fx() const;
        SVGAnimatedLengthImpl *fy() const;
        SVGAnimatedLengthImpl *r() const;

        virtual void parseMappedAttribute(KDOM::MappedAttributeImpl *attr);

    protected:
        virtual void buildGradient(KRenderingPaintServerGradient *grad) const;
        virtual KCPaintServerType gradientType() const { return PS_RADIAL_GRADIENT; }

    private:
        mutable RefPtr<SVGAnimatedLengthImpl> m_cx;
        mutable RefPtr<SVGAnimatedLengthImpl> m_cy;
        mutable RefPtr<SVGAnimatedLengthImpl> m_r;
        mutable RefPtr<SVGAnimatedLengthImpl> m_fx;
        mutable RefPtr<SVGAnimatedLengthImpl> m_fy;
    };
};

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
