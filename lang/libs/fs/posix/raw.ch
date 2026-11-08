// fs::raw — the complete low-level filesystem surface.
//
// Nested namespaces cannot be reopened across files, so every raw entry point
// lives in this single namespace. The canonical safe surface is in `fs` itself
// (`File`, `read`, `write`, `metadata`, `read_dir`, ...).
// Disambiguate the POSIX descriptor syscalls from the safe `fs::read`/`fs::write`.
func __fs_posix_read(fd : int, buf : *mut void, count : size_t) : isize {
    unsafe { return read(fd, buf, count as ulong) }
}
func __fs_posix_write(fd : int, buf : *void, count : size_t) : isize {
    unsafe { return write(fd, buf, count as ulong) }
}

public namespace fs {

public namespace raw {

using std::Result;
using std::string;
using std::string_view;
using std::vector;


// ---- from lang/libs/fs/posix/file_io.ch ----
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
        var f : File; f.handle = fd as i64; f.valid = true;
        return Result.Ok(f);

        }
    }

public func file_close_ref(f : &mut File) : Result<UnitTy, FsError> {
        unsafe {
        if(!f.valid) { return Result.Ok(UnitTy{}); }
        var r = unsafe(close((f.handle as int)));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        f.valid = false;
        return Result.Ok(UnitTy{});
        }
    }

public func file_close(f : *mut File) : Result<UnitTy, FsError> {
        unsafe {
        if(f == null) { return Result.Err(FsError.InvalidInput()); }
        if(!f.valid) { return Result.Ok(UnitTy{}); }
        var r = unsafe(close((f.handle as int)));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        f.valid = false;
        return Result.Ok(UnitTy{});

        }
    }

public func file_read(f : *mut File, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(f == null || !f.valid || (buf == null && buf_len != 0)) { return Result.Err(FsError.InvalidInput()); }
        if(buf_len == 0) { return Result.Ok(0 as size_t); }
        var n = unsafe(__fs_posix_read((f.handle as int), buf as *mut void, buf_len));
        if(n < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(n as size_t);

        }
    }

public func file_write(f : *mut File, buf : *u8, buf_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(f == null || !f.valid || (buf == null && buf_len != 0)) { return Result.Err(FsError.InvalidInput()); }
        if(buf_len == 0) { return Result.Ok(0 as size_t); }
        var n = unsafe(__fs_posix_write((f.handle as int), buf as *void, buf_len));
        if(n < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(n as size_t);

        }
    }

public func file_flush(f : *mut File) : Result<UnitTy, FsError> {
        unsafe {
        if(f == null || !f.valid) { return Result.Err(FsError.InvalidInput()); }
        var r = unsafe(fsync((f.handle as int)));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(UnitTy{});

        }
    }

