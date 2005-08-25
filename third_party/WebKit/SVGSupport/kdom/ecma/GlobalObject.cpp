/*
	Copyright (C) 2002 David Faure <faure@kde.org>
				  2004, 2005 Nikolas Zimmermann <wildfox@kde.org>

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

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#ifndef APPLE_CHANGES
#include <kinputdialog.h>
#endif

#include <qstylesheet.h>

#include "Ecma.h"
#include "DOMLookup.h"
#include "kdom/Helper.h"
#include "DocumentImpl.h"
#include "GlobalObject.moc"
#include "ScriptInterpreter.h"

#include "GlobalObject.lut.h"

using namespace KDOM;

/*
@begin GlobalObject::s_hashTable 15
 closed			GlobalObject::Closed		DontDelete|ReadOnly
 window			GlobalObject::Window		DontDelete|ReadOnly
 evt			GlobalObject::Evt			DontDelete|ReadOnly
 document		GlobalObject::Document		DontDelete|ReadOnly

# Functions
 setTimeout		GlobalObject::SetTimeout	DontDelete|Function 2
 clearTimeout	GlobalObject::ClearTimeout	DontDelete|Function 1
 setInterval	GlobalObject::SetInterval	DontDelete|Function 2
 clearInterval	GlobalObject::ClearInterval	DontDelete|Function 1
 printNode		GlobalObject::PrintNode		DontDelete|Function 1
 alert			GlobalObject::Alert			DontDelete|Function 1
 prompt			GlobalObject::Prompt		DontDelete|Function 2
 confirm		GlobalObject::Confirm		DontDelete|Function 1
 debug			GlobalObject::Debug			DontDelete|Function 1
@end
*/

/*
# Constructors - TODO IN THE NEW ECMA CONCEPT! FIX IT!
 Node				GlobalObject::Node					DontDelete|Function 1
 DOMException		GlobalObject::DOMException			DontDelete|Function 1
 CSSRule			GlobalObject::CSSRule				DontDelete|Function 1
 CSSValue			GlobalObject::CSSValue				DontDelete|Function 1
 CSSPrimitiveValue	GlobalObject::CSSPrimitiveValue	DontDelete|Function 1
 Event				GlobalObject::Event				DontDelete|Function 1
 EventException		GlobalObject::EventException		DontDelete|Function 1
 MutationEvent		GlobalObject::MutationEvent		DontDelete|Function 1
@end
*/

namespace KDOM
{
	ECMA_IMPLEMENT_PROTOFUNC(GlobalObjectFunc, GlobalObject, GlobalObject)
	const KJS::ClassInfo GlobalObject::s_classInfo = { "GlobalObject", 0, &s_hashTable, 0 };
};

class GlobalObject::Private
{
public:
	Private(DocumentImpl *doc) : document(doc), globalq(0) { }
	virtual ~Private() { delete globalq; }

	DocumentImpl *document;
	GlobalQObject *globalq;
};

GlobalObject::GlobalObject(DocumentImpl *doc) : ObjectImp(), d(new Private(doc))
{
	d->globalq = new GlobalQObject(this);
}

GlobalObject::~GlobalObject()
{
	delete d;
}

GlobalObject *GlobalObject::retrieveActive(KJS::ExecState *exec)
{
	KJS::ValueImp *imp = exec->interpreter()->globalObject();
	Q_ASSERT(imp);

	return static_cast<GlobalObject *>(imp);
}

DocumentImpl *GlobalObject::doc() const
{
	if(!d)
		return 0;

	return d->document;
}

bool GlobalObject::hasProperty(KJS::ExecState *, const KJS::Identifier &) const
{
	return true; // See KJS::Window::hasProperty
}

