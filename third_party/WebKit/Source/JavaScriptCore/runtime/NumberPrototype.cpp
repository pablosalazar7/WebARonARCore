/*
 *  Copyright (C) 1999-2000,2003 Harri Porten (porten@kde.org)
 *  Copyright (C) 2007, 2008, 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 *
 */

#include "config.h"
#include "NumberPrototype.h"

#include "BigInteger.h"
#include "Error.h"
#include "JSFunction.h"
#include "JSString.h"
#include "Operations.h"
#include "Uint16WithFraction.h"
#include "dtoa.h"
#include <wtf/Assertions.h>
#include <wtf/DecimalNumber.h>
#include <wtf/MathExtras.h>
#include <wtf/Vector.h>

namespace JSC {

static EncodedJSValue JSC_HOST_CALL numberProtoFuncToString(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncToLocaleString(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncValueOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncToFixed(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncToExponential(ExecState*);
static EncodedJSValue JSC_HOST_CALL numberProtoFuncToPrecision(ExecState*);

}

#include "NumberPrototype.lut.h"

namespace JSC {

const ClassInfo NumberPrototype::s_info = { "Number", &NumberObject::s_info, 0, ExecState::numberPrototypeTable };

/* Source for NumberPrototype.lut.h
@begin numberPrototypeTable
  toString          numberProtoFuncToString         DontEnum|Function 1
  toLocaleString    numberProtoFuncToLocaleString   DontEnum|Function 0
  valueOf           numberProtoFuncValueOf          DontEnum|Function 0
  toFixed           numberProtoFuncToFixed          DontEnum|Function 1
  toExponential     numberProtoFuncToExponential    DontEnum|Function 1
  toPrecision       numberProtoFuncToPrecision      DontEnum|Function 1
@end
*/

ASSERT_CLASS_FITS_IN_CELL(NumberPrototype);

NumberPrototype::NumberPrototype(ExecState* exec, JSGlobalObject* globalObject, Structure* structure)
    : NumberObject(exec->globalData(), structure)
{
    setInternalValue(exec->globalData(), jsNumber(0));

    ASSERT(inherits(&s_info));
    putAnonymousValue(globalObject->globalData(), 0, globalObject);
}

bool NumberPrototype::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<NumberObject>(exec, ExecState::numberPrototypeTable(exec), this, propertyName, slot);
}

bool NumberPrototype::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<NumberObject>(exec, ExecState::numberPrototypeTable(exec), this, propertyName, descriptor);
}

// ------------------------------ Functions ---------------------------

static ALWAYS_INLINE bool toThisNumber(JSValue thisValue, double &x)
{
    JSValue v = thisValue.getJSNumber();
    if (UNLIKELY(!v))
        return false;
    x = v.uncheckedGetNumber();
    return true;
}

static ALWAYS_INLINE bool getIntegerArgumentInRange(ExecState* exec, int low, int high, int& result, bool& isUndefined)
{
    result = 0;
    isUndefined = false;

    JSValue argument0 = exec->argument(0);
    if (argument0.isUndefined()) {
        isUndefined = true;
        return true;
    }

    double asDouble = argument0.toInteger(exec);
    if (asDouble < low || asDouble > high)
        return false;

    result = static_cast<int>(asDouble);
    return true;
}

// The largest finite floating point number is 1.mantissa * 2^(0x7fe-0x3ff).
// Since 2^N in binary is a one bit followed by N zero bits. 1 * 2^3ff requires
// at most 1024 characters to the left of a decimal point, in base 2 (1025 if
// we include a minus sign). For the fraction, a value with an exponent of 0
// has up to 52 bits to the right of the decimal point. Each decrement of the
// exponent down to a minimum of -0x3fe adds an additional digit to the length
// of the fraction. As such the maximum fraction size is 1075 (1076 including
// a point). We pick a buffer size such that can simply place the point in the
// center of the buffer, and are guaranteed to have enough space in each direction
// fo any number of digits an IEEE number may require to represent.
typedef char RadixBuffer[2180];

// Mapping from integers 0..35 to digit identifying this value, for radix 2..36.
static const char* const radixDigits = "0123456789abcdefghijklmnopqrstuvwxyz";

static char* toStringWithRadix(RadixBuffer& buffer, double number, unsigned radix)
{
    ASSERT(isfinite(number));
    ASSERT(radix >= 2 && radix <= 36);

    // Position the decimal point at the center of the string, set
    // the startOfResultString pointer to point at the decimal point.
    char* decimalPoint = buffer + sizeof(buffer) / 2;
    char* startOfResultString = decimalPoint;

    // Extract the sign.
    bool isNegative = number < 0;
    if (signbit(number))
        number = -number;
    double integerPart = floor(number);

    // We use this to test for odd values in odd radix bases.
    // Where the base is even, (e.g. 10), to determine whether a value is even we need only
    // consider the least significant digit. For example, 124 in base 10 is even, because '4'
    // is even. if the radix is odd, then the radix raised to an integer power is also odd.
    // E.g. in base 5, 124 represents (1 * 125 + 2 * 25 + 4 * 5). Since each digit in the value
    // is multiplied by an odd number, the result is even if the sum of all digits is even.
    //
    // For the integer portion of the result, we only need test whether the integer value is
    // even or odd. For each digit of the fraction added, we should invert our idea of whether
    // the number is odd if the new digit is odd.
    //
    // Also initialize digit to this value; for even radix values we only need track whether
    // the last individual digit was odd.
    bool integerPartIsOdd = integerPart <= static_cast<double>(0x1FFFFFFFFFFFFFull) && static_cast<int64_t>(integerPart) & 1;
    ASSERT(integerPartIsOdd == static_cast<bool>(fmod(integerPart, 2)));
    bool isOddInOddRadix = integerPartIsOdd;
    uint32_t digit = integerPartIsOdd;

    // Check if the value has a fractional part to convert.
    double fractionPart = number - integerPart;
    if (fractionPart) {
        // Write the decimal point now.
        *decimalPoint = '.';

        // Higher precision representation of the fractional part.
        Uint16WithFraction fraction(fractionPart);

        bool needsRoundingUp = false;
        char* endOfResultString = decimalPoint + 1;

        // Calculate the delta from the current number to the next & previous possible IEEE numbers.
        double nextNumber = nextafter(number, std::numeric_limits<double>::infinity());
        double lastNumber = nextafter(number, -std::numeric_limits<double>::infinity());
        ASSERT(isfinite(nextNumber) && !signbit(nextNumber));
        ASSERT(isfinite(lastNumber) && !signbit(lastNumber));
        double deltaNextDouble = nextNumber - number;
        double deltaLastDouble = number - lastNumber;
        ASSERT(isfinite(deltaNextDouble) && !signbit(deltaNextDouble));
        ASSERT(isfinite(deltaLastDouble) && !signbit(deltaLastDouble));

        // We track the delta from the current value to the next, to track how many digits of the
        // fraction we need to write. For example, if the value we are converting is precisely
        // 1.2345, so far we have written the digits "1.23" to a string leaving a remainder of
        // 0.45, and we want to determine whether we can round off, or whether we need to keep
        // appending digits ('4'). We can stop adding digits provided that then next possible
        // lower IEEE value is further from 1.23 than the remainder we'd be rounding off (0.45),
        // which is to say, less than 1.2255. Put another way, the delta between the prior
        // possible value and this number must be more than 2x the remainder we'd be rounding off
        // (or more simply half the delta between numbers must be greater than the remainder).
        //
        // Similarly we need track the delta to the next possible value, to dertermine whether
        // to round up. In almost all cases (other than at exponent boundaries) the deltas to
        // prior and subsequent values are identical, so we don't need track then separately.
        if (deltaNextDouble != deltaLastDouble) {
            // Since the deltas are different track them separately. Pre-multiply by 0.5.
            Uint16WithFraction halfDeltaNext(deltaNextDouble, 1);
            Uint16WithFraction halfDeltaLast(deltaLastDouble, 1);

            while (true) {
                // examine the remainder to determine whether we should be considering rounding
                // up or down. If remainder is precisely 0.5 rounding is to even.
                int dComparePoint5 = fraction.comparePoint5();
                if (dComparePoint5 > 0 || (!dComparePoint5 && (radix & 1 ? isOddInOddRadix : digit & 1))) {
                    // Check for rounding up; are we closer to the value we'd round off to than
                    // the next IEEE value would be?
                    if (fraction.sumGreaterThanOne(halfDeltaNext)) {
                        needsRoundingUp = true;
                        break;
                    }
                } else {
                    // Check for rounding down; are we closer to the value we'd round off to than
                    // the prior IEEE value would be?
                    if (fraction < halfDeltaLast)
                        break;
                }

                ASSERT(endOfResultString < (buffer + sizeof(buffer) - 1));
                // Write a digit to the string.
                fraction *= radix;
                digit = fraction.floorAndSubtract();
                *endOfResultString++ = radixDigits[digit];
                // Keep track whether the portion written is currently even, if the radix is odd.
                if (digit & 1)
                    isOddInOddRadix = !isOddInOddRadix;

                // Shift the fractions by radix.
                halfDeltaNext *= radix;
                halfDeltaLast *= radix;
            }
        } else {
            // This code is identical to that above, except since deltaNextDouble != deltaLastDouble
            // we don't need to track these two values separately.
            Uint16WithFraction halfDelta(deltaNextDouble, 1);

            while (true) {
                int dComparePoint5 = fraction.comparePoint5();
                if (dComparePoint5 > 0 || (!dComparePoint5 && (radix & 1 ? isOddInOddRadix : digit & 1))) {
                    if (fraction.sumGreaterThanOne(halfDelta)) {
                        needsRoundingUp = true;
                        break;
                    }
                } else if (fraction < halfDelta)
                    break;

                ASSERT(endOfResultString < (buffer + sizeof(buffer) - 1));
                fraction *= radix;
                digit = fraction.floorAndSubtract();
                if (digit & 1)
                    isOddInOddRadix = !isOddInOddRadix;
                *endOfResultString++ = radixDigits[digit];

                halfDelta *= radix;
            }
        }

        // Check if the fraction needs rounding off (flag set in the loop writing digits, above).
        if (needsRoundingUp) {
            // Whilst the last digit is the maximum in the current radix, remove it.
            // e.g. rounding up the last digit in "12.3999" is the same as rounding up the
            // last digit in "12.3" - both round up to "12.4".
            while (endOfResultString[-1] == radixDigits[radix - 1])
                --endOfResultString;

            // Radix digits are sequential in ascii/unicode, except for '9' and 'a'.
            // E.g. the first 'if' case handles rounding 67.89 to 67.8a in base 16.
            // The 'else if' case handles rounding of all other digits.
            if (endOfResultString[-1] == '9')
                endOfResultString[-1] = 'a';
            else if (endOfResultString[-1] != '.')
                ++endOfResultString[-1];
            else {
                // One other possibility - there may be no digits to round up in the fraction
                // (or all may be been rounded off already), in which case we may need to
                // round into the integer portion of the number. Remove the decimal point.
                --endOfResultString;
                // In order to get here there must have been a non-zero fraction, in which case
                // there must be at least one bit of the value's mantissa not in use in the
                // integer part of the number. As such, adding to the integer part should not
                // be able to lose precision.
                ASSERT((integerPart + 1) - integerPart == 1);
                ++integerPart;
            }
        } else {
            // We only need to check for trailing zeros if the value does not get rounded up.
            while (endOfResultString[-1] == '0')
                --endOfResultString;
        }

        *endOfResultString = '\0';
        ASSERT(endOfResultString < buffer + sizeof(buffer));
    } else
        *decimalPoint = '\0';

    BigInteger units(integerPart);

    // Always loop at least once, to emit at least '0'.
    do {
        ASSERT(buffer < startOfResultString);

        // Read a single digit and write it to the front of the string.
        // Divide by radix to remove one digit from the value.
        digit = units.divide(radix);
        *--startOfResultString = radixDigits[digit];
    } while (!!units);

    // If the number is negative, prepend '-'.
    if (isNegative)
        *--startOfResultString = '-';
    ASSERT(buffer <= startOfResultString);

    return startOfResultString;
}

// toExponential converts a number to a string, always formatting as an expoential.
// This method takes an optional argument specifying a number of *decimal places*
// to round the significand to (or, put another way, this method optionally rounds
// to argument-plus-one significant figures).
EncodedJSValue JSC_HOST_CALL numberProtoFuncToExponential(ExecState* exec)
{
    // Get x (the double value of this, which should be a Number).
    double x;
    if (!toThisNumber(exec->hostThisValue(), x))
        return throwVMTypeError(exec);

    // Get the argument. 
    int decimalPlacesInExponent;
    bool isUndefined;
    if (!getIntegerArgumentInRange(exec, 0, 20, decimalPlacesInExponent, isUndefined))
        return throwVMError(exec, createRangeError(exec, "toExponential() argument must be between 0 and 20"));

    // Handle NaN and Infinity.
    if (!isfinite(x))
        return JSValue::encode(jsString(exec, UString::number(x)));

    // Round if the argument is not undefined, always format as exponential.
    NumberToStringBuffer buffer;
    unsigned length = isUndefined
        ? DecimalNumber(x).toStringExponential(buffer, WTF::NumberToStringBufferLength)
        : DecimalNumber(x, RoundingSignificantFigures, decimalPlacesInExponent + 1).toStringExponential(buffer, WTF::NumberToStringBufferLength);

    return JSValue::encode(jsString(exec, UString(buffer, length)));
}

// toFixed converts a number to a string, always formatting as an a decimal fraction.
// This method takes an argument specifying a number of decimal places to round the
// significand to. However when converting large values (1e+21 and above) this
// method will instead fallback to calling ToString. 
EncodedJSValue JSC_HOST_CALL numberProtoFuncToFixed(ExecState* exec)
{
    // Get x (the double value of this, which should be a Number).
    JSValue thisValue = exec->hostThisValue();
    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);
    double x = v.uncheckedGetNumber();

    // Get the argument. 
    int decimalPlaces;
    bool isUndefined; // This is ignored; undefined treated as 0.
    if (!getIntegerArgumentInRange(exec, 0, 20, decimalPlaces, isUndefined))
        return throwVMError(exec, createRangeError(exec, "toFixed() argument must be between 0 and 20"));

    // 15.7.4.5.7 states "If x >= 10^21, then let m = ToString(x)"
    // This also covers Ininity, and structure the check so that NaN
    // values are also handled by numberToString
    if (!(fabs(x) < 1e+21))
        return JSValue::encode(jsString(exec, UString::number(x)));

    // The check above will return false for NaN or Infinity, these will be
    // handled by numberToString.
    ASSERT(isfinite(x));

    // Convert to decimal with rounding, and format as decimal.
    NumberToStringBuffer buffer;
    unsigned length = DecimalNumber(x, RoundingDecimalPlaces, decimalPlaces).toStringDecimal(buffer, WTF::NumberToStringBufferLength);
    return JSValue::encode(jsString(exec, UString(buffer, length)));
}

// toPrecision converts a number to a string, takeing an argument specifying a
// number of significant figures to round the significand to. For positive
// exponent, all values that can be represented using a decimal fraction will
// be, e.g. when rounding to 3 s.f. any value up to 999 will be formated as a
// decimal, whilst 1000 is converted to the exponential representation 1.00e+3.
// For negative exponents values >= 1e-6 are formated as decimal fractions,
// with smaller values converted to exponential representation.
EncodedJSValue JSC_HOST_CALL numberProtoFuncToPrecision(ExecState* exec)
{
    // Get x (the double value of this, which should be a Number).
    JSValue thisValue = exec->hostThisValue();
    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);
    double x = v.uncheckedGetNumber();

