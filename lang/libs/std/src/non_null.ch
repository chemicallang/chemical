// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * A non-null raw pointer wrapper. It carries no extra validity guarantee: the
 * pointee must still be live and aligned when dereferenced, and the wrapper is
 * neither `Send` nor `Sync` (raw pointers are not auto traits).
 */
@direct_init
public struct NonNull<T> {

    var ptr : *mut T

    @make
    func make(ptr : *mut T) {
        return NonNull<T> {
            ptr : ptr
        }
    }

    public func as_ptr(&self) : *mut T {
        return self.ptr
    }

    public func is_null(&self) : bool {
        return self.ptr == null
    }
}

public func <T> non_null(ptr : *mut T) : NonNull<T> {
    return NonNull<T> {
        ptr : ptr
    }
}

}
