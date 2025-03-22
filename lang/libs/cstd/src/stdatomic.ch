import "./common/atomic_types.ch"

/**
 * TODO These macros are implementation defined
 * #define ATOMIC_BOOL_LOCK_FREE     // implementation-defined
 * #define ATOMIC_CHAR_LOCK_FREE     // implementation-defined
 * #define ATOMIC_CHAR16_T_LOCK_FREE // implementation-defined
 * #define ATOMIC_CHAR32_T_LOCK_FREE // implementation-defined
 * #define ATOMIC_WCHAR_T_LOCK_FREE  // implementation-defined
 * #define ATOMIC_SHORT_LOCK_FREE    // implementation-defined
 * #define ATOMIC_INT_LOCK_FREE      // implementation-defined
 * #define ATOMIC_LONG_LOCK_FREE     // implementation-defined
 * #define ATOMIC_LLONG_LOCK_FREE    // implementation-defined
 * #define ATOMIC_POINTER_LOCK_FREE  // implementation-defined
 * #define ATOMIC_CHAR8_T_LOCK_FREE  // implementation-defined
 * @see https://en.cppreference.com/w/c/atomic/ATOMIC_LOCK_FREE_consts
 */

/**
 * TODO atomic_flag is implementation defined
 * atomic_flag is an atomic boolean type. Unlike other atomic types, it is guaranteed to be lock-free. Unlike atomic_bool, atomic_flag does not provide load or store operations.
 */
@extern
public struct atomic_flag {

}

/**
 * memory_order specifies how memory accesses, including regular, non-atomic memory accesses, are to be ordered around an atomic operation. Absent any constraints on a multi-core system, when multiple threads simultaneously read and write to several variables, one thread can observe the values change in an order different from the order another thread wrote them. Indeed, the apparent order of changes can even differ among multiple reader threads. Some similar effects can occur even on uniprocessor systems due to compiler transformations allowed by the memory model.
 * The default behavior of all atomic operations in the language and the library provides for sequentially consistent ordering (see discussion below). That default can hurt performance, but the library's atomic operations can be given an additional memory_order argument to specify the exact constraints, beyond atomicity, that the compiler and processor must enforce for that operation.
 * TODO implementation values should be verified to match
 * @see https://en.cppreference.com/w/c/atomic/memory_order
 */
public enum memory_order {
    /**
     * Relaxed operation: there are no synchronization or ordering constraints imposed on other reads or writes, only this operation's atomicity is guaranteed
     */
    relaxed,
    /**
     * A load operation with this memory order performs a consume operation on the affected memory location: no reads or writes in the current thread dependent on the value currently loaded can be reordered before this load. Writes to data-dependent variables in other threads that release the same atomic variable are visible in the current thread. On most platforms, this affects compiler optimizations only
     */
    consume,
    /**
     * A load operation with this memory order performs the acquire operation on the affected memory location: no reads or writes in the current thread can be reordered before this load. All writes in other threads that release the same atomic variable are visible in the current thread
     */
    acquire,
    /**
     * A store operation with this memory order performs the release operation: no reads or writes in the current thread can be reordered after this store. All writes in the current thread are visible in other threads that acquire the same atomic variable (see Release-Acquire ordering below) and writes that carry a dependency into the atomic variable become visible in other threads that consume the same atomic
     */
    release,
    /**
     * A read-modify-write operation with this memory order is both an acquire operation and a release operation. No memory reads or writes in the current thread can be reordered before the load, nor after the store. All writes in other threads that release the same atomic variable are visible before the modification and the modification is visible in other threads that acquire the same atomic variable.
     */
    acq_rel,
    /**
     * A load operation with this memory order performs an acquire operation, a store performs a release operation, and read-modify-write performs both an acquire operation and a release operation, plus a single total order exists in which all threads observe all modifications in the same order
     */
    seq_cst
};

/**
 * TODO atomic type is C++ implementation defined
 */
@extern
public struct atomic<T> {

}

/**
 * Determines if the atomic operations on all objects of the type A (the type of the object pointed to by obj) are lock-free. In any given program execution, the result of calling atomic_is_lock_free is the same for all pointers of the same type.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_is_lock_free)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile) -	pointer to the atomic object to inspect
 * @return true if the operations on all objects of the type A are lock-free, false otherwise.
 * @see https://en.cppreference.com/w/c/atomic/atomic_is_lock_free
 */
@extern
public func <A> atomic_is_lock_free(obj : *atomic<A>) : bool