KJS::ValueImp *GlobalObject::get(KJS::ExecState *exec, const KJS::Identifier &p) const
{
	kdDebug(26004) << "GlobalObject (" << this << ")::get " << p.qstring() << endl;

	// we don't want any operations on a closed window
	if(!d || !d->document)
	{
		if(d && p == "closed")
			return KJS::Boolean(true);

		kdDebug(26004) << " - no active document! Failing :(" << endl;
		return KJS::Undefined();
	}

	// Look for overrides first
	KJS::ValueImp *val = KJS::ObjectImp::get(exec, p);
	if(val->type() != KJS::UndefinedType)
		return isSafeScript(exec) ? val : KJS::Undefined();

	// Not the right way in the long run. Should use retrieve(m_doc)...
	ScriptInterpreter *interpreter = static_cast<ScriptInterpreter *>(exec->interpreter());

	const KJS::HashEntry *entry = KJS::Lookup::findEntry(&GlobalObject::s_hashTable, p);
	if(entry)
	{
		switch(entry->value)
		{
			case GlobalObject::Window:
				return const_cast<GlobalObject *>(this);
/*			case GlobalObject::Evt:
				return getDOMEvent(exec, Event(interpreter->currentEvent()));*/
			case GlobalObject::Document:
				// special case, Ecma::setup created it, so we don't need to do it
				return interpreter->getDOMObject(d->document);
			case GlobalObject::SetTimeout:
			case GlobalObject::ClearTimeout:
			case GlobalObject::SetInterval:
			case GlobalObject::ClearInterval:
			case GlobalObject::PrintNode:
			case GlobalObject::Alert:
			case GlobalObject::Confirm:
			case GlobalObject::Debug:
			case GlobalObject::Prompt:
			{
#if 0
				if(isSafeScript(exec))
					return KJS::lookupOrCreateFunction<GlobalObjectFunc>(exec, p, this, entry->value, entry->params, entry->attr);
				else
#endif
					return KJS::Undefined();
			}
		}
	}

	// This isn't necessarily a bug. Some code uses if(!window.blah) window.blah=1
	// But it can also mean something isn't loaded or implemented...
	kdDebug(26004) << "GlobalObject::get property not found: " << p.qstring() << endl;
	return KJS::Undefined();
}

void GlobalObject::put(KJS::ExecState *exec, const KJS::Identifier &propertyName, KJS::ValueImp *value, int attr)
{
	// If we are called by an internal KJS call, then directly jump to ObjectImp -> saves time
	// Also applies if we have a local override (e.g. "var location")
	if((attr != KJS::None && attr != KJS::DontDelete) || (KJS::ObjectImp::getDirect(propertyName) && isSafeScript(exec)))
	{
		KJS::ObjectImp::put(exec, propertyName, value, attr);
		return;
	}

	const KJS::HashEntry *entry = KJS::Lookup::findEntry(&GlobalObject::s_hashTable, propertyName);
	if(entry)
	{
#ifdef KJS_VERBOSE
		kdDebug(26004) << "GlobalObject (" << this << ")::put " << propertyName.qstring() << endl;
#endif
		//switch(entry->value)
		//{
		// ...
		//}
	}

	if(isSafeScript(exec))
		KJS::ObjectImp::put(exec, propertyName, value, attr);
}

bool GlobalObject::isSafeScript(KJS::ExecState *exec) const
{
	if(!d)
		return false;
		
	if(!d->document) // we are deleted? can't grant access :/
	{
		kdDebug(26004) << "GlobalObject::isSafeScript: failing! document = 0!" << endl;
		return false;
	}
	
	DocumentImpl *activeDocument = static_cast<ScriptInterpreter *>(exec->interpreter())->document();
	if(!activeDocument)
	{
		kdDebug(26004) << "GlobalObject::isSafeScript: current interpreter's doc is 0!" << endl;
		return false;
	}
	
	if(activeDocument == d->document) // Not calling from another frame, no problem.
		return true;

	return false;
}

void GlobalObject::clear(KJS::ExecState *exec)
{
	kdDebug(26004) << "GlobalObject::clear " << this << endl;

	if(!d)
		return;
		
	delete d->globalq;
	d->globalq = new GlobalQObject(this);;
	
	// Get rid of everything, those user vars could hold references to DOM nodes
	clearProperties();
	
	// Now recreate a working global object for the next URL that will use us
	KJS::Interpreter *interpreter = exec->interpreter();
	interpreter->initGlobalObject();
}

const KJS::ClassInfo *GlobalObject::classInfo() const
{
	return &s_classInfo;
}

