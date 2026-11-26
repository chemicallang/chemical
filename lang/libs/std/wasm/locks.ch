// Freestanding wasm single-threaded lock stubs.
//
// A wasm module in this wave has one linear memory and no threads (SharedArrayBuffer
// / atomics are out of scope). Lock operations therefore cannot contend; they are
// deliberately no-ops with the same public API and the same pass-through
// synchronization semantics a single thread needs. This is not a fake OS thread
// implementation: code that actually needs concurrency must not be compiled for
// wasm in this wave (threadpool/process/fs/net are excluded from the safe subset).

public namespace std {

    public struct mutex {

        @constructor
        func constructor() {
            return mutex { }
        }

        func lock(&mut self) {
        }

        func try_lock(&mut self) : bool {
            return true
        }

        func unlock(&mut self) {
        }

        @delete
        func delete(&mut self) {
        }
    }

    @maxalign
    public struct recursive_mutex {

        @constructor
        func constructor() {
            return recursive_mutex { }
        }

        func lock(&mut self) {
        }

        func try_lock(&mut self) : bool {
            return true
        }

        func unlock(&mut self) {
        }

        @delete
        func delete(&mut self) {
        }
    }

    @maxalign
    public struct rwlock {

        @constructor
        func constructor() {
            return rwlock { }
        }

        func read_lock(&mut self) {
        }

        func try_read_lock(&mut self) : bool {
            return true
        }

        func write_lock(&mut self) {
        }

        func try_write_lock(&mut self) : bool {
            return true
        }

        func read_unlock(&mut self) {
        }

        func write_unlock(&mut self) {
        }

        @delete
        func delete(&mut self) {
        }
    }

    // A condition variable without threads can never be signalled by another
    // thread. `wait` returns immediately (timed_wait reports timeout) so a
    // single-threaded inspector cannot deadlock; the API stays usable for
    // code that must compile for every target.
    @maxalign
    public struct CondVar {

        @constructor
        func constructor() {
            return CondVar { }
        }

        func wait(&mut self, mutex : &mut std::mutex) {
        }

        func timed_wait(&mut self, mutex : &mut std::mutex, timeout_ms : ulong) : bool {
            return false
        }

        func notify_one(&mut self) {
        }

        func notify_all(&mut self) {
        }

        @delete
        func delete(&mut self) {
        }
    }

    public type condvar = CondVar

}
