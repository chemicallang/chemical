/**
 * Associated with the standard input stream, used for reading conventional input. At program startup, the stream is fully buffered if and only if the stream can be determined to not refer to an interactive device.
 * @see https://en.cppreference.com/w/c/io/std_streams
 */
@extern const stdin : *mut FILE;

/**
 * Associated with the standard output stream, used for writing conventional output. At program startup, the stream is fully buffered if and only if the stream can be determined to not refer to an interactive device.
 * @see https://en.cppreference.com/w/c/io/std_streams
 */
@extern const stdout : *mut FILE;

/**
 * Associated with the standard error stream, used for writing diagnostic output. At program startup, the stream is not fully buffered.
 * @see https://en.cppreference.com/w/c/io/std_streams
 */
@extern const stderr : *mut FILE;

/**
 * cross platform helper function
 */
public func get_stdin() : *mut FILE {
    return stdin;
}

/**
 * cross platform helper function
 */
public func get_stdout() : *mut FILE {
    return stdout;
}

/**
 * cross platform helper function
 */
public func get_stderr() : *mut FILE {
    return stderr;
}