    // Get the argument. 
    int significantFigures;
    bool isUndefined;
    if (!getIntegerArgumentInRange(exec, 1, 21, significantFigures, isUndefined))
        return throwVMError(exec, createRangeError(exec, "toPrecision() argument must be between 1 and 21"));

    // To precision called with no argument is treated as ToString.
    if (isUndefined)
        return JSValue::encode(jsString(exec, UString::number(x)));

    // Handle NaN and Infinity.
    if (!isfinite(x))
        return JSValue::encode(jsString(exec, UString::number(x)));

    // Convert to decimal with rounding.
    DecimalNumber number(x, RoundingSignificantFigures, significantFigures);
    // If number is in the range 1e-6 <= x < pow(10, significantFigures) then format
    // as decimal. Otherwise, format the number as an exponential. Decimal format
    // demands a minimum of (exponent + 1) digits to represent a number, for example
    // 1234 (1.234e+3) requires 4 digits. (See ECMA-262 15.7.4.7.10.c)
    NumberToStringBuffer buffer;
    unsigned length = number.exponent() >= -6 && number.exponent() < significantFigures
        ? number.toStringDecimal(buffer, WTF::NumberToStringBufferLength)
        : number.toStringExponential(buffer, WTF::NumberToStringBufferLength);
    return JSValue::encode(jsString(exec, UString(buffer, length)));
}

