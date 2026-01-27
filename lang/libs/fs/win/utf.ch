public namespace fs {

using std::Result;

func utf8_to_utf16_inplace(src : *char, out : *mut u16, out_len : size_t) : Result<size_t, FsError> {
    var n = MultiByteToWideChar(CP_UTF8, 0u32, src, -1, out as LPWSTR, (out_len as i32));
    if(n == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    // return length excluding null
    return Result.Ok((n as size_t) - 1);
}

func utf16_to_utf8_str(src : *u16) : std::string {
    var out = std::string();
    var needed = WideCharToMultiByte(CP_UTF8, 0u32, src, -1, null, 0, null, null);
    if(needed <= 0) { return out; }
    out.reserve((needed as size_t) - 1);
    out.resize_unsafe((needed as size_t) - 1);
    var n = WideCharToMultiByte(CP_UTF8, 0u32, src, -1, out.mutable_data(), needed, null, null);
    if(n <= 0) { out.resize_unsafe(0); return out; }
    return out;
}

}