/**
 * Atomically replaces the value of the atomic variable pointed to by obj with desired. The operation is atomic write operation.
 * The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order. order must be one of memory_order_relaxed, memory_order_release or memory_order_seq_cst. Otherwise the behavior is undefined.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_store)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj	-	pointer to the atomic object to modify
 * @see https://en.cppreference.com/w/c/atomic/atomic_store
 */
@extern
public func <A, C> atomic_store(obj : *mut atomic<A>, desired : atomic<C>);

/**
 * Atomically replaces the value of the atomic variable pointed to by obj with desired. The operation is atomic write operation.
 * The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order. order must be one of memory_order_relaxed, memory_order_release or memory_order_seq_cst. Otherwise the behavior is undefined.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_store)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj	-	pointer to the atomic object to modify
 * @param order	-	the memory synchronization ordering for this operation
 * @see https://en.cppreference.com/w/c/atomic/atomic_store
 */
@extern
public func <A, C> atomic_store_explicit(obj : *mut atomic<A>, desired : atomic<C>, order : memory_order);

/**
 * Atomically loads and returns the current value of the atomic variable pointed to by obj. The operation is atomic read operation.
 * The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order. order must be one of memory_order_relaxed, memory_order_consume, memory_order_acquire or memory_order_seq_cst. Otherwise the behavior is undefined.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_load)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile)	-	pointer to the atomic object to access
 * @return The current value of the atomic variable pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_load
 */
@extern
public func <A, C> atomic_load(obj : *atomic<A>) : C

/**
 * Atomically loads and returns the current value of the atomic variable pointed to by obj. The operation is atomic read operation.
 * The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order. order must be one of memory_order_relaxed, memory_order_consume, memory_order_acquire or memory_order_seq_cst. Otherwise the behavior is undefined.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_load)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile)	-	pointer to the atomic object to access
 * @param order	-	the memory synchronization ordering for this operation
 * @return The current value of the atomic variable pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_load
 */
@extern
public func <A, C> atomic_load_explicit(obj : *atomic<A>, order : memory_order) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with desired and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_exchange)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined..
 * @param obj (volatile)	-	pointer to the atomic object to modify
 * @param desired	-	the value to replace the atomic object with
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously be the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_exchange
 */
@extern
public func <A, C> atomic_exchange(obj : *atomic<A>, desired : atomic<C>) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with desired and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_exchange)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined..
 * @param obj (volatile)	-	pointer to the atomic object to modify
 * @param desired	-	the value to replace the atomic object with
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously be the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_exchange
 */
@extern
public func <A, C> atomic_exchange_explicit(obj : *atomic<A>, desired : atomic<C>, order : memory_order) : atomic<C>

/**
 * Atomically compares the contents of memory pointed to by obj with the contents of memory pointed to by expected, and if those are bitwise equal, replaces the former with desired (performs read-modify-write operation). Otherwise, loads the actual contents of memory pointed to by obj into *expected (performs load operation).
 * The memory models for the read-modify-write and load operations are succ and fail respectively. The (1-2) versions use memory_order_seq_cst by default.
 * The weak forms ((2) and (4)) of the functions are allowed to fail spuriously, that is, act as if *obj != *expected even if they are equal. When a compare-and-exchange is in a loop, the weak version will yield better performance on some platforms. When a weak compare-and-exchange would require a loop and a strong one would not, the strong one is preferable.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_compare_exchange)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile) -	pointer to the atomic object to test and modify
 * @param expected	-	pointer to the value expected to be found in the atomic object
 * @param desired	-	the value to store in the atomic object if it is as expected
 * @param succ	-	the memory synchronization ordering for the read-modify-write operation if the comparison succeeds. All values are permitted.
 * @param fail	-	the memory synchronization ordering for the load operation if the comparison fails. Cannot be memory_order_release or memory_order_acq_rel and cannot specify stronger ordering than succ
 * @return The result of the comparison: true if *obj was equal to *exp, false otherwise.
 * @see https://en.cppreference.com/w/c/atomic/atomic_compare_exchange
 */
@extern
public func <A, C> atomic_compare_exchange_strong(obj : *atomic<A>, expected : *atomic<C>, desired : atomic<C>) : bool

