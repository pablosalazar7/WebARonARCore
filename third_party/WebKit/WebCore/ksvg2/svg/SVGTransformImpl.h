/*
    Copyright (C) 2004 Nikolas Zimmermann <wildfox@kde.org>
                  2004 Rob Buis <buis@kde.org>

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

#ifndef KSVG_SVGTransformImpl_H
#define KSVG_SVGTransformImpl_H

#include <kdom/Shared.h>

namespace KSVG
{
	class SVGMatrixImpl;
	class SVGTransformImpl : public KDOM::Shared
	{
	public:
		SVGTransformImpl();
		virtual ~SVGTransformImpl();

		unsigned short type() const;

		SVGMatrixImpl *matrix() const;
	
		double angle() const;

		void setMatrix(SVGMatrixImpl *matrix);
		void setTranslate(double tx, double ty);
		void setScale(double sx, double sy);
		void setRotate(double angle, double cx, double cy);
		void setSkewX(double angle);
		void setSkewY(double angle);

	private:
		double m_angle;
		unsigned short m_type;
		SVGMatrixImpl *m_matrix;
	};
};

#endif

// vim:ts=4:noet
