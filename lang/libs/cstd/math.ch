/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
@extern
public func fabsf(arg : float) : float

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
@extern
public func fabs(arg : double) : double

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
@extern
public func fabsl(arg : longdouble) : longdouble

/**
 * TODO
 */
@extern
public type _Decimal32 = float
@extern
public type _Decimal64 = double
@extern
public type _Decimal128 = longdouble

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
@extern
public func fabsd32(arg : _Decimal32) : _Decimal32

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
@extern
public func fabsd64(arg : _Decimal64) : _Decimal64

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
@extern
public func fabsd128(arg : _Decimal128) : _Decimal128

/**
 * TODO macro fabs
 * Type-generic macro: If the argument has type _Decimal128, _Decimal64, _Decimal32,(since C23)long double, double, or float, fabsd128, fabsd64, fabsd32,(since C23)fabsl, fabs, or fabsf is called, respectively. Otherwise, if the argument has integer type, fabs is called. Otherwise, if the argument is complex, then the macro invokes the corresponding complex function (cabsf, cabs, cabsl). Otherwise, the behavior is undefined.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */

/**
 * Computes the floating-point remainder of the division operation x / y.
 * The floating-point remainder of the division operation x / y calculated by this function is exactly the value x - n * y, where n is x / y with its fractional part truncated.
 * The returned value has the same sign as x and is less or equal to y in magnitude.
 * @param x, y	-	floating-point values
 * @return If successful, returns the floating-point remainder of the division x / y as defined above.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fmod
 */
@extern
public func fmodf(x : float, y : float) : float

/**
 * Computes the floating-point remainder of the division operation x / y.
 * The floating-point remainder of the division operation x / y calculated by this function is exactly the value x - n * y, where n is x / y with its fractional part truncated.
 * The returned value has the same sign as x and is less or equal to y in magnitude.
 * @param x, y	-	floating-point values
 * @return If successful, returns the floating-point remainder of the division x / y as defined above.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fmod
 */
@extern
public func fmod(x : double, y : double) : double

/**
 * Computes the floating-point remainder of the division operation x / y.
 * The floating-point remainder of the division operation x / y calculated by this function is exactly the value x - n * y, where n is x / y with its fractional part truncated.
 * The returned value has the same sign as x and is less or equal to y in magnitude.
 * @param x, y	-	floating-point values
 * @return If successful, returns the floating-point remainder of the division x / y as defined above.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fmod
 */
@extern
public func fmodl(x : longdouble, y : longdouble) : longdouble

/**
 * TODO macro fmod
 * #define fmod( x, y )
 * Type-generic macro: If any argument has type long double, fmodl is called. Otherwise, if any argument has integer type or has type double, fmod is called. Otherwise, fmodf is called.
 * @see https://en.cppreference.com/w/c/numeric/math/fmod
 */

/**
 * Computes the IEEE remainder of the floating-point division operation x/y.
 * @param x, y	-	floating-point values
 * @return If successful, returns the IEEE floating-point remainder of the division x/y as defined above.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result is returned.
 * If y is zero, but the domain error does not occur, zero is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/remainder
 */
@extern
public func remainderf(x : float, y : float) : float

/**
 * Computes the IEEE remainder of the floating-point division operation x/y.
 * @param x, y	-	floating-point values
 * @return If successful, returns the IEEE floating-point remainder of the division x/y as defined above.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result is returned.
 * If y is zero, but the domain error does not occur, zero is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/remainder
 */
@extern
public func remainder(x : double, y : double) : double

/**
 * Computes the IEEE remainder of the floating-point division operation x/y.
 * @param x, y	-	floating-point values
 * @return If successful, returns the IEEE floating-point remainder of the division x/y as defined above.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result is returned.
 * If y is zero, but the domain error does not occur, zero is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/remainder
 */
@extern
public func remainderl(x : longdouble, y : longdouble) : longdouble

/**
 * Computes the floating-point remainder of the division operation x/y as the remainder() function does. Additionally, the sign and at least the three of the last bits of x/y will be stored in quo, sufficient to determine the octant of the result within a period.
 * @param x, y	-	floating-point values
 * @param quo	-	pointer to an integer value to store the sign and some bits of x/y
 * @return If successful, returns the floating-point remainder of the division x/y as defined in remainder, and stores, in *quo, the sign and at least three of the least significant bits of x/y (formally, stores a value whose sign is the sign of x/y and whose magnitude is congruent modulo 2n
 * to the magnitude of the integral quotient of x/y, where n is an implementation-defined integer greater than or equal to 3).
 * If y is zero, the value stored in *quo is unspecified.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result is returned if subnormals are supported.
 * If y is zero, but the domain error does not occur, zero is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/remquo
 */
@extern
public func remquof(x : float, y : float, quo : *mut int) : float

/**
 * Computes the floating-point remainder of the division operation x/y as the remainder() function does. Additionally, the sign and at least the three of the last bits of x/y will be stored in quo, sufficient to determine the octant of the result within a period.
 * @param x, y	-	floating-point values
 * @param quo	-	pointer to an integer value to store the sign and some bits of x/y
 * @return If successful, returns the floating-point remainder of the division x/y as defined in remainder, and stores, in *quo, the sign and at least three of the least significant bits of x/y (formally, stores a value whose sign is the sign of x/y and whose magnitude is congruent modulo 2n
 * to the magnitude of the integral quotient of x/y, where n is an implementation-defined integer greater than or equal to 3).
 * If y is zero, the value stored in *quo is unspecified.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result is returned if subnormals are supported.
 * If y is zero, but the domain error does not occur, zero is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/remquo
 */
@extern
public func remquo(x : double, y : double, quo : *mut int) : double

/**
 * Computes the floating-point remainder of the division operation x/y as the remainder() function does. Additionally, the sign and at least the three of the last bits of x/y will be stored in quo, sufficient to determine the octant of the result within a period.
 * @param x, y	-	floating-point values
 * @param quo	-	pointer to an integer value to store the sign and some bits of x/y
 * @return If successful, returns the floating-point remainder of the division x/y as defined in remainder, and stores, in *quo, the sign and at least three of the least significant bits of x/y (formally, stores a value whose sign is the sign of x/y and whose magnitude is congruent modulo 2n
 * to the magnitude of the integral quotient of x/y, where n is an implementation-defined integer greater than or equal to 3).
 * If y is zero, the value stored in *quo is unspecified.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result is returned if subnormals are supported.
 * If y is zero, but the domain error does not occur, zero is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/remquo
 */
@extern
public func remquol(x : longdouble, y : longdouble, quo : *mut int) : longdouble

/**
 * Computes (x * y) + z as if to infinite precision and rounded only once to fit the result type.
 * @param x, y, z	-	floating-point values
 * @return If successful, returns the value of (x * y) + z as if calculated to infinite precision and rounded once to fit the result type (or, alternatively, calculated as a single ternary floating-point operation).
 * If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 */
@extern
public func fmaf(x : float, y : float, z : float) : float

/**
 * Computes (x * y) + z as if to infinite precision and rounded only once to fit the result type.
 * @param x, y, z	-	floating-point values
 * @return If successful, returns the value of (x * y) + z as if calculated to infinite precision and rounded once to fit the result type (or, alternatively, calculated as a single ternary floating-point operation).
 * If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 */
@extern
public func fma(x : double, y : double, z : double) : double

