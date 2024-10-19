/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
func fabsf(arg : float) : float

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
func fabs(arg : double) : double

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
func fabsl(arg : float128) : float128

/**
 * TODO
 */
typealias _Decimal32 = float
typealias _Decimal64 = double
typealias _Decimal128 = float128

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
func fabsd32(arg : _Decimal32) : _Decimal32

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
func fabsd64(arg : _Decimal64) : _Decimal64

/**
 * Computes the absolute value of a floating-point value arg.
 * @param arg	-	floating-point value
 * @return If successful, returns the absolute value of arg (|arg|). The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fabs
 */
func fabsd128(arg : _Decimal128) : _Decimal128

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
func fmodf(x : float, y : float) : float

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
func fmod(x : double, y : double) : double

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
func fmodl(x : float128, y : float128) : float128

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
func remainderf(x : float, y : float) : float

/**
 * Computes the IEEE remainder of the floating-point division operation x/y.
 * @param x, y	-	floating-point values
 * @return If successful, returns the IEEE floating-point remainder of the division x/y as defined above.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result is returned.
 * If y is zero, but the domain error does not occur, zero is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/remainder
 */
func remainder(x : double, y : double) : double

/**
 * Computes the IEEE remainder of the floating-point division operation x/y.
 * @param x, y	-	floating-point values
 * @return If successful, returns the IEEE floating-point remainder of the division x/y as defined above.
 * If a domain error occurs, an implementation-defined value is returned (NaN where supported).
 * If a range error occurs due to underflow, the correct result is returned.
 * If y is zero, but the domain error does not occur, zero is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/remainder
 */
func remainderl(x : float128, y : float128) : float128

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
func remquof(x : float, y : float, quo : *mut int) : float

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
func remquo(x : double, y : double, quo : *mut int) : double

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
func remquol(x : float128, y : float128, quo : *mut int) : float128

/**
 * Computes (x * y) + z as if to infinite precision and rounded only once to fit the result type.
 * @param x, y, z	-	floating-point values
 * @return If successful, returns the value of (x * y) + z as if calculated to infinite precision and rounded once to fit the result type (or, alternatively, calculated as a single ternary floating-point operation).
 * If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 */
func fmaf(x : float, y : float, z : float) : float

/**
 * Computes (x * y) + z as if to infinite precision and rounded only once to fit the result type.
 * @param x, y, z	-	floating-point values
 * @return If successful, returns the value of (x * y) + z as if calculated to infinite precision and rounded once to fit the result type (or, alternatively, calculated as a single ternary floating-point operation).
 * If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 */
func fma(x : double, y : double, z : double) : double

/**
 * Computes (x * y) + z as if to infinite precision and rounded only once to fit the result type.
 * @param x, y, z	-	floating-point values
 * @return If successful, returns the value of (x * y) + z as if calculated to infinite precision and rounded once to fit the result type (or, alternatively, calculated as a single ternary floating-point operation).
 * If a range error due to overflow occurs, ±HUGE_VAL, ±HUGE_VALF, or ±HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 */
func fmal(x : float128, y : float128, z : float128) : float128

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
func fmaxf(x : float, y : float) : float

/**
 * Returns the larger of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the larger of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmax
 */
func fmax(x : double, y : double) : double

/**
 * Returns the larger of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the larger of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmax
 */
func fmaxl(x : float128, y : float128) : float128

/**
 * Returns the smaller of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the smaller of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmin
 */
func fminf(x : float, y : float) : float

/**
 * Returns the smaller of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the smaller of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmin
 */
func fmin(x : double, y : double ) : double

/**
 * Returns the smaller of two floating-point arguments, treating NaNs as missing data (between a NaN and a numeric value, the numeric value is chosen).
 * @param x, y	-	floating-point values
 * @return If successful, returns the smaller of two floating-point values. The value returned is exact and does not depend on any rounding modes.
 * @see https://en.cppreference.com/w/c/numeric/math/fmin
 */
func fminl(x : float128, y : float128) : float128

/**
 * Returns the positive difference between x and y, that is, if x>y, returns x-y, otherwise (if x≤y), returns +0.
 * @param x, y	-	floating-point value
 * @return If successful, returns the positive difference between x and y.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fdim
 */
func fdimf(x : float, y : float) : float

/**
 * Returns the positive difference between x and y, that is, if x>y, returns x-y, otherwise (if x≤y), returns +0.
 * @param x, y	-	floating-point value
 * @return If successful, returns the positive difference between x and y.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fdim
 */
func fdim(x : double, y : double) : double

/**
 * Returns the positive difference between x and y, that is, if x>y, returns x-y, otherwise (if x≤y), returns +0.
 * @param x, y	-	floating-point value
 * @return If successful, returns the positive difference between x and y.
 * If a range error due to overflow occurs, +HUGE_VAL, +HUGE_VALF, or +HUGE_VALL is returned.
 * If a range error due to underflow occurs, the correct value (after rounding) is returned.
 * @see https://en.cppreference.com/w/c/numeric/math/fdim
 */
func fdiml(x : float128, y : float128) : float128

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call /*strtoX*/("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call /*strtoX*/("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call /*strtoX*/("NAN", (char**)NULL);.
 * The parsing function is strtof.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
func nanf(arg : *char) : float

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call /*strtoX*/("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call /*strtoX*/("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call /*strtoX*/("NAN", (char**)NULL);.
 * The parsing function is strtod.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
func nan(arg : *char) : double

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call /*strtoX*/("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call /*strtoX*/("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call /*strtoX*/("NAN", (char**)NULL);.
 * The parsing function is strtold.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
func nanl(arg : *char) : float128

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call /*strtoX*/("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call /*strtoX*/("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call /*strtoX*/("NAN", (char**)NULL);.
 * The parsing function is strtod32.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
func nand32(arg : *char) : _Decimal32

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call /*strtoX*/("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call /*strtoX*/("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call /*strtoX*/("NAN", (char**)NULL);.
 * The parsing function is strtod64.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
func nand64(arg : *char) : _Decimal64

/**
 * Converts the implementation-defined character string arg into the corresponding quiet NaN value, as if by calling the appropriate parsing function strtoX, as follows:
 * The call nan("n-char-sequence"), where n-char-sequence is a sequence of digits, Latin letters, and underscores, is equivalent to the call /*strtoX*/("NAN(n-char-sequence)", (char**)NULL);.
 * The call nan("") is equivalent to the call /*strtoX*/("NAN()", (char**)NULL);.
 * The call nan("string"), where string is neither an n-char-sequence nor an empty string, is equivalent to the call /*strtoX*/("NAN", (char**)NULL);.
 * The parsing function is strtod128.
 * @param arg	-	narrow character string identifying the contents of a NaN
 * @return The quiet NaN value that corresponds to the identifying string arg or zero if the implementation does not support quiet NaNs.
 * If the implementation supports IEEE floating-point arithmetic (IEC 60559), it also supports quiet NaNs.
 * @see https://en.cppreference.com/w/c/numeric/math/nan
 */
func nand128(arg : *char) : _Decimal128