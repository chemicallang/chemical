/**
 * TODO Type generic math header defines macros that use
 * different functions based on different types, we could do that
 * using comptime functions
 */

/**
 * TODO remainder macro
 * #define remainder( x, y )
 * @see https://en.cppreference.com/w/c/numeric/math/remainder
 * Type-generic macro: If any argument has type long double, remainderl is called. Otherwise, if any argument has integer type or has type double, remainder is called. Otherwise, remainderf is called.
 */

/**
 * TODO remainder remquo
 * #define remquo( x, y )
 * @see https://en.cppreference.com/w/c/numeric/math/remquo
 * Type-generic macro: If any non-pointer argument has type long double, remquol is called. Otherwise, if any non-pointer argument has integer type or has type double, remquo is called. Otherwise, remquof is called.
 */

/**
 * TODO remainder fma
 * #define fma( x, y, z )
 * @see https://en.cppreference.com/w/c/numeric/math/fma
 * Type-generic macro: If any argument has type long double, fmal is called. Otherwise, if any argument has integer type or has type double, fma is called. Otherwise, fmaf is called.
 */

/**
 * TODO remainder fmax
 * #define fmax( x, y, z )
 * @see https://en.cppreference.com/w/c/numeric/math/fmax
 * Type-generic macro: If any argument has type long double, fmaxl is called. Otherwise, if any argument has integer type or has type double, fmax is called. Otherwise, fmaxf is called.
 */

/**
 * TODO remainder fmin
 * #define fmin( x, y, z )
 * @see https://en.cppreference.com/w/c/numeric/math/fmin
 * Type-generic macro: If any argument has type long double, fminl is called. Otherwise, if any argument has integer type or has type double, fmin is called. Otherwise, fminf is called.
 */

/**
 * TODO remainder fdim
 * #define fdim( x, y, z )
 * @see https://en.cppreference.com/w/c/numeric/math/fdim
 * Type-generic macro: If any argument has type long double, fdiml is called. Otherwise, if any argument has integer type or has type double, fdim is called. Otherwise, fdimf is called.
 */