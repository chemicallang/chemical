public namespace fs {

using std::Result;

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
    return disk_space_native(path, total_out, free_out, avail_out)
}

}