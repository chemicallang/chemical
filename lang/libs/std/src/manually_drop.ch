// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * A `T` whose destructor is suppressed.
 *
 * The empty destructor is what makes `ManuallyDrop` useful: the contained
 * value is never dropped when the wrapper goes out of scope. `take` moves the
 * value out and leaves the wrapper zeroed, so the caller becomes responsible
 * for dropping it; calling `take` twice is undefined behavior.
 */
@allow_zeroed
@no_drop
@direct_init
public struct ManuallyDrop<T> {

    var value : T

    public func as_ptr(&mut self) : *mut T {
        return &raw mut self.value
    }

    public func as_ptr_const(&self) : *T {
        return &raw self.value
    }

    /** Move the value out without running its destructor. */
    public func take(&mut self) : T {
        var out = zeroed:unsafe<T>()
        unsafe {
            memcpy(&raw mut out, &raw mut self.value, sizeof(T))
            memset(&raw mut self.value as *mut void, 0, sizeof(T))
        }
        return out
    }
}

public func <T> manually_drop(value : T) : ManuallyDrop<T> {
    return ManuallyDrop<T> {
        value : value
    }
}

}