// EcmaScript function handling
KJS::ValueImp *GlobalObjectFunc::callAsFunction(KJS::ExecState *exec, KJS::ObjectImp *thisObj, const KJS::List &args)
{
	if(!thisObj->inherits(&GlobalObject::s_classInfo))
	{
		KJS::ObjectImp *err = throwError(exec, KJS::TypeError);
		return err;
	}
	
	GlobalObject *global = static_cast<GlobalObject *>(thisObj);
	KJS::ValueImp *v = args[0];
	KJS::UString s = v->toString(exec);
	QString str = s.qstring();

	switch(id)
	{
		case GlobalObject::Alert:
#if APPLE_COMPILE_HACK
			//KWQ(part)->runJavaScriptAlert(str);
			fprintf(stderr, "Ignoring JavaScriptAlert: %s (l: %i)\n", str.ascii(), str.length());
#else
			KMessageBox::error(0, QStyleSheet::convertFromPlainText(str), QString::fromLatin1("JavaScript"));
#endif
			return KJS::Undefined();
		case GlobalObject::Confirm:
#if APPLE_COMPILE_HACK
			//KWQ(part)->runJavaScriptConfirm(str);
			fprintf(stderr, "Ignoring JavaScriptConfirm: %s (l: %i)\n", str.ascii(), str.length());
			return KJS::Boolean(false);
#else
		    return KJS::Boolean(KMessageBox::warningYesNo(0, QStyleSheet::convertFromPlainText(str), QString::fromLatin1("JavaScript"), i18n("&OK"), i18n("&Cancel")) == KMessageBox::Yes);
#endif
		case GlobalObject::Debug:
			kdDebug() << "[Debug] " << str << endl;
			return KJS::Undefined();
		case GlobalObject::ClearTimeout:
		case GlobalObject::ClearInterval:
			global->clearTimeout(v->toInt32(exec));
			return KJS::Undefined();
		case GlobalObject::PrintNode:
		{
			QString result;
		    QTextOStream out(&result);
/* FIXME
			Helper::PrintNode(out, ecma_cast<Node>(exec, args[0], &toNode));
*/
			return KJS::String(result);
		}
		case GlobalObject::Prompt:
		{
		    bool ok = false;
		    QString ret;

#if APPLE_COMPILE_HACK
			//ret = KWQ(part)->runJavaScriptPrompt(str, args.size() >= 2 ? args[1].toString(exec).qstring() : QString::null, ret);
			fprintf(stderr, "Ignoring JavaScriptPrompt: %s (l: %i)\n", str.ascii(), str.length());
#else
		    if(args.size() >= 2)
				ret = KInputDialog::getText(i18n("Prompt"), QStyleSheet::convertFromPlainText(str), args[1].toString(exec).qstring(), &ok);
			else
				ret = KInputDialog::getText(i18n("Prompt"), QStyleSheet::convertFromPlainText(str), QString::null, &ok);
#endif
			if(ok)
				return KJS::String(ret);
		    else
				return KJS::Null();
		}
		case GlobalObject::SetInterval:
		{
			if(args.size() >= 2 && v->isString())
			{
				int i = args[1]->toInt32(exec);
				int r = global->installTimeout(s, args[i]->toInt32(exec), false);
				return KJS::Number(r);
			}
			else if(args.size() >= 2 && v->isObject() && static_cast<KJS::ObjectImp *>(v)->implementsCall())
			{
				int i = args[1]->toInt32(exec);
				int r = global->installTimeout(s, i, false);
				return KJS::Number(r);
			}
			else
				return KJS::Undefined();
		}
		case GlobalObject::SetTimeout:
		{
			if(args.size() == 2 && v->isString())
			{
				int i = args[1]->toInt32(exec);
				int r = global->installTimeout(s, i, true); // single shot
				return KJS::Number(r);
			}
			else if(args.size() >= 2 && v->isObject() && static_cast<KJS::ObjectImp *>(v)->implementsCall())
			{
				int i = args[1]->toInt32(exec);
				int r = global->installTimeout(s, i, false);
				return KJS::Number(r);
			}
			else
				return KJS::Undefined();
		}
	}

	return KJS::Undefined();
}