/**
 * Atomically compares the contents of memory pointed to by obj with the contents of memory pointed to by expected, and if those are bitwise equal, replaces the former with desired (performs read-modify-write operation). Otherwise, loads the actual contents of memory pointed to by obj into *expected (performs load operation).
 * The memory models for the read-modify-write and load operations are succ and fail respectively. The (1-2) versions use memory_order_seq_cst by default.
 * The weak forms ((2) and (4)) of the functions are allowed to fail spuriously, that is, act as if *obj != *expected even if they are equal. When a compare-and-exchange is in a loop, the weak version will yield better performance on some platforms. When a weak compare-and-exchange would require a loop and a strong one would not, the strong one is preferable.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_compare_exchange)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile)	-	pointer to the atomic object to test and modify
 * @param expected	-	pointer to the value expected to be found in the atomic object
 * @param desired	-	the value to store in the atomic object if it is as expected
 * @param succ	-	the memory synchronization ordering for the read-modify-write operation if the comparison succeeds. All values are permitted.
 * @param fail	-	the memory synchronization ordering for the load operation if the comparison fails. Cannot be memory_order_release or memory_order_acq_rel and cannot specify stronger ordering than succ
 * @return The result of the comparison: true if *obj was equal to *exp, false otherwise.
 * @see https://en.cppreference.com/w/c/atomic/atomic_compare_exchange
 */
@extern
public func <A, C> atomic_compare_exchange_weak(obj : *atomic<A>, expected : *atomic<C>, desired : atomic<C>) : bool

/**
 * Atomically compares the contents of memory pointed to by obj with the contents of memory pointed to by expected, and if those are bitwise equal, replaces the former with desired (performs read-modify-write operation). Otherwise, loads the actual contents of memory pointed to by obj into *expected (performs load operation).
 * The memory models for the read-modify-write and load operations are succ and fail respectively. The (1-2) versions use memory_order_seq_cst by default.
 * The weak forms ((2) and (4)) of the functions are allowed to fail spuriously, that is, act as if *obj != *expected even if they are equal. When a compare-and-exchange is in a loop, the weak version will yield better performance on some platforms. When a weak compare-and-exchange would require a loop and a strong one would not, the strong one is preferable.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_compare_exchange)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile)	-	pointer to the atomic object to test and modify
 * @param expected	-	pointer to the value expected to be found in the atomic object
 * @param desired	-	the value to store in the atomic object if it is as expected
 * @param succ	-	the memory synchronization ordering for the read-modify-write operation if the comparison succeeds. All values are permitted.
 * @param fail	-	the memory synchronization ordering for the load operation if the comparison fails. Cannot be memory_order_release or memory_order_acq_rel and cannot specify stronger ordering than succ
 * @return The result of the comparison: true if *obj was equal to *exp, false otherwise.
 * @see https://en.cppreference.com/w/c/atomic/atomic_compare_exchange
 */
@extern
public func <A, C> atomic_compare_exchange_strong_explicit(obj : *atomic<A>, expected : *atomic<C>, desired : atomic<C>, succ : memory_order, fail : memory_order);

/**
 * Atomically compares the contents of memory pointed to by obj with the contents of memory pointed to by expected, and if those are bitwise equal, replaces the former with desired (performs read-modify-write operation). Otherwise, loads the actual contents of memory pointed to by obj into *expected (performs load operation).
 * The memory models for the read-modify-write and load operations are succ and fail respectively. The (1-2) versions use memory_order_seq_cst by default.
 * The weak forms ((2) and (4)) of the functions are allowed to fail spuriously, that is, act as if *obj != *expected even if they are equal. When a compare-and-exchange is in a loop, the weak version will yield better performance on some platforms. When a weak compare-and-exchange would require a loop and a strong one would not, the strong one is preferable.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_compare_exchange)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile)	-	pointer to the atomic object to test and modify
 * @param expected	-	pointer to the value expected to be found in the atomic object
 * @param desired	-	the value to store in the atomic object if it is as expected
 * @param succ	-	the memory synchronization ordering for the read-modify-write operation if the comparison succeeds. All values are permitted.
 * @param fail	-	the memory synchronization ordering for the load operation if the comparison fails. Cannot be memory_order_release or memory_order_acq_rel and cannot specify stronger ordering than succ
 * @return The result of the comparison: true if *obj was equal to *exp, false otherwise.
 * @see https://en.cppreference.com/w/c/atomic/atomic_compare_exchange
 */
@extern
public func <A, C> atomic_compare_exchange_weak_explicit(obj : *atomic<A>, expected : *atomic<C>, desired : atomic<C>, succ : memory_order, fail : memory_order) : bool

/**
 * Atomically replaces the value pointed by obj with the result of addition of arg to the old value of obj, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_add)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * For signed integer types, arithmetic is defined to use two’s complement representation. There are no undefined results. For pointer types, the result may be an undefined address, but the operations otherwise have no undefined behavior.
 * @param obj	-	pointer to the atomic object to modify
 * @param arg	-	the value to add to the value stored in the atomic object
 * @return The value held previously by the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_add
 */
