// @dllimport
// @stdcall
// @extern
// public func InitializeConditionVariable(cond : *mut u8) : void
//
// @dllimport
// @stdcall
// @extern
// public func WakeConditionVariable(cond : *mut u8) : void
//
// @dllimport
// @stdcall
// @extern
// public func WakeAllConditionVariable(cond : *mut u8) : void
//
// // (SleepConditionVariableCS returns nonzero on success; 0 on timeout or failure)
// @dllimport
// @stdcall
// @extern
// public func SleepConditionVariableCS(cond : *mut u8, cs : *mut u8, ms : ulong) : int
//
// public namespace std {
//
//     comptime const CRITICAL_SECTION_SIZE = 40   // typical Win32 CRITICAL_SECTION (x64)
//     comptime const CONDITION_VARIABLE_SIZE = 8  // typical Win32 CONDITION_VARIABLE (small user object)
//
//     // Cross-platform CondVar (opaque storage)
//     public struct CondVar {
//
//         var storage : [CONDITION_VARIABLE_SIZE]u8
//
//         @constructor
//         func constructor() {
//             // InitializeConditionVariable does not fail and requires no destroy.
//             InitializeConditionVariable(&mut storage[0])
//         }
//
//         // wait (blocking). Caller must hold mutex before calling.
//         // mutex is your std::mutex (the one storing CRITICAL_SECTION or pthread_mutex_t bytes).
//         func wait(&mut self, mutex : &mut std::mutex) {
//             var ok = SleepConditionVariableCS(&mut storage[0], &mut mutex.storage[0], 0xFFFFFFFFu) // INFINITE
//             if(ok == 0) {
//                 // 0 -> timeout or failure, but since we passed INFINITE it means failure
//                 panic("SleepConditionVariableCS failed in wait")
//             }
//         }
//
//         // timed_wait: returns true if signalled, false if timed out.
//         // timeout_ms is relative timeout in milliseconds.
//         func timed_wait(&mut self, mutex : &mut std::mutex, timeout_ms : ulong) : bool {
//             // SleepConditionVariableCS returns nonzero on success (signalled), 0 on timeout/failure.
//             var ok = SleepConditionVariableCS(&mut storage[0], &mut mutex.storage[0], timeout_ms)
//             return ok != 0
//         }
//
//         func notify_one(&mut self) {
//             WakeConditionVariable(&mut storage[0])
//         }
//
//         func notify_all(&mut self) {
//             WakeAllConditionVariable(&mut storage[0])
//         }
//
//         // destructor: POSIX needs destroy, Windows does not.
//         @delete
//         func delete(&mut self) {
//             // must keep the destructor, don't know the consequences in common code
//         }
//
//     }
// }