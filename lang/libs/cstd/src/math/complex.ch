// TODO complex types haven't been defined yet
// so here's what windows uses
// %struct._C_double_complex = type { [2 x double] }
// %struct._C_float_complex = type { [2 x float] }
// %struct._C_ldouble_complex = type { [2 x double] }
// TODO these functions should be tested

// float complex is being passed as i64 in @crealf(i64)
@extern
public type float_complex = bigint;

// double complex is being passed as pointer to struct
@extern
public struct double_complex {
    var real : double
    var imag : double
}

@extern
public struct longdouble_complex {
    var real : longdouble
    var imag : longdouble
}

/**
Each of these macros expands to an expression that evaluates to the value of the specified complex type, with the real part having the value of real (converted to the specified argument type) and the imaginary part having the value of imag (converted to the specified argument type)
The expressions are suitable for use as initializers for objects with static or thread storage duration, as long as the expressions real and imag are also suitable.
*/

/**
 * real	-	the real part of the complex number to return
 * imag	-	the imaginary part of the complex number to return
 * @return A complex number composed of real and imag as the real and imaginary parts.
 * @see https://en.cppreference.com/w/c/numeric/complex/CMPLX
 */
@extern
public comptime func CMPLXF(real : float, imag : float) : float_complex {
    // TODO this
    return 0;
}

/**
 * real	-	the real part of the complex number to return
 * imag	-	the imaginary part of the complex number to return
 * @return A complex number composed of real and imag as the real and imaginary parts.
 * @see https://en.cppreference.com/w/c/numeric/complex/CMPLX
 */
@extern
public comptime func CMPLX(real : double, imag : double) : double_complex {
    return double_complex { real : real, imag : imag }
}

/**
 * real	-	the real part of the complex number to return
 * imag	-	the imaginary part of the complex number to return
 * @return A complex number composed of real and imag as the real and imaginary parts.
 * @see https://en.cppreference.com/w/c/numeric/complex/CMPLX
 */
@extern
public comptime func CMPLXL(real : longdouble, imag : longdouble) : longdouble_complex {
    return longdouble_complex { real : real, imag : imag }
}

/**
 * Returns the real part of z.
 * @see https://en.cppreference.com/w/c/numeric/complex/creal
 */
@extern
public func crealf(z : float_complex) : float

/**
 * Returns the real part of z.
 * @see https://en.cppreference.com/w/c/numeric/complex/creal
 */
@extern
public func creal(z : double_complex) : double

/**
 * Returns the real part of z.
 * @see https://en.cppreference.com/w/c/numeric/complex/creal
 */
@extern
public func creall(z : longdouble_complex ) : longdouble;

/**
 * TODO create a creal macro that works on different complex types
 *   Type-generic macro: if z has type long double complex, long double imaginary, or long double, creall is called. If z has type float complex, float imaginary, or float, crealf is called. If z has type double complex, double imaginary, double, or any integer type, creal is called.
 */

/**
 * Returns the imaginary part of z.
 * @see https://en.cppreference.com/w/c/numeric/complex/cimag
 */
@extern
public func cimagf(z : float_complex) : float

/**
 * Returns the imaginary part of z.
 * @see https://en.cppreference.com/w/c/numeric/complex/cimag
 */
@extern
public func cimag(z : double_complex) : double;

/**
 * Returns the imaginary part of z.
 * @see https://en.cppreference.com/w/c/numeric/complex/cimag
 */
@extern
public func cimagl(z : longdouble_complex ) : longdouble

/**
 * TODO create a cimag macro that works on different complex types
 *   Type-generic macro: if z has type long double complex, long double imaginary, or long double, cimagl is called. If z has type float complex, float imaginary, or float, cimagf is called. If z has type double complex, double imaginary, double, or any integer type, cimag is called.
 */

/**
 * Computes the complex absolute value (also known as norm, modulus, or magnitude) of z.
 * @return If no errors occur, returns the absolute value (norm, magnitude) of z.
 * Errors and special cases are handled as if the function is implemented as hypot(creal(z), cimag(z))
 * @see https://en.cppreference.com/w/c/numeric/complex/cabs
 */
@extern
public func cabsf(z : float_complex) : float

/**
 * Computes the complex absolute value (also known as norm, modulus, or magnitude) of z.
 * @return If no errors occur, returns the absolute value (norm, magnitude) of z.
 * Errors and special cases are handled as if the function is implemented as hypot(creal(z), cimag(z))
 * @see https://en.cppreference.com/w/c/numeric/complex/cabs
 */
