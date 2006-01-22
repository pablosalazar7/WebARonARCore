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

#include "config.h"
#if SVG_SUPPORT
#include "SVGTSpanElementImpl.h"
#include "SVGNames.h"
#include "render_inline.h"

using namespace KSVG;

SVGTSpanElementImpl::SVGTSpanElementImpl(const KDOM::QualifiedName& tagName, KDOM::DocumentImpl *doc)
: SVGTextPositioningElementImpl(tagName, doc)
{
}

SVGTSpanElementImpl::~SVGTSpanElementImpl()
{
}

bool SVGTSpanElementImpl::childShouldCreateRenderer(DOM::NodeImpl *child) const
{
    if (child->isTextNode() || child->hasTagName(SVGNames::tspanTag))
        return true;
    return false;
}

khtml::RenderObject *SVGTSpanElementImpl::createRenderer(RenderArena *arena, khtml::RenderStyle *)
{
    return new (arena) khtml::RenderInline(this);
}

// vim:ts=4:noet
#endif // SVG_SUPPORT