/// Seek an open file. `whence` is one of SEEK_SET/SEEK_CUR/SEEK_END.
public func file_seek(f : *mut File, offset : i64, whence : int) : Result<u64, FsError> {
        unsafe {
        if(f == null || !f.valid) { return Result.Err(FsError.InvalidInput()); }
        var r = unsafe(lseek((f.handle as int), offset as longlong, whence));
        if(r < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(r as u64);

        }
    }

/// Metadata for an open file handle.
public func file_metadata(f : *mut File) : Result<Metadata, FsError> {
        unsafe {
        if(f == null || !f.valid) { return Result.Err(FsError.InvalidInput()); }
        var st : Stat;
        var r = unsafe(fstat((f.handle as int), &raw mut st));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(stat_to_metadata(st));

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
        fh.handle = fd as i64; fh.valid = true;
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

// ---- from lang/libs/fs/posix/directory.ch ----
public func create_dir_native(path : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        var r = posix_mkdir(path, 0o777);
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(UnitTy{});

        }
    }

// create_dir_all (recursive)
public func create_dir_all(path : *char) : Result<UnitTy, FsError> {
        unsafe {
        var buf : [PATH_MAX_BUF]char;
        var r = normalize_path(path, &raw mut buf[0], PATH_MAX_BUF as size_t);
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(len) = r else unsafe { unreachable };
        // iterate components
        var i : size_t = 1; // if path is absolute, start at 1 to include leading '/'
        if(buf[0] != '/') { i = 0; } // relative
        while(i <= len) {
            if(i == len || buf[i] == '/') {
                // prefix is [0..i)
                var prefix : [PATH_MAX_BUF]char;
                var k : size_t = 0;
                while(k < i) { prefix[k] = buf[k]; k++ }
                prefix[k] = 0;
                // skip empty
                if(k == 0) { i++; continue; }
                var stat_res = metadata(&raw mut prefix[0]);
                if(stat_res is Result.Err) {
                    var Err(e) = stat_res else unsafe { unreachable };
                    if(e is FsError.NotFound) {
                        var c = create_dir(&raw mut prefix[0]);
                        if(c is Result.Err) { var Err(e2) = c else unsafe { unreachable }; return Result.Err(e2); }
                    } else {
                        // if exists fine, otherwise return
                        // continue if AlreadyExists
                        if(e is FsError.AlreadyExists) { /* ok */ } else { return Result.Err(e); }
                    }
                }
            }
            i++;
        }
        return Result.Ok(UnitTy{});

        }
    }

// remove_dir (non-recursive) and remove_dir_all (recursive)
public func remove_dir_native(path : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        var r = unsafe(rmdir(path));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(UnitTy{});

        }
    }

// ---------------------------
// Helper: remove_dir_all_at(fd)
// Remove contents of directory referenced by an open directory fd.
// Does not remove the directory itself; caller should remove it by name (rmdir) or handle separately.
// ---------------------------
func remove_dir_all_at(dirfd : int) : Result<UnitTy, FsError> {
        unsafe {
        // Duplicate fd for fdopendir (fdopendir usually consumes the fd)
        var ddup = unsafe(dup(dirfd));
        if(ddup < 0) {
            var e = get_errno();
            // printf("dup(%d) failed: errno=%d (%s)\n", dirfd, e, strerror(e));
            return Result.Err(posix_errno_to_fs(e));
        }

        var dir = unsafe(fdopendir(ddup));
        if(dir == null) {
            var e = get_errno();
            // printf("fdopendir(%d) failed: errno=%d (%s)\n", ddup, e, strerror(e));
            unsafe { close(ddup) };
            return Result.Err(posix_errno_to_fs(e));
        }

        loop {
            // IMPORTANT: clear errno before calling readdir so a leftover errno doesn't confuse us
            set_errno(0);

            var ent = unsafe(readdir(dir));
            if(ent == null) {
                if(get_errno() != 0) {
                    var re = get_errno();
                    // printf("readdir(dirfd=%d) returned NULL with errno=%d (%s)\n", dirfd, re, strerror(re));
                    unsafe { closedir(dir) };
                    return Result.Err(posix_errno_to_fs(re));
                }
                // end of directory
                break;
            }

            // skip '.' and '..'
            if(ent.d_name[0] == '.' && ent.d_name[1] == 0) { continue; }
            if(ent.d_name[0] == '.' && ent.d_name[1] == '.' && ent.d_name[2] == 0) { continue; }

            // Debug: print the entry name
            // printf("entry: '%s' (dirfd=%d)\n", &ent.d_name[0], dirfd);

            // fstatat relative to dirfd
            var st : Stat;
            set_errno(0);
            if(unsafe(fstatat(dirfd, &raw ent.d_name[0], &raw mut st, AT_SYMLINK_NOFOLLOW)) != 0) {
                var e = get_errno();
                // printf("fstatat(dirfd=%d, name='%s') -> errno=%d (%s)\n", dirfd, &ent.d_name[0], e, strerror(e));
                if(e == 2) { // ENOENT
                    // printf("  (ENOENT) entry disappeared between readdir and fstatat — skipping\n");
                    continue;
                } else {
                    unsafe { closedir(dir) };
                    return Result.Err(posix_errno_to_fs(e));
                }
            }

            // Directory? recurse via openat
            if((st.st_mode & S_IFMT) == S_IFDIR) {
                // printf("  '%s' is DIR — openat\n", &ent.d_name[0]);

                set_errno(0);
                var childfd = unsafe(openat(dirfd, &raw ent.d_name[0], O_RDONLY | O_DIRECTORY, 0));
                if(childfd < 0) {
                    var e2 = get_errno();
                    // printf("  openat(dirfd=%d, name='%s') -> errno=%d (%s)\n", dirfd, &ent.d_name[0], e2, strerror(e2));
                    if(e2 == 2) {
                        // printf("  (ENOENT) was removed before openat; skipping\n");
                        continue;
                    } else {
                        unsafe { closedir(dir) };
                        return Result.Err(posix_errno_to_fs(e2));
                    }
                }

                // Recurse on the child's dirfd
                // printf("  recursing into fd %d\n", childfd);
                var rem = remove_dir_all_at(childfd);

                // Always close the child fd we opened
                unsafe { close(childfd) };

                if(rem is Result.Err) { var Err(e) = rem else unsafe { unreachable }; unsafe { closedir(dir) }; return Result.Err(e); }

                // Remove the directory entry itself using unlinkat(AT_REMOVEDIR)
                set_errno(0);
                if(unsafe(unlinkat(dirfd, &raw ent.d_name[0], AT_REMOVEDIR)) != 0) {
                    var e3 = get_errno();
                    // printf("  unlinkat(AT_REMOVEDIR) dirfd=%d name='%s' -> errno=%d (%s)\n", dirfd, &ent.d_name[0], e3, strerror(e3));
                    if(e3 == 2) {
                        // printf("  (ENOENT) entry gone before unlinkat; benign\n");
                        continue;
                    } else {
                        unsafe { closedir(dir) };
                        return Result.Err(posix_errno_to_fs(e3));
                    }
                } else {
                    // printf("  unlinkat(AT_REMOVEDIR) succeeded for '%s'\n", &ent.d_name[0]);
                }

            } else {
                // Not a directory: remove with unlinkat
                set_errno(0);
                if(unsafe(unlinkat(dirfd, &raw ent.d_name[0], 0)) != 0) {
                    var e4 = get_errno();
                    // printf("  unlinkat(dirfd=%d, name='%s') -> errno=%d (%s)\n", dirfd, &ent.d_name[0], e4, strerror(e4));
                    if(e4 == 2) {
                        // printf("  (ENOENT) already removed — skipping\n");
                        continue;
                    } else {
                        unsafe { closedir(dir) };
                        return Result.Err(posix_errno_to_fs(e4));
                    }
                } else {
                    // printf("  unlinkat succeeded for '%s'\n", &ent.d_name[0]);
                }
            }
        }

        unsafe { closedir(dir) };
        // printf("remove_dir_all_at(dirfd=%d) completed OK\n", dirfd);
        return Result.Ok(UnitTy{});

        }
    }

// -----------------------------
// Recursive remove_dir_all (public)
// -----------------------------
public func remove_dir_all_recursive_native(path : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        // POSIX
        // printf("remove_dir_all_recursive: opening '%s'\n", path);
        var dirfd = unsafe(open(path, O_RDONLY | O_DIRECTORY, 0));
        if(dirfd < 0) {
            var e = get_errno();
            // printf("open('%s') -> errno=%d (%s)\n", path, e, strerror(e));
            return Result.Err(posix_errno_to_fs(e));
        }

        var rem = remove_dir_all_at(dirfd);

        // close dirfd regardless
        var c = unsafe(close(dirfd));

        if(rem is Result.Err) { var Err(e) = rem else unsafe { unreachable }; return Result.Err(e); }

        // remove the directory itself (rmdir)
        set_errno(0);
        if(unsafe(rmdir(path)) != 0) {
            var er = get_errno();
            // printf("rmdir('%s') -> errno=%d (%s)\n", path, er, strerror(er));
            if(er == 2) {
                // printf("rmdir: ENOENT: directory already removed — treating as success\n");
                return Result.Ok(UnitTy{});
            }
            return Result.Err(posix_errno_to_fs(er));
        }
        // printf("remove_dir_all_recursive: removed directory '%s' OK\n", path);
        return Result.Ok(UnitTy{});

        }
    }