@extern
public func cabs(z : double_complex) : double

/**
 * Computes the complex absolute value (also known as norm, modulus, or magnitude) of z.
 * @return If no errors occur, returns the absolute value (norm, magnitude) of z.
 * Errors and special cases are handled as if the function is implemented as hypot(creal(z), cimag(z))
 * @see https://en.cppreference.com/w/c/numeric/complex/cabs
 */
@extern
public func cabsl(z : longdouble_complex) : longdouble

/**
 * TODO create a cabs macro that works on different complex types
 *   Type-generic macro: if z has type long double complex or long double imaginary, cabsl is called. If z has type float complex or float imaginary, cabsf is called. If z has type double complex or double imaginary, cabs is called. For real and integer types, the corresponding version of fabs is called.
 */

/**
 * Computes the argument (also called phase angle) of z, with a branch cut along the negative real axis.
 * @return If no errors occur, returns the phase angle of z in the interval [−π; π].
 * Errors and special cases are handled as if the function is implemented as atan2(cimag(z), creal(z))
 * @see https://en.cppreference.com/w/c/numeric/complex/carg
 */
@extern
public func cargf(z : float_complex) : float

/**
 * Computes the argument (also called phase angle) of z, with a branch cut along the negative real axis.
 * @return If no errors occur, returns the phase angle of z in the interval [−π; π].
 * Errors and special cases are handled as if the function is implemented as atan2(cimag(z), creal(z))
 * @see https://en.cppreference.com/w/c/numeric/complex/carg
 */
@extern
public func carg(z : double_complex) : double

/**
 * Computes the argument (also called phase angle) of z, with a branch cut along the negative real axis.
 * @return If no errors occur, returns the phase angle of z in the interval [−π; π].
 * Errors and special cases are handled as if the function is implemented as atan2(cimag(z), creal(z))
 * @see https://en.cppreference.com/w/c/numeric/complex/carg
 */
@extern
public func cargl(z : longdouble_complex) : longdouble

/**
 * TODO create a carg macro that works on different complex types
 *   Type-generic macro: if z has type long double complex, long double imaginary, or long double, cargl is called. If z has type float complex, float imaginary, or float, cargf is called. If z has type double complex, double imaginary, double, or any integer type, carg is called.
 */

/**
 * Computes the complex conjugate of z by reversing the sign of the imaginary part.
 * @return The complex conjugate of z.
 * @see https://en.wikipedia.org/wiki/Complex_conjugate
 * @see https://en.cppreference.com/w/c/numeric/complex/conj
 */
@extern
public func conjf(z : float_complex) : float_complex

/**
 * Computes the complex conjugate of z by reversing the sign of the imaginary part.
 * @return The complex conjugate of z.
 * @see https://en.wikipedia.org/wiki/Complex_conjugate
 * @see https://en.cppreference.com/w/c/numeric/complex/conj
 */
@extern
public func conj(z : double_complex) : double_complex

/**
 * Computes the complex conjugate of z by reversing the sign of the imaginary part.
 * @return The complex conjugate of z.
 * @see https://en.wikipedia.org/wiki/Complex_conjugate
 * @see https://en.cppreference.com/w/c/numeric/complex/conj
 */
@extern
public func conjl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a conj macro that works on different complex types
 *   Type-generic macro: if z has type long double complex, long double imaginary, or long double, conjl is called. If z has type float complex, float imaginary, or float, conjf is called. If z has type double complex, double imaginary, double, or any integer type, conj is called.
 */

/**
 * Computes the projection of z on the Riemann sphere.
 * @return The projection of z on the Riemann sphere.
 * This function is fully specified for all possible inputs and is not subject to any errors described in math_errhandling
 * @see https://en.cppreference.com/w/c/numeric/complex/cproj
 */
@extern
public func cprojf(z : float_complex) : float_complex

/**
 * Computes the projection of z on the Riemann sphere.
 * @return The projection of z on the Riemann sphere.
 * This function is fully specified for all possible inputs and is not subject to any errors described in math_errhandling
 * @see https://en.cppreference.com/w/c/numeric/complex/cproj
 */
@extern
public func cproj(z : double_complex) : double_complex

/**
 * Computes the projection of z on the Riemann sphere.
 * @return The projection of z on the Riemann sphere.
 * This function is fully specified for all possible inputs and is not subject to any errors described in math_errhandling
 * @see https://en.cppreference.com/w/c/numeric/complex/cproj
 */
