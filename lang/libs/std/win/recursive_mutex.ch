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

    @align(8)
    public struct recursive_mutex {
        var storage : [CRITICAL_SECTION_SIZE]u8

        @constructor
        func constructor() {
            var m = recursive_mutex { storage : [] }
            InitializeCriticalSection(&raw mut m.storage[0])
            return m
        }

        func lock(&mut self) {
            EnterCriticalSection(&raw mut storage[0])
        }

        func try_lock(&mut self) : bool {
            var r = TryEnterCriticalSection(&raw mut storage[0])
            return r != 0
        }

        func unlock(&mut self) {
            LeaveCriticalSection(&raw mut storage[0])
        }

        @delete
        func delete(&mut self) {
            DeleteCriticalSection(&raw mut storage[0])
        }
    }
}
