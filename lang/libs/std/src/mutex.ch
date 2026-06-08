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
            m.unlock()
        }
    }
}