/**
 * Computes (x * y) + z as if to infinite precision and rounded only once to fit the result type.
 * @param x, y, z	-	floating-point values
 * @return If successful, returns the value of (x * y) + z as if calculated to infinite precision and rounded once to fit the result type (or, alternatively, calculated as a single ternary floating-point operation).
 * If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 */
@extern
public func fmal(x : longdouble, y : longdouble, z : longdouble) : longdouble

/**
 * TODO these macros
 * If the macro constants FP_FAST_FMA, FP_FAST_FMAF, or FP_FAST_FMAL are defined, the corresponding function fma, fmaf, or fmal evaluates faster (in addition to being more precise) than the expression x * y + z for double, float, and long double arguments, respectively. If defined, these macros evaluate to integer 1.
 * #define FP_FAST_FMA  // implementation-defined
 * #define FP_FAST_FMAF // implementation-defined
 * #define FP_FAST_FMAL // implementation-defined
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 */

/**
 * Returns the larger of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the larger of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmax
 */
@extern
public func fmaxf(x : float, y : float) : float

/**
 * Returns the larger of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the larger of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmax
 */
@extern
public func fmax(x : double, y : double) : double

/**
 * Returns the larger of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the larger of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmax
 */
@extern
public func fmaxl(x : longdouble, y : longdouble) : longdouble

/**
 * Returns the smaller of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the smaller of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmin
 */
@extern
public func fminf(x : float, y : float) : float

/**
 * Returns the smaller of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the smaller of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmin
 */
@extern
public func fmin(x : double, y : double ) : double

/**
 * Returns the smaller of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the smaller of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmin
 */
@extern
public func fminl(x : longdouble, y : longdouble) : longdouble

/**
 * Returns the positive difference between x and y, that is, if x>y, returns x-y, otherwise (if x≤y), returns +0.
 * @param x, y	-	floating-point value
 * @return If successful, returns the positive difference between x and y.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fdim
 */
@extern
public func fdimf(x : float, y : float) : float

/**
 * Returns the positive difference between x and y, that is, if x>y, returns x-y, otherwise (if x≤y), returns +0.
 * @param x, y	-	floating-point value
 * @return If successful, returns the positive difference between x and y.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fdim
 */
@extern
public func fdim(x : double, y : double) : double

/**
 * Returns the positive difference between x and y, that is, if x>y, returns x-y, otherwise (if x≤y), returns +0.
 * @param x, y	-	floating-point value
 * @return If successful, returns the positive difference between x and y.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fdim
 */
@extern
public func fdiml(x : longdouble, y : longdouble) : longdouble

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call //strtoX//("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call //strtoX//("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call //strtoX//("NAN", (char**)NULL);.
 * The parsing function is strtof.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
@extern
public func nanf(arg : *char) : float

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call //strtoX//("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call //strtoX//("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call //strtoX//("NAN", (char**)NULL);.
 * The parsing function is strtod.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
@extern
public func nan(arg : *char) : double

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call //strtoX//("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call //strtoX//("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call //strtoX//("NAN", (char**)NULL);.
 * The parsing function is strtold.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
@extern
public func nanl(arg : *char) : longdouble

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call //strtoX//("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call //strtoX//("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call //strtoX//("NAN", (char**)NULL);.
 * The parsing function is strtod32.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
@extern
public func nand32(arg : *char) : _Decimal32

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call //strtoX//("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call //strtoX//("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call //strtoX//("NAN", (char**)NULL);.
 * The parsing function is strtod64.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
@extern
public func nand64(arg : *char) : _Decimal64

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call //strtoX//("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call //strtoX//("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call //strtoX//("NAN", (char**)NULL);.
 * The parsing function is strtod128.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
@extern
public func nand128(arg : *char) : _Decimal128

/**
 * Computes e (Euler's number, 2.7182818...) raised to the given power arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the base-e exponential of arg (earg) is returned.
 *         If a range error occurs due to overflow, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/exp
 */
@extern
public func expf(arg : float) : float

/**
 * Computes e (Euler's number, 2.7182818...) raised to the given power arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the base-e exponential of arg (earg) is returned.
 *         If a range error occurs due to overflow, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/exp
 */
@extern
public func exp(arg : double) : double

/**
 * Computes e (Euler's number, 2.7182818...) raised to the given power arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the base-e exponential of arg (earg) is returned.
 *         If a range error occurs due to overflow, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/exp
 */
@extern
public func expl(arg : longdouble ) : longdouble

/**
 * Computes 2 raised to the given power n.
 * @param n	-	floating-point value
 * @return If no errors occur, the base-2 exponential of n (2n) is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/exp2
 */
@extern
public func exp2f(n : float) : float

/**
 * Computes 2 raised to the given power n.
 * @param n	-	floating-point value
 * @return If no errors occur, the base-2 exponential of n (2n) is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/exp2
 */
@extern
public func exp2(n : double)  : double

/**
 * Computes 2 raised to the given power n.
 * @param n	-	floating-point value
 * @return If no errors occur, the base-2 exponential of n (2n) is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/exp2
 */
@extern
public func exp2l(n : longdouble) : longdouble

/**
 * Computes the e (Euler's number, 2.7182818) raised to the given power arg, minus 1.0. This function is more accurate than the expression exp(arg)-1.0 if arg is close to zero.
 * @param arg	-	floating-point value
 * @return If no errors occur earg -1 is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/expm1
 */
@extern
public func expm1f(arg : float) : float

/**
 * Computes the e (Euler's number, 2.7182818) raised to the given power arg, minus 1.0. This function is more accurate than the expression exp(arg)-1.0 if arg is close to zero.
 * @param arg	-	floating-point value
 * @return If no errors occur earg -1 is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/expm1
 */
@extern
public func expm1(arg : double) : double

/**
 * Computes the e (Euler's number, 2.7182818) raised to the given power arg, minus 1.0. This function is more accurate than the expression exp(arg)-1.0 if arg is close to zero.
 * @param arg	-	floating-point value
 * @return If no errors occur earg -1 is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/expm1
 */
@extern
public func expm1l(arg : longdouble) : longdouble

/**
 * Computes the natural (base e) logarithm of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the natural (base-e) logarithm of arg (ln(arg) or loge(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log
 */
@extern
public func logf(arg : float) : float

/**
 * Computes the natural (base e) logarithm of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the natural (base-e) logarithm of arg (ln(arg) or loge(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log
 */
@extern
public func log(arg : double) : double

/**
 * Computes the natural (base e) logarithm of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the natural (base-e) logarithm of arg (ln(arg) or loge(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log
 */
@extern
public func logl(arg : longdouble) : longdouble

/**
 * Computes the common (base-10) logarithm of arg.
 * @param floating-point value
 * @return If no errors occur, the common (base-10) logarithm of arg (log10(arg) or lg(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log10
 */
@extern
public func log10f(arg : float) : float

/**
 * Computes the common (base-10) logarithm of arg.
 * @param floating-point value
 * @return If no errors occur, the common (base-10) logarithm of arg (log10(arg) or lg(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log10
 */
@extern
public func log10(arg : double) : double

/**
 * Computes the common (base-10) logarithm of arg.
 * @param floating-point value
 * @return If no errors occur, the common (base-10) logarithm of arg (log10(arg) or lg(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log10
 */
@extern
public func log10l(arg : longdouble) : longdouble

/**
 * Computes the base 2 logarithm of arg.
 * @param arg	-	floating point value
 * @return If no errors occur, the base-2 logarithm of arg (log2(arg) or lb(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log2
 */
@extern
public func log2f(arg : float) : float

