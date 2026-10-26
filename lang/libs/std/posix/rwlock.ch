@extern
public func pthread_rwlock_init(lock : *mut u8, attr : *void) : int

@extern
public func pthread_rwlock_rdlock(lock : *mut u8) : int

@extern
public func pthread_rwlock_tryrdlock(lock : *mut u8) : int

@extern
public func pthread_rwlock_wrlock(lock : *mut u8) : int

@extern
public func pthread_rwlock_trywrlock(lock : *mut u8) : int

@extern
public func pthread_rwlock_unlock(lock : *mut u8) : int

@extern
public func pthread_rwlock_destroy(lock : *mut u8) : int

public namespace std {

    // pthread_rwlock_t is 56 bytes on glibc x86_64/aarch64; keep headroom for
    // libc variants that pad it further.
    comptime const PTHREAD_RWLOCK_T_SIZE = 64

    @maxalign
    public struct rwlock {

        var storage : [PTHREAD_RWLOCK_T_SIZE]u8

        @constructor
        func constructor() {
            var l = rwlock { storage : [] }
            var res = pthread_rwlock_init(&raw mut l.storage[0], null)
            if(res != 0) {
                panic("pthread_rwlock_init failed")
            }
            return l
        }

        func read_lock(&mut self) {
            var res = pthread_rwlock_rdlock(&raw mut storage[0])
            if(res != 0) {
                panic("pthread_rwlock_rdlock failed")
            }
        }

        func try_read_lock(&mut self) : bool {
            return pthread_rwlock_tryrdlock(&raw mut storage[0]) == 0
        }

        func write_lock(&mut self) {
            var res = pthread_rwlock_wrlock(&raw mut storage[0])
            if(res != 0) {
                panic("pthread_rwlock_wrlock failed")
            }
        }

        func try_write_lock(&mut self) : bool {
            return pthread_rwlock_trywrlock(&raw mut storage[0]) == 0
        }

        func read_unlock(&mut self) {
            var res = pthread_rwlock_unlock(&raw mut storage[0])
            if(res != 0) {
                panic("pthread_rwlock_unlock failed")
            }
        }

        func write_unlock(&mut self) {
            read_unlock()
        }

        @delete
        func delete(&mut self) {
            // best-effort, as with std::mutex
            pthread_rwlock_destroy(&raw mut storage[0])
        }
    }
}
