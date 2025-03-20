
/**
 * TODO setjmp macro is not defined yet
 * #define setjmp(env) // implementation-defined
 * @see https://en.cppreference.com/w/c/program/setjmp
 */

/**
 * TODO jmp_bug type not defined as it's implementation is unknown
 * @see https://en.cppreference.com/w/c/program/jmp_buf
 */
@export
public type jmp_buf = int

/**
 * Loads the execution context env saved by a previous call to setjmp. This function does not return. Control is transferred to the call site of the macro setjmp that set up env. That setjmp then returns the value, passed as the status.
 * If the function that called setjmp has exited (whether by return or by a different longjmp higher up the stack), the behavior is undefined. In other words, only long jumps up the call stack are allowed.
 * Jumping across threads (if the function that called setjmp was executed by another thread) is also undefined behavior.
 * @param env	-	variable referring to the execution state of the program saved by setjmp
 * @param status	-	the value to return from setjmp. If it is equal to ​0​, 1 is used instead
 * @see https://en.cppreference.com/w/c/program/longjmp
 */
@no_return
@export
public func longjmp(env : jmp_buf, status : int);