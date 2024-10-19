
/**
 * Attempts to clear the floating-point exceptions that are listed in the bitmask argument excepts, which is a bitwise OR of the floating-point exception macros.
 * @param excepts	-	bitmask listing the exception flags to clear
 * @return 0 if all indicated exceptions were successfully cleared or if excepts is zero. Returns a non-zero value on error.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feclearexcept
 */
func feclearexcept(excepts : int) : int

/**
 * Determines which of the specified subset of the floating-point exceptions are currently set. The argument excepts is a bitwise OR of the floating-point exception macros.
 * @param excepts	-	bitmask listing the exception flags to test
 * @return Bitwise OR of the floating-point exception macros that are both included in excepts and correspond to floating-point exceptions currently set.
 * @see https://en.cppreference.com/w/c/numeric/fenv/fetestexcept
 */
func fetestexcept(excepts : int) : int

/**
 * Attempts to raise all floating-point exceptions listed in excepts (a bitwise OR of the floating-point exception macros). If one of the exceptions is FE_OVERFLOW or FE_UNDERFLOW, this function may additionally raise FE_INEXACT. The order in which the exceptions are raised is unspecified, except that FE_OVERFLOW and FE_UNDERFLOW are always raised before FE_INEXACT.
 * @param excepts	-	bitmask listing the exception flags to raise
 * @return 0 if all listed exceptions were raised, non-zero value otherwise.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feraiseexcept
 */
func feraiseexcept(excepts : int) : int

/**
 * Attempts to obtain the full contents of the floating-point exception flags that are listed in the bitmask argument excepts, which is a bitwise OR of the floating-point exception macros.
 * The full contents of a floating-point exception flag is not necessarily a boolean value indicating whether the exception is raised or cleared. For example, it may be a struct which includes the boolean status and the address of the code that triggered the exception. These functions obtain all such content and obtain/store it in flagp in implementation-defined format.
 * @param flagp	-	pointer to an fexcept_t object where the flags will be stored or read from
 * @param excepts	-	bitmask listing the exception flags to get/set
 * @return 0 on success, non-zero otherwise.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feexceptflag
 */
int fegetexceptflag(flagp : *mut fexcept_t, excepts : int);

/**
 * Attempts to copy the full contents of the floating-point exception flags that are listed in excepts from flagp into the floating-point environment. Does not raise any exceptions, only modifies the flags.
 * The full contents of a floating-point exception flag is not necessarily a boolean value indicating whether the exception is raised or cleared. For example, it may be a struct which includes the boolean status and the address of the code that triggered the exception. These functions obtain all such content and obtain/store it in flagp in implementation-defined format.
 * @param flagp	-	pointer to an fexcept_t object where the flags will be stored or read from
 * @param excepts	-	bitmask listing the exception flags to get/set
 * @return 0 on success, non-zero otherwise.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feexceptflag
 */
func fesetexceptflag(flagp : *fexcept_t, excepts : int) : int

/**
 * Attempts to establish the floating-point rounding direction equal to the argument round, which is expected to be one of the floating-point rounding macros.
 * @param round	-	rounding direction, one of floating-point rounding macros
 * @return 0 on success, non-zero otherwise.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feround
 */
func fesetround(round : int) : int

/**
 * Returns the value of the floating-point rounding macro that corresponds to the current rounding direction.
 * @return the floating-point rounding macro describing the current rounding direction or a negative value if the direction cannot be determined.s
 * @see https://en.cppreference.com/w/c/numeric/fenv/feround
 */
func fegetround() : int

/**
 * Attempts to store the status of the floating-point environment in the object pointed to by envp.
 * @param envp	-	pointer to the object of type fenv_t which holds the status of the floating-point environment
 * @return 0 on success, non-zero otherwise.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feenv
 */
func fegetenv(envp : *mut fenv_t) : int

/**
 * Attempts to establish the floating-point environment from the object pointed to by envp. The value of that object must be previously obtained by a call to feholdexcept or fegetenv or be a floating-point macro constant. If any of the floating-point status flags are set in envp, they become set in the environment (and are then testable with fetestexcept), but the corresponding floating-point exceptions are not raised (execution continues uninterrupted)
 * @param envp	-	pointer to the object of type fenv_t which holds the status of the floating-point environment
 * @return 0 on success, non-zero otherwise.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feenv
 */
func fesetenv(envp : *fenv_t) : int

/**
 * First, saves the current floating-point environment to the object pointed to by envp (similar to fegetenv), then clears all floating-point status flags, and then installs the non-stop mode: future floating-point exceptions will not interrupt execution (will not trap), until the floating-point environment is restored by feupdateenv or fesetenv.
 * This function may be used in the beginning of a subroutine that must hide the floating-point exceptions it may raise from the caller. If only some exceptions must be suppressed, while others must be reported, the non-stop mode is usually ended with a call to feupdateenv after clearing the unwanted exceptions.
 * @param envp	-	pointer to the object of type fenv_t where the floating-point environment will be stored
 * @return 0 on success, non-zero otherwise.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feholdexcept
 */
func feholdexcept(envp : *mut fenv_t) : int

