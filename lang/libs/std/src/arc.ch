// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * Shared heap block for `Arc<T>` / `ArcWeak<T>`.
 *
 * A single mutex protects both reference counts. Two independent count locks
 * are not enough: the last strong owner and the last weak owner can otherwise
 * both observe the other count as zero and free the block twice. `upgrade`
 * also has to test-and-increment the strong count under the same lock, or a
 * concurrent last-strong drop can destroy the value between the test and the
 * increment.
 *
 * The value is in `ManuallyDrop` so weak handles can outlive the last strong
 * owner.
 */
public struct ArcInner<T> {
    var lock : mutex
    var strong : size_t
    var weak : size_t
    var value : ManuallyDrop<T>
    // 0 = value alive, 1 = destruction in progress, 2 = destroyed or moved.
    // The value destructor must run outside the count lock so a destructor
    // that touches its own weak/strong counts cannot deadlock the allocation.
    var value_state : int
}

/**
 * Thread-safe reference-counted ownership.
 *
 * `Arc<T>` is `Send` and `Sync` when `T: Send + Sync`. Counts are
 * mutex-protected; ownership semantics are the same as an atomic
 * implementation and the count representation can change without affecting
 * callers.
 */
@direct_init
public struct Arc<T> {

    // The allocation and its counts are the invariant; safe code must not be
    // able to retarget or corrupt them.
    private var ptr : *mut ArcInner<T>

    @delete
    func delete(&mut self) {
        if(self.ptr == null) {
            return
        }
        var inner = self.ptr
        var drop_value = false
        var free_now = false
        unsafe {
            var lp = &raw mut inner.lock
            lp.lock()
            inner.strong = inner.strong - 1
            if(inner.strong == 0 && inner.value_state == 0) {
                inner.value_state = 1
                drop_value = true
            }
            if(inner.strong == 0 && inner.weak == 0 && inner.value_state == 2) {
                free_now = true
            }
            lp.unlock()
        }
        if(drop_value) {
            unsafe {
                inner.value.destroy()
                var lp2 = &raw mut inner.lock
                lp2.lock()
                inner.value_state = 2
                if(inner.strong == 0 && inner.weak == 0) {
                    free_now = true
                }
                lp2.unlock()
            }
        }
        self.ptr = null
        if(free_now) {
            unsafe { delete inner }
        }
    }

    public func clone(&self) : Arc<T> {
        if(self.ptr == null) {
            panic("Arc is empty")
        }
        var inner = self.ptr
        unsafe {
            var lp = &raw mut inner.lock
            lp.lock()
            inner.strong = inner.strong + 1
            lp.unlock()
        }
        return Arc<T> {
            ptr : inner
        }
    }

    public func downgrade(&self) : ArcWeak<T> {
        if(self.ptr == null) {
            panic("Arc is empty")
        }
        var inner = self.ptr
        unsafe {
            var lp = &raw mut inner.lock
            lp.lock()
            inner.weak = inner.weak + 1
            lp.unlock()
        }
        return unsafe(arc_weak_unchecked<T>(inner))
    }

    public func get(&self) : &T {
        if(self.ptr == null) {
            panic("Arc is empty")
        }
        return unsafe(&self.ptr.value.value)
    }

    public func strong_count(&self) : size_t {
        if(self.ptr == null) {
            panic("Arc is empty")
        }
        var result : size_t = 0
        unsafe {
            var lp = &raw mut self.ptr.lock
            lp.lock()
            result = self.ptr.strong
            lp.unlock()
        }
        return result
    }

    public func weak_count(&self) : size_t {
        if(self.ptr == null) {
            panic("Arc is empty")
        }
        var result : size_t = 0
        unsafe {
            var lp = &raw mut self.ptr.lock
            lp.lock()
            result = self.ptr.weak
            lp.unlock()
        }
        return result
    }

    /** Mutably access the value only when this is the sole strong owner. */
    public func get_mut(&mut self) : Option<&mut T> {
        if(self.ptr == null) {
            panic("Arc is empty")
        }
        var allowed = false
        unsafe {
            var lp = &raw mut self.ptr.lock
            lp.lock()
            allowed = self.ptr.strong == 1 && self.ptr.weak == 0 && self.ptr.value_state == 0
            lp.unlock()
        }
        if(!allowed) {
            return Option.None<&mut T>()
        }
        return Option.Some<&mut T>(unsafe(&mut self.ptr.value.value))
    }

