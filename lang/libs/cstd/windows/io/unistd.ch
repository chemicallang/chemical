// Windows CRT equivalents of POSIX unistd.h functions.
// On Windows the CRT names are _read/_write/_close (declared in <io.h>).
// These wrappers match the POSIX signatures so cross-platform code compiles.

@extern public func _read(fd : int, buf : *mut void, count : uint) : int
@extern public func _write(fd : int, buf : *void, count : uint) : int
@extern public func _close(fd : int) : int
@extern public func _get_osfhandle(fd : int) : intptr_t

public func read(fd : int, buf : *mut void, count : ulong) : long {
    return _read(fd, buf, count as uint) as long
}

public func write(fd : int, buf : *void, count : ulong) : long {
    return _write(fd, buf, count as uint) as long
}

public func close(fd : int) : int {
    return _close(fd)
}
