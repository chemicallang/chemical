// TODO thrd_t is implementation defined
public struct thrd_t

/**
 * TODO the values for enum members are implementation defined
 * @see https://en.cppreference.com/w/c/thread/thrd_errors
 */
enum thrd_result {
    // indicates successful return value
    thrd_success,
    // indicates unsuccessful return value due to out of memory condition
    thrd_nomem,
    // indicates timed out return value
    thrd_timedout,
    // indicates unsuccessful return value due to resource temporary unavailable
    thrd_busy,
    // indicates unsuccessful return value
    thrd_error
};

/**
 * Checks whether lhs and rhs refer to the same thread.
 * @param lhs, rhs	-	threads to compare
 * @return Non-zero value if lhs and rhs refer to the same value, 0 otherwise.
 * @see https://en.cppreference.com/w/c/thread/thrd_equal
 */
public func thrd_equal(lhs : thrd_t, rhs : thrd_t) : int

/**
 * Returns the identifier of the calling thread.
 * @return The identifier of the calling thread.
 * @see https://en.cppreference.com/w/c/thread/thrd_current
 */
public func thrd_current() : thrd_t

/**
 * Blocks the execution of the current thread for at least until the TIME_UTC based duration pointed to by duration has elapsed.
 * The sleep may resume earlier if a signal that is not ignored is received. In such case, if remaining is not NULL, the remaining time duration is stored into the object pointed to by remaining.
 * @param duration	-	pointer to the duration to sleep for
 * @param remaining	-	pointer to the object to put the remaining time on interruption. May be NULL, in which case it is ignored
 * @return 0 on successful sleep, -1 if a signal occurred, other negative value if an error occurred.
 * @see https://en.cppreference.com/w/c/thread/thrd_sleep
 */
public func thrd_sleep(duration : *timespec, remaining : *timespec) : int

/**
 * Provides a hint to the implementation to reschedule the execution of threads, allowing other threads to run.
 * @see https://en.cppreference.com/w/c/thread/thrd_yield
 */
public func thrd_yield()

/**
 * First, for every thread-specific storage key which was created with a non-null destructor and for which the associated value is non-null (see tss_create), thrd_exit sets the value associated with the key to NULL and then invokes the destructor with the previous value of the key. The order in which the destructors are invoked is unspecified.
 * If, after this, there remain keys with both non-null destructors and values (e.g. if a destructor executed tss_set), the process is repeated up to TSS_DTOR_ITERATIONS times.
 * Finally, the thrd_exit function terminates execution of the calling thread and sets its result code to res.
 * If the last thread in the program is terminated with thrd_exit, the entire program terminates as if by calling exit with EXIT_SUCCESS as the argument (so the functions registered by atexit are executed in the context of that last thread)
 * @param res	-	the result value to return
 * @see https://en.cppreference.com/w/c/thread/thrd_exit
 */
public func thrd_exit(res : int) : void

/**
 * Detaches the thread identified by thr from the current environment. The resources held by the thread will be freed automatically once the thread exits.
 * @param thr	-	identifier of the thread to detach
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/thrd_detach
 */
public func thrd_detach(thr : thrd_t) : thrd_result

/**
 * Blocks the current thread until the thread identified by thr finishes execution.
 * If res is not a null pointer, the result code of the thread is put to the location pointed to by res.
 * The termination of the thread synchronizes-with the completion of this function.
 * The behavior is undefined if the thread was previously detached or joined by another thread.
 * @param thr	-	identifier of the thread to join
 * @param res	-	location to put the result code to
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/thrd_join
 */
public func thrd_join(thr : thrd_t, res : *int) : thrd_result


