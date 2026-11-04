public namespace fs {

using std::Result;

func file_open_native(path : path_ptr, opts : OpenOptions) : Result<File, FsError> {
        unsafe {
        // POSIX open
        var flags : int = 0;
        if(opts.read && !opts.write) { flags = O_RDONLY;
        } else if(opts.read && opts.write) { flags = O_RDWR;
        } else if(!opts.read && opts.write) { flags = O_WRONLY; }
    
        if(opts.create) { flags = flags | O_CREAT; }
        if(opts.truncate) { flags = flags | O_TRUNC; }
        if(opts.create_new) { flags = flags | O_EXCL; }
        if(opts.append) { flags = flags | O_APPEND; }
        var fd = unsafe(open(path, flags, 0o666));
        if(fd < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        var f : File; f._unix.fd = fd; f.valid = true;
        return Result.Ok(f);

        }
    }

public func file_close(f : *mut File) : Result<UnitTy, FsError> {
        unsafe {
        if(f == null) { return Result.Err(FsError.InvalidInput()); }
        if(!f.valid) { return Result.Ok(UnitTy{}); }
        var r = unsafe(close(f._unix.fd));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        f.valid = false;
        return Result.Ok(UnitTy{});

        }
    }

public func file_read(f : *mut File, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(f == null || (buf == null && buf_len != 0)) { return Result.Err(FsError.InvalidInput()); }
        if(buf_len == 0) { return Result.Ok(0 as size_t); }
        var n = unsafe(read(f._unix.fd, buf as *mut void, buf_len));
        if(n < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(n as size_t);

        }
    }

public func file_write(f : *mut File, buf : *u8, buf_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(f == null || (buf == null && buf_len != 0)) { return Result.Err(FsError.InvalidInput()); }
        if(buf_len == 0) { return Result.Ok(0 as size_t); }
        var n = unsafe(write(f._unix.fd, buf as *void, buf_len));
        if(n < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(n as size_t);

        }
    }

public func file_flush(f : *mut File) : Result<UnitTy, FsError> {
        unsafe {
        if(f == null) { return Result.Err(FsError.InvalidInput()); }
        var r = unsafe(fsync(f._unix.fd));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(UnitTy{});

        }
    }

public func remove_file_native(path : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        if(unsafe(unlink(path)) != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(UnitTy{});

        }
    }

func set_times_native(path : path_ptr, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
        unsafe {
        // use utimensat or utimes
        var times : [2]timespec;
        times[0].tv_sec = atime;
        times[0].tv_nsec = 0;
        times[1].tv_sec = mtime;
        times[1].tv_nsec = 0;
        var r = unsafe(utimensat(AT_FDCWD, path, &raw times[0], 0));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(UnitTy{});

        }
    }

func copy_file_native(src : path_ptr, dst : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
        // POSIX: open src, dst and copy in chunks
        var srcopts : OpenOptions; 
        srcopts.read = true; srcopts.write = false; srcopts.append = false; srcopts.create = false; srcopts.create_new = false; srcopts.truncate = false; srcopts.binary = true;
        var sres = file_open(src, srcopts);
        if(sres is Result.Err) {
            var Err(e) = sres else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(sf) = sres else unsafe { unreachable }

        var dstopts : OpenOptions; 
        dstopts.read = false; dstopts.write = true; dstopts.append = false; dstopts.create = true; dstopts.create_new = false; dstopts.truncate = true; dstopts.binary = true;
        var dres = file_open(dst, dstopts);
        if(dres is Result.Err) {
            file_close(&raw mut sf)
            var Err(e) = dres else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(df) = dres else unsafe { unreachable }
        // TODO: use COPY_CHUNK as the array size here
        var buf : [64 * 1024]u8;
        while(true) {
            var r = file_read(&raw mut sf, &raw mut buf[0], sizeof(buf));
            if(r is Result.Err) {
                file_close(&raw mut sf);
                file_close(&raw mut df)
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(e)
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { break; }
            var w = file_write_all(&raw mut df, &raw mut buf[0], n);
            if(w is Result.Err) {
                file_close(&raw mut sf)
                file_close(&raw mut df)
                var Err(e) = w else unsafe { unreachable }
                return Result.Err(e)
            }
        }
        file_close(&raw mut sf); file_close(&raw mut df);
        return Result.Ok(UnitTy{});

        }
    }

func create_temp_file_in_native(dir : path_ptr, prefix : path_ptr, out_path : mut_path_ptr,
                                out_len : size_t, fh : *mut File) : Result<UnitTy, FsError> {
        unsafe {
        if(dir == null || prefix == null || out_path == null || fh == null) {
            return Result.Err(FsError.InvalidInput());
        }
        // create template like /tmp/prefixXXXXXX
        var tmpl : [PATH_MAX_BUF]char;
        const cap = PATH_MAX_BUF as size_t;
        var p : size_t = 0; while(p < cap && dir[p] != 0) { p++ }
        if(p == cap) { return Result.Err(FsError.PathTooLong()); }
        if(p > 0 && dir[p-1] != '/') {
            if(p + 1 >= cap) { return Result.Err(FsError.PathTooLong()); }
            p += 1;
        }
        var q : size_t = 0; while(q < cap && prefix[q] != 0) { q++ }
        if(q == cap || p > cap - q) { return Result.Err(FsError.PathTooLong()); }
        const suffix_len = 7 as size_t; // "XXXXXX" plus NUL
        if(q > cap - suffix_len || p + q + suffix_len > cap) {
            return Result.Err(FsError.PathTooLong());
        }
        var i : size_t = 0;
        while(i < p && i < cap) { tmpl[i] = dir[i]; i++ }
        if(p > 0 && dir[p-1] != '/') { tmpl[p-1] = '/'; }
        var j : size_t = 0;
        while(j < q) { tmpl[p + j] = prefix[j]; j++ }
        var sfx : *char = "XXXXXX\0";
        j = 0;
        while(j <= 6) { tmpl[p + q + j] = sfx[j]; j++ }
        // The caller's output buffer must hold the template plus a NUL.
        if(p + q + suffix_len > out_len) { return Result.Err(FsError.PathTooLong()); }
        // mkstemp modifies template
        var fd = unsafe(mkstemp(&raw mut tmpl[0])); // user-provided extern
        if(fd < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        // return path
        i = 0; while(tmpl[i] != 0) { out_path[i] = tmpl[i]; i++ } out_path[i] = 0;
        // wrap fd into File
        fh._unix.fd = fd; fh.valid = true;
        return Result.Ok(UnitTy{});

        }
    }

public func remove_file(path : *char) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        return remove_file_native(path);

        }
    }

public func copy_file(src : *char, dst : *char) : Result<UnitTy, FsError> {
        unsafe {
        if(src == null || dst == null) { return Result.Err(FsError.InvalidInput()); }
        return copy_file_native(src, dst)

        }
    }

func create_temp_file_in(dir : *char, prefix : *char, out_path : *mut char, out_len : size_t, fh : *mut File) : Result<UnitTy, FsError> {
        unsafe {
        return create_temp_file_in_native(dir, prefix, out_path, out_len, fh)

        }
    }

public func file_open(path : *char, opts : OpenOptions) : Result<File, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        return file_open_native(path, opts)

        }
    }

public func set_times(path : *char, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        return set_times_native(path, atime, mtime)

        }
    }


}