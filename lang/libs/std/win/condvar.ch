@dllimport
@stdcall
@extern
public func InitializeConditionVariable(cond : *mut u8) : void

@dllimport
@stdcall
@extern
public func WakeConditionVariable(cond : *mut u8) : void

@dllimport
@stdcall
@extern
public func WakeAllConditionVariable(cond : *mut u8) : void

// (SleepConditionVariableSRW returns nonzero on success; 0 on timeout or failure)
@dllimport
@stdcall
@extern
public func SleepConditionVariableSRW(cond : *mut u8, srw : *mut u8, ms : ulong, flags : ulong) : int

public namespace std {

    comptime const CONDITION_VARIABLE_SIZE = 8  // typical Win32 CONDITION_VARIABLE (small user object)

    // Cross-platform CondVar (opaque storage)
    @align(8)
    public struct CondVar {

        var storage : [CONDITION_VARIABLE_SIZE]u8

        @constructor
        func constructor() {
            var c = CondVar { storage : [] }
            // InitializeConditionVariable does not fail and requires no destroy.
            InitializeConditionVariable(&raw mut c.storage[0])
            return c;
        }

        // wait (blocking). Caller must hold mutex before calling.
        func wait(&mut self, mutex : &mut std::mutex) {
            var ok = SleepConditionVariableSRW(&raw mut storage[0], &raw mut mutex.storage[0], 0xFFFFFFFFu, 0u) // INFINITE
            if(ok == 0) {
                panic("SleepConditionVariableSRW failed in wait")
            }
        }

        // timed_wait: returns true if signalled, false if timed out.
        // timeout_ms is relative timeout in milliseconds.
        func timed_wait(&mut self, mutex : &mut std::mutex, timeout_ms : ulong) : bool {
            var ok = SleepConditionVariableSRW(&raw mut storage[0], &raw mut mutex.storage[0], timeout_ms, 0u)
            return ok != 0
        }

        func notify_one(&mut self) {
            WakeConditionVariable(&raw mut storage[0])
        }

        func signal(&mut self) {
            notify_one()
        }

        func notify_all(&mut self) {
            WakeAllConditionVariable(&raw mut storage[0])
        }

        // destructor: Windows CONDITION_VARIABLE does not need explicit cleanup.
        @delete
        func delete(&mut self) {
            // must keep the destructor, don't know the consequences in common code
        }

    }

    public type condvar = CondVar

}