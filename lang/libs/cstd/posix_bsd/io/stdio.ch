/**
 * FreeBSD exposes the standard streams through the __stdinp/__stdoutp/
 * __stderrp globals rather than exporting stdin/stdout/stderr directly.
 */
@extern public var __stdinp : *mut FILE;
@extern public var __stdoutp : *mut FILE;
@extern public var __stderrp : *mut FILE;

public func get_stdin() : *mut FILE {
    return unsafe(__stdinp);
}

public func get_stdout() : *mut FILE {
    return unsafe(__stdoutp);
}

public func get_stderr() : *mut FILE {
    return unsafe(__stderrp);
}

@extern
public func pclose(__stream : *mut FILE) : int

@extern
public func popen(__command : *char, __modes : *char) : *mut FILE