public func temp_dir(out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(out == null) { return Result.Err(FsError.InvalidInput()); }
        var tmp : *char = "/tmp\0";
        var needed : size_t = 5; // "/tmp" plus NUL
        if(out_len < needed) { return Result.Err(FsError.PathTooLong()); }
        var i : size_t = 0; while(tmp[i] != 0) { out[i] = tmp[i]; i++ }
        out[i] = 0;
        return Result.Ok(i);

        }
    }

// read_dir: callback style to avoid allocations. Callback signature: fn(name : *char, name_len : size_t, is_dir : bool) -> bool
public func read_dir(path : *char, callback : std::function<(name : *char, name_len : size_t, is_dir : bool) => bool>) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        var d = unsafe(opendir(path));
        if(d == null) { return Result.Err(posix_errno_to_fs(get_errno())); }
        while(true) {
            var ent = unsafe(readdir(d));
            if(ent == null) { break; }
            var name_ptr = &raw ent.d_name[0];
            var nl : size_t = 0; while(name_ptr[nl] != 0) { nl++ }
            var isdir : bool = false;
            // TODO:
            // if(def.HAVE_DIRENT_D_TYPE)
            // isdir = (ent.d_type == DT_DIR);
            // else {
            // fallback: stat child. The child path is assembled into a fixed
            // stack buffer, so every component must be bounded before writing.
            var child : [PATH_MAX_BUF]char;
            var p : size_t = 0;
            while(path[p] != 0) {
                if(p + 1 >= PATH_MAX_BUF as size_t) {
                    unsafe { closedir(d) };
                    return Result.Err(FsError.PathTooLong());
                }
                child[p] = path[p];
                p++;
            }
            var separator : size_t = 0;
            if(p > 0 && child[p-1] != '/') { separator = 1; }
            if(p + separator + nl + 1 > PATH_MAX_BUF as size_t) {
                unsafe { closedir(d) };
                return Result.Err(FsError.PathTooLong());
            }
            if(separator != 0) { child[p++] = '/'; }
            var q : size_t = 0;
            while(q <= nl) { child[p + q] = name_ptr[q]; q++ }
            var st : Stat;
            var r = unsafe(lstat(&raw child[0], unsafe(&raw mut st)));
            if(r == 0) { isdir = ((st.st_mode & 0xF000) == 0x4000); }
            // }
            var cont = callback(name_ptr, nl, isdir);
            if(!cont) { break; }
        }
        unsafe { closedir(d) };
        return Result.Ok(UnitTy{});

        }
    }

public func create_dir(path : *char) : Result<UnitTy, FsError> {
        unsafe {
        return create_dir_native(path);

        }
    }

public func remove_dir(path : *char) : Result<UnitTy, FsError> {
        unsafe {
        return remove_dir_native(path)

        }
    }

public func remove_dir_all_recursive(path : *char) : Result<UnitTy, FsError> {
        unsafe {
        return remove_dir_all_recursive_native(path)

        }
    }

public func mkdir(pathname : *char) : int {
        unsafe {
        return posix_mkdir(pathname, PermissionMode.S_IRWXU as uint)

        }
    }

// ---- from lang/libs/fs/posix/link.ch ----
// hard link / symlink
func create_hard_link_native(existing : path_ptr, newpath : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
        var r = unsafe(link(existing, newpath));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(UnitTy{});

        }
    }

func create_symlink_native(target : path_ptr, linkpath : path_ptr, dir : bool) : Result<UnitTy, FsError> {
        unsafe {
        var r = unsafe(symlink(target, linkpath));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        return Result.Ok(UnitTy{});

        }
    }

func read_link_native(path : path_ptr, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        var r = unsafe(readlink(path, out, out_len));
        if(r < 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        if((r as size_t) < out_len) { out[r] = 0; }
        return Result.Ok(r as size_t);

        }
    }