/**
 * Computes the base 2 logarithm of arg.
 * @param arg	-	floating point value
 * @return If no errors occur, the base-2 logarithm of arg (log2(arg) or lb(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log2
 */
@extern
public func log2(arg : double) : double

/**
 * Computes the base 2 logarithm of arg.
 * @param arg	-	floating point value
 * @return If no errors occur, the base-2 logarithm of arg (log2(arg) or lb(arg)) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log2
 */
@extern
public func log2l(arg : longdouble) : longdouble

/**
 * Computes the natural (base e) logarithm of 1 + arg. This function is more precise than the expression log(1 + arg) if arg is close to zero.
 * @param arg	-	floating-point value
 * @return If no errors occur ln(1 + arg) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log1p
 */
@extern
public func log1pf(arg : float) : float

/**
 * Computes the natural (base e) logarithm of 1 + arg. This function is more precise than the expression log(1 + arg) if arg is close to zero.
 * @param arg	-	floating-point value
 * @return If no errors occur ln(1 + arg) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log1p
 */
@extern
public func log1p(arg : double) : double

/**
 * Computes the natural (base e) logarithm of 1 + arg. This function is more precise than the expression log(1 + arg) if arg is close to zero.
 * @param arg	-	floating-point value
 * @return If no errors occur ln(1 + arg) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/log1p
 */
@extern
public func log1pl(arg :longdouble) : longdouble

/**
 * Computes the value of base raised to the power exponent
 * @param base	-	base as floating-point value
 * @param exponent	-	exponent as floating-point value
 * @return If no errors occur, base raised to the power of exponent (baseexponent) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error or a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/pow
 */
@extern
public func powf(base : float, exponent : float) : float

/**
 * Computes the value of base raised to the power exponent
 * @param base	-	base as floating-point value
 * @param exponent	-	exponent as floating-point value
 * @return If no errors occur, base raised to the power of exponent (baseexponent) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error or a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/pow
 */
@extern
public func pow(base : double, exponent : double) : double

/**
 * Computes the value of base raised to the power exponent
 * @param base	-	base as floating-point value
 * @param exponent	-	exponent as floating-point value
 * @return If no errors occur, base raised to the power of exponent (baseexponent) is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error or a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/pow
 */
@extern
public func powl(base : longdouble, exponent : longdouble) : longdouble

/**
 * Computes square root of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, square root of arg (√arg), is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sqrt
 */
@extern
public func sqrtf(arg : float) : float

/**
 * Computes square root of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, square root of arg (√arg), is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sqrt
 */
@extern
public func sqrt(arg : double) : double

/**
 * Computes square root of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, square root of arg (√arg), is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sqrt
 */
@extern
public func sqrtl(arg : longdouble) : longdouble

/**
 * Computes the cube root of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the cube root of arg (3√arg), is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cbrt
 */
@extern
public func cbrtf(arg : float) : float

/**
 * Computes the cube root of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the cube root of arg (3√arg), is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cbrt
 */
@extern
public func cbrt(arg : double) : double

/**
 * Computes the cube root of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the cube root of arg (3√arg), is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cbrt
 */
@extern
public func cbrtl(arg : longdouble) : longdouble

/**
 * Computes the square root of the sum of the squares of x and y, without undue overflow or underflow at intermediate stages of the computation.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the hypotenuse of a right-angled triangle,√x2+y2, is returned.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/hypot
 */
@extern
public func hypotf(x : float, y : float) : float

/**
 * Computes the square root of the sum of the squares of x and y, without undue overflow or underflow at intermediate stages of the computation.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the hypotenuse of a right-angled triangle,√x2+y2, is returned.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/hypot
 */
@extern
public func hypot(x : double, y : double) : double

/**
 * this is just to support declarations using __float128 in Tiny CC
 */
if(compiler::is_tcc_based()) {

    public type __float_128 = longdouble;

} else {

    public type __float_128 = float128;

}

/**
 * Computes the square root of the sum of the squares of x and y, without undue overflow or underflow at intermediate stages of the computation.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the hypotenuse of a right-angled triangle,√x2+y2, is returned.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/hypot
 */
@extern
public func hypotl(x : longdouble, y : __float_128) : __float_128

/**
 * Computes the sine of arg (measured in radians).
 * @param arg	-	floating-point value representing an angle in radians
 * @return If no errors occur, the sine of arg (sin(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sin
 */
@extern
public func sinf(arg : float) : float

/**
 * Computes the sine of arg (measured in radians).
 * @param arg	-	floating-point value representing an angle in radians
 * @return If no errors occur, the sine of arg (sin(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sin
 */
@extern
public func sin(arg : double) : double

/**
 * Computes the sine of arg (measured in radians).
 * @param arg	-	floating-point value representing an angle in radians
 * @return If no errors occur, the sine of arg (sin(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sin
 */
@extern
public func sinl(arg : __float_128) : __float_128

/**
 * Computes the sine of arg (measured in radians).
 * @param arg	-	floating-point value representing an angle in radians
 * @return If no errors occur, the sine of arg (sin(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sin
 */
@extern
public func sind32(arg : _Decimal32) : _Decimal32

/**
 * Computes the sine of arg (measured in radians).
 * @param arg	-	floating-point value representing an angle in radians
 * @return If no errors occur, the sine of arg (sin(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sin
 */
@extern
public func sind64(arg : _Decimal64) : _Decimal64

/**
 * Computes the sine of arg (measured in radians).
 * @param arg	-	floating-point value representing an angle in radians
 * @return If no errors occur, the sine of arg (sin(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sin
 */
@extern
public func sind128(arg : _Decimal128) : _Decimal128

/**
 * Computes the cosine of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the cosine of arg (cos(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cos
 */
@extern
public func cosf(arg : float) : float

/**
 * Computes the cosine of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the cosine of arg (cos(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cos
 */
@extern
public func cos(arg : double) : double

/**
 * Computes the cosine of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the cosine of arg (cos(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cos
 */
@extern
public func cosl(arg : __float_128) : __float_128

/**
 * Computes the cosine of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the cosine of arg (cos(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cos
 */
@extern
public func cosd32(arg : _Decimal32) : _Decimal32

/**
 * Computes the cosine of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the cosine of arg (cos(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cos
 */
@extern
public func cosd64(arg : _Decimal64) : _Decimal64

/**
 * Computes the cosine of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the cosine of arg (cos(arg)) in the range [-1 ; +1], is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cos
 */
@extern
public func cosd128(arg : _Decimal128) : _Decimal128

/**
 * Computes the tangent of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the tangent of arg (tan(arg)) is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tan
 */
@extern
public func tanf(arg : float) : float

/**
 * Computes the tangent of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the tangent of arg (tan(arg)) is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tan
 */
@extern
public func tan(arg : double) : double

/**
 * Computes the tangent of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the tangent of arg (tan(arg)) is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tan
 */
@extern
public func tanl(arg : __float_128) : __float_128

/**
 * Computes the tangent of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the tangent of arg (tan(arg)) is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tan
 */
@extern
public func tand32(arg : _Decimal32) : _Decimal32

/**
 * Computes the tangent of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the tangent of arg (tan(arg)) is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tan
 */
@extern
public func tand64(arg : _Decimal64) : _Decimal64

/**
 * Computes the tangent of arg (measured in radians).
 * @param arg	-	floating-point value representing angle in radians
 * @return If no errors occur, the tangent of arg (tan(arg)) is returned.
 *         The result may have little or no significance if the magnitude of arg is large.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tan
 */
