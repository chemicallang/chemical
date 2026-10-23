// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * Storage for a `T` that may not be initialized yet.
 *
 * `MaybeUninit<T>` deliberately has an empty destructor: the bytes of `value`
 * are raw storage until `assume_init` moves them out. Reading or dropping an
 * uninitialized value is therefore a caller responsibility, which is why
 * `assume_init` requires an `unsafe` block at the call site. `write` may be
 * called repeatedly; it overwrites the raw bytes without dropping the old
 * value, exactly like Rust's `MaybeUninit::write`.
 */
@allow_zeroed
@no_drop
@direct_init
public struct MaybeUninit<T> {

    var value : T

    public func write(&mut self, value : T) {
        unsafe {
            memcpy(&raw mut self.value, &raw value, sizeof(T))
            intrinsics::forget(value)
        }
    }

    /**
     * Take the initialized value out and leave the storage zeroed. Calling this
     * twice, or before `write`, is undefined behavior; callers must uphold the
     * initialization invariant inside an `unsafe` block.
     */
    public func assume_init(&mut self) : T {
        var out = zeroed:unsafe<T>()
        unsafe {
            memcpy(&raw mut out, &raw mut self.value, sizeof(T))
            memset(&raw mut self.value as *mut void, 0, sizeof(T))
        }
        return out
    }

    public func as_ptr(&mut self) : *mut T {
        return &raw mut self.value
    }

    public func as_ptr_const(&self) : *T {
        return &raw self.value
    }
}

public func <T> maybe_uninit() : MaybeUninit<T> {
    return zeroed:unsafe<MaybeUninit<T>>()
}

}
