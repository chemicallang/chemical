//
// 2) CRT functions to wrap a HANDLE → FILE*
//    These are all in old msvcrt.dll, so TCC will find them.
//
@extern public func   _open_osfhandle(h: intptr_t, flags: int) : int;
@extern public func   _fdopen(fd: int, mode: *char)   : *mut FILE;

// cache slots so we only do the syscall+CRT conversion once
var _in  : *mut FILE = null;
var _out : *mut FILE = null;
var _err : *mut FILE = null;

public func get_stdin() : *mut FILE {
    if (_in == null) {
        const h  = GetStdHandle(-10 as uint);
        const fd = _open_osfhandle(h as intptr_t, /*_O_RDONLY=*/0);
        _in    = _fdopen(fd, "r\0");
    }
    return _in;
}

public func get_stdout() : *mut FILE {
    if (_out == null) {
        const h  = GetStdHandle(-11 as uint);
        const fd = _open_osfhandle(h as intptr_t, /*_O_WRONLY=*/1);
        _out   = _fdopen(fd, "w\0");
    }
    return _out;
}

public func get_stderr() : *mut FILE {
    if (_err == null) {
        const h  = GetStdHandle(-12 as uint);
        const fd = _open_osfhandle(h as intptr_t, /*_O_WRONLY=*/1);
        _err   = _fdopen(fd, "w\0");
    }
    return _err;
}

// popen/pclose on Windows are named _popen/_pclose in the CRT.
// We expose the POSIX names for cross-platform parity (see posix/io/stdio.ch).
@extern public func _popen(__command : *char, __modes : *char) : *mut FILE;
@extern public func _pclose(__stream : *mut FILE) : int;

/* Create a new stream connected to a pipe running the given command. */
public func popen(__command : *char, __modes : *char) : *mut FILE {
    return _popen(__command, __modes);
}

/* Close a stream opened by popen and return the status of its child. */
public func pclose(__stream : *mut FILE) : int {
    return _pclose(__stream);
}