// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * A non-null raw pointer wrapper.
 *
 * The field is private and the safe constructors (`non_null`, `non_null_try`)
 * establish the non-null invariant. `non_null_unchecked` is the explicit
 * unsafe constructor for callers that already have a non-null guarantee (for
 * example immediately after a successful allocation check). The pointee must
 * still be live and aligned when dereferenced, and the wrapper is neither
 * `Send` nor `Sync` because it stores a raw pointer.
 */
@direct_init
public struct NonNull<T> {

    private var ptr : *mut T

    public func as_ptr(&self) : *mut T {
        return self.ptr
    }

    public func is_null(&self) : bool {
        return self.ptr == null
    }
}

/**
 * Safe constructor. Wrapping null is a programming error (the type's invariant
 * is non-null), so it aborts instead of producing an invalid wrapper. Use
 * `non_null_try` when null is an expected outcome.
 */
public func <T> non_null(ptr : *mut T) : NonNull<T> {
    if(ptr == null) {
        panic("NonNull cannot wrap a null pointer")
    }
    return unsafe(non_null_unchecked<T>(ptr))
}

/**
 * Checked constructor that preserves the non-null invariant and reports null
 * as `None` instead of aborting.
 */
public func <T> non_null_try(ptr : *mut T) : Option<NonNull<T>> {
    if(ptr == null) {
        return Option.None<NonNull<T>>()
    }
    return Option.Some<NonNull<T>>(non_null<T>(ptr))
}

/**
 * Unsafe constructor for an already-checked non-null pointer.
 */
public unsafe func <T> non_null_unchecked(ptr : *mut T) : NonNull<T> {
    return NonNull<T> {
        ptr : ptr
    }
}

}
