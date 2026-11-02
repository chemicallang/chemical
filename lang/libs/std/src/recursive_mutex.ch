public namespace std {
    public struct recursive_lock_guard {
        var m : *mut recursive_mutex

        @constructor
        func constructor(mtx : &mut recursive_mutex) {
            mtx.lock()
            return recursive_lock_guard {
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
    impl !Send for recursive_lock_guard { }
    impl !Sync for recursive_lock_guard { }
}