@extern
public func cprojl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a cproj macro that works on different complex types
 *   Type-generic macro: if z has type long double complex, long double imaginary, or long double, cprojl is called. If z has type float complex, float imaginary, or float, cprojf is called. If z has type double complex, double imaginary, double, or any integer type, cproj is called.
 */

/**
 * Computes the complex base-e exponential of z.
 * @return If no errors occur, e raised to the power of z,
 * @see https://en.cppreference.com/w/c/numeric/complex/cexp
 */
@extern
public func cexpf(z : float_complex) : float_complex

/**
 * Computes the complex base-e exponential of z.
 * @return If no errors occur, e raised to the power of z,
 * @see https://en.cppreference.com/w/c/numeric/complex/cexp
 */
@extern
public func cexp(z : double_complex) : double_complex

/**
 * Computes the complex base-e exponential of z.
 * @return If no errors occur, e raised to the power of z,
 * @see https://en.cppreference.com/w/c/numeric/complex/cexp
 */
@extern
public func cexpl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a cexp macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, cexpl is called. if z has type double complex, cexp is called, if z has type float complex, cexpf is called. If z is real or integer, then the macro invokes the corresponding real function (expf, exp, expl). If z is imaginary, the corresponding complex argument version is called.
 */

/**
 * Computes the complex natural (base-e) logarithm of z with branch cut along the negative real axis.
 * @return If no errors occur, the complex natural logarithm of z is returned, in the range of a strip in the interval [−iπ, +iπ] along the imaginary axis and mathematically unbounded along the real axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/clogs
 */
@extern
public func clogf(z : float_complex) : float_complex

/**
 * Computes the complex natural (base-e) logarithm of z with branch cut along the negative real axis.
 * @return If no errors occur, the complex natural logarithm of z is returned, in the range of a strip in the interval [−iπ, +iπ] along the imaginary axis and mathematically unbounded along the real axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/clogs
 */
@extern
public func clog(z : double_complex) : double_complex

/**
 * Computes the complex natural (base-e) logarithm of z with branch cut along the negative real axis.
 * @return If no errors occur, the complex natural logarithm of z is returned, in the range of a strip in the interval [−iπ, +iπ] along the imaginary axis and mathematically unbounded along the real axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/clogs
 */
@extern
public func clogl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a clog macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, clogl is called. if z has type double complex, clog is called, if z has type float complex, clogf is called. If z is real or integer, then the macro invokes the corresponding real function (logf, log, logl). If z is imaginary, the corresponding complex number version is called.
 */

/**
 * Computes the complex power function xy, with branch cut for the first parameter along the negative real axis.
 * @return If no errors occur, the complex power xy, is returned.
 * Errors and special cases are handled as if the operation is implemented by cexp(y*clog(x)), except that the implementation is allowed to treat special cases more carefully.
 * @see https://en.cppreference.com/w/c/numeric/complex/cpow
 */
@extern
public func cpowf(x : float_complex, y : float_complex) : float_complex

/**
 * Computes the complex power function xy, with branch cut for the first parameter along the negative real axis.
 * @return If no errors occur, the complex power xy, is returned.
 * Errors and special cases are handled as if the operation is implemented by cexp(y*clog(x)), except that the implementation is allowed to treat special cases more carefully.
 * @see https://en.cppreference.com/w/c/numeric/complex/cpow
 */
@extern
public func cpow(x : double_complex, y : double_complex) : double_complex

/**
 * Computes the complex power function xy, with branch cut for the first parameter along the negative real axis.
 * @return If no errors occur, the complex power xy, is returned.
 * Errors and special cases are handled as if the operation is implemented by cexp(y*clog(x)), except that the implementation is allowed to treat special cases more carefully.
 * @see https://en.cppreference.com/w/c/numeric/complex/cpow
 */
@extern
public func cpowl(x : longdouble_complex, y : longdouble_complex) : longdouble_complex

/**
 * TODO create a cpow macro that works on different complex types
 *   Type-generic macro: If any argument has type long double complex, cpowl is called. if any argument has type double complex, cpow is called, if any argument has type float complex, cpowf is called. If the arguments are real or integer, then the macro invokes the corresponding real function (powf, pow, powl). If any argument is imaginary, the corresponding complex number version is called.
 */