EncodedJSValue JSC_HOST_CALL numberProtoFuncToString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);

    JSValue radixValue = exec->argument(0);
    int radix;
    if (radixValue.isInt32())
        radix = radixValue.asInt32();
    else if (radixValue.isUndefined())
        radix = 10;
    else
        radix = static_cast<int>(radixValue.toInteger(exec)); // nan -> 0

    if (radix == 10)
        return JSValue::encode(jsString(exec, v.toString(exec)));

    // Fast path for number to character conversion.
    if (radix == 36) {
        if (v.isInt32()) {
            int x = v.asInt32();
            if (static_cast<unsigned>(x) < 36) { // Exclude negatives
                JSGlobalData* globalData = &exec->globalData();
                return JSValue::encode(globalData->smallStrings.singleCharacterString(globalData, radixDigits[x]));
            }
        }
    }

    if (radix < 2 || radix > 36)
        return throwVMError(exec, createRangeError(exec, "toString() radix argument must be between 2 and 36"));

    double x = v.uncheckedGetNumber();
    if (!isfinite(x))
        return JSValue::encode(jsString(exec, UString::number(x)));

    RadixBuffer s;
    return JSValue::encode(jsString(exec, toStringWithRadix(s, x, radix)));
}

EncodedJSValue JSC_HOST_CALL numberProtoFuncToLocaleString(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    // FIXME: Not implemented yet.

    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);

    return JSValue::encode(jsString(exec, v.toString(exec)));
}

EncodedJSValue JSC_HOST_CALL numberProtoFuncValueOf(ExecState* exec)
{
    JSValue thisValue = exec->hostThisValue();
    JSValue v = thisValue.getJSNumber();
    if (!v)
        return throwVMTypeError(exec);

    return JSValue::encode(v);
}

} // namespace JSC
