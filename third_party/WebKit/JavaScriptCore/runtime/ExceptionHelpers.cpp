/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ExceptionHelpers.h"

#include "CodeBlock.h"
#include "CallFrame.h"
#include "JSObject.h"
#include "JSNotAnObject.h"
#include "Interpreter.h"
#include "Nodes.h"

namespace JSC {

static void substitute(UString& string, const UString& substring)
{
    int position = string.find("%s");
    ASSERT(position != -1);
    UString newString = string.substr(0, position);
    newString.append(substring);
    newString.append(string.substr(position + 2));
    string = newString;
}

class InterruptedExecutionError : public JSObject {
public:
    InterruptedExecutionError(JSGlobalData* globalData)
        : JSObject(globalData->interruptedExecutionErrorStructure)
    {
    }

    virtual bool isWatchdogException() const { return true; }
};

JSValuePtr createInterruptedExecutionException(JSGlobalData* globalData)
{
    return new (globalData) InterruptedExecutionError(globalData);
}

JSValuePtr createError(ExecState* exec, ErrorType e, const char* msg)
{
    return Error::create(exec, e, msg, -1, -1, 0);
}

JSValuePtr createError(ExecState* exec, ErrorType e, const char* msg, const Identifier& label)
{
    UString message = msg;
    substitute(message, label.ustring());
    return Error::create(exec, e, message, -1, -1, 0);
}

JSValuePtr createError(ExecState* exec, ErrorType e, const char* msg, JSValuePtr v)
{
    UString message = msg;
    substitute(message, v->toString(exec));
    return Error::create(exec, e, message, -1, -1, 0);
}

JSValuePtr createStackOverflowError(ExecState* exec)
{
    return createError(exec, RangeError, "Maximum call stack size exceeded.");
}

JSValuePtr createUndefinedVariableError(ExecState* exec, const Identifier& ident, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);
    UString message = "Can't find variable: ";
    message.append(ident.ustring());
    JSObject* exception = Error::create(exec, ReferenceError, message, line, codeBlock->ownerNode()->sourceID(), codeBlock->ownerNode()->sourceURL());
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}
    
bool isStrWhiteSpace(UChar c);

static UString createErrorMessage(ExecState* exec, CodeBlock* codeBlock, int, int expressionStart, int expressionStop, JSValuePtr value, UString error)
{
    if (!expressionStop || expressionStart > codeBlock->source()->length()) {
        UString errorText = value->toString(exec);
        errorText.append(" is ");
        errorText.append(error);
        return errorText;
    }

    UString errorText = "Result of expression ";
    
    if (expressionStart < expressionStop) {
        errorText.append('\'');
        errorText.append(codeBlock->source()->getRange(expressionStart, expressionStop));
        errorText.append("' [");
        errorText.append(value->toString(exec));
        errorText.append("] is ");
    } else {
        // No range information, so give a few characters of context
        const UChar* data = codeBlock->source()->data();
        int dataLength = codeBlock->source()->length();
        int start = expressionStart;
        int stop = expressionStart;
        // Get up to 20 characters of context to the left and right of the divot, clamping to the line.
        // then strip whitespace.
        while (start > 0 && (expressionStart - start < 20) && data[start - 1] != '\n')
            start--;
        while (start < (expressionStart - 1) && isStrWhiteSpace(data[start]))
            start++;
        while (stop < dataLength && (stop - expressionStart < 20) && data[stop] != '\n')
            stop++;
        while (stop > expressionStart && isStrWhiteSpace(data[stop]))
            stop--;
        errorText.append("near '...");
        errorText.append(codeBlock->source()->getRange(start, stop));
        errorText.append("...' [");
        errorText.append(value->toString(exec));
        errorText.append("] is ");
    }
    errorText.append(error);
    errorText.append(".");
    return errorText;
}

JSObject* createInvalidParamError(ExecState* exec, const char* op, JSValuePtr value, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    UString message = "not a valid argument for '";
    message.append(op);
    message.append("'");
    
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);
    UString errorMessage = createErrorMessage(exec, codeBlock, line, divotPoint, divotPoint + endOffset, value, message);
    JSObject* exception = Error::create(exec, TypeError, errorMessage, line, codeBlock->ownerNode()->sourceID(), codeBlock->ownerNode()->sourceURL());
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}

JSObject* createNotAConstructorError(ExecState* exec, JSValuePtr value, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);

    // We're in a "new" expression, so we need to skip over the "new.." part
    int startPoint = divotPoint - (startOffset ? startOffset - 4 : 0); // -4 for "new "
    const UChar* data = codeBlock->source()->data();
    while (startPoint < divotPoint && isStrWhiteSpace(data[startPoint]))
        startPoint++;
    
    UString errorMessage = createErrorMessage(exec, codeBlock, line, startPoint, divotPoint, value, "not a constructor");
    JSObject* exception = Error::create(exec, TypeError, errorMessage, line, codeBlock->ownerNode()->sourceID(), codeBlock->ownerNode()->sourceURL());
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}

JSValuePtr createNotAFunctionError(ExecState* exec, JSValuePtr value, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);
    UString errorMessage = createErrorMessage(exec, codeBlock, line, divotPoint - startOffset, divotPoint, value, "not a function");
    JSObject* exception = Error::create(exec, TypeError, errorMessage, line, codeBlock->ownerNode()->sourceID(), codeBlock->ownerNode()->sourceURL());    
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}

JSNotAnObjectErrorStub* createNotAnObjectErrorStub(ExecState* exec, bool isNull)
{
    return new (exec) JSNotAnObjectErrorStub(exec, isNull);
}

JSObject* createNotAnObjectError(ExecState* exec, JSNotAnObjectErrorStub* error, unsigned bytecodeOffset, CodeBlock* codeBlock)
{
    // Both op_construct and op_instanceof require a use of op_get_by_id to get
    // the prototype property from an object. The exception messages for exceptions
    // thrown by these instances op_get_by_id need to reflect this.
    OpcodeID followingOpcodeID;
    if (codeBlock->getByIdExceptionInfoForBytecodeOffset(exec, bytecodeOffset, followingOpcodeID)) {
        ASSERT(followingOpcodeID == op_construct || followingOpcodeID == op_instanceof);
        if (followingOpcodeID == op_construct)
            return createNotAConstructorError(exec, error->isNull() ? jsNull() : jsUndefined(), bytecodeOffset, codeBlock);
        return createInvalidParamError(exec, "instanceof", error->isNull() ? jsNull() : jsUndefined(), bytecodeOffset, codeBlock);
    }

    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    int line = codeBlock->expressionRangeForBytecodeOffset(exec, bytecodeOffset, divotPoint, startOffset, endOffset);
    UString errorMessage = createErrorMessage(exec, codeBlock, line, divotPoint - startOffset, divotPoint, error->isNull() ? jsNull() : jsUndefined(), "not an object");
    JSObject* exception = Error::create(exec, TypeError, errorMessage, line, codeBlock->ownerNode()->sourceID(), codeBlock->ownerNode()->sourceURL());
    exception->putWithAttributes(exec, Identifier(exec, expressionBeginOffsetPropertyName), jsNumber(exec, divotPoint - startOffset), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionCaretOffsetPropertyName), jsNumber(exec, divotPoint), ReadOnly | DontDelete);
    exception->putWithAttributes(exec, Identifier(exec, expressionEndOffsetPropertyName), jsNumber(exec, divotPoint + endOffset), ReadOnly | DontDelete);
    return exception;
}

} // namespace JSC