/*
 * Computes the complex square root of z with branch cut along the negative real axis.
 * @return If no errors occur, returns the square root of z, in the range of the right half-plane, including the imaginary axis ([0; +∞) along the real axis and (−∞; +∞) along the imaginary axis.)
 * @see https://en.cppreference.com/w/c/numeric/complex/csqrt
 */
@extern
public func csqrtf(z : float_complex) : float_complex

/*
 * Computes the complex square root of z with branch cut along the negative real axis.
 * @return If no errors occur, returns the square root of z, in the range of the right half-plane, including the imaginary axis ([0; +∞) along the real axis and (−∞; +∞) along the imaginary axis.)
 * @see https://en.cppreference.com/w/c/numeric/complex/csqrt
 */
@extern
public func csqrt(z : double_complex) : double_complex

/*
 * Computes the complex square root of z with branch cut along the negative real axis.
 * @return If no errors occur, returns the square root of z, in the range of the right half-plane, including the imaginary axis ([0; +∞) along the real axis and (−∞; +∞) along the imaginary axis.)
 * @see https://en.cppreference.com/w/c/numeric/complex/csqrt
 */
@extern
public func csqrtl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a csqrt macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, csqrtl is called. if z has type double complex, csqrt is called, if z has type float complex, csqrtf is called. If z is real or integer, then the macro invokes the corresponding real function (sqrtf, sqrt, sqrtl). If z is imaginary, the corresponding complex number version is called.
 */

/**
 * Computes the complex sine of z.
 * @return If no errors occur, the complex sine of z.
 * Errors and special cases are handled as if the operation is implemented by -I * csinh(I*z)
 * @see https://en.cppreference.com/w/c/numeric/complex/csin
 */
@extern
public func csinf(z : float_complex) : float_complex

/**
 * Computes the complex sine of z.
 * @return If no errors occur, the complex sine of z.
 * Errors and special cases are handled as if the operation is implemented by -I * csinh(I*z)
 * @see https://en.cppreference.com/w/c/numeric/complex/csin
 */
@extern
public func csin(z : double_complex) : double_complex

/**
 * Computes the complex sine of z.
 * @return If no errors occur, the complex sine of z.
 * Errors and special cases are handled as if the operation is implemented by -I * csinh(I*z)
 * @see https://en.cppreference.com/w/c/numeric/complex/csin
 */
@extern
public func csinl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a sin macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, csinl is called. if z has type double complex, csin is called, if z has type float complex, csinf is called. If z is real or integer, then the macro invokes the corresponding real function (sinf, sin, sinl). If z is imaginary, then the macro invokes the corresponding real version of the function sinh, implementing the formula sin(iy) = i ∙ sinh(y), and the return type of the macro is imaginary.
 */


/**
 * Computes the complex cosine of z.
 * @return If no errors occur, the complex cosine of z is returned.
 * Errors and special cases are handled as if the operation is implemented by ccosh(I*z).
 * @see https://en.cppreference.com/w/c/numeric/complex/ccos
 */
@extern
public func ccosf(z : float_complex) : float_complex

/**
 * Computes the complex cosine of z.
 * @return If no errors occur, the complex cosine of z is returned.
 * Errors and special cases are handled as if the operation is implemented by ccosh(I*z).
 * @see https://en.cppreference.com/w/c/numeric/complex/ccos
 */
@extern
public func ccos(z : double_complex) : double_complex

/**
 * Computes the complex cosine of z.
 * @return If no errors occur, the complex cosine of z is returned.
 * Errors and special cases are handled as if the operation is implemented by ccosh(I*z).
 * @see https://en.cppreference.com/w/c/numeric/complex/ccos
 */
@extern
public func ccosl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a cos macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, ccosl is called. if z has type double complex, ccos is called, if z has type float complex, ccosf is called. If z is real or integer, then the macro invokes the corresponding real function (cosf, cos, cosl). If z is imaginary, then the macro invokes the corresponding real version of the function cosh, implementing the formula cos(iy) = cosh(y), and the return type is real.
 */

/**
 * Computes the complex tangent of z.
 * @return If no errors occur, the complex tangent of z is returned.
 * Errors and special cases are handled as if the operation is implemented by -i * ctanh(i*z), where i is the imaginary unit.
 * @see https://en.cppreference.com/w/c/numeric/complex/ctan
 */
@extern
public func ctanf(z : float_complex) : float_complex