/**
 * First, remembers the currently raised floating-point exceptions, then restores the floating-point environment from the object pointed to by envp (similar to fesetenv), then raises the floating-point exceptions that were saved.
 * This function may be used to end the non-stop mode established by an earlier call to feholdexcept.
 * @param envp	-	pointer to the object of type fenv_t set by an earlier call to feholdexcept or fegetenv or equal to FE_DFL_ENV
 * @return 0 on success, non-zero otherwise.
 * @see https://en.cppreference.com/w/c/numeric/fenv/feupdateenv
 */
func feupdateenv(envp : *fenv_t) : int

/**
 * TODO these macros
 * #define FE_DIVBYZERO    // implementation defined power of 2
 * #define FE_INEXACT      // implementation defined power of 2
 * #define FE_INVALID      // implementation defined power of 2
 * #define FE_OVERFLOW     // implementation defined power of 2
 * #define FE_UNDERFLOW    // implementation defined power of 2
 * #define FE_ALL_EXCEPT  FE_DIVBYZERO | FE_INEXACT | \
 *                        FE_INVALID | FE_OVERFLOW |  \
 *                        FE_UNDERFLOW
 *
 * All these macro constants (except FE_ALL_EXCEPT) expand to integer constant expressions that are distinct powers of 2, which uniquely identify all supported floating-point exceptions. Each macro is only defined if it is supported.
 *
 * The macro constant FE_ALL_EXCEPT, which expands to the bitwise OR of all other FE_*, is always defined and is zero if floating-point exceptions are not supported by the implementation.
 *
 * Constant	Explanation
 * FE_DIVBYZERO	pole error occurred in an earlier floating-point operation
 * FE_INEXACT	inexact result: rounding was necessary to store the result of an earlier floating-point operation
 * FE_INVALID	domain error occurred in an earlier floating-point operation
 * FE_OVERFLOW	the result of an earlier floating-point operation was too large to be representable
 * FE_UNDERFLOW	the result of an earlier floating-point operation was subnormal with a loss of precision
 * FE_ALL_EXCEPT	bitwise OR of all supported floating-point exceptions
 * The implementation may define additional macro constants in <fenv.h> to identify additional floating-point exceptions. All such constants begin with FE_ followed by at least one uppercase letter.
 *
 * See https://en.cppreference.com/w/c/numeric/math/math_errhandling for further details.
 */

/**
 * TODO these macros
 * #define FE_DOWNWARD     // implementation defined
 * #define FE_TONEAREST    // implementation defined
 * #define FE_TOWARDZERO   // implementation defined
 * #define FE_UPWARD       // implementation defined
 *
 * Each of these macro constants expands to a nonnegative integer constant expression, which can be used with fesetround and fegetround to indicate one of the supported floating-point rounding modes. The implementation may define additional rounding mode constants in <fenv.h>, which should all begin with FE_ followed by at least one uppercase letter. Each macro is only defined if it is supported.
 *
 * Each of these macro constants expands to a nonnegative integer constant expression, which can be used with fesetround and fegetround to indicate one of the supported floating-point rounding modes. The implementation may define additional rounding mode constants in <fenv.h>, which should all begin with FE_ followed by at least one uppercase letter. Each macro is only defined if it is supported.
 *
 * Constant	Explanation
 * FE_DOWNWARD	rounding towards negative infinity
 * FE_TONEAREST	rounding towards nearest representable value
 * FE_TOWARDZERO	rounding towards zero
 * FE_UPWARD	rounding towards positive infinity
 * Additional rounding modes may be supported by an implementation.
 *
 * The current rounding mode affects the following:
 *
 * results of floating-point arithmetic operators outside of constant expressions
 * double x = 1;
 * x / 10; // 0.09999999999999999167332731531132594682276248931884765625 or
 *         // 0.1000000000000000055511151231257827021181583404541015625
 * results of standard library mathematical functions
 * sqrt(2); // 1.41421356237309492343001693370752036571502685546875 or
 *          // 1.4142135623730951454746218587388284504413604736328125
 * floating-point to floating-point implicit conversion and casts
 * double d = 1 + DBL_EPSILON;
 * float f = d; // 1.00000000000000000000000 or
 *              // 1.00000011920928955078125
 * string conversions such as strtod or printf
 * strtof("0.1", NULL); // 0.0999999940395355224609375 or
 *                      // 0.100000001490116119384765625
 * the library rounding functions nearbyint, rint, lrint
 * lrint(2.1); // 2 or 3
 * The current rounding mode does NOT affect the following:
 *
 * floating-point to integer implicit conversion and casts (always towards zero)
 * results of floating-point arithmetic operators in constant expressions executed at compile time (always to nearest)
 * the library functions round, lround, llround, ceil, floor, trunc
 * As with any floating-point environment functionality, rounding is only guaranteed if #pragma STDC FENV_ACCESS ON is set.
 *
 * Compilers that do not support the pragma may offer their own ways to support current rounding mode. For example Clang and GCC have the option -frounding-math intended to disable optimizations that would change the meaning of rounding-sensitive code.
 */

/**
 * TODO this macro
 * #define FE_DFL_ENV  // implementation defined
 * The macro constant FE_DFL_ENV expands to an expression of type const fenv_t*, which points to a full copy of the default floating-point environment, that is, the environment as loaded at program startup.
 * Additional macros that begin with FE_ followed by uppercase letters, and have the type const fenv_t*, may be supported by an implementation.
 */