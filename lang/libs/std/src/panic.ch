public struct Location {
    var filename : *char
    var line : uint
    var character : uint
}

@extern
public func __chc_panic(message : *char, file : *char, line : uint, column : uint)

@retained
public func raw_panic(message : *char, location : Location) {
    unsafe { __chc_panic(message, location.filename, location.line, location.character) }
}

@expand
comptime func decode_location(loc : ubigint) : Location {
    return intrinsics::decode_location<Location>(loc) as Location
}

@expand
public comptime func panic(message : %runtime<*char>) {
    raw_panic(message, decode_location(intrinsics::get_call_loc(999)))
}