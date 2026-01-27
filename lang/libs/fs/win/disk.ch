public namespace fs {

using std::Result;

func disk_space_native(path : path_ptr, total_out : *mut u64, free_out : *mut u64, avail_out : *mut u64) : Result<UnitTy, FsError> {
    var ok = GetDiskFreeSpaceExW(path as LPCWSTR, free_out as PULARGE_INTEGER, total_out as PULARGE_INTEGER, avail_out as PULARGE_INTEGER);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(UnitTy{});
}

func disk_space(path : *char, total_out : *mut u64, free_out : *mut u64, avail_out : *mut u64) : Result<UnitTy, FsError> {
    var w : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(path, &mut w[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unreachable;
        return Result.Err(e);
    }
    return disk_space_native(&mut w[0], total_out, free_out, avail_out)
}

}