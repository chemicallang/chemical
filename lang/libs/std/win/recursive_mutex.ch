@dllimport
@stdcall
@extern
public func InitializeCriticalSection(cs : *mut u8) : void

@dllimport
@stdcall
@extern
public func EnterCriticalSection(cs : *mut u8) : void

@dllimport
@stdcall
@extern
public func TryEnterCriticalSection(cs : *mut u8) : int

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

    @maxalign
    public struct recursive_mutex {
        internal var storage : [CRITICAL_SECTION_SIZE]u8

        @constructor
        func constructor() {
            var m = recursive_mutex { storage : [] }
            unsafe { InitializeCriticalSection(&raw mut m.storage[0]) }
            return m
        }

        func lock(&mut self) {
            unsafe { EnterCriticalSection(&raw mut storage[0]) }
        }

        func try_lock(&mut self) : bool {
            var r = unsafe(TryEnterCriticalSection(&raw mut storage[0]))
            return r != 0
        }

        func unlock(&mut self) {
            unsafe { LeaveCriticalSection(&raw mut storage[0]) }
        }

        @delete
        func delete(&mut self) {
            unsafe { DeleteCriticalSection(&raw mut storage[0]) }
        }
    }
}