func is_symlink(path : *char) : Result<bool, FsError> {
        unsafe {
        var st : Stat;
        var r = unsafe(lstat(path, unsafe(&raw mut st)));
        if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
        var islnk = ((st.st_mode & 0xF000) == 0xA000);
        return Result.Ok(islnk);

        }
    }

func create_symlink(target : *char, linkpath : *char, dir : bool) : Result<UnitTy, FsError> {
        unsafe {
        return create_symlink_native(target, linkpath, dir)

        }
    }

func read_link(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        return read_link_native(path, out, out_len)

        }
    }

func create_hard_link(existing : *char, newpath : *char) : Result<UnitTy, FsError> {
        unsafe {
        return create_hard_link_native(existing, newpath)

        }
    }

// ---- from lang/libs/fs/posix/lock.ch ----
func lock_file_shared(path : *char) : Result<File, FsError> {
    var opts : OpenOptions; opts.read = true; opts.write = false;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unsafe { unreachable };
        return Result.Err(e);
    }
    var Ok(f) = fo else unsafe { unreachable };
    var r = unsafe(flock((f.handle as int), LOCK_SH));
    if(r != 0) { file_close(&raw mut f); return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(f);
}

func lock_file_exclusive(path : *char) : Result<File, FsError> {
    var opts : OpenOptions; opts.read = true; opts.write = true;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unsafe { unreachable };
        return Result.Err(e);
    }
    var Ok(f) = fo else unsafe { unreachable };
    var r = unsafe(flock((f.handle as int), LOCK_EX));
    if(r != 0) { file_close(&raw mut f); return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(f);
}

// ---- from lang/libs/fs/posix/permission.ch ----
public func set_permissions_native(path : path_ptr, mode : u32) : Result<UnitTy, FsError> {
    var r = unsafe(chmod(path, mode));
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(UnitTy{});
}

public func set_permissions(path : *char, mode : u32) : Result<UnitTy, FsError> {
    return set_permissions_native(path, mode)
}

// ---- from lang/libs/fs/posix/disk.ch ----
func disk_space_native(path : path_ptr, total_out : *mut u64, free_out : *mut u64, avail_out : *mut u64) : Result<UnitTy, FsError> { unsafe {
    var st : Statvfs;
    var r = statvfs(path, unsafe(&raw mut st));
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    *total_out = (st.f_blocks as u64) * (st.f_frsize as u64);
    *free_out = (st.f_bfree as u64) * (st.f_frsize as u64);
    *avail_out = (st.f_bavail as u64) * (st.f_frsize as u64);
    return Result.Ok(UnitTy{});
 }}

func disk_space(path : *char, total_out : *mut u64, free_out : *mut u64, avail_out : *mut u64) : Result<UnitTy, FsError> {
    return disk_space_native(path, total_out, free_out, avail_out)
}

// ---- from lang/libs/fs/posix/metadata.ch ----
public func stat_to_metadata(st : Stat) : Metadata {
    var m : Metadata;
    var mode : int = st.st_mode as int;
    m.is_dir = ((mode & 0xF000) == 0x4000);
    m.is_file = ((mode & 0xF000) == 0x8000);
    m.is_symlink = ((mode & 0xF000) == 0xA000);
    m.len = st.st_size as size_t;
    m.modified = st.st_mtime.tv_sec as i64;
    m.accessed = st.st_atime.tv_sec as i64;
    m.created = st.st_ctime.tv_sec as i64;
    m.perms = (st.st_mode & 0x1FF) as u32;
    return m;
}

public func metadata(path : *char) : Result<Metadata, FsError> {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    var st : Stat;
    var r = unsafe(lstat(path, unsafe(&raw mut st)));
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(stat_to_metadata(st));
}

// ---- from lang/libs/fs/posix/path.ch ----
func canonicalize(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        // POSIX: realpath
        var resptr = unsafe(fs::realpath(path, out));
        if(resptr == 0) {
            // errno must be mapped by user; assume errno accessor exists
            return Result.Err(FsError.Io(-1, string("realpath failed")));
        }
        var plen : size_t = 0; while(out[plen] != 0) { plen++ }
        return Result.Ok(plen);

        }
    }

// ---- from lang/libs/fs/src/path.ch ----
public func basename(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(path == null || out == null) { return Result.Err(FsError.InvalidInput()); }
        var len : size_t = 0;
        while(path[len] != 0) { len++ }
        if(len == 0) {
            if(out_len < 2) { return Result.Err(FsError.PathTooLong()); }
            out[0] = '.'; out[1] = 0; return Result.Ok(1);
        }
        var i : size_t = len;
        while(i > 0 && (path[i-1] == '/' || path[i-1] == '\\')) { i-- }
        if(i == 0) { // path was "/" or "\" repeated
            if(out_len < 2) { return Result.Err(FsError.PathTooLong()); }
            out[0] = '/'; out[1] = 0; return Result.Ok(1);
        }
        var j : size_t = i;
        while(j > 0 && path[j-1] != '/' && path[j-1] != '\\') { j-- }
        var comp_len = i - j;
        if(comp_len >= out_len) { return Result.Err(FsError.PathTooLong()); }
        var k : size_t = 0;
        while(k < comp_len) { out[k] = path[j + k]; k++ }
        out[k] = 0;
        return Result.Ok(comp_len);

        }
    }

