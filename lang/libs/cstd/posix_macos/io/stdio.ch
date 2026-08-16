/**
 * macOS (Darwin) does not export the stdin/stdout/stderr symbols directly.
 * stdin/stdout/stderr are macros for __stdinp/__stdoutp/__stderrp, and the
 * actual symbols exported by libSystem are ___stdinp/___stdoutp/___stderrp.
 */
@extern public var __stdinp : *mut FILE;
@extern public var __stdoutp : *mut FILE;
@extern public var __stderrp : *mut FILE;

/**
 * cross platform helper function
 */
public func get_stdin() : *mut FILE {
    return __stdinp;
}

/**
 * cross platform helper function
 */
public func get_stdout() : *mut FILE {
    return __stdoutp;
}

/**
 * cross platform helper function
 */
public func get_stderr() : *mut FILE {
    return __stderrp;
}

/* Close a stream opened by popen and return the status of its child.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
@extern
public func pclose(__stream : *mut FILE) : int

/* Create a new stream connected to a pipe running the given command.

   This function is a possible cancellation point and therefore not
   marked with __THROW.  */
@extern
public func popen(__command : *char, __modes : *char) : *mut FILE
