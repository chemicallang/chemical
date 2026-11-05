/**
 * FreeBSD exposes errno through __error(), like Darwin.
 */
@extern
public func __error() : *mut int

public func get_errno() : int {
    return unsafe(*__error());
}

public func set_errno(value : int) {
    unsafe { *__error() = value; }
}
