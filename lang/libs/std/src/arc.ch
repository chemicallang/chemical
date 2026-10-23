// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * A mutex-protected `size_t` used by `Arc` for its strong and weak counts.
 * Keeping it monomorphic avoids generic-field method inference inside the
 * generic `ArcInner<T>`; the public API stays identical to an atomic counter
 * and can be swapped later.
 */
@direct_init
public struct ArcCount {

    var m : mutex
    var value : size_t

    @constructor
    func make(value : size_t) {
        return ArcCount {
            m : mutex(),
            value : value
        }
    }

    func increment(&self) {
        var mp = unsafe(&raw mut self.m)
        unsafe {
            mp.lock()
        }
        var vp = unsafe(&raw mut self.value)
        unsafe {
            *vp = *vp + 1
            mp.unlock()
        }
    }

    func decrement(&self) : size_t {
        var mp = unsafe(&raw mut self.m)
        unsafe {
            mp.lock()
        }
        var vp = unsafe(&raw mut self.value)
        var result : size_t = 0
        unsafe {
            *vp = *vp - 1
            result = *vp
            mp.unlock()
        }
        return result
    }

    func load(&self) : size_t {
        var mp = unsafe(&raw mut self.m)
        unsafe {
            mp.lock()
        }
        var result = self.value
        unsafe {
            mp.unlock()
        }
        return result
    }
}

/**
 * Shared heap block for `Arc<T>` / `ArcWeak<T>`. The counts are protected by
 * OS mutexes; the value is in `ManuallyDrop` so weak handles can outlive the
 * last strong owner.
 */
public struct ArcInner<T> {
    var strong : ArcCount
    var weak : ArcCount
    var value : ManuallyDrop<T>
}

/**
 * Thread-safe reference-counted ownership.
 *
 * `Arc<T>` is `Send` and `Sync` when `T: Send + Sync`. Counts are currently
 * mutex-protected rather than atomic; ownership semantics are the same, and
 * the count implementation can be replaced without changing callers.
 */
@direct_init
public struct Arc<T> {

    var ptr : *mut ArcInner<T>

    @delete
    func delete(&mut self) {
        if(self.ptr == null) {
            return
        }
        var strong_left = self.ptr.strong.decrement()
        if(strong_left == 0) {
            unsafe {
                self.ptr.value.destroy()
            }
            if(self.ptr.weak.load() == 0) {
                unsafe {
                    delete self.ptr
                }
            }
        }
        self.ptr = null
    }

    public func clone(&self) : Arc<T> {
        var inner = self.ptr
        inner.strong.increment()
        return Arc<T> {
            ptr : inner
        }
    }

    public func downgrade(&self) : ArcWeak<T> {
        var inner = self.ptr
        inner.weak.increment()
        return ArcWeak<T> {
            ptr : inner
        }
    }

    public func get(&self) : &T {
        return unsafe(&self.ptr.value.value)
    }

    public func strong_count(&self) : size_t {
        return self.ptr.strong.load()
    }

    public func weak_count(&self) : size_t {
        return self.ptr.weak.load()
    }
}

/**
 * Non-owning handle to an `Arc<T>` allocation. It is named `ArcWeak` because
 * the flat `std` namespace already has `Rc`'s `Weak<T>`.
 */
@direct_init
public struct ArcWeak<T> {

    var ptr : *mut ArcInner<T>

    @delete
    func delete(&mut self) {
        if(self.ptr == null) {
            return
        }
        var weak_left = self.ptr.weak.decrement()
        if(weak_left == 0) {
            if(self.ptr.strong.load() == 0) {
                unsafe {
                    delete self.ptr
                }
            }
        }
        self.ptr = null
    }

    public func upgrade(&self) : Option<Arc<T>> {
        if(self.ptr == null) {
            return Option.None<Arc<T>>()
        }
        var inner = self.ptr
        if(inner.strong.load() == 0) {
            return Option.None<Arc<T>>()
        }
        inner.strong.increment()
        return Option.Some<Arc<T>>(Arc<T> {
            ptr : inner
        })
    }

    public func strong_count(&self) : size_t {
        return self.ptr.strong.load()
    }

    public func weak_count(&self) : size_t {
        return self.ptr.weak.load()
    }
}

public func <T> arc(value : T) : Arc<T> {
    var inner = new ArcInner<T> {
        strong : ArcCount.make(1 as size_t),
        weak : ArcCount.make(0 as size_t),
        value : manually_drop(value)
    }
    return Arc<T> {
        ptr : inner
    }
}

unsafe impl<T> Send for Arc<T> where T: Send + Sync { }
unsafe impl<T> Sync for Arc<T> where T: Send + Sync { }

}
