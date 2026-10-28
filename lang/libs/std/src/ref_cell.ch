// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * Reads the borrow state of a `RefCell`. Kept as a monomorphic helper so the
 * generic `RefCell<T>` methods get the concrete `*mut int` instead of a
 * generic-pointer type that cannot be compared.
 */
/**
 * Interior mutability with runtime borrow tracking.
 *
 * `try_borrow` and `try_borrow_mut` return `None` instead of panicking when the
 * requested access conflicts with a live guard. The returned `Ref`/`RefMut`
 * decrement the state when they leave scope. A `RefCell` is not `Sync` (like
 * Rust), because its state is mutated through shared references.
 */
@direct_init
public struct RefCell<T> {

    internal var state : int
    internal var value : UnsafeCell<T>

    func release_shared(&self) {
        var s = unsafe(&raw mut self.state)
        unsafe {
            *s = *s - 1
        }
    }

    func release_mut(&self) {
        var s = unsafe(&raw mut self.state)
        unsafe {
            *s = 0
        }
    }

    public func try_borrow(&self) : Option<Ref<T>> {
        var s = unsafe(&raw mut self.state)
        unsafe {
            if(*s < 0) {
                return Option.None<Ref<T>>()
            }
            *s = *s + 1
        }
        return Option.Some<Ref<T>>(Ref<T> {
            cell : self
        })
    }

    public func try_borrow_mut(&self) : Option<RefMut<T>> {
        var s = unsafe(&raw mut self.state)
        unsafe {
            if(*s != 0) {
                return Option.None<RefMut<T>>()
            }
            *s = -1
        }
        return Option.Some<RefMut<T>>(RefMut<T> {
            cell : self
        })
    }

    public func borrow_count(&self) : int {
        return self.state
    }
}

public struct Ref<'a, T> {

    internal var cell : &'a RefCell<T>

    @delete
    func delete(&mut self) {
        self.cell.release_shared()
    }

    public func get(&self) : &T {
        return unsafe(self.cell.value.get_ref())
    }
}

public struct RefMut<'a, T> {

    internal var cell : &'a RefCell<T>

    @delete
    func delete(&mut self) {
        self.cell.release_mut()
    }

    public func get_mut(&mut self) : &mut T {
        return unsafe(&mut *self.cell.value.get())
    }
}

public func <T> ref_cell(value : T) : RefCell<T> {
    return RefCell<T> {
        state : 0,
        value : unsafe_cell(value)
    }
}

}
