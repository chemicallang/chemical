public namespace fs {

using std::Result;

public func set_permissions_native(path : path_ptr, perms : u32) : Result<UnitTy, FsError> {
    var r = fs::chmod(path, perms as u32);
    if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
    return Result.Ok(UnitTy{});
}

public func set_permissions(path : *char, perms : u32) : Result<UnitTy, FsError> {
    return set_permissions_native(path, perms);
}

}