@extern
public func <A, M, C> atomic_fetch_add(obj : *atomic<A>, arg : atomic<M>) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of addition of arg to the old value of obj, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_add)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * For signed integer types, arithmetic is defined to use two’s complement representation. There are no undefined results. For pointer types, the result may be an undefined address, but the operations otherwise have no undefined behavior.
 * @param obj	-	pointer to the atomic object to modify
 * @param arg	-	the value to add to the value stored in the atomic object
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously by the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_add
 */
@extern
public func <A, M, C> atomic_fetch_add_explicit(obj : *atomic<A>, arg : atomic<M>, order : memory_order) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of subtraction of arg from the old value of obj, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_sub)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * For signed integer types, arithmetic is defined to use two’s complement representation. There are no undefined results. For pointer types, the result may be an undefined address, but the operations otherwise have no undefined behavior.
 * @param obj (volatile) -	pointer to the atomic object to modify
 * @param arg	-	the value to subtract from the value stored in the atomic object
 * @return The value held previously by the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_sub
 */
@extern
public func <A, M, C> atomic_fetch_sub(obj : *atomic<A>, arg : atomic<M>) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of subtraction of arg from the old value of obj, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_sub)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * For signed integer types, arithmetic is defined to use two’s complement representation. There are no undefined results. For pointer types, the result may be an undefined address, but the operations otherwise have no undefined behavior.
 * @param obj (volatile) -	pointer to the atomic object to modify
 * @param arg	-	the value to subtract from the value stored in the atomic object
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously by the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_sub
 */
@extern
public func <A, M, C> atomic_fetch_sub_explicit(obj : *atomic<A>, arg : atomic<M>, order : memory_order) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of bitwise OR between the old value of obj and arg, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_or)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile) -	pointer to the atomic object to modify
 * @param arg	-	the value to bitwise OR to the value stored in the atomic object
 * @return The value held previously be the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_or
 */
@extern
public func <A, M, C> atomic_fetch_or(obj : *atomic<A>, arg : atomic<M>) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of bitwise OR between the old value of obj and arg, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_or)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile) -	pointer to the atomic object to modify
 * @param arg	-	the value to bitwise OR to the value stored in the atomic object
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously be the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_or
 */
@extern
public func <A, M, C> atomic_fetch_or_explicit(obj : *atomic<A>, arg : atomic<M>, order : memory_order) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of bitwise XOR between the old value of obj and arg, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_xor)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile) -	pointer to the atomic object to modify
 * @param arg	-	the value to bitwise XOR to the value stored in the atomic object
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously be the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_xor
 */
@extern
public func <A, M, C> atomic_fetch_xor(obj : *atomic<A>, arg : atomic<M>) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of bitwise XOR between the old value of obj and arg, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_xor)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile) -	pointer to the atomic object to modify
 * @param arg	-	the value to bitwise XOR to the value stored in the atomic object
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously be the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_xor
 */
@extern
public func <A, M, C> atomic_fetch_xor_explicit(obj : *atomic<A>, arg : atomic<M>, order : memory_order) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of bitwise AND between the old value of obj and arg, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_and)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile) -	pointer to the atomic object to modify
 * @param arg	-	the value to bitwise AND to the value stored in the atomic object
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously be the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_and
 */
@extern
public func <A, M, C> atomic_fetch_and(obj : *atomic<A>, arg : atomic<M>) : atomic<C>

/**
 * Atomically replaces the value pointed by obj with the result of bitwise AND between the old value of obj and arg, and returns the value obj held previously. The operation is read-modify-write operation. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. M is either the non-atomic type corresponding to A if A is atomic integer type, or ptrdiff_t if A is atomic pointer type.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_fetch_and)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj (volatile) -	pointer to the atomic object to modify
 * @param arg	-	the value to bitwise AND to the value stored in the atomic object
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The value held previously be the atomic object pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_fetch_and
 */
@extern
public func <A, M, C> atomic_fetch_and_explicit(obj : *atomic<A>, arg : atomic<M>, order : memory_order) : atomic<C>

/**
 * Atomically changes the state of a atomic_flag pointed to by obj to set (true) and returns the previous value. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * The argument is pointer to a volatile atomic flag to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic flags.
 * @param obj (volatile) -	pointer to the atomic flag object to modify
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The previous value held by the atomic flag pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_flag_test_and_set
 */
@extern
public func atomic_flag_test_and_set(obj : *atomic_flag) : bool

