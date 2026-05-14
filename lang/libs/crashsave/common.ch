@extern
public func backtrace(buffer : *mut *void, size : int) : int

@extern
public func backtrace_symbols(buffer : *mut *void, size : int) : *mut *mut char

/* get_stacktrace() captures the current call stack and returns it as a multi-line string.
   Frame 0 (this function) is omitted from the output.
   Each line is formatted as:  [N] binary(func+offset) [address]  */
public func get_stacktrace() : std::string {
    var bt : [128]*void;
    var n = backtrace(bt, 128);
    var result = std::string();

    if (n <= 1) {
        result.append_view(std::string_view.make_no_len("(empty stacktrace)"));
        return result;
    }

    var strings = backtrace_symbols(bt, n);
    if (!strings) {
        result.append_view(std::string_view.make_no_len("(no backtrace symbols)"));
        return result;
    }

    for (var i : int = 1; i < n; ++i) {
        var line : [1024]char;
        snprintf(line, sizeof(line), "  [%d] %s\n", i - 1, strings[i]);
        result.append_view(std::string_view.make_no_len(line));
    }

    return result;
}
