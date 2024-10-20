
/**
 * The va_start macro enables access to the variable arguments following the named argument parmN(until C23).
 * va_start shall be invoked with an instance to a valid va_list object ap before any calls to va_arg.
 * Only the first argument passed to va_start is evaluated. Any additional arguments are neither expanded nor used in any way.
 * @param ap	-	an instance of the va_list type
 * @param parmN	-	the named parameter preceding the first variable parameter
 * @see https://en.cppreference.com/w/c/variadic/va_start
 */
public func va_start(ap : va_list, parmN : ... );

/**
 * TODO macro
 * T va_arg( va_list ap, T );
 * @see https://en.cppreference.com/w/c/variadic/va_arg
 */

/**
 * TODO typealias va_list
 * @see https://en.cppreference.com/w/c/variadic/va_list
 */
public typealias va_list = int

/**
 * The va_copy macro copies src to dest.
 * va_end should be called on dest before the function returns or any subsequent re-initialization of dest (via calls to va_start or va_copy).
 * @param dest	-	an instance of the va_list type to initialize
 * @param src	-	the source va_list that will be used to initialize dest
 * @see https://en.cppreference.com/w/c/variadic/va_copy
 */
public func va_copy(dest : va_list, src : va_list);

/**
 * The va_end macro performs cleanup for an ap object initialized by a call to va_start or va_copy. va_end may modify ap so that it is no longer usable.
 * If there is no corresponding call to va_start or va_copy, or if va_end is not called before a function that calls va_start or va_copy returns, the behavior is undefined.
 * @param ap	-	an instance of the va_list type to clean up
 */
public func va_end(ap : va_list);