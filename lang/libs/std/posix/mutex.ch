@extern
public func pthread_mutex_init(m : *mut u8, attr : *void) : int

@extern
public func pthread_mutex_lock(m : *mut u8) : int

@extern
public func pthread_mutex_trylock(m : *mut u8) : int

@extern
public func pthread_mutex_unlock(m : *mut u8) : int

@extern
public func pthread_mutex_destroy(m : *mut u8) : int

public namespace std {

    // Opaque pthread types as bytes (we don't inspect internals)
    // size chosen conservatively; if your platform requires a different size adjust it.
    comptime const PTHREAD_MUTEX_T_SIZE = 40  // common Linux x86_64 size; adjust if needed

    // ----- std::mutex implementation -----
    public struct mutex {

        var storage : [PTHREAD_MUTEX_T_SIZE]u8;

        // constructor: initialize native mutex
        @constructor
        func constructor() {
            // pthread_mutex_init expects pointer to pthread_mutex_t; we pass raw bytes pointer
            // NULL attr -> default attributes
            var res = pthread_mutex_init(&mut storage[0], null)
            // If initialization fails, we abort (you can change behavior to return an error)
            if(res != 0) {
                panic("pthread_mutex_init failed")
            }
        }

        // lock: blocking
        func lock(&mut self) {
            var res = pthread_mutex_lock(&mut storage[0])
            if(res != 0) {
                panic("pthread_mutex_lock failed")
            }
        }

        // try_lock: non-blocking; returns true on success, false otherwise
        func try_lock(&mut self) : bool {
            var res = pthread_mutex_trylock(&mut storage[0])
            if(res == 0) {
                return true
            } else {
                // EBUSY (on success we returned 0). Any other error we treat as failure (false),
                // caller can decide what to do. Could also panic on real error codes.
                return false
            }
        }

        // unlock: release
        func unlock(&mut self) {
            var res = pthread_mutex_unlock(&mut storage[0])
            if(res != 0) {
                panic("pthread_mutex_unlock failed")
            }
        }

        // destructor: destroy native mutex
        @delete
        func delete(&mut self) {
            var res = pthread_mutex_destroy(&mut storage[0])
            if(res != 0) {
                // best-effort; don't panic in destructor since it may be called during unwinding
            }
        }
    }

}