@extern
public func tand128(arg : _Decimal128) : _Decimal128

/**
 * Computes the principal values of the arc sine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc sine of arg (arcsin(arg)) in the range [-π2; +π2], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asin
 */
@extern
public func asinf(arg : float) : float

/**
 * Computes the principal values of the arc sine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc sine of arg (arcsin(arg)) in the range [-π2; +π2], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asin
 */
@extern
public func asin(arg : double) : double

/**
 * Computes the principal values of the arc sine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc sine of arg (arcsin(arg)) in the range [-π2; +π2], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asin
 */
@extern
public func asinl(arg : __float_128) : __float_128

/**
 * Computes the principal values of the arc sine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc sine of arg (arcsin(arg)) in the range [-π2; +π2], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asin
 */
@extern
public func asind32(arg : _Decimal32) : _Decimal32

/**
 * Computes the principal values of the arc sine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc sine of arg (arcsin(arg)) in the range [-π2; +π2], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asin
 */
@extern
public func asind64(arg : _Decimal64) : _Decimal64

/**
 * Computes the principal values of the arc sine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc sine of arg (arcsin(arg)) in the range [-π2; +π2], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asin
 */
@extern
public func asind128(arg : _Decimal128) : _Decimal128

/**
 * Computes the principal value of the arc cosine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc cosine of arg (arccos(arg)) in the range [0 ; π], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/acos
 */
@extern
public func acosf(arg : float) : float

/**
 * Computes the principal value of the arc cosine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc cosine of arg (arccos(arg)) in the range [0 ; π], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/acos
 */
@extern
public func acos(arg : double) : double

/**
 * Computes the principal value of the arc cosine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc cosine of arg (arccos(arg)) in the range [0 ; π], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/acos
 */
@extern
public func acosl(arg : __float_128) : __float_128

/**
 * Computes the principal value of the arc cosine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc cosine of arg (arccos(arg)) in the range [0 ; π], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/acos
 */
@extern
public func acosd32(arg : _Decimal32) : _Decimal32

/**
 * Computes the principal value of the arc cosine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc cosine of arg (arccos(arg)) in the range [0 ; π], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/acos
 */
@extern
public func acosd64(arg : _Decimal64) : _Decimal64

/**
 * Computes the principal value of the arc cosine of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc cosine of arg (arccos(arg)) in the range [0 ; π], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/acos
 */
@extern
public func acosd128(arg : _Decimal128) : _Decimal128

/**
 * Computes the principal value of the arc tangent of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc tangent of arg (arctan(arg)) in the range [-π2; +π2] radians, is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan
 */
@extern
public func atanf(arg : float) : float

/**
 * Computes the principal value of the arc tangent of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc tangent of arg (arctan(arg)) in the range [-π2; +π2] radians, is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan
 */
@extern
public func atan(arg : double) : double

/**
 * Computes the principal value of the arc tangent of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc tangent of arg (arctan(arg)) in the range [-π2; +π2] radians, is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan
 */
@extern
public func atanl(arg : __float_128) : __float_128

/**
 * Computes the principal value of the arc tangent of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc tangent of arg (arctan(arg)) in the range [-π2; +π2] radians, is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan
 */
@extern
public func atand32(arg : _Decimal32) : _Decimal32

/**
 * Computes the principal value of the arc tangent of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc tangent of arg (arctan(arg)) in the range [-π2; +π2] radians, is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan
 */
@extern
public func atand64(arg : _Decimal64) : _Decimal64

/**
 * Computes the principal value of the arc tangent of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the arc tangent of arg (arctan(arg)) in the range [-π2; +π2] radians, is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan
 */
@extern
public func atand128(arg : _Decimal128) : _Decimal128

/**
 * Computes the arc tangent of y / x using the signs of arguments to determine the correct quadrant.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the arc tangent of y / x (arctan(y/x)) in the range [-π ; +π] radians, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan2
 */
@extern
public func atan2f(y : float, x : float ) : float

/**
 * Computes the arc tangent of y / x using the signs of arguments to determine the correct quadrant.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the arc tangent of y / x (arctan(y/x)) in the range [-π ; +π] radians, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan2
 */
@extern
public func atan2(y : double, x : double ) : double

/**
 * Computes the arc tangent of y / x using the signs of arguments to determine the correct quadrant.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the arc tangent of y / x (arctan(y/x)) in the range [-π ; +π] radians, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan2
 */
@extern
public func atan2l(y : __float_128, x : __float_128 ) : __float_128

/**
 * Computes the arc tangent of y / x using the signs of arguments to determine the correct quadrant.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the arc tangent of y / x (arctan(y/x)) in the range [-π ; +π] radians, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan2
 */
@extern
public func atan2d32(y : _Decimal32, x : _Decimal32 ) : _Decimal32

/**
 * Computes the arc tangent of y / x using the signs of arguments to determine the correct quadrant.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the arc tangent of y / x (arctan(y/x)) in the range [-π ; +π] radians, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan2
 */
@extern
public func atan2d64(y : _Decimal64, x : _Decimal64 ) : _Decimal64

/**
 * Computes the arc tangent of y / x using the signs of arguments to determine the correct quadrant.
 * @param x, y	-	floating-point value
 * @return If no errors occur, the arc tangent of y / x (arctan(y/x)) in the range [-π ; +π] radians, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atan2
 */
@extern
public func atan2d128(y : _Decimal128, x : _Decimal128 ) : _Decimal128

/**
 * Computes hyperbolic sine of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic sine of arg (sinh(arg), or earg-e-arg2) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sinh
 */
@extern
public func sinhf(arg : float) : float

/**
 * Computes hyperbolic sine of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic sine of arg (sinh(arg), or earg-e-arg2) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sinh
 */
@extern
public func sinh(arg : double) : double

/**
 * Computes hyperbolic sine of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic sine of arg (sinh(arg), or earg-e-arg2) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/sinh
 */
@extern
public func sinhl(arg : __float_128) : __float_128

/**
 * Computes the hyperbolic cosine of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic cosine of arg (cosh(arg), or earg+e-arg2) is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cosh
 */
@extern
public func coshf(arg : float) : float

/**
 * Computes the hyperbolic cosine of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic cosine of arg (cosh(arg), or earg+e-arg2) is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cosh
 */
@extern
public func cosh(arg : double) : double

/**
 * Computes the hyperbolic cosine of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic cosine of arg (cosh(arg), or earg+e-arg2) is returned.
 *         If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/cosh
 */
@extern
public func coshl(arg : __float_128) : __float_128

/**
 * Computes the hyperbolic tangent of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic tangent of arg (tanh(arg), or earg-e-argearg+e-arg) is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tanh
 */
@extern
public func tanhf(arg : float) : float

/**
 * Computes the hyperbolic tangent of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic tangent of arg (tanh(arg), or earg-e-argearg+e-arg) is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tanh
 */
@extern
public func tanh(arg : double) : double

/**
 * Computes the hyperbolic tangent of arg.
 * @param arg	-	floating-point value representing a hyperbolic angle
 * @return If no errors occur, the hyperbolic tangent of arg (tanh(arg), or earg-e-argearg+e-arg) is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/tanh
 */
@extern
public func tanhl(arg : __float_128) : __float_128

/**
 * Computes the inverse hyperbolic sine of arg.
 * @param arg	-	floating-point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic sine of arg (sinh-1 (arg), or arsinh(arg)), is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asinh
 */
