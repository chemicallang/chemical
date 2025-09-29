@dllimport
@stdcall
@extern
public func InitializeCriticalSectionAndSpinCount(cs : *mut u8, spin : ulong) : void

@dllimport
@stdcall
@extern
public func EnterCriticalSection(cs : *mut u8) : void

@dllimport
@stdcall
@extern
public func TryEnterCriticalSection(cs : *mut u8) : int // nonzero on success

@dllimport
@stdcall
@extern
public func LeaveCriticalSection(cs : *mut u8) : void

@dllimport
@stdcall
@extern
public func DeleteCriticalSection(cs : *mut u8) : void

public namespace std {

    comptime const CRITICAL_SECTION_SIZE = 40

    // ----- std::mutex implementation -----
    public struct mutex {

        var storage : [CRITICAL_SECTION_SIZE]u8;

        // constructor: initialize native mutex
        @constructor
        func constructor() {
            // use a reasonable spin count (e.g., 4000)
            InitializeCriticalSectionAndSpinCount(&mut storage[0], 4000u)
        }

        // lock: blocking
        func lock(&mut self) {
            EnterCriticalSection(&mut storage[0])
        }

        // try_lock: non-blocking; returns true on success, false otherwise
        func try_lock(&mut self) : bool {
            // TryEnterCriticalSection returns non-zero on success
            var r = TryEnterCriticalSection(&mut storage[0])
            return r != 0
        }

        // unlock: release
        func unlock(&mut self) {
            LeaveCriticalSection(&mut storage[0])
        }

        // destructor: destroy native mutex
        @delete
        func delete(&mut self) {
            DeleteCriticalSection(&mut storage[0])
        }
    }

}
