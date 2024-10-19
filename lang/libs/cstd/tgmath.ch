/**
 * TODO Type generic math header defines macros that use
 * different functions based on different types, we could do that
 * using comptime functions
 */

/**
 * TODO macro remainder
 * #define remainder( x, y )
 * @see https://en.cppreference.com/w/c/numeric/math/remainder
 * Type-generic macro: If any argument has type long double, remainderl is called. Otherwise, if any argument has integer type or has type double, remainder is called. Otherwise, remainderf is called.
 */

/**
 * TODO macro remquo
 * #define remquo( x, y )
 * @see https://en.cppreference.com/w/c/numeric/math/remquo
 * Type-generic macro: If any non-pointer argument has type long double, remquol is called. Otherwise, if any non-pointer argument has integer type or has type double, remquo is called. Otherwise, remquof is called.
 */

/**
 * TODO macro fma
 * #define fma( x, y, z )
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 * Type-generic macro: If any argument has type long double, fmal is called. Otherwise, if any argument has integer type or has type double, fma is called. Otherwise, fmaf is called.
 */

/**
 * TODO macro fmax
 * #define fmax( x, y, z )
 * @see https://en.cppreference.com/w/c/numeric/math/fmax
 * Type-generic macro: If any argument has type long double, fmaxl is called. Otherwise, if any argument has integer type or has type double, fmax is called. Otherwise, fmaxf is called.
 */

/**
 * TODO macro fmin
 * #define fmin( x, y, z )
 * @see https://en.cppreference.com/w/c/numeric/math/fmin
 * Type-generic macro: If any argument has type long double, fminl is called. Otherwise, if any argument has integer type or has type double, fmin is called. Otherwise, fminf is called.
 */

/**
 * TODO macro fdim
 * #define fdim( x, y, z )
 * @see https://en.cppreference.com/w/c/numeric/math/fdim
 * Type-generic macro: If any argument has type long double, fdiml is called. Otherwise, if any argument has integer type or has type double, fdim is called. Otherwise, fdimf is called.
 */

/**
 * TODO macro exp
 * #define exp( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/exp
 * Type-generic macro: If arg has type long double, expl is called. Otherwise, if arg has integer type or the type double, exp is called. Otherwise, expf is called. If arg is complex or imaginary, then the macro invokes the corresponding complex function (cexpf, cexp, cexpl).
 */

/**
 * TODO macro exp2
 * #define exp2(n)
 * @see https://en.cppreference.com/w/c/numeric/math/exp2
 * Type-generic macro: If n has type long double, exp2l is called. Otherwise, if n has integer type or the type double, exp2 is called. Otherwise, exp2f is called.
 */

/**
 * TODO macro exp2
 * #define expm1( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/expm1
 * Type-generic macro: If arg has type long double, expm1l is called. Otherwise, if arg has integer type or the type double, expm1 is called. Otherwise, expm1f is called.
 */

/**
 * TODO macro log
 * #define log( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/log
 * Type-generic macro: If arg has type long double, logl is called. Otherwise, if arg has integer type or the type double, log is called. Otherwise, logf is called. If arg is complex or imaginary, then the macro invokes the corresponding complex function (clogf, clog, clogl).
 */

/**
 * TODO macro log10
 * #define log10( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/log10
 * Type-generic macro: Type-generic macro: If arg has type long double, log10l is called. Otherwise, if arg has integer type or the type double, log10 is called. Otherwise, log10f is called.
 */

/**
 * TODO macro log2
 * #define log2( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/log2
 * Type-generic macro: If arg has type long double, log2l is called. Otherwise, if arg has integer type or the type double, log2 is called. Otherwise, log2f is called.
 */

/**
 * TODO macro log1p
 * #define log1p( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/log1p
 * Type-generic macro: If arg has type long double, log1pl is called. Otherwise, if arg has integer type or the type double, log1p is called. Otherwise, log1pf is called.
 */

/**
 * TODO macro pow
 * #define pow( base, exponent )
 * @see https://en.cppreference.com/w/c/numeric/math/pow
 * Type-generic macro: If any argument has type long double, powl is called. Otherwise, if any argument has integer type or has type double, pow is called. Otherwise, powf is called. If at least one argument is complex or imaginary, then the macro invokes the corresponding complex function (cpowf, cpow, cpowl).
 */

/**
 * TODO macro sqrt
 * #define sqrt( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/sqrt
 * Type-generic macro: If arg has type long double, sqrtl is called. Otherwise, if arg has integer type or the type double, sqrt is called. Otherwise, sqrtf is called. If arg is complex or imaginary, then the macro invokes the corresponding complex function (csqrtf, csqrt, csqrtl).
 */

/**
 * TODO macro cbrt
 * #define cbrt( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/cbrt
 * Type-generic macro: If arg has type long double, cbrtl is called. Otherwise, if arg has integer type or the type double, cbrt is called. Otherwise, cbrtf is called.
 */

