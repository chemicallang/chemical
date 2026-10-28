@dllimport
@stdcall
@extern
public func InitializeSRWLock(srw : *mut u8) : void

@dllimport
@stdcall
@extern
public func AcquireSRWLockShared(srw : *mut u8) : void

@dllimport
@stdcall
@extern
public func TryAcquireSRWLockShared(srw : *mut u8) : u8

@dllimport
@stdcall
@extern
public func ReleaseSRWLockShared(srw : *mut u8) : void

@dllimport
@stdcall
@extern
public func AcquireSRWLockExclusive(srw : *mut u8) : void

@dllimport
@stdcall
@extern
public func TryAcquireSRWLockExclusive(srw : *mut u8) : u8

@dllimport
@stdcall
@extern
public func ReleaseSRWLockExclusive(srw : *mut u8) : void

public namespace std {

    comptime const SRWLOCK_SIZE = 8

    @maxalign
    public struct rwlock {

        internal var storage : [SRWLOCK_SIZE]u8

        @constructor
        func constructor() {
            var l = rwlock { storage : [] }
            InitializeSRWLock(&raw mut l.storage[0])
            return l
        }

        func read_lock(&mut self) {
            AcquireSRWLockShared(&raw mut storage[0])
        }

        func try_read_lock(&mut self) : bool {
            return TryAcquireSRWLockShared(&raw mut storage[0]) != 0
        }

        func write_lock(&mut self) {
            AcquireSRWLockExclusive(&raw mut storage[0])
        }

        func try_write_lock(&mut self) : bool {
            return TryAcquireSRWLockExclusive(&raw mut storage[0]) != 0
        }

        func read_unlock(&mut self) {
            ReleaseSRWLockShared(&raw mut storage[0])
        }

        func write_unlock(&mut self) {
            ReleaseSRWLockExclusive(&raw mut storage[0])
        }
    }
}
