public namespace fs {

using std::Result;

public func set_permissions_native(path : path_ptr, mode : u32) : Result<UnitTy, FsError> {
    var r = chmod(path, mode);
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(UnitTy{});
}

public func set_permissions(path : *char, mode : u32) : Result<UnitTy, FsError> {
    return set_permissions_native(path, mode)
}

}