@extern
public func asinhf(arg : float) : float

/**
 * Computes the inverse hyperbolic sine of arg.
 * @param arg	-	floating-point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic sine of arg (sinh-1 (arg), or arsinh(arg)), is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asinh
 */
@extern
public func asinh(arg : double) : double

/**
 * Computes the inverse hyperbolic sine of arg.
 * @param arg	-	floating-point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic sine of arg (sinh-1 (arg), or arsinh(arg)), is returned.
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/asinh
 */
@extern
public func asinhl(arg : __float_128) : __float_128

/**
 * Computes the inverse hyperbolic cosine of arg.
 * @param arg	-	floating-point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic cosine of arg (cosh-1 (arg), or arcosh(arg)) on the interval [0, +∞], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * @see https://en.cppreference.com/w/c/numeric/math/acosh
 */
@extern
public func acoshf(arg : float) : float

/**
 * Computes the inverse hyperbolic cosine of arg.
 * @param arg	-	floating-point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic cosine of arg (cosh-1 (arg), or arcosh(arg)) on the interval [0, +∞], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * @see https://en.cppreference.com/w/c/numeric/math/acosh
 */
@extern
public func acosh(arg : double) : double

/**
 * Computes the inverse hyperbolic cosine of arg.
 * @param arg	-	floating-point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic cosine of arg (cosh-1 (arg), or arcosh(arg)) on the interval [0, +∞], is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * @see https://en.cppreference.com/w/c/numeric/math/acosh
 */
@extern
public func acoshl(arg : __float_128) : __float_128

/**
 * Computes the inverse hyperbolic tangent of arg.
 * @param arg	-	floating_point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic tangent of arg (tanh-1 (arg), or artanh(arg)), is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the correct sign).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atanh
 */
@extern
public func atanhf(arg : float) : float

/**
 * Computes the inverse hyperbolic tangent of arg.
 * @param arg	-	floating_point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic tangent of arg (tanh-1 (arg), or artanh(arg)), is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the correct sign).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atanh
 */
@extern
public func atanh(arg : double) : double

/**
 * Computes the inverse hyperbolic tangent of arg.
 * @param arg	-	floating_point value representing the area of a hyperbolic sector
 * @return If no errors occur, the inverse hyperbolic tangent of arg (tanh-1 (arg), or artanh(arg)), is returned.
 *         If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 *         If a pole error occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the correct sign).
 *         If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/atanh
 */
@extern
public func atanhl(arg : __float_128) : __float_128

/**
 * Computes the error function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, value of the error function of arg, that is2√π∫arg0e−t2dt, is returned. If a range error occurs due to underflow, the correct result (after rounding), that is2⋅arg√π, is returned.
 * @see https://en.wikipedia.org/wiki/Error_function
 * @see https://en.cppreference.com/w/c/numeric/math/erf
 */
@extern
public func erff(arg : float) : float

/**
 * Computes the error function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, value of the error function of arg, that is2√π∫arg0e−t2dt, is returned. If a range error occurs due to underflow, the correct result (after rounding), that is2⋅arg√π, is returned.
 * @see https://en.wikipedia.org/wiki/Error_function
 * @see https://en.cppreference.com/w/c/numeric/math/erf
 */
@extern
public func erf(arg : double) : double

/**
 * Computes the error function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, value of the error function of arg, that is 2√π∫arg0e−t2dt, is returned. If a range error occurs due to underflow, the correct result (after rounding), that is2⋅arg√π, is returned.
 * @see https://en.wikipedia.org/wiki/Error_function
 * @see https://en.cppreference.com/w/c/numeric/math/erf
 */
@extern
public func erfl(arg : __float_128) : __float_128

/**
 * Computes the complementary error function of arg, that is 1.0 - erf(arg), but without loss of precision for large arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, value of the complementary error function of arg, that is 2√π∫∞arge−t2dt or1−erf(arg), is returned.If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/erfc
 * @see https://en.wikipedia.org/wiki/Complementary_error_function
 */
@extern
public func erfcf(arg : float) : float

/**
 * Computes the complementary error function of arg, that is 1.0 - erf(arg), but without loss of precision for large arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, value of the complementary error function of arg, that is 2√π∫∞arge−t2dt or1−erf(arg), is returned.If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/erfc
 * @see https://en.wikipedia.org/wiki/Complementary_error_function
 */
@extern
public func erfc(arg : double) : double

/**
 * Computes the complementary error function of arg, that is 1.0 - erf(arg), but without loss of precision for large arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, value of the complementary error function of arg, that is 2√π∫∞arge−t2dt or1−erf(arg), is returned.If a range error occurs due to underflow, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/erfc
 * @see https://en.wikipedia.org/wiki/Complementary_error_function
 */
@extern
public func erfcl(arg : __float_128) : __float_128

/**
 * Computes the gamma function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the value of the gamma function of arg, that is Γ(arg)=∫∞0targ−1e−tdt, is returned.
 *         If a domain error occurs, an implementation-defined value (NaN where supported) is returned.
 *         If a pole error occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.wikipedia.org/wiki/Gamma_function
 * @see https://en.cppreference.com/w/c/numeric/math/tgamma
 */
@extern
public func tgammaf(arg : float) : float

/**
 * Computes the gamma function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the value of the gamma function of arg, that is Γ(arg)=∫∞0targ−1e−tdt, is returned.
 *         If a domain error occurs, an implementation-defined value (NaN where supported) is returned.
 *         If a pole error occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.wikipedia.org/wiki/Gamma_function
 * @see https://en.cppreference.com/w/c/numeric/math/tgamma
 */
@extern
public func tgamma(arg : double) : double

/**
 * Computes the gamma function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the value of the gamma function of arg, that is Γ(arg)=∫∞0targ−1e−tdt, is returned.
 *         If a domain error occurs, an implementation-defined value (NaN where supported) is returned.
 *         If a pole error occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.wikipedia.org/wiki/Gamma_function
 * @see https://en.cppreference.com/w/c/numeric/math/tgamma
 */
@extern
public func tgammal(arg : __float_128) : __float_128

/**
 * Computes the natural logarithm of the absolute value of the gamma function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the value of the logarithm of the gamma function of arg, that is loge|∫∞0targ−1e−tdt|, is returned.
 *         If a pole error occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/lgamma
 */
@extern
public func lgammaf(arg : float) : float

/**
 * Computes the natural logarithm of the absolute value of the gamma function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the value of the logarithm of the gamma function of arg, that is loge|∫∞0targ−1e−tdt|, is returned.
 *         If a pole error occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/lgamma
 */
@extern
public func lgamma(arg : double) : double

/**
 * Computes the natural logarithm of the absolute value of the gamma function of arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the value of the logarithm of the gamma function of arg, that is loge|∫∞0targ−1e−tdt|, is returned.
 *         If a pole error occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/lgamma
 */
@extern
public func lgammal(arg : __float_128) : __float_128

/**
 * Computes the smallest integer value not less than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the smallest integer value not less than arg, that is ⌈arg⌉, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/ceil
 */
@extern
public func ceilf(arg : float) : float

/**
 * Computes the smallest integer value not less than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the smallest integer value not less than arg, that is ⌈arg⌉, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/ceil
 */
@extern
public func ceil(arg : double) : double

/**
 * Computes the smallest integer value not less than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the smallest integer value not less than arg, that is ⌈arg⌉, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/ceil
 */
