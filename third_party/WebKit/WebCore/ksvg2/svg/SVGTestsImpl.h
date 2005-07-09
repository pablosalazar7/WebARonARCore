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

#ifndef KSVG_SVGTestsImpl_H
#define KSVG_SVGTestsImpl_H

namespace KDOM
{
	class DOMString;
	class AttributeImpl;
};

namespace KSVG
{
	class SVGStringListImpl;

	class SVGTestsImpl
	{
	public:
		SVGTestsImpl();
		virtual ~SVGTestsImpl();

		// 'SVGTests' functions
		SVGStringListImpl *requiredFeatures() const;
		SVGStringListImpl *requiredExtensions() const;
		SVGStringListImpl *systemLanguage() const;

		bool hasExtension(const KDOM::DOMString &extension) const;

		bool isValid();
		
		bool parseAttribute(KDOM::AttributeImpl *attr);

	private:
		mutable SVGStringListImpl *m_features;
		mutable SVGStringListImpl *m_extensions;
		mutable SVGStringListImpl *m_systemLanguage;
	};
};

#endif

// vim:ts=4:noet
