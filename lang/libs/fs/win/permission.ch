public namespace fs {

using std::Result;

public func set_permissions_native(path : path_ptr, perms : u32) : Result<UnitTy, FsError> {
    // map perms -> FILE_ATTRIBUTE_READONLY
    var attrs : u32 = 0;
    if((perms & 0x200) != 0) { attrs = attrs | FILE_ATTRIBUTE_READONLY; } // owner write bit cleared -> readonly
    var ok = SetFileAttributesW(path as LPCWSTR, attrs);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(UnitTy{});
}

public func set_permissions(path : *char, perms : u32) : Result<UnitTy, FsError> {
    // map readonly bit
    var wbuf : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unreachable
        return Result.Err(e)
    }
    return set_permissions_native(&mut wbuf[0], perms)
}

}