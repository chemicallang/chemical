/**
 * macOS (Darwin) exposes errno through the __error() function instead of
 * __errno_location(). Its exported symbol is ___error.
 */
@extern
public func __error() : *mut int

public func get_errno() : int {
    return *__error();
}

public func set_errno(value : int) {
    *__error() = value;
}
