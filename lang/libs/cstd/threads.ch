import "./time.ch"

/**
 * TODO mtx_t is implementation defined
 * @see https://en.cppreference.com/w/c/thread
 */
public struct mtx_t {

}

/**
 * TODO thrd_t is implementation defined
 * @see https://en.cppreference.com/w/c/thread
 */
public struct thrd_t {

}

/**
 * TODO cnd_t is implementation defined
 * @see https://en.cppreference.com/w/c/thread
 */
public struct cnd_t {

}

/**
 * TODO tss_t is implementation defined
 * @see https://en.cppreference.com/w/c/thread
 */
public struct tss_t {

}

/**
 * TODO tss_dtor_t is implementation defined
 * @see https://en.cppreference.com/w/c/thread
 */
public struct tss_dtor_t {

}

/**
 * TODO once_flag is implementation defined
 * @see https://en.cppreference.com/w/c/thread
 */
public struct once_flag {

}

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
@no_return
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

/**
 * Creates a new mutex object with type. The object pointed to by mutex is set to an identifier of the newly created mutex.
 * type must have one of the following values:
 * mtx_plain - a simple, non-recursive mutex is created.
 * mtx_timed - a non-recursive mutex, that supports timeout, is created.
 * mtx_plain | mtx_recursive - a recursive mutex is created.
 * mtx_timed | mtx_recursive - a recursive mutex, that supports timeout, is created.
 * @param mutex	-	pointer to the mutex to initialize
 * @param type	-	the type of the mutex
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/mtx_init
 */
public func mtx_init(mutex : *mut mtx_t, type : int ) : thrd_result

/**
 * Blocks the current thread until the mutex pointed to by mutex is locked.
 * The behavior is undefined if the current thread has already locked the mutex and the mutex is not recursive.
 * Prior calls to mtx_unlock on the same mutex synchronize-with this operation, and all lock/unlock operations on any given mutex form a single total order (similar to the modification order of an atomic)
 * @param mutex	-	pointer to the mutex to lock
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/mtx_lock
 */
public func mtx_lock(mutex : *mut mtx_t) : thrd_result

/**
 * Blocks the current thread until the mutex pointed to by mutex is locked or until the TIME_UTC based absolute calendar time point pointed to by time_point has been reached.
 * Since this function takes an absolute time, if a duration is required, the calendar time point must be calculated manually.
 * The behavior is undefined if the current thread has already locked the mutex and the mutex is not recursive.
 * The behavior is undefined if the mutex does not support timeout.
 * Prior calls to mtx_unlock on the same mutex synchronize-with this operation (if this operation succeeds), and all lock/unlock operations on any given mutex form a single total order (similar to the modification order of an atomic)
 * @param mutex (restricted)	-	pointer to the mutex to lock
 * @param time_point (restricted) -	pointer to the absolute calendar time until which to wait for the timeout
 * @return thrd_success if successful, thrd_timedout if the timeout time has been reached before the mutex is locked, thrd_error if an error occurs.
 * @see https://en.cppreference.com/w/c/thread/mtx_timedlock
 */
public func mtx_timedlock(mutex : *mtx_t, time_point : *timespec) : thrd_result

/**
 * Tries to lock the mutex pointed to by mutex without blocking. Returns immediately if the mutex is already locked.
 * Prior calls to mtx_unlock on the same mutex synchronize-with this operation (if this operation succeeds), and all lock/unlock operations on any given mutex form a single total order (similar to the modification order of an atomic)
 * @param mutex	-	pointer to the mutex to lock
 * @return thrd_success if successful, thrd_busy if the mutex has already been locked or due to a spurious failure to acquire an available mutex, thrd_error if an error occurs.
 * @see https://en.cppreference.com/w/c/thread/mtx_trylock
 */
public func mtx_trylock(mutex : *mtx_t) : thrd_result

/**
 * Unlocks the mutex pointed to by mutex.
 * The behavior is undefined if the mutex is not locked by the calling thread.
 * This function synchronizes-with subsequent mtx_lock, mtx_trylock, or mtx_timedlock on the same mutex. All lock/unlock operations on any given mutex form a single total order (similar to the modification order of an atomic).
 * @param mutex	-	pointer to the mutex to unlock
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/mtx_unlock
 */
public func mtx_unlock(mutex : *mtx_t) : thrd_result

/**
 * Destroys the mutex pointed to by mutex.
 * If there are threads waiting on mutex, the behavior is undefined.
 * @param mutex	-	pointer to the mutex to destroy
 * @see https://en.cppreference.com/w/c/thread/mtx_destroy
 */
public func mtx_destroy(mutex : *mtx_t);

/**
 * When passed to mtx_init, identifies the type of a mutex to create.
 * @see https://en.cppreference.com/w/c/thread/mtx_types
 */
enum mtx_type {
    plain,
    recursive,
    timed
};

/**
 * Calls function func exactly once, even if invoked from several threads. The completion of the function func synchronizes with all previous or subsequent calls to call_once with the same flag variable.
 * @param flag	-	pointer to an object of type call_once that is used to ensure func is called only once
 * @param func	-	the function to execute only once
 * @see https://en.cppreference.com/w/c/thread/call_once
 */
public func call_once(flag : *mut once_flag, func : () => void);

/**
 * TODO typedef
 * typedef unspecified once_flag
 * Complete object type capable of holding a flag used by call_once.
 * @see https://en.cppreference.com/w/c/thread/call_once
 */

/**
 * TODO macro ONCE_FLAG_INIT
 * #define ONCE_FLAG_INIT
 * Expands to a value that can be used to initialize an object of type once_flag.
 * @see https://en.cppreference.com/w/c/thread/call_once
 */

