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

#ifndef KSVG_SVGUseElementImpl_H
#define KSVG_SVGUseElementImpl_H

#include "SVGTestsImpl.h"
#include "SVGLangSpaceImpl.h"
#include "SVGURIReferenceImpl.h"
#include "SVGStyledElementImpl.h"
#include "SVGTransformableImpl.h"
#include "SVGExternalResourcesRequiredImpl.h"

namespace KSVG
{
	//class SVGElementInstanceImpl;
	class SVGAnimatedLengthImpl;
	class SVGUseElementImpl : public SVGStyledElementImpl,
							  public SVGTestsImpl,
							  public SVGLangSpaceImpl,
							  public SVGExternalResourcesRequiredImpl,
							  public SVGTransformableImpl,
							  public SVGURIReferenceImpl
	{
	public:
		SVGUseElementImpl(KDOM::DocumentPtr *doc, KDOM::NodeImpl::Id id,  KDOM::DOMStringImpl *prefix);
		virtual ~SVGUseElementImpl();

		// Derived from: 'ElementImpl'
		virtual bool hasChildNodes() const;

		virtual void close();

		// 'SVGUseElement' functions
		SVGAnimatedLengthImpl *x() const;
		SVGAnimatedLengthImpl *y() const;

		SVGAnimatedLengthImpl *width() const;
		SVGAnimatedLengthImpl *height() const;

		virtual void parseAttribute(KDOM::AttributeImpl *attr);

		virtual bool implementsCanvasItem() const { return true; }
		virtual KCanvasItem *createCanvasItem(KCanvas *canvas, KRenderingStyle *style) const;

		// TODO: not sure about this API yet
		// SVGElementInstanceImpl *instanceRoot() const;

	private:
		mutable SVGAnimatedLengthImpl *m_x;
		mutable SVGAnimatedLengthImpl *m_y;
		mutable SVGAnimatedLengthImpl *m_width;
		mutable SVGAnimatedLengthImpl *m_height;
	};
};

#endif

// vim:ts=4:noet
