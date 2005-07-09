/*
    Copyright(C) 2004 Richard Moore <rich@kde.org>
    Copyright(C) 2005 Frans Englich <frans.englich@telia.com>

    This file is part of the KDE project

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or(at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#include <qmap.h>

#include "ContextImpl.h"
#include "NodeImpl.h"
#include "NodeSetImpl.h"
#include "ScopeImpl.h"

using namespace KDOM;
using namespace KDOM::XPath;

ContextImpl::ContextImpl(ScopeImpl *scope) : m_scope(scope)
{
	Q_ASSERT(m_scope);
	m_scope->ref();
}

ContextImpl::ContextImpl(ScopeImpl *scope, NodeImpl * /*node*/) : m_scope(scope)
{
	Q_ASSERT(m_scope);
	m_scope->ref();
}

ContextImpl::ContextImpl(ScopeImpl *scope, NodeSetImpl * /*nodeset*/) : m_scope(scope)
{
	Q_ASSERT(m_scope);
	m_scope->ref();
}

ContextImpl::~ContextImpl()
{
	m_scope->deref();
}

NodeImpl *ContextImpl::current() const
{
	return m_nodes->current();
}

NodeSetImpl *ContextImpl::nodeSet() const
{
	return m_nodes;
}

ScopeImpl *ContextImpl::scope() const
{
	return m_scope;
}

// vim:ts=4:noet