public func dirname(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(path == null || out == null) { return Result.Err(FsError.InvalidInput()); }
        var len : size_t = 0;
        while(path[len] != 0) { len++ }
        if(len == 0) {
            if(out_len < 2) { return Result.Err(FsError.PathTooLong()); }
            out[0] = '.'; out[1] = 0; return Result.Ok(1);
        }
        var i : size_t = len;
        while(i > 0 && (path[i-1] == '/' || path[i-1] == '\\')) { i-- }
        if(i == 0) { if(out_len < 2) { return Result.Err(FsError.PathTooLong()); } out[0] = '/'; out[1] = 0; return Result.Ok(1); }
        var j : size_t = i;
        while(j > 0 && path[j-1] != '/' && path[j-1] != '\\') { j-- }
        if(j == 0) { if(out_len < 2) { return Result.Err(FsError.PathTooLong()); } out[0] = '.'; out[1] = 0; return Result.Ok(1); }
        var end = j;
        while(end > 0 && (path[end-1] == '/' || path[end-1] == '\\')) { end-- }
        if(end == 0) { if(out_len < 2) { return Result.Err(FsError.PathTooLong()); } out[0] = '/'; out[1] = 0; return Result.Ok(1); }
        if(end >= out_len) { return Result.Err(FsError.PathTooLong()); }
        var k : size_t = 0;
        while(k < end) { out[k] = path[k]; k++ }
        out[k] = 0;
        return Result.Ok(end);

        }
    }

// extension (returns extension starting at '.' or empty)
public func extension(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(path == null || out == null) { return Result.Err(FsError.InvalidInput()); }
        var base_buf : [PATH_MAX_BUF]char;
        var r = basename(path, &raw mut base_buf[0], PATH_MAX_BUF as size_t);
        if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err<size_t, FsError>(e); }
        var Ok(base_len) = r else unsafe { unreachable };
        var i : size_t = base_len;
        while(i > 0 && base_buf[i-1] != '.') { i-- }
        if(i == 0) {
            if(out_len < 1) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[0] = 0; return Result.Ok<size_t, FsError>(0);
        }
        var ext_len = base_len - i;
        if(ext_len >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        var k : size_t = 0;
        while(k < ext_len) { out[k] = base_buf[i + k]; k++ }
        out[k] = 0;
        return Result.Ok<size_t, FsError>(ext_len);

        }
    }

// join path a + b -> out (normalizes separators). Caller must provide out buffer.
public func join_path(a : *char, b : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(a == null || b == null || out == null) { return Result.Err(FsError.InvalidInput()); }
        const max_size = (0 as size_t) - 1;
        var a_len : size_t = 0; while(a[a_len] != 0) { a_len++ }
        var b_len : size_t = 0; while(b[b_len] != 0) { b_len++ }
        if(a_len == 0) {
            if(b_len >= out_len) { return Result.Err(FsError.PathTooLong()); }
            var i : size_t = 0; while(i <= b_len) { out[i] = b[i]; i++ }
            return Result.Ok(b_len);
        }
        var need_sep : bool = false; if(a[a_len-1] != '/' && a[a_len-1] != '\\') { need_sep = true }
        if(a_len > max_size - b_len) { return Result.Err(FsError.PathTooLong()); }
        var total = a_len + b_len;
        if(need_sep) {
            if(total == max_size) { return Result.Err(FsError.PathTooLong()); }
            total += 1;
        }
        if(total == max_size || total + 1 > out_len) { return Result.Err(FsError.PathTooLong()); }
        var pos : size_t = 0; var i : size_t = 0;
        while(i < a_len) { out[pos++] = a[i++]; }
        if(need_sep) { out[pos++] = '/'; }
        i = 0; while(i <= b_len) { out[pos++] = b[i++]; }
        return Result.Ok(total);

        }
    }

// normalize_path: resolve "." and ".." (does not resolve symlinks). Uses stack arrays for components.
// normalize path removing "." and resolving ".." relative components (no symlink resolution)
public func normalize_path(path_in : *char, out_buf : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(path_in == null || out_buf == null) { return Result.Err(FsError.InvalidInput()); }
        var len : size_t = 0; while(path_in[len] != 0) { len++ }
        // Preserve an absolute root so absolute paths stay absolute (critical for
        // create_dir_all: dropping "/" would turn "/tmp/x" into a relative
        // "tmp/x" created under the process CWD).
        var is_abs : bool = false;
        if(len > 0 && (path_in[0] == '/' || path_in[0] == '\\')) { is_abs = true }
        const MAX_COMPS = 512;
        var offs : [MAX_COMPS]size_t;
        var lens : [MAX_COMPS]size_t;
        var count : size_t = 0;
        var i : size_t = 0;
        while(i < len) {
            while(i < len && (path_in[i] == '/' || path_in[i] == '\\')) { i++ }
            if(i >= len) { break; }
            var start = i;
            while(i < len && path_in[i] != '/' && path_in[i] != '\\') { i++ }
            var c_len = i - start;
            if(c_len == 1 && path_in[start] == '.') {
                // skip
            } else if(c_len == 2 && path_in[start] == '.' && path_in[start+1] == '.') {
                if(count > 0) { count -= 1; } else {
                    // ".." at the root of an absolute path is a no-op (can't go up
                    // past "/"); for relative paths keep it.
                    if(!is_abs) {
                        if(count >= MAX_COMPS) { return Result.Err(FsError.PathTooLong()); }
                        offs[count] = start; lens[count] = c_len; count++;
                    }
                }
            } else {
                if(count >= MAX_COMPS) { return Result.Err(FsError.PathTooLong()); }
                offs[count] = start; lens[count] = c_len; count++;
            }
        }
        if(count == 0) {
            if(is_abs) {
                if(out_len < 2) { return Result.Err(FsError.PathTooLong()); }
                out_buf[0] = '/'; out_buf[1] = 0; return Result.Ok(1);
            }
            if(out_len < 2) { return Result.Err(FsError.PathTooLong()); }
            out_buf[0] = '.'; out_buf[1] = 0; return Result.Ok(1);
        }
        var pos : size_t = 0; var j : size_t = 0;
        // Emit the leading "/" for absolute paths before the first component.
        if(is_abs) {
            if(pos + 1 >= out_len) { return Result.Err(FsError.PathTooLong()); }
            out_buf[pos++] = '/';
        }
        while(j < count) {
            if(j > 0) {
                if(pos + 1 >= out_len) { return Result.Err(FsError.PathTooLong()); }
                out_buf[pos++] = '/';
            }
            var off = offs[j]; var l = lens[j];
            if(pos + l >= out_len) { return Result.Err(FsError.PathTooLong()); }
            var k : size_t = 0; while(k < l) { out_buf[pos + k] = path_in[off + k]; k++ }
            pos += l; j++
        }
        if(pos >= out_len) { return Result.Err(FsError.PathTooLong()); }
        out_buf[pos] = 0;
        return Result.Ok(pos);

        }
    }

