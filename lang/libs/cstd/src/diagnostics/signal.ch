
/**
 * Sets the error handler for signal sig. The signal handler can be set so that default handling will occur, signal is ignored, or a user-defined function is called.
 * When signal handler is set to a function and a signal occurs, it is implementation defined whether signal(sig, SIG_DFL) will be executed immediately before the start of signal handler. Also, the implementation can prevent some implementation-defined set of signals from occurring while the signal handler runs.
 * @param sig	-	the signal to set the signal handler to. It can be an implementation-defined value or one of the following values:
 *       SIGABRT
 *       SIGFPE
 *       SIGILL defines signal types
 *       SIGINT
 *       SIGSEGV
 *       SIGTERM
 * @param handler	-	the signal handler. This must be one of the following:
 *       SIG_DFL macro. The signal handler is set to default signal handler.
 *       SIG_IGN macro. The signal is ignored.
 *       pointer to a function. The signature of the function must be equivalent to the following:
 *       void fun(int sig);
 * @return Previous signal handler on success or SIG_ERR on failure (setting a signal handler can be disabled on some implementations).
 * @see https://en.cppreference.com/w/c/program/signal
 */
@extern
public func signal(sig : int, handler : (sig : int) => void) : int

/**
 * Sends signal sig to the program. The signal handler, specified using signal(), is invoked.
 * If the user-defined signal handling strategy is not set using signal() yet, it is implementation-defined whether the signal will be ignored or default handler will be invoked.
 * @return 0 upon success, non-zero value on failure.
 * @see https://en.cppreference.com/w/c/program/raise
 */
@extern
public func raise(sig : int) : int


/**
 * TODO using int as sig_atomic_t as implementation is unknown
 * An integer type which can be accessed as an atomic entity even in the presence of asynchronous interrupts made by signals.
 * @see https://en.cppreference.com/w/c/program/sig_atomic_t
 */
@extern
public type sig_atomic_t = int

/**
 * TODO these macros haven't been done
 * #define SIG_DFL // implementation defined
 * #define SIG_IGN // implementation defined
 * @see https://en.cppreference.com/w/c/program/SIG_strategies
 */


/**
 * TODO these macros haven't been done
 * #define SIG_ERR // implementation defined
 * @see https://en.cppreference.com/w/c/program/SIG_ERR
 */

/**
 * TODO these macros haven't been done
 * #define SIGTERM // implementation defined
 * #define SIGSEGV // implementation defined
 * #define SIGINT // implementation defined
 * #define SIGILL // implementation defined
 * #define SIGABRT // implementation defined
 * #define SIGFPE // implementation defined
 * @see https://en.cppreference.com/w/c/program/SIG_types
 */