// Copyright (c) Chemical Language Foundation 2026.

public namespace std {
public namespace mem {

/**
 * Explicit unchecked zero-initialization for audited library internals that
 * immediately initialize or overwrite the value. Safe code must use
 * `zeroed<T>()` with `T: Zeroable`.
 */
public unsafe func <T> zeroed_unchecked() : T {
    return zeroed:unsafe<T>()
}

/**
 * Move `src` into `*dest` and return the previous value.
 *
 * The old value is moved out before the store, so its destructor runs exactly
 * once (when the caller drops the returned value). `src` is moved into the
 * destination and is not dropped by this function.
 */
public func <T> replace(dest : &mut T, src : T) : T {
    var old = zeroed:unsafe<T>()
    unsafe {
        var dest_ptr = dest as *mut T
        memcpy((&raw mut old) as *mut void, dest_ptr as *mut void, sizeof(T))
        memcpy(dest_ptr as *mut void, (&raw src) as *mut void, sizeof(T))
    }
    intrinsics::forget(src)
    return old
}

/**
 * Exchange the values behind two mutable references.
 *
 * Safe callers cannot pass aliasing references, so each value is moved once
 * and dropped once.
 */
public func <T> swap(a : &mut T, b : &mut T) {
    var tmp = zeroed:unsafe<T>()
    unsafe {
        var a_ptr = a as *mut T
        var b_ptr = b as *mut T
        memcpy((&raw mut tmp) as *mut void, a_ptr as *mut void, sizeof(T))
        memcpy(a_ptr as *mut void, b_ptr as *mut void, sizeof(T))
        memcpy(b_ptr as *mut void, (&raw mut tmp) as *mut void, sizeof(T))
    }
    intrinsics::forget(tmp)
}

/**
 * Consume a value without running its destructor.
 *
 * This is safe: leaking memory cannot violate memory safety. Use it when
 * ownership is transferred somewhere the compiler cannot see (for example an
 * FFI-owned allocation).
 */
public func <T> forget(value : T) {
    intrinsics::forget(value)
}

}
}
