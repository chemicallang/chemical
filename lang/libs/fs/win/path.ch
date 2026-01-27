public namespace fs {

using std::Result;

func canonicalize(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    // Windows: GetFullPathNameW
    var wtmp : [WIN_MAX_PATH]u16;
    var r = utf8_to_utf16(path, &mut wtmp[0], WIN_MAX_PATH as size_t);
    if(r is Result.Err) {
        var Err(e) = r else unreachable
        return Result.Err(e)
    }
    var Ok(wlen) = r else unreachable
    var wout : [WIN_MAX_PATH]u16;
    // extern func GetFullPathNameW(lpFileName : *WCHAR, nBufferLength : DWORD, lpBuffer : *WCHAR, lpFilePart : *mut *WCHAR) : DWORD;
    var res = GetFullPathNameW((&mut wtmp[0]) as LPCWSTR, WIN_MAX_PATH as u32, (&mut wout[0]) as LPWSTR, null);
    if(res == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    var conv = utf16_to_utf8(&mut wout[0], out, out_len);
    if(conv is Result.Err) {
        var Err(e) = conv else unreachable
        return Result.Err(e)
    }
    var Ok(n) = conv else unreachable
    return Result.Ok(n);
}

}