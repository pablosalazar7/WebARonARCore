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

#ifndef KDOM_CSSRuleImpl_H
#define KDOM_CSSRuleImpl_H

#include <kdom/css/kdomcss.h>
#include <kdom/css/impl/StyleBaseImpl.h>

namespace KDOM
{
	class DOMStringImpl;
	class CSSStyleSheetImpl;
	class CSSRuleImpl : public StyleBaseImpl
	{
	public:
		CSSRuleImpl(StyleBaseImpl *parent);
		virtual ~CSSRuleImpl();

		// 'CSSRule' functions
		virtual unsigned short type() const;

		virtual DOMStringImpl *cssText() const;
		virtual void setCssText(DOMStringImpl *cssText);

		virtual CSSStyleSheetImpl *parentStyleSheet() const;
		virtual CSSRuleImpl *parentRule() const;

		virtual void init() { }
		virtual bool isRule() const { return true; }

	protected:
		RuleType m_type;
	};
};

#endif

// vim:ts=4:noet
