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

#ifndef SVGTransform_h
#define SVGTransform_h
#ifdef SVG_SUPPORT

#include "AffineTransform.h"
#include "Shared.h"
#include <wtf/RefPtr.h>

namespace WebCore {

    class SVGTransform {
    public:
        enum SVGTransformType {
            SVG_TRANSFORM_UNKNOWN           = 0,
            SVG_TRANSFORM_MATRIX            = 1,
            SVG_TRANSFORM_TRANSLATE         = 2,
            SVG_TRANSFORM_SCALE             = 3,
            SVG_TRANSFORM_ROTATE            = 4,
            SVG_TRANSFORM_SKEWX             = 5,
            SVG_TRANSFORM_SKEWY             = 6
        };
 
        SVGTransform();
        explicit SVGTransform(const AffineTransform&);
        virtual ~SVGTransform();
               
        unsigned short type() const;

        AffineTransform matrix() const;
    
        double angle() const;

        void setMatrix(const AffineTransform&);
        void setTranslate(double tx, double ty);
        void setScale(double sx, double sy);
        void setRotate(double angle, double cx, double cy);
        void setSkewX(double angle);
        void setSkewY(double angle);
        
        bool isValid();

    private:
        unsigned short m_type;
        double m_angle;
        AffineTransform m_matrix;
    };

} // namespace WebCore

#endif // SVG_SUPPORT
#endif

// vim:ts=4:noet
