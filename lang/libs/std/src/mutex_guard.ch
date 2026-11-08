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

    // Atomically release the guard's native mutex and block on `cv`. When this
    // returns, the same guard still holds the lock.
    public func wait(&mut self, cv : &Condvar) {
        unsafe {
            var cvp = &raw mut cv.cv
            cvp.wait(&mut *self.mtx)
        }
    }

    /** Returns `true` when notified, `false` when the timeout elapsed. */
    public func wait_timeout(&mut self, cv : &Condvar, timeout_ms : ulong) : bool {
        unsafe {
            var cvp = &raw mut cv.cv
            return cvp.timed_wait(&mut *self.mtx, timeout_ms)
        }
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

/**
 * A condition variable paired with `Mutex<T>`.
 *
 * `MutexGuard::wait(&cv)` atomically releases the guard's native mutex and
 * blocks; when it returns, the mutex is locked again and the same guard stays
 * valid. `notify_one`/`notify_all` take `&self` because the underlying
 * pthread/Windows primitive is stateless from the caller's point of view.
 * `Condvar` is `Send + Sync`; `MutexGuard` remains `!Send`.
 *
 * ```chemical
 * struct Signal { var m : Mutex<int>; var cv : Condvar }
 * var sig = std::arc(Signal { m : std::new_mutex(0), cv : std::new_condvar() })
 * var guard = sig.get().m.lock()
 * while(*guard.get() == 0) { guard.wait(&sig.get().cv) }
 * ```
 */
@direct_init
public struct Condvar {

    internal var cv : CondVar

    public func notify_one(&self) {
        unsafe {
            var cvp = &raw mut self.cv
            cvp.notify_one()
        }
    }

    public func notify_all(&self) {
        unsafe {
            var cvp = &raw mut self.cv
            cvp.notify_all()
        }
    }
}

public func new_condvar() : Condvar {
    return Condvar {
        cv : CondVar()
    }
}

// Rust-shaped free helpers: `condvar_wait(&cv, &mut guard)`. A method on
// `MutexGuard<T>` (`guard.wait(&cv)`) also works and infers `T` from the
// receiver.
public func <T> condvar_wait(cv : &Condvar, guard : &mut MutexGuard<T>) {
    guard.wait(cv)
}

public func <T> condvar_wait_timeout(cv : &Condvar, guard : &mut MutexGuard<T>, timeout_ms : ulong) : bool {
    return guard.wait_timeout(cv, timeout_ms)
}

// The native condition variable is explicitly usable from any thread; only
// the guard passed to `wait` is tied to the locking thread.
unsafe impl Send for Condvar { }
unsafe impl Sync for Condvar { }

}
