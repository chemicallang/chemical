// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * Internal heap block shared by all clones of an `Rc<T>`. The `strong` count
 * is maintained by `Rc`; it is not atomic, which is why `Rc` is neither Send
 * nor Sync (matching the structural raw-pointer rule).
 */
public struct RcInner<T> {
    var strong : size_t
    var value : T
}

/**
 * Single-threaded reference-counted ownership.
 *
 * `Rc<T>` is intentionally neither `Send` nor `Sync`: the count is updated
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
                delete self.ptr
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

    public func get(&self) : &T {
        return unsafe(&self.ptr.value)
    }

    public func strong_count(&self) : size_t {
        return unsafe(self.ptr.strong)
    }

    /** Mutably access the value only when this is the sole owner. */
    public func get_mut(&mut self) : Option<&mut T> {
        if(self.strong_count() != 1) {
            return Option.None<&mut T>()
        }
        return Option.Some<&mut T>(unsafe(&mut self.ptr.value))
    }

    /** Move the value out only when this is the sole owner. */
    public func try_unwrap(&mut self) : Option<T> {
        if(self.strong_count() != 1) {
            return Option.None<T>()
        }
        var out = zeroed:unsafe<T>()
        unsafe {
            memcpy(&raw mut out, &raw mut self.ptr.value, sizeof(T))
            free(self.ptr as *mut void)
        }
        self.ptr = null
        return Option.Some<T>(out)
    }
}

public func <T> rc(value : T) : Rc<T> {
    var inner = new RcInner<T> {
        strong : 1 as size_t,
        value : value
    }
    return Rc<T> {
        ptr : inner
    }
}

}
