// -*- c-basic-offset: 2 -*-
/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2002 Apple Computer, Inc
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *  Boston, MA 02111-1307, USA.
 *
 */

#ifndef _KJS_COMPLETION_H_
#define _KJS_COMPLETION_H_

#include "value.h"

namespace KJS {

  /**
   * Completion types.
   */
  enum ComplType { Normal, Break, Continue, ReturnValue, Throw };

  /**
   * Completion objects are used to convey the return status and value
   * from functions.
   *
   * See @ref FunctionImp::execute()
   *
   * @see FunctionImp
   *
   * @short Handle for a Completion type.
   */
  class Completion : private Value {
  public:
    Completion(ComplType c = Normal, const Value& v = Value(),
               const UString &t = UString::null);
    Completion(CompletionImp *v);

    ComplType complType() const;
    Value value() const;
    UString target() const;
    bool isValueCompletion() const;
  private:
    ComplType comp;
    Value val;
    UString tar;
  };

}

#endif 
