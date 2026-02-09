@extern
public func pthread_cond_init(cond : *mut u8, attr : *void) : int

@extern
public func pthread_cond_destroy(cond : *mut u8) : int

@extern
public func pthread_cond_wait(cond : *mut u8, mutex : *mut u8) : int

@extern
public func pthread_cond_timedwait(cond : *mut u8, mutex : *mut u8, abstime : *timespec) : int

@extern
public func pthread_cond_signal(cond : *mut u8) : int

@extern
public func pthread_cond_broadcast(cond : *mut u8) : int

@extern
public func clock_gettime(clk_id : int, ts : *mut timespec) : int

public namespace std {

    // conservative sizes (x86_64 glibc / typical Windows values)
    comptime const PTHREAD_MUTEX_T_SIZE = 40    // common glibc x86_64
    comptime const PTHREAD_COND_T_SIZE  = 48    // common glibc x86_64

    // Helper: compute absolute timespec = now + timeout_ms (uses CLOCK_REALTIME)
    // CLOCK_REALTIME is 0 on Linux; if you prefer MONOTONIC you can change this and
    // ensure pthread_condattr_setclock used at cond init.
    comptime const CLOCK_REALTIME = 0

    func compute_abstime_ms(out : *mut timespec, timeout_ms : ulong) {
        var now : timespec
        var rc = clock_gettime(CLOCK_REALTIME, &mut now)
        if(rc != 0) {
            panic("clock_gettime failed")
        }
        var add_s : i64 = (timeout_ms / 1000) as i64
        var add_ns : i64 = ((timeout_ms % 1000) * 1000000) as i64
        out.tv_sec = now.tv_sec + add_s
        out.tv_nsec = now.tv_nsec + add_ns
        if(out.tv_nsec >= 1000000000) {
            out.tv_sec = out.tv_sec + 1
            out.tv_nsec = out.tv_nsec - 1000000000
        }
    }

    // Cross-platform CondVar (opaque storage)
    public struct CondVar {

        var storage : [PTHREAD_COND_T_SIZE]u8;

        @constructor
        func constructor() {
            var c = CondVar { storage : [] }
            var res = pthread_cond_init(&mut c.storage[0], null)
            if(res != 0) {
                panic("pthread_cond_init failed")
            }
            return c;
        }

        // wait (blocking). Caller must hold mutex before calling.
        // mutex is your std::mutex (the one storing CRITICAL_SECTION or pthread_mutex_t bytes).
        func wait(&mut self, mutex : &mut std::mutex) {
            var r = pthread_cond_wait(&mut storage[0], &mut mutex.storage[0])
            if(r != 0) {
                panic("pthread_cond_wait failed")
            }
        }

        // timed_wait: returns true if signalled, false if timed out.
        // timeout_ms is relative timeout in milliseconds.
        func timed_wait(&mut self, mutex : &mut std::mutex, timeout_ms : ulong) : bool {
            var ts : timespec
            compute_abstime_ms(&mut ts, timeout_ms)
            var r = pthread_cond_timedwait(&mut storage[0], &mut mutex.storage[0], &ts)
            // pthread_cond_timedwait returns 0 on success, ETIMEDOUT on timeout
            return r == 0
        }

        func notify_one(&mut self) {
            var r = pthread_cond_signal(&mut storage[0])
            if(r != 0) {
                panic("pthread_cond_signal failed")
            }
        }

        func notify_all(&mut self) {
            var r = pthread_cond_broadcast(&mut storage[0])
            if(r != 0) {
                panic("pthread_cond_broadcast failed")
            }
        }

        // destructor: POSIX needs destroy, Windows does not.
        @delete
        func delete(&mut self) {
            var r = pthread_cond_destroy(&mut storage[0])
            // best-effort, do not panic in destructor
        }
    }

    public type condvar = CondVar

}