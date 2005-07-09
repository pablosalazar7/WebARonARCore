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

#include "RGBColorImpl.h"
#include "CSSPrimitiveValueImpl.h"

using namespace KDOM;

RGBColorImpl::RGBColorImpl(CDFInterface *interface) : Shared(true), m_interface(interface)
{
}

RGBColorImpl::RGBColorImpl(CDFInterface *interface, const QRgb &color) : Shared(true), m_interface(interface)
{
	m_color = color;
}

RGBColorImpl::RGBColorImpl(CDFInterface *interface, const QColor &color) : Shared(true), m_interface(interface)
{
	m_color = color.rgb();
}

RGBColorImpl::~RGBColorImpl()
{
}

CSSPrimitiveValueImpl *RGBColorImpl::red() const
{
	return new CSSPrimitiveValueImpl(m_interface, float(qAlpha(m_color) ? qRed(m_color) : 0), CSS_DIMENSION);
}

CSSPrimitiveValueImpl *RGBColorImpl::green() const
{
	return new CSSPrimitiveValueImpl(m_interface, float(qAlpha(m_color) ? qGreen(m_color) : 0), CSS_DIMENSION);
}

CSSPrimitiveValueImpl *RGBColorImpl::blue() const
{
	return new CSSPrimitiveValueImpl(m_interface, float(qAlpha(m_color) ? qBlue(m_color) : 0), CSS_DIMENSION);
}

// vim:ts=4:noet
