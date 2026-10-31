// Copyright (c) Chemical Language Foundation 2026.

public namespace std {

/**
 * A value protected by an OS mutex. `lock` returns a guard that owns the
 * exclusive access and unlocks the mutex from its destructor; `try_lock`
 * returns `None` when another guard is live.
 *
 * The mutex itself is `Sync` when `T: Send` (and `Send` under the same bound,
 * because ownership of the value can move with the mutex). Guards are
 * intentionally not auto-trait safe: they carry raw pointers and must stay on
 * the locking thread.
 */
@direct_init
public struct Mutex<T> {

    internal var m : mutex
    internal var value : UnsafeCell<T>

    public func lock(&self) : MutexGuard<T> {
        var mp = unsafe(&raw mut self.m)
        unsafe {
            mp.lock()
        }
        return MutexGuard<T> {
            mtx : mp,
            value : self.value.get()
        }
    }

    public func try_lock(&self) : Option<MutexGuard<T>> {
        var mp = unsafe(&raw mut self.m)
        var got = unsafe(mp.try_lock())
        if(!got) {
            return Option.None<MutexGuard<T>>()
        }
        return Option.Some<MutexGuard<T>>(MutexGuard<T> {
            mtx : mp,
            value : self.value.get()
        })
    }

    public func get_mut(&mut self) : &mut T {
        return unsafe(&mut *self.value.get())
    }
}

public struct MutexGuard<T> {

    internal var mtx : *mut mutex
    internal var value : *mut T

    @delete
    func delete(&mut self) {
        unsafe {
            self.mtx.unlock()
        }
    }

    public func get(&mut self) : &mut T {
        return unsafe(&mut *self.value)
    }
}

// A guard may only be used on the thread that acquired the lock. The raw
// pointers already derive !Send/!Sync; keep the explicit negative impls so a
// future representation change cannot silently make guards transferable.
impl<T> !Send for MutexGuard<T> { }
impl<T> !Sync for MutexGuard<T> { }

public func <T> new_mutex(value : T) : Mutex<T> {
    return Mutex<T> {
        m : mutex(),
        value : unsafe_cell(value)
    }
}

unsafe impl<T> Send for Mutex<T> where T: Send { }
unsafe impl<T> Sync for Mutex<T> where T: Send { }

}