/**
 * Computes the complex tangent of z.
 * @return If no errors occur, the complex tangent of z is returned.
 * Errors and special cases are handled as if the operation is implemented by -i * ctanh(i*z), where i is the imaginary unit.
 * @see https://en.cppreference.com/w/c/numeric/complex/ctan
 */
@extern
public func ctan(z : double_complex) : double_complex

/**
 * Computes the complex tangent of z.
 * @return If no errors occur, the complex tangent of z is returned.
 * Errors and special cases are handled as if the operation is implemented by -i * ctanh(i*z), where i is the imaginary unit.
 * @see https://en.cppreference.com/w/c/numeric/complex/ctan
 */
@extern
public func ctanl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a tan macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, ctanl is called. if z has type double complex, ctan is called, if z has type float complex, ctanf is called. If z is real or integer, then the macro invokes the corresponding real function (tanf, tan, tanl). If z is imaginary, then the macro invokes the corresponding real version of the function tanh, implementing the formula tan(iy) = i tanh(y), and the return type is imaginary.
 */

/**
 * Computes the complex arc sine of z with branch cuts outside the interval [−1,+1] along the real axis.
 * @return If no errors occur, complex arc sine of z is returned, in the range of a strip unbounded along the imaginary axis and in the interval [−π/2; +π/2] along the real axis.
 * Errors and special cases are handled as if the operation is implemented by -I * casinh(I*z)
 * @see https://en.cppreference.com/w/c/numeric/complex/casin
 */
@extern
public func casinf(z : float_complex) : float_complex

/**
 * Computes the complex arc sine of z with branch cuts outside the interval [−1,+1] along the real axis.
 * @return If no errors occur, complex arc sine of z is returned, in the range of a strip unbounded along the imaginary axis and in the interval [−π/2; +π/2] along the real axis.
 * Errors and special cases are handled as if the operation is implemented by -I * casinh(I*z)
 * @see https://en.cppreference.com/w/c/numeric/complex/casin
 */
@extern
public func casin(z : double_complex) : double_complex

/**
 * Computes the complex arc sine of z with branch cuts outside the interval [−1,+1] along the real axis.
 * @return If no errors occur, complex arc sine of z is returned, in the range of a strip unbounded along the imaginary axis and in the interval [−π/2; +π/2] along the real axis.
 * Errors and special cases are handled as if the operation is implemented by -I * casinh(I*z)
 * @see https://en.cppreference.com/w/c/numeric/complex/casin
 */
@extern
public func casinl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a asin macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, casinl is called. if z has type double complex, casin is called, if z has type float complex, casinf is called. If z is real or integer, then the macro invokes the corresponding real function (asinf, asin, asinl). If z is imaginary, then the macro invokes the corresponding real version of the function asinh, implementing the formula arcsin(iy)=iarsinh(y), and the return type of the macro is imaginary.
 */

/**
 * Computes the complex arc cosine of z with branch cuts outside the interval [−1,+1] along the real axis.
 * @return If no errors occur, complex arc cosine of z is returned, in the range a strip unbounded along the imaginary axis and in the interval [0; π] along the real axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/cacos
 */
@extern
public func cacosf(z : float_complex) : float_complex

/**
 * Computes the complex arc cosine of z with branch cuts outside the interval [−1,+1] along the real axis.
 * @return If no errors occur, complex arc cosine of z is returned, in the range a strip unbounded along the imaginary axis and in the interval [0; π] along the real axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/cacos
 */
@extern
public func cacos(z : double_complex) : double_complex

/**
 * Computes the complex arc cosine of z with branch cuts outside the interval [−1,+1] along the real axis.
 * @return If no errors occur, complex arc cosine of z is returned, in the range a strip unbounded along the imaginary axis and in the interval [0; π] along the real axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/cacos
 */
@extern
public func cacosl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a acos macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, cacosl is called. if z has type double complex, cacos is called, if z has type float complex, cacosf is called. If z is real or integer, then the macro invokes the corresponding real function (acosf, acos, acosl). If z is imaginary, then the macro invokes the corresponding complex number version.
 */

/**
 * Computes the complex arc tangent of z with branch cuts outside the interval [−i,+i] along the imaginary axis.
 * @return If no errors occur, complex arc tangent of z is returned, in the range of a strip unbounded along the imaginary axis and in the interval [−π/2; +π/2] along the real axis.
 * Errors and special cases are handled as if the operation is implemented by -I * catanh(I*z).
 * @see https://en.cppreference.com/w/c/numeric/complex/catan
 */