/**
 * Atomically changes the state of a atomic_flag pointed to by obj to set (true) and returns the previous value. The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * The argument is pointer to a volatile atomic flag to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic flags.
 * @param obj (volatile) -	pointer to the atomic flag object to modify
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @return The previous value held by the atomic flag pointed to by obj.
 * @see https://en.cppreference.com/w/c/atomic/atomic_flag_test_and_set
 */
@extern
public func atomic_flag_test_and_set_explicit(obj : *atomic_flag, order : memory_order) : bool

/**
 * Atomically changes the state of a atomic_flag pointed to by obj to clear (false). The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * The argument is pointer to a volatile atomic flag to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic flags.
 * @param obj (volatile) -	pointer to the atomic flag object to modify
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @see https://en.cppreference.com/w/c/atomic/atomic_flag_clear
 */
@extern
public func atomic_flag_clear(obj : *atomic_flag)

/**
 * Atomically changes the state of a atomic_flag pointed to by obj to clear (false). The first version orders memory accesses according to memory_order_seq_cst, the second version orders memory accesses according to order.
 * The argument is pointer to a volatile atomic flag to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic flags.
 * @param obj  (volatile) -	pointer to the atomic flag object to modify
 * @param order	-	the memory synchronization ordering for this operation: all values are permitted
 * @see https://en.cppreference.com/w/c/atomic/atomic_flag_clear
 */
@extern
public func atomic_flag_clear_explicit(obj : *atomic_flag, order : memory_order)

/**
 * Initializes the default-constructed atomic object obj with the value desired. The function is not atomic: concurrent access from another thread, even through an atomic operation, is a data race.
 * This is a generic function defined for all atomic object types A. The argument is pointer to a volatile atomic type to accept addresses of both non-volatile and volatile (e.g. memory-mapped I/O) atomic objects, and volatile semantic is preserved when applying this operation to volatile atomic objects. C is the non-atomic type corresponding to A.
 * It is unspecified whether the name of a generic function is a macro or an identifier declared with external linkage. If a macro definition is suppressed in order to access an actual function (e.g. parenthesized like (atomic_init)(...)), or a program defines an external identifier with the name of a generic function, the behavior is undefined.
 * @param obj	-	pointer to an atomic object to initialize
 * @param desired	-	the value to initialize atomic object with
 * @see https://en.cppreference.com/w/c/atomic/atomic_init
 */
@extern
public func <A, C> atomic_init(obj : *atomic<A>, desired : atomic<C>);

/**
 * TODO macro
 * Expands to an initializer that can be used to initialize atomic_flag type to the clear state. The value of an atomic_flag that is not initialized using this macro is indeterminate.
 * #define ATOMIC_FLAG_INIT
 * @see https://en.cppreference.com/w/c/atomic/ATOMIC_FLAG_INIT
 */

/**
 * Informs the compiler that the dependency tree started by an memory_order_consume atomic load operation does not extend past the return value of kill_dependency; that is, the argument does not carry a dependency into the return value.
 * The function is implemented as a macro. A is the type of y.
 * @param y	-	the expression whose return value is to be removed from a dependency tree
 * @return Returns y, no longer a part of a dependency tree.
 * @see https://en.cppreference.com/w/c/atomic/kill_dependency
 */
@comptime
@extern
public func <A> kill_dependency(y : atomic<A>) : atomic<A> {
    // TODO do something
    return y;
}

/**
 * Establishes memory synchronization ordering of non-atomic and relaxed atomic accesses, as instructed by order, without an associated atomic operation. For example, all non-atomic and relaxed atomic stores that happen before a memory_order_release fence in thread A will be synchronized with non-atomic and relaxed atomic loads from the same locations made in thread B after an memory_order_acquire fence.
 * @param order	-	the memory ordering executed by this fence
 * @see https://en.cppreference.com/w/c/atomic/atomic_thread_fence
 */
@extern
public func atomic_thread_fence(order : memory_order)

/**
 * Establishes memory synchronization ordering of non-atomic and relaxed atomic accesses, as instructed by order, between a thread and a signal handler executed on the same thread. This is equivalent to atomic_thread_fence, except no CPU instructions for memory ordering are issued. Only reordering of the instructions by the compiler is suppressed as order instructs. For example, a fence with release semantics prevents reads or writes from being moved past subsequent writes and a fence with acquire semantics prevents reads or writes from being moved ahead of preceding reads.
 * @param order	-	the memory ordering executed by this fence
 * @see https://en.cppreference.com/w/c/atomic/atomic_signal_fence
 */
@extern
public func atomic_signal_fence(order : memory_order)