/**
 * Initializes new condition variable. The object pointed to by cond will be set to value that identifies the condition variable.
 * @param cond	-	pointer to a variable to store identifier of the condition variable to
 * @return thrd_success if the condition variable was successfully created. Otherwise returns thrd_nomem if there was insufficient amount of memory or thrd_error if another error occurred.
 */
public func cnd_init(cond : *mut cnd_t) : thrd_result

/**
 * Unblocks one thread that currently waits on condition variable pointed to by cond. If no threads are blocked, does nothing and returns thrd_success.
 * @param cond	-	pointer to a condition variable
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/cnd_signal
 */
public func cnd_signal(cond : *mut cnd_t) : thrd_result

/**
 * Unblocks all threads that are blocked on condition variable cond at the time of the call. If no threads are blocked on cond, the function does nothing and returns thrd_success.
 * @param cond	-	pointer to a condition variable
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/cnd_broadcast
 */
public func cnd_broadcast(cond : *cnd_t) : thrd_result

/**
 * Atomically unlocks the mutex pointed to by mutex and blocks on the condition variable pointed to by cond until the thread is signalled by cnd_signal or cnd_broadcast, or until a spurious wake-up occurs. The mutex is locked again before the function returns.
 * The behavior is undefined if the mutex is not already locked by the calling thread.
 * @param cond	-	pointer to the condition variable to block on
 * @param mutex	-	pointer to the mutex to unlock for the duration of the block
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/cnd_wait
 */
public func cnd_wait(cond : *mut cnd_t, mutex : *mut mtx_t) : thrd_result

/**
 * Atomically unlocks the mutex pointed to by mutex and blocks on the condition variable pointed to by cond until the thread is signalled by cnd_signal or cnd_broadcast, or until the TIME_UTC based time point pointed to by time_point has been reached, or until a spurious wake-up occurs. The mutex is locked again before the function returns.
 * The behavior is undefined if the mutex is not already locked by the calling thread.
 * @param cond	-	pointer to the condition variable to block on
 * @param mutex	-	pointer to the mutex to unlock for the duration of the block
 * @param time_point	-	pointer to an object specifying timeout time to wait until
 * @return thrd_success if successful, thrd_timedout if the timeout time has been reached before the mutex is locked, or thrd_error if an error occurred.
 * @see https://en.cppreference.com/w/c/thread/cnd_timedwait
 */
public func cnd_timedwait(cond : *mut cnd_t, mutex : *mut mtx_t, time_point : *timespec) : thrd_result

/**
 * Destroys the condition variable pointed to by cond.
 * If there are threads waiting on cond, the behavior is undefined.
 * @param cond	-	pointer to the condition variable to destroy
 * @see https://en.cppreference.com/w/c/thread/cnd_destroy
 */
public func cnd_destroy(cond : *mut cnd_t);

/**
 * TODO macro TSS_DTOR_ITERATIONS
 * #define TSS_DTOR_ITERATIONS
 * @see https://en.cppreference.com/w/c/thread/TSS_DTOR_ITERATIONS
 */

/**
 * Creates new thread-specific storage key and stores it in the object pointed to by tss_key. Although the same key value may be used by different threads, the values bound to the key by tss_set are maintained on a per-thread basis and persist for the life of the calling thread.
 * The value NULL is associated with the newly created key in all existing threads, and upon thread creation, the values associated with all TSS keys is initialized to NULL.
 * If destructor is not a null pointer, then also associates the destructor which is called when the storage is released by thrd_exit (but not by tss_delete and not at program termination by exit).
 * A call to tss_create from within a thread-specific storage destructor results in undefined behavior.
 * @param tss_key	-	pointer to memory location to store the new thread-specific storage key
 * @param destructor	-	pointer to a function to call at thread exit
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/tss_create
 */
public func tss_create(tss_key : *mut tss_t, destructor : tss_dtor_t) : thrd_result

/**
 * Returns the value held in thread-specific storage for the current thread identified by tss_key. Different threads may get different values identified by the same key.
 * On thread startup (see thrd_create), the values associated with all TSS keys are NULL. Different value may be placed in the thread-specific storage with tss_set.
 * @param tss_key	-	thread-specific storage key, obtained from tss_create and not deleted by tss_delete
 * @return The value on success, NULL on failure.
 * @see https://en.cppreference.com/w/c/thread/tss_get
 */
public func tss_get(tss_key : tss_t) : *mut void

/**
 * Sets the value of the thread-specific storage identified by tss_id for the current thread to val. Different threads may set different values to the same key.
 * The destructor, if available, is not invoked.
 * @param tss_id	-	thread-specific storage key, obtained from tss_create and not deleted by tss_delete
 * @param val	-	value to set thread-specific storage to
 * @return thrd_success if successful, thrd_error otherwise.
 * @see https://en.cppreference.com/w/c/thread/tss_set
 */
public func tss_set(tss_id : tss_t, val : *mut void) : thrd_result

/**
 * Destroys the thread-specific storage identified by tss_id.
 * The destructor, if one was registered by tss_create, is not called (they are only called at thread exit, either by thrd_exit or by returning from the thread function), it is the responsibility of the programmer to ensure that every thread that is aware of tss_id performed all necessary cleanup, before the call to tss_delete is made.
 * If tss_delete is called while another thread is executing destructors for tss_id, it's unspecified whether this changes the number of invocations to the associated destructor.
 * If tss_delete is called while the calling thread is executing destructors, then the destructor associated with tss_id will not be executed again on this thread.
 * @param tss_id	-	thread-specific storage key previously returned by tss_create and not yet deleted by tss_delete
 * @see https://en.cppreference.com/w/c/thread/tss_delete
 */
public func tss_delete(tss_id : tss_t);