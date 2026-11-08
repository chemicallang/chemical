// Safe helpers for the test-report surface.
public namespace test_report_safe {

public func log_type_name_view(t : LogType) : std::string_view {
    var p = log_type_name(t)
    if(p == null) { return std::string_view() }
    return unsafe(std::string_view(p))
}

public func argv_has_flag_str(argv : **char, argc : int, flag : &str) : bool {
    if(argv == null || argc <= 0) { return false }
    var made = std::cstring_make(flag)
    if(made is std::Result.Err) { return false }
    var Ok(c) = made else unsafe { unreachable }
    return argv_has_flag(argv, argc, unsafe(c.as_ptr()))
}

}
