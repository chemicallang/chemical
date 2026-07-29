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
            m.unlock()
        }
    }
}
