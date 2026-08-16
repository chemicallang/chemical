/**
 * macOS (Darwin) exposes the environment through _NSGetEnviron() (symbol
 * __NSGetEnviron). There is no exported __environ symbol.
 */
@extern
public func _NSGetEnviron() : ***mut char

/**
 * cross platform helper function returning the process environment
 */
public func get_environ() : **mut char {
    return *_NSGetEnviron()
}
