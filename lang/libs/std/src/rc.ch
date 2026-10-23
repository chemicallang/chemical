// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * Internal heap block shared by all clones of an `Rc<T>` and its `Weak<T>`
 * handles. The value is stored in `ManuallyDrop` so the last strong owner can
 * destroy it while weak handles keep the allocation (and the counts) alive.
 */
public struct RcInner<T> {
    var strong : size_t
    var weak : size_t
    var value : ManuallyDrop<T>
}

/**
 * Single-threaded reference-counted ownership.
 *
 * `Rc<T>` is intentionally neither `Send` nor `Sync`: the counts are updated
 * non-atomically. Use `Rc` only on one thread; `Arc` is the thread-safe form.
 */
@direct_init
public struct Rc<T> {

    var ptr : *mut RcInner<T>

    @delete
    func delete(&mut self) {
        if(self.ptr == null) {
            return
        }
        unsafe {
            self.ptr.strong = self.ptr.strong - 1
            if(self.ptr.strong == 0) {
                self.ptr.value.destroy()
                if(self.ptr.weak == 0) {
                    free(self.ptr as *mut void)
                }
            }
        }
        self.ptr = null
    }

    public func clone(&self) : Rc<T> {
        var inner = self.ptr
        unsafe {
            inner.strong = inner.strong + 1
        }
        return Rc<T> {
            ptr : inner
        }
    }

    public func downgrade(&self) : Weak<T> {
        var inner = self.ptr
        unsafe {
            inner.weak = inner.weak + 1
        }
        return Weak<T> {
            ptr : inner
        }
    }

    public func get(&self) : &T {
        return unsafe(&self.ptr.value.value)
    }

    public func strong_count(&self) : size_t {
        return unsafe(self.ptr.strong)
    }

    public func weak_count(&self) : size_t {
        return unsafe(self.ptr.weak)
    }

    /** Mutably access the value only when this is the sole owner. */
    public func get_mut(&mut self) : Option<&mut T> {
        if(self.strong_count() != 1) {
            return Option.None<&mut T>()
        }
        return Option.Some<&mut T>(unsafe(&mut self.ptr.value.value))
    }

    /** Move the value out only when this is the sole owner. */
    public func try_unwrap(&mut self) : Option<T> {
        if(self.strong_count() != 1) {
            return Option.None<T>()
        }
        var out = unsafe(self.ptr.value.take())
        unsafe {
            self.ptr.strong = 0
            if(self.ptr.weak == 0) {
                free(self.ptr as *mut void)
            }
        }
        self.ptr = null
        return Option.Some<T>(out)
    }
}

/**
 * A non-owning handle to an `Rc<T>` allocation. Weak handles keep the
 * allocation and its counts alive but never keep the value alive; `upgrade`
 * returns `None` once the last strong owner is gone.
 */
@direct_init
public struct Weak<T> {

    var ptr : *mut RcInner<T>

    @delete
    func delete(&mut self) {
        if(self.ptr == null) {
            return
        }
        unsafe {
            self.ptr.weak = self.ptr.weak - 1
            if(self.ptr.weak == 0 && self.ptr.strong == 0) {
                free(self.ptr as *mut void)
            }
        }
        self.ptr = null
    }

    public func upgrade(&self) : Option<Rc<T>> {
        if(self.ptr == null) {
            return Option.None<Rc<T>>()
        }
        var inner = self.ptr
        if(unsafe(inner.strong) == 0) {
            return Option.None<Rc<T>>()
        }
        unsafe {
            inner.strong = inner.strong + 1
        }
        return Option.Some<Rc<T>>(Rc<T> {
            ptr : inner
        })
    }

    public func strong_count(&self) : size_t {
        return unsafe(self.ptr.strong)
    }

    public func weak_count(&self) : size_t {
        return unsafe(self.ptr.weak)
    }
}

public func <T> rc(value : T) : Rc<T> {
    var inner = new RcInner<T> {
        strong : 1 as size_t,
        weak : 0 as size_t,
        value : manually_drop(value)
    }
    return Rc<T> {
        ptr : inner
    }
}

}
