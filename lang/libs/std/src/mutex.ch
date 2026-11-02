public namespace std {
    // Convenience RAII locker
    public struct lock_guard {
        var m : *mut mutex
        @constructor
        func constructor(mtx : &mut mutex) {
            mtx.lock()
            return lock_guard {
                m : &raw mut mtx
            }
        }
        @delete
        func delete(&mut self) {
        unsafe {
                m.unlock()
        
        }
    }
    }

    // A guard unlocks the mutex in its destructor and must stay on the locking
    // thread. Keep the negative impls explicit so a future field representation
    // cannot silently make it transferable.
    impl !Send for lock_guard { }
    impl !Sync for lock_guard { }
}
