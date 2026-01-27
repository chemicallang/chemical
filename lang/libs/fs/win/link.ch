public namespace fs {

using std::Result;

// hard link / symlink
func create_hard_link_native(existing : path_ptr, newpath : path_ptr) : Result<UnitTy, FsError> {
    var ok = CreateHardLinkW(newpath as LPCWSTR, existing as LPCWSTR, null);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});
}

func create_symlink_native(target : path_ptr, linkpath : path_ptr, dir : bool) : Result<UnitTy, FsError> {
    var flags : u32 = 0;
    if(dir) { flags = 1; } // SYMBOLIC_LINK_FLAG_DIRECTORY
    var ok = CreateSymbolicLinkW(linkpath as LPCWSTR, target as LPCWSTR, flags);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});
}

func read_link_native(path : path_ptr, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var h = CreateFileW(path as LPCWSTR, 0, 0, null, 3 /*OPEN_EXISTING*/, (FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS) as DWORD, null);
    if(h == INVALID_HANDLE_VALUE) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    var wout : [WIN_MAX_PATH]u16;
    var n = GetFinalPathNameByHandleW(h, (&mut wout[0]) as LPWSTR, WIN_MAX_PATH as u32, 0);
    CloseHandle(h);
    if(n == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    var conv = utf16_to_utf8(&mut wout[0], out, out_len);
    if(conv is Result.Err) {
        var Err(e) = conv else unreachable;
        return Result.Err(e);
    }
    var Ok(rn) = conv else unreachable
    return Result.Ok(rn);
}

func is_symlink(path : *char) : Result<bool, FsError> {
    var m = metadata(path);
    if(m is Result.Err) {
        var Err(e) = m else unreachable;
        return Result.Err(e);
    }
    var Ok(md) = m else unreachable;
    return Result.Ok(md.is_symlink);
}

func create_symlink(target : *char, linkpath : *char, dir : bool) : Result<UnitTy, FsError> {
    var wtarget : [WIN_MAX_PATH]u16; var wlink : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(target, &mut wtarget[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unreachable
        return Result.Err(e)
    }
    var f2 = utf8_to_utf16(linkpath, &mut wlink[0], WIN_MAX_PATH as size_t)
    if(f2 is Result.Err) {
        var Err(e) = f2 else unreachable
        return Result.Err(e)
    }
    return create_symlink_native(&mut wtarget[0], &mut wlink[0], dir)
}

func read_link(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    // On Windows readlink is more involved; use DeviceIoControl or GetFinalPathNameByHandle
    // Simpler approach: open file and call GetFinalPathNameByHandleW
    var wpath : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(path, &mut wpath[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unreachable;
        return Result.Err(e);
    }
    return read_link_native(&mut wpath[0], out, out_len)
}

func create_hard_link(existing : *char, newpath : *char) : Result<UnitTy, FsError> {
    var wexist : [WIN_MAX_PATH]u16; var wnew : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(existing, &mut wexist[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unreachable
        return Result.Err(e)
    }
    var f2 = utf8_to_utf16(newpath, &mut wnew[0], WIN_MAX_PATH as size_t)
    if(f2 is Result.Err) {
        var Err(e) = f2 else unreachable
        return Result.Err(e)
    }
    return create_hard_link_native(&mut wexist[0], &mut wnew[0])
}

}