int GlobalObject::installTimeout(const KJS::UString &handler, int t, bool singleShot)
{
	if(!d || !d->globalq)
		return 0;
	
	return d->globalq->installTimeout(handler, t, singleShot);
}

void GlobalObject::clearTimeout(int timerId)
{
	if(!d || !d->globalq)
		return;

	d->globalq->clearTimeout(timerId);
}

////////////////////// ScheduledAction ////////////////////////
ScheduledAction::ScheduledAction(KJS::ObjectImp *func, KJS::List args, bool singleShot)
{
	m_func = func;
	m_args = args;
	m_isFunction = true;
	m_singleShot = singleShot;
}

ScheduledAction::ScheduledAction(const QString &code, bool singleShot)
{
	m_code = code;
	m_isFunction = false;
	m_singleShot = singleShot;
}

ScheduledAction::~ScheduledAction()
{
}

void ScheduledAction::execute(GlobalObject *global)
{
	if(!global)
		return;

	Q_ASSERT(global->doc() != 0);

/* FIXME
	ScriptInterpreter *interpreter = global->doc()->ecmaEngine()->interpreter();
	if(m_isFunction)
	{
		if(m_func.implementsCall())
		{
			KJS::ExecState *exec = interpreter->globalExec();
			Q_ASSERT(global == interpreter->globalObject().imp());
			
			KJS::Object obj(global);
			m_func.call(exec, obj, m_args); // note that call() creates its own execution state for the func call
		}
	}
	else
		interpreter->evaluate(m_code);
*/
}

////////////////////// GlobalQObject ////////////////////////
GlobalQObject::GlobalQObject(GlobalObject *obj) : parent(obj)
{
}

GlobalQObject::~GlobalQObject()
{
	parentDestroyed(); // reuse same code
}

void GlobalQObject::parentDestroyed()
{
	killTimers();
	
	QMapIterator<int, ScheduledAction *> it;
	for(it = scheduledActions.begin(); it != scheduledActions.end(); ++it)
	{
		ScheduledAction *action = *it;
		delete action;
	}
	
	scheduledActions.clear();
}

int GlobalQObject::installTimeout(const KJS::UString &handler, int t, bool singleShot)
{
	if(t < 10) t = 10;
	int id = startTimer(t);
	ScheduledAction *action = new ScheduledAction(handler.qstring(), singleShot);
	scheduledActions.insert(id, action);
	return id;
}

int GlobalQObject::installTimeout(KJS::ValueImp *func, KJS::List args, int t, bool singleShot)
{
	KJS::ObjectImp *objFunc = static_cast<KJS::ObjectImp *>(func);
	if(t < 10) t = 10;
	int id = startTimer(t);
	scheduledActions.insert(id, new ScheduledAction(objFunc, args, singleShot));
	return id;
}

void GlobalQObject::clearTimeout(int timerId, bool delAction)
{
	killTimer(timerId);
	
	if(delAction)
	{
		QMapIterator<int, ScheduledAction *> it = scheduledActions.find(timerId);
		if(it != scheduledActions.end())
		{
			ScheduledAction *action = *it;
			scheduledActions.remove(it);
			delete action;
		}
	}
}

void GlobalQObject::timerEvent(QTimerEvent *e)
{
	QMapIterator<int, ScheduledAction *> it = scheduledActions.find(e->timerId());
	if(it != scheduledActions.end())
	{
		ScheduledAction *action = *it;

		// remove single shots installed by setTimeout()
		if(action->m_singleShot)
		{
			clearTimeout(e->timerId(), false);
			scheduledActions.remove(it);
		}
		
		if(parent->doc() != 0)
			action->execute(parent);

		parent->afterTimeout();
		
		// It is important to test singleShot and not action->singleShot here - the
		// action could have been deleted already if not single shot and if the
		// JS code called by execute() calls clearTimeout().
		if(action->m_singleShot)
			delete action;
	}
	else
		kdWarning(6070) << "GlobalQObject::timerEvent this=" << this << " timer " << e->timerId() << " not found (" << scheduledActions.count() << " actions in map)" << endl;
}

void GlobalQObject::timeoutClose()
{
}

// vim:ts=4:noet
