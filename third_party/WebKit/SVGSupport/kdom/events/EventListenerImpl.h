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

#ifndef KDOM_EventListenerImpl_H
#define KDOM_EventListenerImpl_H

#include <kdom/Shared.h>
#include <kdom/ecma/Ecma.h>
#include <kdom/impl/DocumentImpl.h>

namespace KDOM
{
	class EventListenerImpl : public Shared
	{
	public:
		EventListenerImpl();
		virtual ~EventListenerImpl();

		virtual void handleEvent(EventImpl *evt);
		
		DOMString internalType() const;
		KJS::Value ecmaListener() const;

		// Internal
		void initListener(DocumentImpl *doc, bool ecmaEventListener, KJS::Object listener, KJS::Value compareListener, const DOMString &internalType);

	private:
		DocumentImpl *m_doc;

		bool m_ecmaEventListener;
		DOMString m_internalType;

		KJS::Object m_listener;
		KJS::Value m_compareListener;
	};
};

#endif

// vim:ts=4:noet
