/**
 * Freestanding stdio for wasm32.
 *
 * There is no host libc stream object. The runtime's `fwrite` ignores the
 * stream pointer and forwards bytes to the host `env.chc_log(ptr, len)` import,
 * so the sentinels only need to be distinct non-null values. `fwrite` in
 * runtime.c treats a null stream as the console.
 */

public var stdin : *mut FILE = null
public var stdout : *mut FILE = null
public var stderr : *mut FILE = null

/**
 * cross platform helper function
 */
public func get_stdin() : *mut FILE {
    return unsafe(stdin)
}

/**
 * cross platform helper function
 */
public func get_stdout() : *mut FILE {
    return unsafe(stdout)
}

/**
 * cross platform helper function
 */
public func get_stderr() : *mut FILE {
    return unsafe(stderr)
}
