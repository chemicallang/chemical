// Windows CRT equivalents of POSIX unistd.h functions.
// On Windows the CRT names are _read/_write/_close (declared in <io.h>).
// These wrappers match the POSIX signatures so cross-platform code compiles.

@extern public func _read(fd : int, buf : *mut void, count : uint) : int
@extern public func _write(fd : int, buf : *void, count : uint) : int
@extern public func _close(fd : int) : int
@extern public func _open(path : *char, flags : int, mode : int) : int
@extern public func _lseek(fd : int, offset : longlong, whence : int) : longlong
@extern public func _get_osfhandle(fd : int) : intptr_t

public unsafe func read(fd : int, buf : *mut void, count : ulong) : long {
    return unsafe(_read(fd, buf, count as uint)) as long
}

public unsafe func write(fd : int, buf : *void, count : ulong) : long {
    return unsafe(_write(fd, buf, count as uint)) as long
}

public func close(fd : int) : int {
    return unsafe(_close(fd))
}

// `open`/`lseek` keep the POSIX names so portable code (`http`'s `send_file`,
// which opens a file and seeks it) compiles unchanged on Windows.
public func open(path : *char, flags : int, mode : int) : int {
    return unsafe(_open(path, flags, mode))
}

public func lseek(fd : int, offset : longlong, whence : int) : longlong {
    return unsafe(_lseek(fd, offset, whence))
}

// <fcntl.h> open modes (CRT values). `SEEK_SET`/`SEEK_CUR`/`SEEK_END` live in
// `cstd/src/io/stdio.ch`, which every platform shares.
public comptime const O_RDONLY = 0x0000
public comptime const O_WRONLY = 0x0001
public comptime const O_RDWR = 0x0002
public comptime const O_CREAT = 0x0100
public comptime const O_TRUNC = 0x0200
public comptime const O_APPEND = 0x0008
