public namespace fs {

using std::Result;

func file_open_native(path : path_ptr, opts : OpenOptions) : Result<File, FsError> {
    // POSIX open
    var flags : int = 0;
    if(opts.read && !opts.write) { flags = 0; /** O_RDONLY **/
    } else if(opts.read && opts.write) { flags = 2; /** O_RDWR **/
    } else if(!opts.read && opts.write) { flags = 1; /** O_WRONLY **/ }
    const O_CREAT = 0x40;
    const O_TRUNC = 0x200;
    const O_EXCL  = 0x80;
    const O_APPEND = 0x400;
    if(opts.create) { flags = flags | O_CREAT; }
    if(opts.truncate) { flags = flags | O_TRUNC; }
    if(opts.create_new) { flags = flags | O_EXCL; }
    if(opts.append) { flags = flags | O_APPEND; }
    var fd = open(path, flags, 0o666);
    if(fd < 0) { return Result.Err(posix_errno_to_fs(-fd)); } // note: user should replace with errno read
    var f : File; f.unix.fd = fd; f.valid = true;
    return Result.Ok(f);
}

func file_close(f : *mut File) : Result<UnitTy, FsError> {
    if(!f.valid) { return Result.Ok(UnitTy{}); }
    var r = close(f.unix.fd);
    if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
    f.valid = false;
    return Result.Ok(UnitTy{});
}

func file_read(f : *mut File, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
    var n = read(f.unix.fd, buf as *mut void, buf_len);
    if(n < 0) { return Result.Err(posix_errno_to_fs(-n)); }
    return Result.Ok(n as size_t);
}

func file_write(f : *mut File, buf : *u8, buf_len : size_t) : Result<size_t, FsError> {
    var n = write(f.unix.fd, buf as *void, buf_len);
    if(n < 0) { return Result.Err(posix_errno_to_fs(-n)); }
    return Result.Ok(n as size_t);
}

func file_flush(f : *mut File) : Result<UnitTy, FsError> {
    var r = fsync(f.unix.fd);
    if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
    return Result.Ok(UnitTy{});
}

public func remove_file_native(path : path_ptr) : Result<UnitTy, FsError> {
    if(unlink(path) != 0) { return Result.Err(posix_errno_to_fs(-get_errno())); }
    return Result.Ok(UnitTy{});
}

func set_times_native(path : path_ptr, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
    // use utimensat or utimes
    var times : [2]timespec;
    times[0].tv_sec = atime;
    times[0].tv_nsec = 0;
    times[1].tv_sec = mtime;
    times[1].tv_nsec = 0;
    var r = utimensat(0, path, &times[0], 0);
    if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
    return Result.Ok(UnitTy{});
}

func copy_file_native(src : path_ptr, dst : path_ptr) : Result<UnitTy, FsError> {
    // POSIX: open src, dst and copy in chunks
    var srcopts : OpenOptions; srcopts.read = true; srcopts.write = false; srcopts.create = false;
    var sres = file_open(src, srcopts);
    if(sres is Result.Err) {
        var Err(e) = sres else unreachable
        return Result.Err(e)
    }
    var Ok(sf) = sres else unreachable

    var dstopts : OpenOptions; dstopts.read = false; dstopts.write = true; dstopts.create = true; dstopts.truncate = true;
    var dres = file_open(dst, dstopts);
    if(dres is Result.Err) {
        file_close(&mut sf)
        var Err(e) = dres else unreachable
        return Result.Err(e)
    }
    var Ok(df) = dres else unreachable
    // TODO: use COPY_CHUNK as the array size here
    var buf : [64 * 1024]u8;
    while(true) {
        var r = file_read(&mut sf, &mut buf[0], sizeof(buf));
        if(r is Result.Err) {
            file_close(&mut sf);
            file_close(&mut df)
            var Err(e) = r else unreachable
            return Result.Err(e)
        }
        var Ok(n) = r else unreachable
        if(n == 0) { break; }
        var w = file_write_all(&mut df, &mut buf[0], n);
        if(w is Result.Err) {
            file_close(&mut sf)
            file_close(&mut df)
            var Err(e) = w else unreachable
            return Result.Err(e)
        }
    }
    file_close(&mut sf); file_close(&mut df);
    return Result.Ok(UnitTy{});
}

func create_temp_file_in_native(dir : path_ptr, prefix : path_ptr, out_path : mut_path_ptr, fh : *mut File) : Result<UnitTy, FsError> {
    // create template like /tmp/prefixXXXXXX
    var tmpl : [PATH_MAX_BUF]char;
    var p : size_t = 0; while(dir[p] != 0) { tmpl[p] = dir[p]; p++ }
    if(p > 0 && tmpl[p-1] != '/') { tmpl[p++] = '/'; }
    var q : size_t = 0; while(prefix[q] != 0) { tmpl[p + q] = prefix[q]; q++ }
    p += q;
    var sfx : *char = "XXXXXX\0";
    var r : size_t = 0; while(sfx[r] != 0) { tmpl[p + r] = sfx[r]; r++ }
    tmpl[p + r] = 0;
    // mkstemp modifies template
    var fd = mkstemp(&mut tmpl[0]); // user-provided extern
    if(fd < 0) { return Result.Err(FsError.Io(-1, "mkstemp failed\0")); }
    // return path
    var i : size_t = 0; while(tmpl[i] != 0) { out_path[i] = tmpl[i]; i++ } out_path[i] = 0;
    // wrap fd into File
    fh.unix.fd = fd; fh.valid = true;
    return Result.Ok(UnitTy{});
}

public func remove_file(path : *char) : Result<UnitTy, FsError> {
    return remove_file_native(path);
}

public func copy_file(src : *char, dst : *char) : Result<UnitTy, FsError> {
    return copy_file_native(src, dst)
}

func create_temp_file_in(dir : *char, prefix : *char, out_path : *mut char, out_len : size_t, fh : *mut File) : Result<UnitTy, FsError> {
    return create_temp_file_in_native(dir, prefix, out_path, fh)
}

func file_open(path : *char, opts : OpenOptions) : Result<File, FsError> {
    return file_open_native(path, opts)
}

func set_times(path : *char, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
    return set_times_native(path, atime, mtime)
}


}