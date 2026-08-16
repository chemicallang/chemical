/**
 * Linux exposes errno through the __errno_location() function (glibc/musl).
 */
@extern
public func __errno_location() : *mut int

public func get_errno() : int {
    return *__errno_location();
}

public func set_errno(value : int) {
    *__errno_location() = value;
}