@extern
public func ceill(arg : __float_128) : __float_128

/**
 * Computes the largest integer value not greater than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the largest integer value not greater than arg, that is ⌊arg⌋, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/floor
 */
@extern
public func floorf(arg : float) : float

/**
 * Computes the largest integer value not greater than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the largest integer value not greater than arg, that is ⌊arg⌋, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/floor
 */
@extern
public func floor(arg : double) : double

/**
 * Computes the largest integer value not greater than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the largest integer value not greater than arg, that is ⌊arg⌋, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/floor
 */
@extern
public func floorl(arg : __float_128) : __float_128

/**
 * Computes the nearest integer not greater in magnitude than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value not greater in magnitude than arg (in other words, arg rounded towards zero), is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/trunc
 */
@extern
public func truncf(arg : float) : float

/**
 * Computes the nearest integer not greater in magnitude than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value not greater in magnitude than arg (in other words, arg rounded towards zero), is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/trunc
 */
@extern
public func trunc(arg : double) : double

/**
 * Computes the nearest integer not greater in magnitude than arg.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value not greater in magnitude than arg (in other words, arg rounded towards zero), is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/trunc
 */
@extern
public func truncl(arg : __float_128) : __float_128

/**
 * Computes the nearest integer value to arg (in floating-point format), rounding halfway cases away from zero, regardless of the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func roundf(arg : float) : float

/**
 * Computes the nearest integer value to arg (in floating-point format), rounding halfway cases away from zero, regardless of the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func round(arg : double) : double

/**
 * Computes the nearest integer value to arg (in floating-point format), rounding halfway cases away from zero, regardless of the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func roundl(arg : __float_128) : __float_128

/**
 * Computes the nearest integer value to arg (in integer format), rounding halfway cases away from zero, regardless of the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func lroundf(arg : float) : long

/**
 * Computes the nearest integer value to arg (in integer format), rounding halfway cases away from zero, regardless of the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func lround(arg : double) : long

/**
 * Computes the nearest integer value to arg (in integer format), rounding halfway cases away from zero, regardless of the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func lroundl(arg : __float_128) : long

/**
 * Type-generic macros: If arg has type long double, roundl, lroundl, llroundl is called. Otherwise, if arg has integer type or the type double, round, lround, llround is called. Otherwise, roundf, lroundf, llroundf is called, respectively.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func llroundf(arg : float) : bigint

/**
 * Type-generic macros: If arg has type long double, roundl, lroundl, llroundl is called. Otherwise, if arg has integer type or the type double, round, lround, llround is called. Otherwise, roundf, lroundf, llroundf is called, respectively.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func llround(arg : double) : bigint

/**
 * Type-generic macros: If arg has type long double, roundl, lroundl, llroundl is called. Otherwise, if arg has integer type or the type double, round, lround, llround is called. Otherwise, roundf, lroundf, llroundf is called, respectively.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, rounding halfway cases away from zero, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/round
 */
@extern
public func llroundl(arg : __float_128) : bigint

/**
 * Rounds the floating-point argument arg to an integer value in floating-point format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return The nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nearbyint
 */
@extern
public func nearbyintf(arg : float) : float

/**
 * Rounds the floating-point argument arg to an integer value in floating-point format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return The nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nearbyint
 */
@extern
public func nearbyint(arg : double) : double

/**
 * Rounds the floating-point argument arg to an integer value in floating-point format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return The nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nearbyint
 */
@extern
public func nearbyintl(arg : __float_128) : __float_128

/**
 * Rounds the floating-point argument arg to an integer value in floating-point format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func rintf(arg : float) : float

/**
 * Rounds the floating-point argument arg to an integer value in floating-point format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func rint(arg : double) : double

/**
 * Rounds the floating-point argument arg to an integer value in floating-point format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func rintl(arg : __float_128) : __float_128

/**
 * Rounds the floating-point argument arg to an integer value in integer format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func lrintf(arg : float) : long

/**
 * Rounds the floating-point argument arg to an integer value in integer format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func lrint(arg : double) : long

/**
 * Rounds the floating-point argument arg to an integer value in integer format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func lrintl(arg : __float_128) : long

/**
 * Rounds the floating-point argument arg to an integer value in integer format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func llrintf(arg : float) : bigint

/**
 * Rounds the floating-point argument arg to an integer value in integer format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func llrint(arg : double) : bigint

/**
 * Rounds the floating-point argument arg to an integer value in integer format, using the current rounding mode.
 * @param arg	-	floating-point value
 * @return If no errors occur, the nearest integer value to arg, according to the current rounding mode, is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/rint
 */
@extern
public func llrintl(arg : __float_128) : bigint

/**
 * Decomposes given floating-point value x into a normalized fraction and an integral power of two.
 * @parm arg	-	floating-point value
 * @parm exp	-	pointer to integer value to store the exponent to
 * @return If arg is zero, returns zero and stores zero in *exp.
 *         Otherwise (if arg is not zero), if no errors occur, returns the value x in the range (-1;-0.5], [0.5; 1) and stores an integer value in *exp such that x×2(*exp)=arg.
 *         If the value to be stored in *exp is outside the range of int, the behavior is unspecified.
 *         If arg is not a floating-point number, the behavior is unspecified.
 * @see https://en.cppreference.com/w/c/numeric/math/frexp
 */
@extern
public func frexpf(arg : float, exp : *int) : float

/**
 * Decomposes given floating-point value x into a normalized fraction and an integral power of two.
 * @parm arg	-	floating-point value
 * @parm exp	-	pointer to integer value to store the exponent to
 * @return If arg is zero, returns zero and stores zero in *exp.
 *         Otherwise (if arg is not zero), if no errors occur, returns the value x in the range (-1;-0.5], [0.5; 1) and stores an integer value in *exp such that x×2(*exp)=arg.
 *         If the value to be stored in *exp is outside the range of int, the behavior is unspecified.
 *         If arg is not a floating-point number, the behavior is unspecified.
 * @see https://en.cppreference.com/w/c/numeric/math/frexp
 */
@extern
public func frexp(arg : double, exp : *int) : double

/**
 * Decomposes given floating-point value x into a normalized fraction and an integral power of two.
 * @parm arg	-	floating-point value
 * @parm exp	-	pointer to integer value to store the exponent to
 * @return If arg is zero, returns zero and stores zero in *exp.
 *         Otherwise (if arg is not zero), if no errors occur, returns the value x in the range (-1;-0.5], [0.5; 1) and stores an integer value in *exp such that x×2(*exp)=arg.
 *         If the value to be stored in *exp is outside the range of int, the behavior is unspecified.
 *         If arg is not a floating-point number, the behavior is unspecified.
 * @see https://en.cppreference.com/w/c/numeric/math/frexp
 */
@extern
public func frexpl(arg : __float_128, exp : *int) : __float_128

/**
 * Multiplies a floating-point value arg by the number 2 raised to the exp power.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by 2 to the power of exp (arg×2exp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/ldexp
 */
@extern
public func ldexpf(arg : float, exp : int) : float

/**
 * Multiplies a floating-point value arg by the number 2 raised to the exp power.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by 2 to the power of exp (arg×2exp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/ldexp
 */
@extern
public func ldexp(arg : double, exp : int) : double

/**
 * Multiplies a floating-point value arg by the number 2 raised to the exp power.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by 2 to the power of exp (arg×2exp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/ldexp
 */
@extern
public func ldexpl(arg : __float_128, exp : int) : __float_128