/**
 * TODO macro hypot
 * #define hypot( x, y )
 * @see https://en.cppreference.com/w/c/numeric/math/hypot
 * Type-generic macro: If any argument has type long double, the long double version of the function is called. Otherwise, if any argument has integer type or has type double, the double version of the function is called. Otherwise, the float version of the function is called.
 */

/**
 * TODO macro sin
 * #define sin( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/sin
 * Type-generic macro: If the argument has type long double, (3) (sinl) is called. Otherwise, if the argument has integer type or the type double, (2) (sin) is called. Otherwise, (1) (sinf) is called. If the argument is complex, then the macro invokes the corresponding complex function (csinl, csin, csinf).
 */

/**
 * TODO macro cos
 * #define cos( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/cos
 * Type-generic macro: If the argument has type long double, (3) (cosl) is called. Otherwise, if the argument has integer type or the type double, (2) (cos) is called. Otherwise, (1) (cosf) is called. If the argument is complex, then the macro invokes the corresponding complex function (ccosf, ccos, ccosl).
 */

/**
 * TODO macro tan
 * #define tan( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/tan
 * Type-generic macro: If the argument has type long double, (3) (tanl) is called. Otherwise, if the argument has integer type or the type double, (2) (tan) is called. Otherwise, (1) (tanf) is called. If the argument is complex, then the macro invokes the corresponding complex function (ctanf, ctan, ctanl).
 */

/**
 * TODO macro asin
 * #define asin( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/asin
 * Type-generic macro: If the argument has type long double, (3) (asinl) is called. Otherwise, if the argument has integer type or the type double, (2) (asin) is called. Otherwise, (1) (asinf) is called. If the argument is complex, then the macro invokes the corresponding complex function (casinf, casin, casinl).
 */

/**
 * TODO macro acos
 * #define acos( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/acos
 * Type-generic macro: If the argument has type long double, (3) (acosl) is called. Otherwise, if the argument has integer type or the type double, (2) (acos) is called. Otherwise, (1) (acosf) is called. If the argument is complex, then the macro invokes the corresponding complex function (cacosf, cacos, cacosl).
 */

/**
 * TODO macro atan
 * #define atan( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/atan
 * Type-generic macro: If the argument has type long double, (3) (atanl) is called. Otherwise, if the argument has integer type or the type double, (2) (atan) is called. Otherwise, (1) (atanf) is called. If the argument is complex, then the macro invokes the corresponding complex function (catanf, catan, catanl).
 */

/**
 * TODO macro atan2
 * #define atan2( y, x )
 * @see https://en.cppreference.com/w/c/numeric/math/atan2
 * Type-generic macro: If any argument has type long double, (3) (atan2l) is called. Otherwise, if any argument has integer type or has type double, (2) (atan2) is called. Otherwise, (1) (atan2f) is called.
 */

/**
 * TODO macro sinh
 * #define sinh( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/sinh
 * Type-generic macro: If the argument has type long double, sinhl is called. Otherwise, if the argument has integer type or the type double, sinh is called. Otherwise, sinhf is called. If the argument is complex, then the macro invokes the corresponding complex function (csinhf, csinh, csinhl).
 */

/**
 * TODO macro tanh
 * #define tanh( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/tanh
 * Type-generic macro: If the argument has type long double, tanhl is called. Otherwise, if the argument has integer type or the type double, tanh is called. Otherwise, tanhf is called. If the argument is complex, then the macro invokes the corresponding complex function (ctanhf, ctanh, ctanhl).
 */

/**
 * TODO macro asinh
 * #define asinh( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/asinh
 * Type-generic macro: If the argument has type long double, asinhl is called. Otherwise, if the argument has integer type or the type double, asinh is called. Otherwise, asinhf is called. If the argument is complex, then the macro invokes the corresponding complex function (casinhf, casinh, casinhl).
 */

/**
 * TODO macro acosh
 * #define acosh( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/acosh
 * Type-generic macro: If the argument has type long double, acoshl is called. Otherwise, if the argument has integer type or the type double, acosh is called. Otherwise, acoshf is called. If the argument is complex, then the macro invokes the corresponding complex function (cacoshf, cacosh, cacoshl).
 */

/**
 * TODO macro atanh
 * #define atanh( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/atanh
 * Type-generic macro: If the argument has type long double, atanhl is called. Otherwise, if the argument has integer type or the type double, atanh is called. Otherwise, atanhf is called. If the argument is complex, then the macro invokes the corresponding complex function (catanhf, catanh, catanhl).
 */

/**
 * TODO macro erfc
 * #define erfc( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/erfc
 * Type-generic macro: If arg has type long double, erfcl is called. Otherwise, if arg has integer type or the type double, erfc is called. Otherwise, erfcf is called.
 */


/**
 * TODO macro tgamma
 * #define tgamma( arg )
 * @see https://en.cppreference.com/w/c/numeric/math/tgamma
 * Type-generic macro: If arg has type long double, tgammal is called. Otherwise, if arg has integer type or the type double, tgamma is called. Otherwise, tgammaf is called.
 */