@extern
public func catanf(z : float_complex) : float_complex

/**
 * Computes the complex arc tangent of z with branch cuts outside the interval [−i,+i] along the imaginary axis.
 * @return If no errors occur, complex arc tangent of z is returned, in the range of a strip unbounded along the imaginary axis and in the interval [−π/2; +π/2] along the real axis.
 * Errors and special cases are handled as if the operation is implemented by -I * catanh(I*z).
 * @see https://en.cppreference.com/w/c/numeric/complex/catan
 */
@extern
public func catan(z : double_complex) : double_complex

/**
 * Computes the complex arc tangent of z with branch cuts outside the interval [−i,+i] along the imaginary axis.
 * @return If no errors occur, complex arc tangent of z is returned, in the range of a strip unbounded along the imaginary axis and in the interval [−π/2; +π/2] along the real axis.
 * Errors and special cases are handled as if the operation is implemented by -I * catanh(I*z).
 * @see https://en.cppreference.com/w/c/numeric/complex/catan
 */
@extern
public func catanl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a atan macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, catanl is called. if z has type double complex, catan is called, if z has type float complex, catanf is called. If z is real or integer, then the macro invokes the corresponding real function (atanf, atan, atanl). If z is imaginary, then the macro invokes the corresponding real version of the function atanh, implementing the formula atan(iy) = i atanh(y), and the return type of the macro is imaginary.
 */

/**
 * Computes the complex hyperbolic sine of z.
 * @return If no errors occur, complex hyperbolic sine of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/csinh
 */
@extern
public func csinhf(z : float_complex) : float_complex

/**
 * Computes the complex hyperbolic sine of z.
 * @return If no errors occur, complex hyperbolic sine of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/csinh
 */
@extern
public func csinh(z : double_complex) : double_complex

/**
 * Computes the complex hyperbolic sine of z.
 * @return If no errors occur, complex hyperbolic sine of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/csinh
 */
@extern
public func csinhl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a sinh macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, csinhl is called. if z has type double complex, csinh is called, if z has type float complex, csinhf is called. If z is real or integer, then the macro invokes the corresponding real function (sinhf, sinh, sinhl). If z is imaginary, then the macro invokes the corresponding real version of the function sin, implementing the formula sinh(iy) = i sin(y), and the return type is imaginary.
 */

/**
 * Computes the complex hyperbolic cosine of z.
 * @return If no errors occur, complex hyperbolic cosine of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/ccosh
 */
@extern
public func ccoshf(z : float_complex) : float_complex

/**
 * Computes the complex hyperbolic cosine of z.
 * @return If no errors occur, complex hyperbolic cosine of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/ccosh
 */
@extern
public func ccosh(z : double_complex) : double_complex

/**
 * Computes the complex hyperbolic cosine of z.
 * @return If no errors occur, complex hyperbolic cosine of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/ccosh
 */
@extern
public func ccoshl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a cosh macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, ccoshl is called. if z has type double complex, ccosh is called, if z has type float complex, ccoshf is called. If z is real or integer, then the macro invokes the corresponding real function (coshf, cosh, coshl). If z is imaginary, then the macro invokes the corresponding real version of the function cos, implementing the formula cosh(iy) = cos(y), and the return type is real.
 */

/**
 * Computes the complex hyperbolic tangent of z.
 * @return If no errors occur, complex hyperbolic tangent of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/ctanh
 */
@extern
public func ctanhf(z : float_complex) : float_complex

/**
 * Computes the complex hyperbolic tangent of z.
 * @return If no errors occur, complex hyperbolic tangent of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/ctanh
 */
@extern
public func ctanh(z : double_complex) : double_complex

/**
 * Computes the complex hyperbolic tangent of z.
 * @return If no errors occur, complex hyperbolic tangent of z is returned
 * @see https://en.cppreference.com/w/c/numeric/complex/ctanh
 */
@extern
public func ctanhl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a tanh macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, ctanhl is called. if z has type double complex, ctanh is called, if z has type float complex, ctanhf is called. If z is real or integer, then the macro invokes the corresponding real function (tanhf, tanh, tanhl). If z is imaginary, then the macro invokes the corresponding real version of the function tan, implementing the formula tanh(iy) = i tan(y), and the return type is imaginary.
 */

