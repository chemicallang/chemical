// Shared helper for library tests (lang/tests/src/libs/*).
//
// Builds a writable path inside the OS temp directory so tests are
// cross-platform. The lib tests used to hardcode "/tmp/..." which works
// on Linux but not on Windows: a native Windows process resolves
// "/tmp/..." to "\tmp\..." on the current drive, which usually does not
// exist, so save operations fail. fs::temp_dir() returns the user's temp
// directory on Windows (%TEMP%) and "/tmp" on POSIX.

func make_temp_test_path(name : *char) : std::string {
    var out = std::string()
    var buf : [4096]char
    var r = fs::temp_dir(&raw mut buf[0], 4096 as size_t)
    if(r is std::Result.Ok) {
        var Ok(len) = r else unreachable
        var i : size_t = 0
        while(i < len) {
            out.append(buf[i])
            i += 1
        }
    }
    // ensure a trailing separator before appending the file name
    // (POSIX returns "/tmp" without one, Windows returns "%TEMP%\" with one)
    if(out.size() > 0) {
        var last = out.get(out.size() - 1)
        if(last != '/' && last != '\\') {
            out.append('/')
        }
    }
    out.append_char_ptr(name)
    return out
}
