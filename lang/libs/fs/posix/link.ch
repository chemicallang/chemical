public namespace fs {

using std::Result;

// hard link / symlink
func create_hard_link_native(existing : path_ptr, newpath : path_ptr) : Result<UnitTy, FsError> {
    var r = link(existing, newpath);
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(UnitTy{});
}

func create_symlink_native(target : path_ptr, linkpath : path_ptr, dir : bool) : Result<UnitTy, FsError> {
    var r = symlink(target, linkpath);
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(UnitTy{});
}

func read_link_native(path : path_ptr, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var r = readlink(path, out, out_len);
    if(r < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    if((r as size_t) < out_len) { out[r] = 0; }
    return Result.Ok(r as size_t);
}

func is_symlink(path : *char) : Result<bool, FsError> {
    var st : Stat;
    var r = lstat(path, &mut st);
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    var islnk = ((st.st_mode & 0xF000) == 0xA000);
    return Result.Ok(islnk);
}

func create_symlink(target : *char, linkpath : *char, dir : bool) : Result<UnitTy, FsError> {
    return create_symlink_native(target, linkpath, dir)
}

func read_link(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    return read_link_native(path, out, out_len)
}

func create_hard_link(existing : *char, newpath : *char) : Result<UnitTy, FsError> {
    return create_hard_link_native(existing, newpath)
}

}