/**
 * Computes the complex arc hyperbolic sine of z with branch cuts outside the interval [−i; +i] along the imaginary axis.
 * @return If no errors occur, the complex arc hyperbolic sine of z is returned, in the range of a strip mathematically unbounded along the real axis and in the interval [−iπ/2; +iπ/2] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/casinh
 */
@extern
public func casinhf(z : float_complex) : float_complex

/**
 * Computes the complex arc hyperbolic sine of z with branch cuts outside the interval [−i; +i] along the imaginary axis.
 * @return If no errors occur, the complex arc hyperbolic sine of z is returned, in the range of a strip mathematically unbounded along the real axis and in the interval [−iπ/2; +iπ/2] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/casinh
 */
@extern
public func casinh(z : double_complex) : double_complex

/**
 * Computes the complex arc hyperbolic sine of z with branch cuts outside the interval [−i; +i] along the imaginary axis.
 * @return If no errors occur, the complex arc hyperbolic sine of z is returned, in the range of a strip mathematically unbounded along the real axis and in the interval [−iπ/2; +iπ/2] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/casinh
 */
@extern
public func casinhl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a asinh macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, casinhl is called. if z has type double complex, casinh is called, if z has type float complex, casinhf is called. If z is real or integer, then the macro invokes the corresponding real function (asinhf, asinh, asinhl). If z is imaginary, then the macro invokes the corresponding real version of the function asin, implementing the formula asinh(iy) = i asin(y), and the return type is imaginary.
 */

/**
 * Computes complex arc hyperbolic cosine of a complex value z with branch cut at values less than 1 along the real axis.
 * @return The complex arc hyperbolic cosine of z in the interval [0; ∞) along the real axis and in the interval [−iπ; +iπ] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/cacosh
 */
@extern
public func cacoshf(z : float_complex) : float_complex

/**
 * Computes complex arc hyperbolic cosine of a complex value z with branch cut at values less than 1 along the real axis.
 * @return The complex arc hyperbolic cosine of z in the interval [0; ∞) along the real axis and in the interval [−iπ; +iπ] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/cacosh
 */
@extern
public func cacosh(z : double_complex) : double_complex

/**
 * Computes complex arc hyperbolic cosine of a complex value z with branch cut at values less than 1 along the real axis.
 * @return The complex arc hyperbolic cosine of z in the interval [0; ∞) along the real axis and in the interval [−iπ; +iπ] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/cacosh
 */
@extern
public func cacoshl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a acosh macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, cacoshl is called. if z has type double complex, cacosh is called, if z has type float complex, cacoshf is called. If z is real or integer, then the macro invokes the corresponding real function (acoshf, acosh, acoshl). If z is imaginary, then the macro invokes the corresponding complex number version and the return type is complex.
 */

/**
 * Computes the complex arc hyperbolic tangent of z with branch cuts outside the interval [−1; +1] along the real axis.
 * @return If no errors occur, the complex arc hyperbolic tangent of z is returned, in the range of a half-strip mathematically unbounded along the real axis and in the interval [−iπ/2; +iπ/2] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/catanh
 */
@extern
public func catanhf(z : float_complex) : float_complex

/**
 * Computes the complex arc hyperbolic tangent of z with branch cuts outside the interval [−1; +1] along the real axis.
 * @return If no errors occur, the complex arc hyperbolic tangent of z is returned, in the range of a half-strip mathematically unbounded along the real axis and in the interval [−iπ/2; +iπ/2] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/catanh
 */
@extern
public func catanh(z : double_complex) : double_complex

/**
 * Computes the complex arc hyperbolic tangent of z with branch cuts outside the interval [−1; +1] along the real axis.
 * @return If no errors occur, the complex arc hyperbolic tangent of z is returned, in the range of a half-strip mathematically unbounded along the real axis and in the interval [−iπ/2; +iπ/2] along the imaginary axis.
 * @see https://en.cppreference.com/w/c/numeric/complex/catanh
 */
@extern
public func catanhl(z : longdouble_complex) : longdouble_complex

/**
 * TODO create a atanh macro that works on different complex types
 *   Type-generic macro: If z has type long double complex, catanhl is called. if z has type double complex, catanh is called, if z has type float complex, catanhf is called. If z is real or integer, then the macro invokes the corresponding real function (atanhf, atanh, atanhl). If z is imaginary, then the macro invokes the corresponding real version of atan, implementing the formula atanh(iy) = i atan(y), and the return type is imaginary.
 */