func find_last_pos_of_or(str : *char, len : size_t, value : char, value2 : char) : int {
        unsafe {
        if(len == 0) return -1;
        var i : int = len as int - 1;
        while(i >= 0) {
            // printf("checking %c against %c and %c \n", str[i], value, value2)
            if(str[i] == value || str[i] == value2) {
                return i;
            }
            // printf("found not\n");
            i--;
        }
        return -1;

        }
    }

public func parent_path_view(str : std::string_view) : std::string {
        unsafe {
        var final = std::string()
        var pos : int = 0
        comptime if(def.windows) {
            pos = find_last_pos_of_or(str.data(), str.size(), '\\', '/')
        } else {
            pos = find_last_pos_of_or(str.data(), str.size(), '/', '/')
        }
        if(pos > 0) {
            final.append_with_len(str.data(), (pos + 1) as size_t)
        }
        return final;

        }
    }

// ---- from lang/libs/fs/src/file_io.ch ----
// read_exact: read until buf_len bytes filled or error
func file_read_exact(f : *mut File, buf : *mut u8, buf_len : size_t) : Result<UnitTy, FsError> {
        unsafe {
        if(f == null || (buf == null && buf_len != 0)) { return Result.Err(FsError.InvalidInput()); }
        var pos : size_t = 0;
        while(pos < buf_len) {
            var r = file_read(f, buf + pos, buf_len - pos);
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(e)
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Err(FsError.Io(0, string("unexpected EOF"))); }
            pos += n;
        }
        return Result.Ok(UnitTy{});

        }
    }

// write_all: loop till all bytes written
func file_write_all(f : *mut File, buf : *u8, buf_len : size_t) : Result<UnitTy, FsError> {
        unsafe {
        if(f == null || (buf == null && buf_len != 0)) { return Result.Err(FsError.InvalidInput()); }
        var pos : size_t = 0;
        while(pos < buf_len) {
            var r = file_write(f, buf + pos, buf_len - pos);
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(e)
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Err(FsError.Io(0, string("write returned 0"))); }
            pos += n;
        }
        return Result.Ok(UnitTy{});

        }
    }

// util
func int_to_str(v : int, out : *mut char, out_len : size_t) : size_t {
        unsafe {
        if(out_len == 0) { return 0; }
        var tmp : [32]char;
        var i : size_t = 0;
        var val = v;
        if(val == 0) { out[0] = '0'; out[1] = 0; return 1; }
        var neg : bool = false;
        if(val < 0) { neg = true; val = -val; }
        while(val > 0) {
            var d = (val % 10) as u8;
            tmp[i++] = (('0' as u8) + d) as char;
            val = val / 10;
        }
        var pos : size_t = 0;
        if(neg) { if(pos + 1 >= out_len) { return 0; } out[pos++] = '-'; }
        // reverse
        while(i > 0) { i -= 1; if(pos + 1 >= out_len) { return 0; } out[pos++] = tmp[i]; }
        if(pos >= out_len) { return 0; }
        out[pos] = 0;
        return pos;

        }
    }

