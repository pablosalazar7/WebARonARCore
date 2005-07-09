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

#include "SVGFEFuncBElement.h"
#include "SVGFEFuncBElementImpl.h"

using namespace KSVG;

// The qdom way...
#define impl (static_cast<SVGFEFuncBElementImpl *>(d))

SVGFEFuncBElement SVGFEFuncBElement::null;

SVGFEFuncBElement::SVGFEFuncBElement() : SVGComponentTransferFunctionElement()
{
}

SVGFEFuncBElement::SVGFEFuncBElement(SVGFEFuncBElementImpl *i) : SVGComponentTransferFunctionElement(i)
{
}

SVGFEFuncBElement::SVGFEFuncBElement(const SVGFEFuncBElement &other) : SVGComponentTransferFunctionElement()
{
	(*this) = other;
}

SVGFEFuncBElement::SVGFEFuncBElement(const KDOM::Node &other) : SVGComponentTransferFunctionElement()
{
	(*this) = other;
}

SVGFEFuncBElement::~SVGFEFuncBElement()
{
}

SVGFEFuncBElement &SVGFEFuncBElement::operator=(const SVGFEFuncBElement &other)
{
	SVGComponentTransferFunctionElement::operator=(other);
	return *this;
}

SVGFEFuncBElement &SVGFEFuncBElement::operator=(const KDOM::Node &other)
{
	SVGFEFuncBElementImpl *ohandle = static_cast<SVGFEFuncBElementImpl *>(other.handle());
	if(d != ohandle)
	{
		if(!ohandle || ohandle->nodeType() != KDOM::ELEMENT_NODE)
		{
			if(d)
				d->deref();
			
			d = 0;
		}
		else
		{
			SVGComponentTransferFunctionElement::operator=(other);
		}
	}

	return *this;
}

// vim:ts=4:noet
