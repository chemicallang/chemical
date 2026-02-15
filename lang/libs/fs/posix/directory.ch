public namespace fs {

using std::Result;

public func create_dir_native(path : path_ptr) : Result<UnitTy, FsError> {
    var r = posix_mkdir(path, 0o777);
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(UnitTy{});
}

// create_dir_all (recursive)
public func create_dir_all(path : *char) : Result<UnitTy, FsError> {
    var buf : [PATH_MAX_BUF]char;
    var r = normalize_path(path, &mut buf[0], PATH_MAX_BUF as size_t);
    if(r is Result.Err) {
        var Err(e) = r else unreachable
        return Result.Err(e)
    }
    var Ok(len) = r else unreachable;
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
            var stat_res = metadata(&mut prefix[0]);
            if(stat_res is Result.Err) {
                var Err(e) = stat_res else unreachable;
                if(e is FsError.NotFound) {
                    var c = create_dir(&mut prefix[0]);
                    if(c is Result.Err) { var Err(e2) = c else unreachable; return Result.Err(e2); }
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

// remove_dir (non-recursive) and remove_dir_all (recursive)
public func remove_dir_native(path : path_ptr) : Result<UnitTy, FsError> {
    var r = rmdir(path);
    if(r != 0) { return Result.Err(posix_errno_to_fs(get_errno())); }
    return Result.Ok(UnitTy{});
}

// ---------------------------
// Helper: remove_dir_all_at(fd)
// Remove contents of directory referenced by an open directory fd.
// Does not remove the directory itself; caller should remove it by name (rmdir) or handle separately.
// ---------------------------
func remove_dir_all_at(dirfd : int) : Result<UnitTy, FsError> {
    // Duplicate fd for fdopendir (fdopendir usually consumes the fd)
    var ddup = dup(dirfd);
    if(ddup < 0) {
        var e = get_errno();
        // printf("dup(%d) failed: errno=%d (%s)\n", dirfd, e, strerror(e));
        return Result.Err(posix_errno_to_fs(e));
    }

    var dir = fdopendir(ddup);
    if(dir == null) {
        var e = get_errno();
        // printf("fdopendir(%d) failed: errno=%d (%s)\n", ddup, e, strerror(e));
        close(ddup);
        return Result.Err(posix_errno_to_fs(e));
    }

    loop {
        // IMPORTANT: clear errno before calling readdir so a leftover errno doesn't confuse us
        set_errno(0);

        var ent = readdir(dir);
        if(ent == null) {
            if(get_errno() != 0) {
                var re = get_errno();
                // printf("readdir(dirfd=%d) returned NULL with errno=%d (%s)\n", dirfd, re, strerror(re));
                closedir(dir);
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
        if(fstatat(dirfd, &ent.d_name[0], &mut st, AT_SYMLINK_NOFOLLOW) != 0) {
            var e = get_errno();
            // printf("fstatat(dirfd=%d, name='%s') -> errno=%d (%s)\n", dirfd, &ent.d_name[0], e, strerror(e));
            if(e == 2) { // ENOENT
                // printf("  (ENOENT) entry disappeared between readdir and fstatat — skipping\n");
                continue;
            } else {
                closedir(dir);
                return Result.Err(posix_errno_to_fs(e));
            }
        }

        // Directory? recurse via openat
        if((st.st_mode & S_IFMT) == S_IFDIR) {
            // printf("  '%s' is DIR — openat\n", &ent.d_name[0]);

            set_errno(0);
            var childfd = openat(dirfd, &ent.d_name[0], O_RDONLY | O_DIRECTORY, 0);
            if(childfd < 0) {
                var e2 = get_errno();
                // printf("  openat(dirfd=%d, name='%s') -> errno=%d (%s)\n", dirfd, &ent.d_name[0], e2, strerror(e2));
                if(e2 == 2) {
                    // printf("  (ENOENT) was removed before openat; skipping\n");
                    continue;
                } else {
                    closedir(dir);
                    return Result.Err(posix_errno_to_fs(e2));
                }
            }

            // Recurse on the child's dirfd
            // printf("  recursing into fd %d\n", childfd);
            var rem = remove_dir_all_at(childfd);

            // Always close the child fd we opened
            close(childfd);

            if(rem is Result.Err) { var Err(e) = rem else unreachable; closedir(dir); return Result.Err(e); }

            // Remove the directory entry itself using unlinkat(AT_REMOVEDIR)
            set_errno(0);
            if(unlinkat(dirfd, &ent.d_name[0], AT_REMOVEDIR) != 0) {
                var e3 = get_errno();
                // printf("  unlinkat(AT_REMOVEDIR) dirfd=%d name='%s' -> errno=%d (%s)\n", dirfd, &ent.d_name[0], e3, strerror(e3));
                if(e3 == 2) {
                    // printf("  (ENOENT) entry gone before unlinkat; benign\n");
                    continue;
                } else {
                    closedir(dir);
                    return Result.Err(posix_errno_to_fs(e3));
                }
            } else {
                // printf("  unlinkat(AT_REMOVEDIR) succeeded for '%s'\n", &ent.d_name[0]);
            }

        } else {
            // Not a directory: remove with unlinkat
            set_errno(0);
            if(unlinkat(dirfd, &ent.d_name[0], 0) != 0) {
                var e4 = get_errno();
                // printf("  unlinkat(dirfd=%d, name='%s') -> errno=%d (%s)\n", dirfd, &ent.d_name[0], e4, strerror(e4));
                if(e4 == 2) {
                    // printf("  (ENOENT) already removed — skipping\n");
                    continue;
                } else {
                    closedir(dir);
                    return Result.Err(posix_errno_to_fs(e4));
                }
            } else {
                // printf("  unlinkat succeeded for '%s'\n", &ent.d_name[0]);
            }
        }
    }

    closedir(dir);
    // printf("remove_dir_all_at(dirfd=%d) completed OK\n", dirfd);
    return Result.Ok(UnitTy{});
}

// -----------------------------
// Recursive remove_dir_all (public)
// -----------------------------
public func remove_dir_all_recursive_native(path : path_ptr) : Result<UnitTy, FsError> {
    // POSIX
    // printf("remove_dir_all_recursive: opening '%s'\n", path);
    var dirfd = open(path, O_RDONLY | O_DIRECTORY, 0);
    if(dirfd < 0) {
        var e = get_errno();
        // printf("open('%s') -> errno=%d (%s)\n", path, e, strerror(e));
        return Result.Err(posix_errno_to_fs(e));
    }

    var rem = remove_dir_all_at(dirfd);

    // close dirfd regardless
    var c = close(dirfd);

    if(rem is Result.Err) { var Err(e) = rem else unreachable; return Result.Err(e); }

    // remove the directory itself (rmdir)
    set_errno(0);
    if(rmdir(path) != 0) {
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

public func temp_dir(out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var tmp : *char = "/tmp\0";
    var i : size_t = 0; while(tmp[i] != 0) { out[i] = tmp[i]; i++ }
    out[i] = 0;
    return Result.Ok(i);
}

// read_dir: callback style to avoid allocations. Callback signature: fn(name : *char, name_len : size_t, is_dir : bool) -> bool
public func read_dir(path : *char, callback : std::function<(name : *char, name_len : size_t, is_dir : bool) => bool>) : Result<UnitTy, FsError> {
    var d = opendir(path);
    if(d == null) { return Result.Err(posix_errno_to_fs(get_errno())); }
    while(true) {
        var ent = readdir(d);
        if(ent == null) { break; }
        var name_ptr = &ent.d_name[0];
        var nl : size_t = 0; while(name_ptr[nl] != 0) { nl++ }
        var isdir : bool = false;
        // TODO:
        // if(def.HAVE_DIRENT_D_TYPE)
        // isdir = (ent.d_type == DT_DIR);
        // else {
        // fallback: stat child
        var child : [PATH_MAX_BUF]char;
        var p : size_t = 0; while(path[p] != 0) { child[p] = path[p]; p++ }
        if(p > 0 && child[p-1] != '/') { child[p++] = '/'; }
        var q : size_t = 0; while(q <= nl) { child[p + q] = name_ptr[q]; q++ }
        var st : Stat;
        var r = lstat(&child[0], &mut st);
        if(r == 0) { isdir = ((st.st_mode & 0xF000) == 0x4000); }
        // }
        var cont = callback(name_ptr, nl, isdir);
        if(!cont) { break; }
    }
    closedir(d);
    return Result.Ok(UnitTy{});
}

public func create_dir(path : *char) : Result<UnitTy, FsError> {
    return create_dir_native(path);
}

public func remove_dir(path : *char) : Result<UnitTy, FsError> {
    return remove_dir_native(path)
}

public func remove_dir_all_recursive(path : *char) : Result<UnitTy, FsError> {
    return remove_dir_all_recursive_native(path)
}

public func mkdir(pathname : *char) : int {
    return posix_mkdir(pathname, PermissionMode.S_IRWXU as uint)
}

}