// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * A reader-writer lock. Any number of readers may hold the lock at once;
 * a writer is exclusive. Guards release the lock from their destructor, and
 * `try_read`/`try_write` return `None` instead of blocking.
 *
 * `RwLock<T>` is `Send` when `T: Send` and `Sync` when `T: Send + Sync`,
 * matching the protected value's requirements.
 */
@direct_init
public struct RwLock<T> {

    internal var lock : rwlock
    internal var value : UnsafeCell<T>

    public func read(&self) : RwLockReadGuard<T> {
        var lp = unsafe(&raw mut self.lock)
        unsafe {
            lp.read_lock()
        }
        return RwLockReadGuard<T> {
            lock : lp,
            value : self.value.get()
        }
    }

    public func try_read(&self) : Option<RwLockReadGuard<T>> {
        var lp = unsafe(&raw mut self.lock)
        var got = unsafe(lp.try_read_lock())
        if(!got) {
            return Option.None<RwLockReadGuard<T>>()
        }
        return Option.Some<RwLockReadGuard<T>>(RwLockReadGuard<T> {
            lock : lp,
            value : self.value.get()
        })
    }

    public func write(&self) : RwLockWriteGuard<T> {
        var lp = unsafe(&raw mut self.lock)
        unsafe {
            lp.write_lock()
        }
        return RwLockWriteGuard<T> {
            lock : lp,
            value : self.value.get()
        }
    }

    public func try_write(&self) : Option<RwLockWriteGuard<T>> {
        var lp = unsafe(&raw mut self.lock)
        var got = unsafe(lp.try_write_lock())
        if(!got) {
            return Option.None<RwLockWriteGuard<T>>()
        }
        return Option.Some<RwLockWriteGuard<T>>(RwLockWriteGuard<T> {
            lock : lp,
            value : self.value.get()
        })
    }

    public func get_mut(&mut self) : &mut T {
        return unsafe(&mut *self.value.get())
    }
}

public struct RwLockReadGuard<T> {

    internal var lock : *mut rwlock
    internal var value : *mut T

    @delete
    func delete(&mut self) {
        unsafe {
            self.lock.read_unlock()
        }
    }

    public func get(&self) : &T {
        return unsafe(&*self.value)
    }
}

public struct RwLockWriteGuard<T> {

    internal var lock : *mut rwlock
    internal var value : *mut T

    @delete
    func delete(&mut self) {
        unsafe {
            self.lock.write_unlock()
        }
    }

    public func get(&mut self) : &mut T {
        return unsafe(&mut *self.value)
    }
}

public func <T> new_rwlock(value : T) : RwLock<T> {
    return RwLock<T> {
        lock : rwlock(),
        value : unsafe_cell(value)
    }
}

unsafe impl<T> Send for RwLock<T> where T: Send { }
unsafe impl<T> Sync for RwLock<T> where T: Send + Sync { }

}