/**
 * Decomposes given floating-point value arg into integral and fractional parts, each having the same type and sign as arg. The integral part (in floating-point format) is stored in the object pointed to by iptr.
 * @param arg	-	floating-point value
 * @param iptr	-	pointer to floating-point value to store the integral part to
 * @return If no errors occur, returns the fractional part of arg with the same sign as arg. The integral part is put into the value pointed to by iptr.
 * The sum of the returned value and the value stored in *iptr gives arg (allowing for rounding).
 * @see https://en.cppreference.com/w/c/numeric/math/modf
 */
@extern
public func modff(arg : float, iptr : *float) : float

/**
 * Decomposes given floating-point value arg into integral and fractional parts, each having the same type and sign as arg. The integral part (in floating-point format) is stored in the object pointed to by iptr.
 * @param arg	-	floating-point value
 * @param iptr	-	pointer to floating-point value to store the integral part to
 * @return If no errors occur, returns the fractional part of arg with the same sign as arg. The integral part is put into the value pointed to by iptr.
 * The sum of the returned value and the value stored in *iptr gives arg (allowing for rounding).
 * @see https://en.cppreference.com/w/c/numeric/math/modf
 */
@extern
public func modf(arg : double, iptr : *double) : double

/**
 * Decomposes given floating-point value arg into integral and fractional parts, each having the same type and sign as arg. The integral part (in floating-point format) is stored in the object pointed to by iptr.
 * @param arg	-	floating-point value
 * @param iptr	-	pointer to floating-point value to store the integral part to
 * @return If no errors occur, returns the fractional part of arg with the same sign as arg. The integral part is put into the value pointed to by iptr.
 * The sum of the returned value and the value stored in *iptr gives arg (allowing for rounding).
 * @see https://en.cppreference.com/w/c/numeric/math/modf
 */
@extern
public func modfl(arg : __float_128, iptr : *__float_128) : __float_128

/**
 * Multiplies a floating-point value arg by FLT_RADIX raised to power exp.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by FLT_RADIX to the power of exp (arg×FLT_RADIXexp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/scalbn
 */
@extern
public func scalbnf(arg : float, exp : int) : float

/**
 * Multiplies a floating-point value arg by FLT_RADIX raised to power exp.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by FLT_RADIX to the power of exp (arg×FLT_RADIXexp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/scalbn
 */
@extern
public func scalbn(arg : double, exp : int) : double

/**
 * Multiplies a floating-point value arg by FLT_RADIX raised to power exp.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by FLT_RADIX to the power of exp (arg×FLT_RADIXexp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/scalbn
 */
@extern
public func scalbnl(arg : __float_128, exp : int) : __float_128

/**
 * Multiplies a floating-point value arg by FLT_RADIX raised to power exp.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by FLT_RADIX to the power of exp (arg×FLT_RADIXexp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/scalbn
 */
@extern
public func scalblnf(arg : float, exp : long) : float

/**
 * Multiplies a floating-point value arg by FLT_RADIX raised to power exp.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by FLT_RADIX to the power of exp (arg×FLT_RADIXexp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/scalbn
 */
@extern
public func scalbln(arg : double, exp : long) : double

/**
 * Multiplies a floating-point value arg by FLT_RADIX raised to power exp.
 * @param arg	-	floating-point value
 * @param exp	-	integer value
 * @return If no errors occur, arg multiplied by FLT_RADIX to the power of exp (arg×FLT_RADIXexp) is returned.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 *         If a range error due to underflow occurs, the correct result (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/scalbn
 */
@extern
public func scalblnl(arg : __float_128, exp : long) : __float_128

/**
 * Extracts the value of the unbiased exponent from the floating-point argument arg, and returns it as a signed integer value.
 * @param arg	-	floating-point value
 * @return If no errors occur, the unbiased exponent of arg is returned as a signed int value.
 *         If arg is zero, FP_ILOGB0 is returned.
 *         If arg is infinite, INT_MAX is returned.
 *         If arg is a NaN, FP_ILOGBNAN is returned.
 *         If the correct result is greater than INT_MAX or smaller than INT_MIN, the return value is unspecified and a domain error or range error may occur.
 * @see https://en.cppreference.com/w/c/numeric/math/ilogb
 */
@extern
public func ilogbf(arg : float) : int

/**
 * Extracts the value of the unbiased exponent from the floating-point argument arg, and returns it as a signed integer value.
 * @param arg	-	floating-point value
 * @return If no errors occur, the unbiased exponent of arg is returned as a signed int value.
 *         If arg is zero, FP_ILOGB0 is returned.
 *         If arg is infinite, INT_MAX is returned.
 *         If arg is a NaN, FP_ILOGBNAN is returned.
 *         If the correct result is greater than INT_MAX or smaller than INT_MIN, the return value is unspecified and a domain error or range error may occur.
 * @see https://en.cppreference.com/w/c/numeric/math/ilogb
 */
@extern
public func ilogb(arg : double) : int

/**
 * Extracts the value of the unbiased exponent from the floating-point argument arg, and returns it as a signed integer value.
 * @param arg	-	floating-point value
 * @return If no errors occur, the unbiased exponent of arg is returned as a signed int value.
 *         If arg is zero, FP_ILOGB0 is returned.
 *         If arg is infinite, INT_MAX is returned.
 *         If arg is a NaN, FP_ILOGBNAN is returned.
 *         If the correct result is greater than INT_MAX or smaller than INT_MIN, the return value is unspecified and a domain error or range error may occur.
 * @see https://en.cppreference.com/w/c/numeric/math/ilogb
 */
@extern
public func ilogbl(arg : __float_128) : int

/**
 * TODO these macros
 * #define FP_ILOGB0    // implementation-defined
 * #define FP_ILOGBNAN  // implementation-defined
 * @see https://en.cppreference.com/w/c/numeric/math/ilogb
 */

/**
 * First, converts both arguments to the type of the function, then returns the next representable value of from in the direction of to. If from equals to to, to is returned.
 * @param from, to	-	floating-point values
 * @return If no errors occur, the next representable value of from in the direction of to. is returned. If from equals to, then to is returned, converted to the type of the function.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the same sign as from).
 *         If a range error occurs due to underflow, the correct result is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nextafter
 */
@extern
public func nextafterf(from : float, to : float) : float

/**
 * First, converts both arguments to the type of the function, then returns the next representable value of from in the direction of to. If from equals to to, to is returned.
 * @param from, to	-	floating-point values
 * @return If no errors occur, the next representable value of from in the direction of to. is returned. If from equals to, then to is returned, converted to the type of the function.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the same sign as from).
 *         If a range error occurs due to underflow, the correct result is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nextafter
 */
@extern
public func nextafter(from : double, to : double) : double

/**
 * First, converts both arguments to the type of the function, then returns the next representable value of from in the direction of to. If from equals to to, to is returned.
 * @param from, to	-	floating-point values
 * @return If no errors occur, the next representable value of from in the direction of to. is returned. If from equals to, then to is returned, converted to the type of the function.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the same sign as from).
 *         If a range error occurs due to underflow, the correct result is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nextafter
 */
@extern
public func nextafterl(from : __float_128, to : __float_128) : __float_128

/**
 * First, converts the first argument to the type of the function, then returns the next representable value of from in the direction of to. If from equals to to, to is returned, converted from long double to the return type of the function without loss of range or precision.
 * @param from, to	-	floating-point values
 * @return If no errors occur, the next representable value of from in the direction of to. is returned. If from equals to, then to is returned, converted to the type of the function.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the same sign as from).
 *         If a range error occurs due to underflow, the correct result is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nextafter
 */