    /** Move the value out only when this is the sole strong owner. */
    public func try_unwrap(&mut self) : Option<T> {
        if(self.ptr == null) {
            panic("Arc is empty")
        }
        unsafe {
            var lp = &raw mut self.ptr.lock
            lp.lock()
            if(self.ptr.strong != 1 || self.ptr.value_state != 0) {
                lp.unlock()
                return Option.None<T>()
            }
            var out = self.ptr.value.take()
            self.ptr.strong = 0
            self.ptr.value_state = 2
            var free_now = self.ptr.weak == 0
            var inner = self.ptr
            lp.unlock()
            self.ptr = null
            if(free_now) {
                delete inner
            }
            return Option.Some<T>(out)
        }
    }
}

/**
 * Non-owning handle to an `Arc<T>` allocation. It is named `ArcWeak` because
 * the flat `std` namespace already has `Rc`'s `Weak<T>`.
 *
 * Counts and upgrades are serialized by the shared `ArcInner` lock, so a weak
 * handle may cross threads (with the same `T: Send + Sync` bound as `Arc`).
 */
@direct_init
public struct ArcWeak<T> {

    private var ptr : *mut ArcInner<T>

    @delete
    func delete(&mut self) {
        if(self.ptr == null) {
            return
        }
        var inner = self.ptr
        var free_now = false
        unsafe {
            var lp = &raw mut inner.lock
            lp.lock()
            inner.weak = inner.weak - 1
            if(inner.weak == 0 && inner.strong == 0 && inner.value_state == 2) {
                free_now = true
            }
            lp.unlock()
        }
        self.ptr = null
        if(free_now) {
            unsafe { delete inner }
        }
    }

    public func upgrade(&self) : Option<Arc<T>> {
        if(self.ptr == null) {
            return Option.None<Arc<T>>()
        }
        var inner = self.ptr
        var alive = false
        unsafe {
            var lp = &raw mut inner.lock
            lp.lock()
            alive = inner.strong != 0
            if(alive) {
                inner.strong = inner.strong + 1
            }
            lp.unlock()
        }
        if(!alive) {
            return Option.None<Arc<T>>()
        }
        return Option.Some<Arc<T>>(unsafe(arc_unchecked<T>(inner)))
    }

    public func strong_count(&self) : size_t {
        if(self.ptr == null) {
            panic("ArcWeak is empty")
        }
        var result : size_t = 0
        unsafe {
            var lp = &raw mut self.ptr.lock
            lp.lock()
            result = self.ptr.strong
            lp.unlock()
        }
        return result
    }

    public func weak_count(&self) : size_t {
        if(self.ptr == null) {
            panic("ArcWeak is empty")
        }
        var result : size_t = 0
        unsafe {
            var lp = &raw mut self.ptr.lock
            lp.lock()
            result = self.ptr.weak
            lp.unlock()
        }
        return result
    }
}

public func <T> arc(value : T) : Arc<T> {
    var inner = new ArcInner<T> {
        lock : mutex(),
        strong : 1 as size_t,
        weak : 0 as size_t,
        value : manually_drop(value),
        value_state : 0
    }
    return unsafe(arc_unchecked<T>(inner))
}

public unsafe func <T> arc_unchecked(inner : *mut ArcInner<T>) : Arc<T> {
    return Arc<T> {
        ptr : inner
    }
}

public unsafe func <T> arc_weak_unchecked(inner : *mut ArcInner<T>) : ArcWeak<T> {
    return ArcWeak<T> {
        ptr : inner
    }
}

unsafe impl<T> Send for Arc<T> where T: Send + Sync { }
unsafe impl<T> Sync for Arc<T> where T: Send + Sync { }

unsafe impl<T> Send for ArcWeak<T> where T: Send + Sync { }
unsafe impl<T> Sync for ArcWeak<T> where T: Send + Sync { }

}
