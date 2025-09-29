public namespace std {
    // Convenience RAII locker
    public struct lock_guard {
        var m : *mut mutex
        @constructor
        func constructor(mtx : &mut mutex) {
            m = &mut mtx
            m.lock()
        }
        @delete
        func delete(&mut self) {
            m.unlock()
        }
    }
}