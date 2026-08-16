@extern
public func pthread_mutexattr_init(attr : *mut u8) : int

@extern
public func pthread_mutexattr_settype(attr : *mut u8, type : int) : int

@extern
public func pthread_mutexattr_destroy(attr : *mut u8) : int

// pthread_mutex_* functions are declared in mutex.ch (same module)

public namespace std {
    comptime const PTHREAD_MUTEXATTR_T_SIZE = 4
    comptime const PTHREAD_MUTEX_RECURSIVE = 1

    @maxalign
    public struct recursive_mutex {
        var storage : [PTHREAD_MUTEX_T_SIZE]u8

        @constructor
        func constructor() {
            var m = recursive_mutex { storage : [] }
            var attr_storage : [PTHREAD_MUTEXATTR_T_SIZE]u8
            var rc = pthread_mutexattr_init(&raw mut attr_storage[0])
            if(rc != 0) {
                panic("pthread_mutexattr_init failed")
            }
            rc = pthread_mutexattr_settype(&raw mut attr_storage[0], PTHREAD_MUTEX_RECURSIVE)
            if(rc != 0) {
                panic("pthread_mutexattr_settype failed")
            }
            rc = pthread_mutex_init(&raw mut m.storage[0], &raw mut attr_storage[0])
            pthread_mutexattr_destroy(&raw mut attr_storage[0])
            if(rc != 0) {
                panic("pthread_mutex_init failed")
            }
            return m
        }

        func lock(&mut self) {
            var rc = pthread_mutex_lock(&raw mut storage[0])
            if(rc != 0) {
                panic("pthread_mutex_lock failed")
            }
        }

        func try_lock(&mut self) : bool {
            var rc = pthread_mutex_trylock(&raw mut storage[0])
            return rc == 0
        }

        func unlock(&mut self) {
            var rc = pthread_mutex_unlock(&raw mut storage[0])
            if(rc != 0) {
                panic("pthread_mutex_unlock failed")
            }
        }

        @delete
        func delete(&mut self) {
            pthread_mutex_destroy(&raw mut storage[0])
        }
    }
}
