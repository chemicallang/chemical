/* NULL-terminated array of "NAME=VALUE" environment variables.  */
@extern
public var __environ : **mut char;

if(def.gnu) {
    @extern
    public var environ : **mut char;
}

/**
 * cross platform helper function returning the process environment
 */
public func get_environ() : **mut char {
    return __environ
}