@extern
public func nexttowardf(from : float, to : __float_128) : float

/**
 * First, converts the first argument to the type of the function, then returns the next representable value of from in the direction of to. If from equals to to, to is returned, converted from long double to the return type of the function without loss of range or precision.
 * @param from, to	-	floating-point values
 * @return If no errors occur, the next representable value of from in the direction of to. is returned. If from equals to, then to is returned, converted to the type of the function.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the same sign as from).
 *         If a range error occurs due to underflow, the correct result is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nextafter
 */
@extern
public func nexttoward(from : double, to : __float_128) : double

/**
 * First, converts the first argument to the type of the function, then returns the next representable value of from in the direction of to. If from equals to to, to is returned, converted from long double to the return type of the function without loss of range or precision.
 * @param from, to	-	floating-point values
 * @return If no errors occur, the next representable value of from in the direction of to. is returned. If from equals to, then to is returned, converted to the type of the function.
 *         If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned (with the same sign as from).
 *         If a range error occurs due to underflow, the correct result is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/nextafter
 */
@extern
public func nexttowardl(from : __float_128, to : __float_128) : __float_128

/**
 * Extracts the value of the unbiased radix-independent exponent from the floating-point argument arg, and returns it as a floating-point value.
 * @param arg	-	floating-point value
 * @return If no errors occur, the unbiased exponent of arg is returned as a signed floating-point value.
 *         If a domain error occurs, an implementation-defined value is returned.
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/logb
 */
@extern
public func logbf(arg : float) : float

/**
 * Extracts the value of the unbiased radix-independent exponent from the floating-point argument arg, and returns it as a floating-point value.
 * @param arg	-	floating-point value
 * @return If no errors occur, the unbiased exponent of arg is returned as a signed floating-point value.
 *         If a domain error occurs, an implementation-defined value is returned.
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/logb
 */
@extern
public func logb(arg : double) : double

/**
 * Extracts the value of the unbiased radix-independent exponent from the floating-point argument arg, and returns it as a floating-point value.
 * @param arg	-	floating-point value
 * @return If no errors occur, the unbiased exponent of arg is returned as a signed floating-point value.
 *         If a domain error occurs, an implementation-defined value is returned.
 *         If a pole error occurs, -HUGE_VAL, -HUGE_VALF, or -HUGE_VALL is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/logb
 */
@extern
public func logbl(arg : __float_128) : __float_128

/**
 * Composes a floating-point value with the magnitude of x and the sign of y.
 * @param x, y	-	floating-point values
 * @return If no errors occur, the floating-point value with the magnitude of x and the sign of y is returned.
 *         If x is NaN, then NaN with the sign of y is returned.
 *         If y is -0, the result is only negative if the implementation supports the signed zero consistently in arithmetic operations.
 * @see https://en.cppreference.com/w/c/numeric/math/copysign
 */
@extern
public func copysignf(x : float, y : float) : float

/**
 * Composes a floating-point value with the magnitude of x and the sign of y.
 * @param x, y	-	floating-point values
 * @return If no errors occur, the floating-point value with the magnitude of x and the sign of y is returned.
 *         If x is NaN, then NaN with the sign of y is returned.
 *         If y is -0, the result is only negative if the implementation supports the signed zero consistently in arithmetic operations.
 * @see https://en.cppreference.com/w/c/numeric/math/copysign
 */
@extern
public func copysign(x : double, y : double) : double

/**
 * Composes a floating-point value with the magnitude of x and the sign of y.
 * @param x, y	-	floating-point values
 * @return If no errors occur, the floating-point value with the magnitude of x and the sign of y is returned.
 *         If x is NaN, then NaN with the sign of y is returned.
 *         If y is -0, the result is only negative if the implementation supports the signed zero consistently in arithmetic operations.
 * @see https://en.cppreference.com/w/c/numeric/math/copysign
 */
@extern
public func copysignl(x : __float_128, y : __float_128) : __float_128

/**
 * TODO macro fpclassify
 * #define fpclassify(arg)
 * @see https://en.cppreference.com/w/c/numeric/math/fpclassify
 */

/**
 * TODO macro isfinite
 * #define isfinite(arg)
 * @see https://en.cppreference.com/w/c/numeric/math/isfinite
 */

/**
 * TODO macro isinf
 * #define isinf(arg)
 * @see https://en.cppreference.com/w/c/numeric/math/isinf
 */

/**
 * TODO macro isnan
 * #define isnan(arg)
 * @see https://en.cppreference.com/w/c/numeric/math/isnan
 */

/**
 * TODO macro isnormal
 * #define isnormal(arg)
 * @see https://en.cppreference.com/w/c/numeric/math/isnormal
 */

/**
 * TODO macro signbit
 * #define signbit( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/signbit
 */

/**
 * TODO macro isgreater
 * #define isgreater(x, y)
 * @see https://en.cppreference.com/w/c/numeric/math/isgreater
 */

/**
 * TODO macro isgreaterequal
 * #define isgreaterequal(x, y)
 * @see https://en.cppreference.com/w/c/numeric/math/isgreaterequal
 */

/**
 * TODO macro isless
 * #define isless(x, y)
 * @see https://en.cppreference.com/w/c/numeric/math/isless
 */


/**
 * TODO macro islessequal
 * #define islessequal(x, y)
 * @see https://en.cppreference.com/w/c/numeric/math/islessequal
 */

/**
 * TODO macro islessgreater
 * #define islessgreater(x, y)
 * @see https://en.cppreference.com/w/c/numeric/math/islessgreater
 */

/**
 * TODO macro isunordered
 * #define isunordered(x, y)
 * @see https://en.cppreference.com/w/c/numeric/math/isunordered
 */

/**
 * @see https://en.cppreference.com/w/c/numeric/math/float_t
 */
@extern
public type float_t = float

/**
 * @see https://en.cppreference.com/w/c/numeric/math/float_t
 */
@extern
public type double_t = double

/**
 * @see https://en.cppreference.com/w/c/types/limits/FLT_EVAL_METHOD
 * @see https://en.cppreference.com/w/c/numeric/math/float_t
 */
@comptime
public const FLT_EVAL_METHOD = 0

/**
 * TODO these macros
 * #define HUGE_VALF //implementation defined
 * #define HUGE_VAL  //implementation defined
 * #define HUGE_VALL //implementation defined
 * @see https://en.cppreference.com/w/c/numeric/math/HUGE_VAL
 */

/**
 * TODO these macros
 * #define INFINITY // implementation defined
 * @see https://en.cppreference.com/w/c/numeric/math/INFINITY
 */

/**
 * TODO these macros
 * #define NAN // implementation defined
 * @see https://en.cppreference.com/w/c/numeric/math/NAN
 */

/**
 * TODO these macros
 * #define MATH_ERRNO
 * #define MATH_ERREXCEPT
 * #define math_errhandling
 * @see https://en.cppreference.com/w/c/numeric/math/math_errhandling
 */

/**
 * TODO these macros
 * #define FP_NORMAL    // implementation defined
 * #define FP_SUBNORMAL // implementation defined
 * #define FP_ZERO      // implementation defined
 * #define FP_INFINITE  // implementation defined
 * #define FP_NAN       // implementation defined
 * @see https://en.cppreference.com/w/c/numeric/math/FP_categories
 */