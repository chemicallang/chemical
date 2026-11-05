/**
 * FreeBSD exports the process environment through the environ global.
 */
@extern
public var environ : **mut char;

public func get_environ() : **mut char {
    return unsafe(environ)
}
