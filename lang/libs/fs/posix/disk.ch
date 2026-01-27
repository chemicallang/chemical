public namespace fs {

func disk_space_native(path : path_ptr, total_out : *mut u64, free_out : *mut u64, avail_out : *mut u64) : Result<UnitTy, FsError> {
    var st : Statvfs;
    var r = statvfs(path, &mut st);
    if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
    *total_out = (st.f_blocks as u64) * (st.f_frsize as u64);
    *free_out = (st.f_bfree as u64) * (st.f_frsize as u64);
    *avail_out = (st.f_bavail as u64) * (st.f_frsize as u64);
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