public func atomic_write(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null || (data == null && data_len != 0)) {
            return Result.Err(FsError.InvalidInput());
        }
        // write to temp in same dir then rename
        // compose tmp path: path + ".tmpXXXX"
        var dir_buf : [PATH_MAX_BUF]char;
        var r = dirname(path, &raw mut dir_buf[0], PATH_MAX_BUF as size_t);
        if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(e); }
        var Ok(dir_len) = r else unsafe { unreachable };
        var tmpbuf : [PATH_MAX_BUF]char;
        var tmpname : [64]char;
        var p : size_t = 0;
        while(p < dir_len) { tmpbuf[p] = dir_buf[p]; p++ }
        if(p > 0 && tmpbuf[p-1] != '/') { tmpbuf[p++] = '/'; }
        // append prefix + pid + random (we'll do pid)
        var pref : *char = ".tmpfs.";
        var pi : size_t = 0;
        while(pref[pi] != 0) { pi++ }
        // naive PID -> decimal (user can replace with syscall)
        var PID : int = 12345; // user should replace with getpid()
        var tlen : size_t = int_to_str(PID, &raw mut tmpname[0], 64);
        if(tlen == 0) { return Result.Err(FsError.Other(string("pid conversion failed"))); }
        // Reserve the prefix, the decimal suffix, and a terminating NUL before
        // writing any of them; a path near PATH_MAX_BUF must fail, not overflow
        // the stack buffer.
        const cap = PATH_MAX_BUF as size_t;
        if(p > cap || pi > cap - p || tlen >= cap - p - pi) {
            return Result.Err(FsError.PathTooLong());
        }
        var q : size_t = 0;
        while(q < pi) { tmpbuf[p + q] = pref[q]; q++ }
        p += pi;
        q = 0;
        while(q <= tlen) { tmpbuf[p + q] = tmpname[q]; q++ }
        // ensure null
        tmpbuf[p + tlen] = 0;
        // write file
        var wr = write_text_file(&raw mut tmpbuf[0], data, data_len);
        if(wr is Result.Err) { var Err(e) = wr else unsafe { unreachable }; return Result.Err(e); }
        // rename temp to final
        var rnm = move_path(&raw mut tmpbuf[0], path);
        if(rnm is Result.Err) { var Err(e) = rnm else unsafe { unreachable }; return Result.Err(e); }
        return Result.Ok(UnitTy{});

        }
    }

public func read_entire_file(path : *char) : Result<std::vector<u8>, FsError> {
        unsafe {
        if(path == null) { return Result.Err(FsError.InvalidInput()); }
        // open file for reading
        var opts : OpenOptions;
        opts.read = true;
        opts.write = false;
        opts.append = false;
        opts.create = false;
        opts.create_new = false;
        opts.truncate = false;
        opts.binary = true;

        var fo = file_open(path, opts);
        if(fo is Result.Err) {
            var Err(e) = fo else unsafe { unreachable };
            return Result.Err(e);
        }
        var Ok(f) = fo else unsafe { unreachable };

        // a vector
        var vec = std::vector<u8>()
        var cap : size_t = 1048;        // start with 8KiB
        vec.reserve(cap) // 1kb

        var pos : size_t = 0;

        while(true) {
            // ensure there's some writable capacity: if full, double it
            if(pos >= cap) {
                // grow capacity
                var newcap = cap * 2;
                // guard against 0 or overflow
                if(newcap <= cap) { // overflow or stuck
                    file_close(&raw mut f);
                    return Result.Err(FsError.Other(string("file too large to read")));
                }
                cap = newcap;
                vec.reserve(cap);
            }

            // pointer to writable region in the string's buffer
            var ptr = vec.data() + pos as isize; // cast to pointer arithmetic if needed

            // read up to (cap - pos) bytes
            var want = cap - pos;
            var r = file_read(&raw mut f, ptr as *mut u8, want);
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable };
                file_close(&raw mut f);
                return Result.Err(e);
            }
            var Ok(n) = r else unsafe { unreachable };

            if(n == 0) {
                // EOF
                break;
            }

            // advance write position
            pos += n;
            // loop continues; if pos == cap, we'll grow next iteration
        }

        // set the string's size to the number of bytes read
        unsafe { vec.set_len(pos) };

        file_close(&raw mut f);
        return Result.Ok(vec);

        }
    }


public func read_to_buffer(path : *char, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
        unsafe {
        if(path == null || (buf == null && buf_len != 0)) {
            return Result.Err(FsError.InvalidInput());
        }
        var opts : OpenOptions; 
        opts.read = true; opts.write = false; opts.append = false; opts.create = false; opts.create_new = false; opts.truncate = false; opts.binary = true;
        var fo = file_open(path, opts);
        if(fo is Result.Err) {
            var Err(e) = fo else unsafe { unreachable };
            return Result.Err<size_t, FsError>(e);
        }
        var Ok(f) = fo else unsafe { unreachable }
        var pos : size_t = 0;
        while(true) {
            var r = file_read(&raw mut f, buf + pos, buf_len - pos);
            if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; file_close(&raw mut f); return Result.Err<size_t, FsError>(e); }
            var Ok(n) = r else unsafe { unreachable };
            if(n == 0) { break; } // EOF
            pos += n;
            if(pos >= buf_len) { file_close(&raw mut f); return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        }
        file_close(&raw mut f);
        return Result.Ok<size_t, FsError>(pos);

        }
    }

public func write_text_file(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
        unsafe {
        if(path == null || (data == null && data_len != 0)) {
            return Result.Err(FsError.InvalidInput());
        }
        var opts : OpenOptions; 
        opts.read = false; opts.write = true; opts.append = false; opts.create = true; opts.create_new = false; opts.truncate = true; opts.binary = true;
        var fo = file_open(path, opts);
        if(fo is Result.Err) {
            var Err(e) = fo else unsafe { unreachable }; return Result.Err(e);
        }
        var Ok(f) = fo else unsafe { unreachable };
        var r = file_write_all(&raw mut f, data, data_len);
        if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; file_close(&raw mut f); return Result.Err(e); }
        file_close(&raw mut f);
        return Result.Ok(UnitTy{});

        }
    }

