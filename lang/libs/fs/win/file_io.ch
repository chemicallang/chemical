public namespace fs {

using std::Result;

func file_open_native(path : path_ptr, opts : OpenOptions) : Result<File, FsError> {
    var access : u32 = 0;
    if(opts.read) { access = access | 0x80000000; } // GENERIC_READ
    if(opts.write) { access = access | 0x40000000; } // GENERIC_WRITE
    var create_disp : u32 = 0;
    if(opts.create_new) {
        // CREATE_NEW
        create_disp = 1;
    } else if(opts.create && opts.truncate) {
        // CREATE_ALWAYS
        create_disp = 2;
    } else if(opts.create && !opts.truncate) {
        // OPEN_ALWAYS
        create_disp = 4;
    } else if(!opts.create && opts.truncate) {
        // TRUNCATE_EXISTING
        create_disp = 5;
    } else {
        // OPEN_EXISTING
        create_disp = 3;
    }
    var share : u32 = 1u | 2u | 4u; // FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE
    var flags : u32 = 0x80; // FILE_ATTRIBUTE_NORMAL as default
    var handle = CreateFileW(path as LPCWSTR, access, share, null, create_disp, flags, null);
    if(handle == INVALID_HANDLE_VALUE) {
        var err = GetLastError();
        return Result.Err(winerr_to_fs(err as int));
    }
    var f : File; f.win.handle = handle; f.valid = true;
    return Result.Ok(f);
}

func file_close(f : *mut File) : Result<UnitTy, FsError> {
    if(!f.valid) { return Result.Ok(UnitTy{}); }
    var ok = CloseHandle(f.win.handle);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    f.valid = false;
    return Result.Ok(UnitTy{});
}

func file_read(f : *mut File, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
    var read_out : u32 = 0;
    var ok = ReadFile(f.win.handle, buf as *mut void, buf_len as u32, (&mut read_out) as *mut DWORD, null);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(read_out as size_t);
}

func file_write(f : *mut File, buf : *u8, buf_len : size_t) : Result<size_t, FsError> {
    var written : u32 = 0;
    var ok = WriteFile(f.win.handle, buf as *void, buf_len as u32, (&mut written) as *mut DWORD, null);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(written as size_t);
}

func file_flush(f : *mut File) : Result<UnitTy, FsError> {
    var ok = FlushFileBuffers(f.win.handle);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(UnitTy{});
}

public func remove_file_native(path : path_ptr) : Result<UnitTy, FsError> {
    var deleted = DeleteFileW(path as LPCWSTR);
    if(deleted == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});
}

func set_times_native(path : path_ptr, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
    var h = CreateFileW(path as LPCWSTR, 0x40000000 /*GENERIC_WRITE*/, 0, null, 3 /*OPEN_EXISTING*/, 0, null);
    if(h == INVALID_HANDLE_VALUE) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    var cft = unix_to_filetime(0); // unused creation
    var aft = unix_to_filetime(atime);
    var mft = unix_to_filetime(mtime);
    var ok = SetFileTime(h, &cft, &aft, &mft);
    CloseHandle(h);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(UnitTy{});
}

func copy_file_native(src : path_ptr, dst : path_ptr) : Result<UnitTy, FsError> {
    var ok = CopyFileW(src as LPCWSTR, dst as LPCWSTR, 0); // overwrite
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});
}

func create_temp_file_in_native(dir : path_ptr, prefix : path_ptr, out_path : mut_path_ptr, fh : *mut File) : Result<UnitTy, FsError> {
    var res = GetTempFileNameW(dir as LPCWSTR, prefix as LPCWSTR, 0, out_path as LPWSTR);
    if(res == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    // open file
    var opts : OpenOptions; 
    opts.read = true; opts.write = true; opts.append = false; opts.create = false; opts.create_new = false; opts.truncate = false; opts.binary = true;
    var fo = file_open_native(out_path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unreachable
        return Result.Err(e)
    }
    var Ok(f) = fo else unreachable;
    fh.valid = f.valid; fh.win.handle = f.win.handle;
    return Result.Ok(UnitTy{});
}

public func remove_file(path : *char) : Result<UnitTy, FsError> {
    var w : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16_inplace(path, &mut w[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) { var Err(e) = conv else unreachable; return Result.Err(e); }
    return remove_file_native(&mut w[0])
}

public func copy_file(src : *char, dst : *char) : Result<UnitTy, FsError> {
    var wsrc : [WIN_MAX_PATH]u16; var wdst : [WIN_MAX_PATH]u16;
    var r1 = utf8_to_utf16(src, &mut wsrc[0], WIN_MAX_PATH as size_t);
    if(r1 is Result.Err) {
        var Err(e) = r1 else unreachable
        return Result.Err(e)
    }
    var r2 = utf8_to_utf16(dst, &mut wdst[0], WIN_MAX_PATH as size_t);
    if(r2 is Result.Err) {
        var Err(e) = r2 else unreachable
        return Result.Err(e)
    }
    return copy_file_native(&mut wsrc[0], &mut wdst[0])
}

func create_temp_file_in(dir : *char, prefix : *char, out_path : *mut char, out_len : size_t, fh : *mut File) : Result<UnitTy, FsError> {
    var wdir : [WIN_MAX_PATH]u16; var wprefix : [TEMP_NAME_MAX]u16; var wout : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(dir, &mut wdir[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unreachable
        return Result.Err(e)
    }
    var f2 = utf8_to_utf16(prefix, &mut wprefix[0], TEMP_NAME_MAX as size_t)
    if(f2 is Result.Err) {
        var Err(e) = f2 else unreachable
        return Result.Err(e)
    }
    var result = create_temp_file_in_native(&mut wdir[0], &mut wprefix[0], &mut wout[0], fh)
    if(result is Result.Err) {
        var Err(e) = result else unreachable
        return Result.Err(e)
    }
    var conv = utf16_to_utf8(&mut wout[0], out_path, out_len);
    if(conv is Result.Err) {
        var Err(e) = conv else unreachable
        return Result.Err(e);
    }
    return result
}

func file_open(path : *char, opts : OpenOptions) : Result<File, FsError> {
    // Convert path to UTF-16 and call CreateFileW
    var wbuf : [WIN_MAX_PATH]u16;
    var r = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
    if(r is Result.Err) {
        var Err(e) = r else unreachable
        return Result.Err(e)
    }
    return file_open_native(&mut wbuf[0], opts)
}

func set_times(path : *char, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
    var wbuf : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unreachable
        return Result.Err(e)
    }
    return set_times_native(&mut wbuf[0], atime, mtime)
}


}