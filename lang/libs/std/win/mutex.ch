public namespace std {

    @maxalign
    public struct mutex {

        internal var storage : [SRWLOCK_SIZE]u8;

        @constructor
        func constructor() {
            var m = mutex { storage : [] }
            unsafe { InitializeSRWLock(&raw mut m.storage[0]) }
            return m;
        }

        func lock(&mut self) {
            unsafe { AcquireSRWLockExclusive(&raw mut storage[0]) }
        }

        func try_lock(&mut self) : bool {
            var r = unsafe(TryAcquireSRWLockExclusive(&raw mut storage[0]))
            return r != 0
        }

        func unlock(&mut self) {
            unsafe { ReleaseSRWLockExclusive(&raw mut storage[0]) }
        }
    }

}
