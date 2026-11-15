// Panic policy: every panic aborts. There is no unwinding profile or
// catch_unwind/UnwindSafe surface; a panic in a worker thread or destructor
// terminates the process.
public struct Location {
    var filename : *char
    var line : uint
    var character : uint
}

@extern
// Compiler runtime hook: `panic(...)` and generated failure paths call this
// directly. It is an ABI symbol, not a source-level API, so it stays a plain
// declaration — marking it `unsafe func` would make ordinary `panic("x")`
// require an unsafe block through the expansion.
public func __chc_panic(message : *char, file : *char, line : uint, column : uint)

@retained
public unsafe func raw_panic(message : *char, location : Location) {
    unsafe { __chc_panic(message, location.filename, location.line, location.character) }
}

@expand
comptime func decode_location(loc : ubigint) : Location {
    return intrinsics::decode_location<Location>(loc) as Location
}

@expand
public comptime func panic(message : %runtime<*char>) {
    unsafe(raw_panic(message, decode_location(intrinsics::get_call_loc(999))))
}