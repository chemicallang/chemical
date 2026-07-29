@dllimport
@stdcall
@extern
public func InitializeSRWLock(srw : *mut u8) : void

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

    public struct mutex {

        var storage : [SRWLOCK_SIZE]u8;

        @constructor
        func constructor() {
            var m = mutex { storage : [] }
            InitializeSRWLock(&raw mut m.storage[0])
            return m;
        }

        func lock(&mut self) {
            AcquireSRWLockExclusive(&raw mut storage[0])
        }

        func try_lock(&mut self) : bool {
            var r = TryAcquireSRWLockExclusive(&raw mut storage[0])
            return r != 0
        }

        func unlock(&mut self) {
            ReleaseSRWLockExclusive(&raw mut storage[0])
        }
    }

}
