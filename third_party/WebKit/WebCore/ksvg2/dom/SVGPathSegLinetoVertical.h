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

#ifndef KSVG_SVGPathSegLinetoVertical_H
#define KSVG_SVGPathSegLinetoVertical_H

#include <ksvg2/dom/SVGPathSeg.h>

namespace KSVG
{
	class SVGPathSegLinetoVerticalAbsImpl;
	class SVGPathSegLinetoVerticalAbs : public SVGPathSeg
	{
	public:
		SVGPathSegLinetoVerticalAbs();
		explicit SVGPathSegLinetoVerticalAbs(SVGPathSegLinetoVerticalAbsImpl *);
		SVGPathSegLinetoVerticalAbs(const SVGPathSegLinetoVerticalAbs &);
		SVGPathSegLinetoVerticalAbs(const SVGPathSeg &);
		virtual ~SVGPathSegLinetoVerticalAbs();

		// Operators
		SVGPathSegLinetoVerticalAbs &operator=(const SVGPathSegLinetoVerticalAbs &other);
		SVGPathSegLinetoVerticalAbs &operator=(const SVGPathSeg &other);

		void setY(float);
		float y() const;

		// Internal
		KSVG_INTERNAL(SVGPathSegLinetoVerticalAbs)

	public: // EcmaScript section
		KDOM_GET
		KDOM_PUT

		KJS::Value getValueProperty(KJS::ExecState *exec, int token) const;
		void putValueProperty(KJS::ExecState *exec, int token, const KJS::Value &value, int attr);
	};

	class SVGPathSegLinetoVerticalRelImpl;
	class SVGPathSegLinetoVerticalRel : public SVGPathSeg
	{
	public:
		SVGPathSegLinetoVerticalRel();
		explicit SVGPathSegLinetoVerticalRel(SVGPathSegLinetoVerticalRelImpl *);
		SVGPathSegLinetoVerticalRel(const SVGPathSegLinetoVerticalRel &);
		SVGPathSegLinetoVerticalRel(const SVGPathSeg &);
		virtual ~SVGPathSegLinetoVerticalRel();

		// Operators
		SVGPathSegLinetoVerticalRel &operator=(const SVGPathSegLinetoVerticalRel &other);
		SVGPathSegLinetoVerticalRel &operator=(const SVGPathSeg &other);

		void setY(float);
		float y() const;

		// Internal
		KSVG_INTERNAL(SVGPathSegLinetoVerticalRel)

	public: // EcmaScript section
		KDOM_GET
		KDOM_PUT

		KJS::Value getValueProperty(KJS::ExecState *exec, int token) const;
		void putValueProperty(KJS::ExecState *exec, int token, const KJS::Value &value, int attr);
	};
};

#endif

// vim:ts=4:noet
