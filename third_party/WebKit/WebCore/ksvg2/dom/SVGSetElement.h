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

#ifndef KSVG_SVGSetElement_H
#define KSVG_SVGSetElement_H

#include <ksvg2/ecma/SVGLookup.h>
#include <ksvg2/dom/SVGAnimationElement.h>

namespace KSVG
{
	class SVGSetElementImpl;
	class SVGSetElement : public SVGAnimationElement
	{
	public:
		SVGSetElement();
		explicit SVGSetElement(SVGSetElementImpl *i);
		SVGSetElement(const SVGSetElement &other);
		SVGSetElement(const KDOM::Node &other);
		virtual ~SVGSetElement();

		// Operators
		SVGSetElement &operator=(const SVGSetElement &other);
		SVGSetElement &operator=(const KDOM::Node &other);

		// Internal
		KSVG_INTERNAL(SVGSetElement)

	public: // EcmaScript section
		KDOM_GET
		KDOM_FORWARDPUT

		KJS::Value getValueProperty(KJS::ExecState *exec, int token) const;
	};
};

#endif

// vim:ts=4:noet
