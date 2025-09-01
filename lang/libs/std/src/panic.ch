public struct Location {
    var filename : *char
    var line : uint
    var character : uint
}

public func raw_panic(message : *char, location : Location) {
    printf("panic with message '%s' at '%s:%d:%d'\n", message, location.filename, location.line, location.character);
    abort()
}

comptime func decode_location(loc : ubigint) : Location {
    return intrinsics::decode_location<Location>(loc) as Location
}

public comptime func panic(message : *char) {
    intrinsics::destruct_call_site()
    var loc = intrinsics::get_call_loc(999)
    var decoded = decode_location(loc)
    return (intrinsics::wrap(raw_panic(message, decoded)) as void);
}