public func move_path(src : *char, dst : *char) : Result<UnitTy, FsError> {
        unsafe {
        if(src == null || dst == null) { return Result.Err(FsError.InvalidInput()); }
        var r = unsafe(rename(src, dst));
        if(r == 0) { return Result.Ok(UnitTy{}); }
        // if rename fails (e.g. cross-device) fallback to copy+remove
        var src_meta = metadata(src);
        if(src_meta is Result.Err) {
            var Err(e) = src_meta else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(m) = src_meta else unsafe { unreachable }
        if(m.is_dir) {
            var c = copy_directory(src, dst, true);
            if(c is Result.Err) {
                var Err(e) = c else unsafe { unreachable }
                return Result.Err(e)
            }
            var rem = remove_dir_all_recursive(src);
            if(rem is Result.Err) {
                var Err(e) = rem else unsafe { unreachable }
                return Result.Err(e)
            }
            return Result.Ok(UnitTy{});
        } else {
            var c = copy_file(src, dst);
            if(c is Result.Err) {
                var Err(e) = c else unsafe { unreachable }
                return Result.Err(e)
            }
            var rem = remove_file(src);
            if(rem is Result.Err) {
                var Err(e) = rem else unsafe { unreachable }
                return Result.Err(e)
            }
            return Result.Ok(UnitTy{});
        }

        }
    }

// ---- from lang/libs/fs/src/directory.ch ----
public func copy_directory(src : *char, dst : *char, preserve_metadata : bool) : Result<UnitTy, FsError> {
        unsafe {
        // recursive copy: create dest dir then iterate src and copy files/dirs
        var st_res = metadata(src);
        if(st_res is Result.Err) {
            var Err(e) = st_res else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(st) = st_res else unsafe { unreachable };
        if(!st.is_dir) { return Result.Err(FsError.NotADirectory()); }
        // create dst
        var cd = create_dir_all(dst);
        if(cd is Result.Err) {
            var Err(e) = cd else unsafe { unreachable }
            return Result.Err(e)
        }
        // iterate src
        var res = read_dir(src, |src, dst, preserve_metadata|(name : *char, name_len : size_t, is_dir : bool) => {
            // skip . and ..
            if(name_len == 1 && name[0] == '.') { return true; }
            if(name_len == 2 && name[0] == '.' && name[1] == '.') { return true; }
            var srcchild : [PATH_MAX_BUF]char;
            var dstchild : [PATH_MAX_BUF]char;
            var p : size_t = 0; while(p < PATH_MAX_BUF as size_t && src[p] != 0) { srcchild[p] = src[p]; p++ }
            if(p == PATH_MAX_BUF as size_t) { return false; }
            if(p > 0 && srcchild[p-1] != '/') {
                if(p + 1 >= PATH_MAX_BUF as size_t) { return false; }
                srcchild[p++] = '/';
            }
            if(name_len >= PATH_MAX_BUF as size_t - p) { return false; }
            var q : size_t = 0; while(q <= name_len) { srcchild[p + q] = name[q]; q++ }
            var r : size_t = 0; while(r < PATH_MAX_BUF as size_t && dst[r] != 0) { dstchild[r] = dst[r]; r++ }
            if(r == PATH_MAX_BUF as size_t) { return false; }
            if(r > 0 && dstchild[r-1] != '/') {
                if(r + 1 >= PATH_MAX_BUF as size_t) { return false; }
                dstchild[r++] = '/';
            }
            if(name_len >= PATH_MAX_BUF as size_t - r) { return false; }
            q = 0; while(q <= name_len) { dstchild[r + q] = name[q]; q++ }
            if(is_dir) {
                var c = copy_directory(&raw mut srcchild[0], &raw mut dstchild[0], preserve_metadata);
                if(c is Result.Err) { var Err(e) = c else unsafe { unreachable }; return false; }
            } else {
                var c = copy_file(&raw mut srcchild[0], &raw mut dstchild[0]);
                if(c is Result.Err) { var Err(e) = c else unsafe { unreachable }; return false; }
                if(preserve_metadata) {
                    var meta = metadata(&raw mut srcchild[0]);
                    if(meta is Result.Err) {
                        var Err(e) = meta else unsafe { unreachable }
                        // TODO: cannot return the result
                        // return Result.Err<UnitTy, FsError>(e)
                        return false;
                    }
                    var Ok(m) = meta else unsafe { unreachable };
                    var setp = set_permissions(&raw mut dstchild[0], m.perms);
                    if(setp is Result.Err) { var Err(e) = setp else unsafe { unreachable }; /* non-fatal? */ }
                    var stimes = set_times(&raw mut dstchild[0], m.accessed, m.modified);
                    if(stimes is Result.Err) { var Err(e) = stimes else unsafe { unreachable }; }
                }
            }
            return true;
        });
        if(res is Result.Err) {
            var Err(e) = res else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(_) = res else unsafe { unreachable };
        return Result.Ok(UnitTy{});

        }
    }

// ---- from lang/libs/fs/src/metadata.ch ----
// exists / is_file / is_dir convenience
public func exists(path : *char) : bool {
    var r = metadata(path);
    return !(r is Result.Err);
}

public func is_file(path : *char) : Result<bool, FsError> {
    var r = metadata(path);
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(e); }
    var Ok(m) = r else unsafe { unreachable };
    return Result.Ok(m.is_file);
}

public func is_dir(path : *char) : Result<bool, FsError> {
    var r = metadata(path);
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(e); }
    var Ok(m) = r else unsafe { unreachable };
    return Result.Ok(m.is_dir);
}

} // namespace raw
} // namespace fs
