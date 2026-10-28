// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * Interior-mutability cell. The value may be mutated through a shared
 * reference; `!Sync` keeps a shared cell from crossing threads while `Send`
 * still follows the value's own auto-trait derivation.
 */
@direct_init
public struct UnsafeCell<T> {

    internal var value : T

    @make
    func make(value : T) {
        return UnsafeCell<T> {
            value : value
        }
    }

    public func get(&self) : *mut T {
        return &raw mut self.value
    }

    public unsafe func get_ref(&self) : &T {
        return &self.value
    }

    public func get_mut(&mut self) : &mut T {
        return &mut self.value
    }

    public func into_inner(&mut self) : T where T: Copy {
        return unsafe(*self.get())
    }
}

public func <T> unsafe_cell(value : T) : UnsafeCell<T> {
    return UnsafeCell<T> {
        value : value
    }
}

impl<T> !Sync for UnsafeCell<T> {}

/**
 * A `Copy`-only interior-mutability cell. Reads copy the value, so `T` must be
 * `Copy`; writes go through the underlying `UnsafeCell`.
 */
@direct_init
public struct Cell<T> {

    internal var inner : UnsafeCell<T>

    @make
    func make(value : T) {
        return Cell<T> {
            inner : UnsafeCell<T> {
                value : value
            }
        }
    }

    public func get(&self) : T where T: Copy {
        return unsafe(*self.inner.get())
    }

    public func set(&mut self, value : T) {
        unsafe {
            *self.inner.get() = value
        }
    }

    public func get_mut(&mut self) : &mut T {
        return self.inner.get_mut()
    }

    public func as_ptr(&self) : *mut T {
        return self.inner.get()
    }
}

public func <T> cell(value : T) : Cell<T> {
    return Cell<T> {
        inner : unsafe_cell(value)
    }
}

impl<T> !Sync for Cell<T> {}

}
