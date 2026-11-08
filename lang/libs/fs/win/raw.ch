// fs::raw — the complete low-level filesystem surface.
//
// Nested namespaces cannot be reopened across files, so every raw entry point
// lives in this single namespace. The canonical safe surface is in `fs` itself
// (`File`, `read`, `write`, `metadata`, `read_dir`, ...).
public namespace fs {

public namespace raw {

using std::Result;
using std::string;
using std::string_view;
using std::vector;


// ---- from lang/libs/fs/win/directory.ch ----
public func create_dir_native(path : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    var ok = CreateDirectoryW(path as LPCWSTR, null);
    if(ok == 0) {
        var e = GetLastError();
        return Result.Err(winerr_to_fs(e as int));
    }
    return Result.Ok(UnitTy{});

        }}

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

        }}

// remove_dir (non-recursive) and remove_dir_all (recursive)
public func remove_dir_native(path : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    var ok = RemoveDirectoryW(path as LPCWSTR);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});

        }}

struct CustomDeletor<T> {
    var _value : *mut T
    @make
    func make(value: *mut T) {
        unsafe {
        return CustomDeletor<T> {
            _value : value
        }

        }}
    @delete
    func delete(&self) {
        unsafe {
        // destruct + free
        delete _value;

        }}
}

// -----------------------------
// Recursive remove_dir_all (public)
// -----------------------------
public func remove_dir_all_recursive_native(path : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    // Build pattern path\* for enumeration
    var search_ptr = malloc(sizeof(u16) * WIN_MAX_PATH) as *mut u16
    if (search_ptr == null) {
        return Result.Err(winerr_to_fs(8 as int)); // ERROR_NOT_ENOUGH_MEMORY
    }
    var deletor = CustomDeletor<u16>(search_ptr)

    var p : size_t = 0;
    while(p < WIN_MAX_PATH as size_t && path[p] != 0) {
        search_ptr[p] = path[p]; p += 1;
    }
    if(p == WIN_MAX_PATH as size_t) {
        return Result.Err(FsError.PathTooLong());
    }
    var search_sep : size_t = 0;
    if(p > 0) {
        var last = search_ptr[p - 1];
        if(last != '\\' as u16 && last != '/' as u16) { search_sep = 1; }
    }
    if(p + search_sep + 2 > WIN_MAX_PATH as size_t) {
        return Result.Err(FsError.PathTooLong());
    }
    if(search_sep != 0) { search_ptr[p] = '\\' as u16; p += 1; }
    search_ptr[p] = '*' as u16; p += 1;
    search_ptr[p] = 0;

    var finddata_ptr = malloc(sizeof(WIN32_FIND_DATAW)) as *mut WIN32_FIND_DATAW
    if (finddata_ptr == null) {
        return Result.Err(winerr_to_fs(8 as int));
    }
    var deletor2 = CustomDeletor<WIN32_FIND_DATAW>(finddata_ptr)

    var h = FindFirstFileW(search_ptr as LPCWSTR, finddata_ptr);
    if(h == INVALID_HANDLE_VALUE) {
        // If directory cannot be opened, return mapped error
        var err = GetLastError();
        return Result.Err(winerr_to_fs(err as int));
    }

    // allocate child buffer on heap (was previously on stack)
    var child_ptr = malloc(sizeof(u16) * WIN_MAX_PATH) as *mut u16;
    if (child_ptr == null) {
        FindClose(h);
        return Result.Err(winerr_to_fs(8 as int));
    }
    var child_owner = CustomDeletor<u16>(child_ptr)

    loop {
        // skip "." and ".."
        var name_w = &raw mut finddata_ptr.cFileName[0];
        if(!(name_w[0] == '.' as u16 && name_w[1] == 0) &&
           !(name_w[0] == '.' as u16 && name_w[1] == '.' as u16 && name_w[2] == 0)) {

            // copy original wide path, leaving room for the separator, name,
            // and NUL terminator
            var i : size_t = 0;
            while(i < WIN_MAX_PATH as size_t && path[i] != 0) {
                child_ptr[i] = path[i]; i += 1;
            }
            if(i == WIN_MAX_PATH as size_t) {
                FindClose(h);
                return Result.Err(FsError.PathTooLong());
            }
            var child_sep : size_t = 0;
            if(i > 0) {
                var last = child_ptr[i - 1];
                if(last != '\\' as u16 && last != '/' as u16) { child_sep = 1; }
            }
            if(i + child_sep + 2 > WIN_MAX_PATH as size_t) {
                FindClose(h);
                return Result.Err(FsError.PathTooLong());
            }
            if(child_sep != 0) { child_ptr[i] = '\\' as u16; i += 1; }
            // append name_w
            var j : size_t = 0;
            while(name_w[j] != 0) {
                if(i + j + 2 > WIN_MAX_PATH as size_t) {
                    FindClose(h);
                    return Result.Err(FsError.PathTooLong());
                }
                child_ptr[i + j] = name_w[j]; j += 1;
            }
            child_ptr[i + j] = 0;

            var is_dir2 = (finddata_ptr.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
            if(is_dir2) {
                var rem = remove_dir_all_recursive_native(child_ptr);
                if(rem is Result.Err) { var Err(e) = rem else unsafe { unreachable }; FindClose(h); return Result.Err(e); }
            } else {
                // delete file
                var del = DeleteFileW(child_ptr as LPCWSTR);
                if(del == 0) { var err = GetLastError(); FindClose(h); return Result.Err(winerr_to_fs(err as int)); }
            }
        }

        var more = FindNextFileW(h, finddata_ptr);
        if(more == 0) {
            var lasterr = GetLastError();
            if(lasterr == 18u32) { // ERROR_NO_MORE_FILES
                break;
            } else {
                FindClose(h);
                return Result.Err(winerr_to_fs(lasterr as int));
            }
        }
    }

    FindClose(h);

    // finally remove the directory itself
    var remd = RemoveDirectoryW(path as LPCWSTR);
    if(remd == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});

        }}

public func temp_dir(out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
    if(out == null || out_len == 0) { return Result.Err(FsError.InvalidInput()); }
    var wbuf : [WIN_MAX_PATH]u16;
    var n = GetTempPathW(WIN_MAX_PATH as u32, (&mut wbuf[0]) as LPWSTR);
    if(n == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    if(n >= WIN_MAX_PATH as u32) { return Result.Err(FsError.PathTooLong()); }
    var conv = utf16_to_utf8(&raw mut wbuf[0], out, out_len);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable };
        return Result.Err(e);
    }
    var Ok(len) = conv else unsafe { unreachable };
    return Result.Ok(len);

        }}

// read_dir: callback style to avoid allocations. Callback signature: fn(name : *char, name_len : size_t, is_dir : bool) -> bool
public func read_dir(path : *char, callback : std::function<(name : *char, name_len : size_t, is_dir : bool) => bool>) : Result<UnitTy, FsError> {
        unsafe {
    // Windows implementation using FindFirstFileW / FindNextFileW
    var wpath : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &raw mut wpath[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(wlen) = conv else unsafe { unreachable }
    // append \* pattern
    var pattern : [WIN_MAX_PATH]u16;
    var p : size_t = 0; while(p < WIN_MAX_PATH as size_t && wpath[p] != 0) { pattern[p] = wpath[p]; p++ }
    if(p == WIN_MAX_PATH as size_t) { return Result.Err(FsError.PathTooLong()); }
    if(p == 0) { if(p + 1 >= WIN_MAX_PATH as size_t) { return Result.Err(FsError.PathTooLong()); } pattern[p++] = '.' as u16; }
    if(p > 0 && pattern[p-1] != '\\' && pattern[p-1] != '/') {
        if(p + 1 >= WIN_MAX_PATH as size_t) { return Result.Err(FsError.PathTooLong()); }
        pattern[p++] = '\\';
    }
    if(p + 2 > WIN_MAX_PATH as size_t) { return Result.Err(FsError.PathTooLong()); }
    pattern[p] = '*'; pattern[p+1] = 0;
    var findData : WIN32_FIND_DATAW;
    var h = FindFirstFileW((&mut pattern[0]) as LPCWSTR, &raw mut findData);
    if(h == INVALID_HANDLE_VALUE) {
        var e = GetLastError();
        if(e == ERROR_FILE_NOT_FOUND) { return Result.Ok(UnitTy{}); } // empty directory
        return Result.Err(winerr_to_fs(e as int));
    }
    while(true) {
        // convert name to utf8
        var name_utf8 : [DIR_ENT_NAME_MAX]char;
        var conv2 = utf16_to_utf8(&raw mut findData.cFileName[0], &raw mut name_utf8[0], DIR_ENT_NAME_MAX as size_t);
        if(conv2 is Result.Err) {
            FindClose(h);
            var Err(e) = conv2 else unsafe { unreachable }
            return Result.Err(e)
        }
        var Ok(nlen) = conv2 else unsafe { unreachable }
        var is_dir2 : bool = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
        var cont = callback(&raw mut name_utf8[0], nlen, is_dir2);
        if(!cont) { FindClose(h); return Result.Ok(UnitTy{}); }
        var ok = FindNextFileW(h, &raw mut findData);
        if(ok == 0) { var err = GetLastError(); if(err == ERROR_NO_MORE_FILES) { break; } FindClose(h); return Result.Err(winerr_to_fs(err as int)); }
    }
    FindClose(h);
    return Result.Ok(UnitTy{});

        }}

public func create_dir(path : *char) : Result<UnitTy, FsError> {
        unsafe {
    var w : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &raw mut w[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable }
        return Result.Err(e)
    }
    return create_dir_native(&raw mut w[0]);

        }}

public func remove_dir(path : *char) : Result<UnitTy, FsError> {
        unsafe {
    var w : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &raw mut w[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable }
        return Result.Err(e)
    }
    return remove_dir_native(&raw mut w[0])

        }}

public func remove_dir_all_recursive(path : *char) : Result<UnitTy, FsError> {
        unsafe {
    var wbuf : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16_inplace(path, &raw mut wbuf[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) { var Err(e) = conv else unsafe { unreachable }; return Result.Err(e); }
    return remove_dir_all_recursive_native(&raw mut wbuf[0]);

        }}

public func mkdir(pathname : *char) : int {
        unsafe {
    return _mkdir(pathname)

        }}

// ---- from lang/libs/fs/win/file_io.ch ----
func file_open_native(path : path_ptr, opts : OpenOptions) : Result<File, FsError> {
        unsafe {
    var access : u32 = 0;
    if(opts.read) { access = access | 0x80000000u32; } // GENERIC_READ
    if(opts.write) { access = access | 0x40000000u32; } // GENERIC_WRITE
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
    var f : File; f.handle = handle as i64; f.valid = true;
    return Result.Ok(f);

        }}

public func file_close_ref(f : &mut File) : Result<UnitTy, FsError> {
        unsafe {
    if(!f.valid) { return Result.Ok(UnitTy{}); }
    var ok = CloseHandle((f.handle as *mut void));
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    f.valid = false;
    return Result.Ok(UnitTy{});
        }}
public func file_close(f : *mut File) : Result<UnitTy, FsError> {
        unsafe {
    if(f == null) { return Result.Err(FsError.InvalidInput()); }
    if(!f.valid) { return Result.Ok(UnitTy{}); }
    var ok = CloseHandle((f.handle as *mut void));
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    f.valid = false;
    return Result.Ok(UnitTy{});

        }}

public func file_read(f : *mut File, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
        unsafe {
    if(f == null || !f.valid || (buf == null && buf_len != 0)) { return Result.Err(FsError.InvalidInput()); }
    if(buf_len == 0) { return Result.Ok(0 as size_t); }
    var read_out : u32 = 0;
    var ok = ReadFile((f.handle as *mut void), buf as *mut void, buf_len as u32, (&mut read_out) as *mut DWORD, null);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(read_out as size_t);

        }}

public func file_write(f : *mut File, buf : *u8, buf_len : size_t) : Result<size_t, FsError> {
        unsafe {
    if(f == null || !f.valid || (buf == null && buf_len != 0)) { return Result.Err(FsError.InvalidInput()); }
    if(buf_len == 0) { return Result.Ok(0 as size_t); }
    var written : u32 = 0;
    var ok = WriteFile((f.handle as *mut void), buf as *void, buf_len as u32, (&mut written) as *mut DWORD, null);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(written as size_t);

        }}

public func file_flush(f : *mut File) : Result<UnitTy, FsError> {
        unsafe {
    if(f == null || !f.valid) { return Result.Err(FsError.InvalidInput()); }
    var ok = FlushFileBuffers((f.handle as *mut void));
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(UnitTy{});

        }}

public func remove_file_native(path : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    var deleted = DeleteFileW(path as LPCWSTR);
    if(deleted == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});

        }}

func set_times_native(path : path_ptr, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
        unsafe {
    var h = CreateFileW(path as LPCWSTR, 0x40000000 /*GENERIC_WRITE*/, 0, null, 3 /*OPEN_EXISTING*/, 0, null);
    if(h == INVALID_HANDLE_VALUE) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    var cft = unix_to_filetime(0); // unused creation
    var aft = unix_to_filetime(atime);
    var mft = unix_to_filetime(mtime);
    var ok = SetFileTime(h, &raw cft, &raw aft, &raw mft);
    CloseHandle(h);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(UnitTy{});

        }}

func copy_file_native(src : path_ptr, dst : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
    var ok = CopyFileW(src as LPCWSTR, dst as LPCWSTR, 0); // overwrite
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});

        }}

func create_temp_file_in_native(dir : path_ptr, prefix : path_ptr, out_path : mut_path_ptr, fh : *mut File) : Result<UnitTy, FsError> {
        unsafe {
    var res = GetTempFileNameW(dir as LPCWSTR, prefix as LPCWSTR, 0, out_path as LPWSTR);
    if(res == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    // open file
    var opts : OpenOptions;
    opts.read = true; opts.write = true; opts.append = false; opts.create = false; opts.create_new = false; opts.truncate = false; opts.binary = true;
    var fo = file_open_native(out_path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(f) = fo else unsafe { unreachable };
    fh.valid = f.valid; fh.handle = f.handle;
    return Result.Ok(UnitTy{});

        }}

public func remove_file(path : *char) : Result<UnitTy, FsError> {
        unsafe {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    var w : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16_inplace(path, &raw mut w[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) { var Err(e) = conv else unsafe { unreachable }; return Result.Err(e); }
    return remove_file_native(&raw mut w[0])

        }}

public func copy_file(src : *char, dst : *char) : Result<UnitTy, FsError> {
        unsafe {
    if(src == null || dst == null) { return Result.Err(FsError.InvalidInput()); }
    var wsrc : [WIN_MAX_PATH]u16; var wdst : [WIN_MAX_PATH]u16;
    var r1 = utf8_to_utf16(src, &raw mut wsrc[0], WIN_MAX_PATH as size_t);
    if(r1 is Result.Err) {
        var Err(e) = r1 else unsafe { unreachable }
        return Result.Err(e)
    }
    var r2 = utf8_to_utf16(dst, &raw mut wdst[0], WIN_MAX_PATH as size_t);
    if(r2 is Result.Err) {
        var Err(e) = r2 else unsafe { unreachable }
        return Result.Err(e)
    }
    return copy_file_native(&raw mut wsrc[0], &raw mut wdst[0])

        }}

func create_temp_file_in(dir : *char, prefix : *char, out_path : *mut char, out_len : size_t, fh : *mut File) : Result<UnitTy, FsError> {
        unsafe {
    var wdir : [WIN_MAX_PATH]u16; var wprefix : [TEMP_NAME_MAX]u16; var wout : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(dir, &raw mut wdir[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unsafe { unreachable }
        return Result.Err(e)
    }
    var f2 = utf8_to_utf16(prefix, &raw mut wprefix[0], TEMP_NAME_MAX as size_t)
    if(f2 is Result.Err) {
        var Err(e) = f2 else unsafe { unreachable }
        return Result.Err(e)
    }
    var result = create_temp_file_in_native(&raw mut wdir[0], &raw mut wprefix[0], &raw mut wout[0], fh)
    if(result is Result.Err) {
        var Err(e) = result else unsafe { unreachable }
        return Result.Err(e)
    }
    var conv = utf16_to_utf8(&raw mut wout[0], out_path, out_len);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable }
        return Result.Err(e);
    }
    return result

        }}

public func file_open(path : *char, opts : OpenOptions) : Result<File, FsError> {
        unsafe {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    // Convert path to UTF-16 and call CreateFileW
    var wbuf : [WIN_MAX_PATH]u16;
    var r = utf8_to_utf16(path, &raw mut wbuf[0], WIN_MAX_PATH as size_t);
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        return Result.Err(e)
    }
    return file_open_native(&raw mut wbuf[0], opts)

        }}

public func set_times(path : *char, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
        unsafe {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    var wbuf : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &raw mut wbuf[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable }
        return Result.Err(e)
    }
    return set_times_native(&raw mut wbuf[0], atime, mtime)

        }}


/// Seek an open file. `whence` is one of 0=start, 1=current, 2=end.
public func file_seek(f : *mut File, offset : i64, whence : int) : Result<u64, FsError> {
        unsafe {
    if(f == null || !f.valid) { return Result.Err(FsError.InvalidInput()); }
    var distance : LARGE_INTEGER = zeroed<LARGE_INTEGER>();
    distance.QuadPart = offset as LONGLONG;
    var newpos : LARGE_INTEGER = zeroed<LARGE_INTEGER>();
    var method : DWORD = FILE_BEGIN;
    if(whence == 1) { method = FILE_CURRENT; }
    if(whence == 2) { method = FILE_END; }
    var ok = SetFilePointerEx((f.handle as *mut void), distance, &raw mut newpos, method);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(newpos.QuadPart as u64);

        }}

/// Metadata for an open file handle (size and regular-file flag).
public func file_metadata(f : *mut File) : Result<Metadata, FsError> {
        unsafe {
    if(f == null || !f.valid) { return Result.Err(FsError.InvalidInput()); }
    var size : LARGE_INTEGER = zeroed<LARGE_INTEGER>();
    var ok = GetFileSizeEx((f.handle as *mut void), &raw mut size);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    var m : Metadata;
    m.is_file = true;
    m.is_dir = false;
    m.is_symlink = false;
    m.len = size.QuadPart as size_t;
    m.modified = 0;
    m.accessed = 0;
    m.created = 0;
    m.perms = 0;
    return Result.Ok(m);

        }}

// ---- from lang/libs/fs/win/metadata.ch ----
func metadata_native(path : path_ptr) : Result<Metadata, FsError> {
        unsafe {
    if(path == null) { return Result.Err(FsError.InvalidInput()); }
    var fi : WIN32_FILE_ATTRIBUTE_DATA;
    var ok = GetFileAttributesExW(path as LPCWSTR, GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard, &raw mut fi);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    var m : Metadata;
    var attrs = fi.dwFileAttributes;
    m.is_dir = ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
    m.is_file = !m.is_dir;
    m.is_symlink = ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
    // convert LARGE_INTEGER FILETIME to epoch seconds (simplified)
    m.len = (((fi.nFileSizeHigh as u64) << 32) | (fi.nFileSizeLow as u64)) as size_t;
    m.modified = filetime_to_unix(fi.ftLastWriteTime);
    m.accessed = filetime_to_unix(fi.ftLastAccessTime);
    m.created = filetime_to_unix(fi.ftCreationTime);
    m.perms = attrs as u32;
    return Result.Ok(m);

        }}

public func metadata(path : *char) : Result<Metadata, FsError> {
        unsafe {
    var wbuf : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &raw mut wbuf[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable }
        return Result.Err(e)
    }
    return metadata_native(&raw mut wbuf[0])

        }}

// ---- from lang/libs/fs/win/path.ch ----
func canonicalize(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
    // Windows: GetFullPathNameW
    var wtmp : [WIN_MAX_PATH]u16;
    var r = utf8_to_utf16(path, &raw mut wtmp[0], WIN_MAX_PATH as size_t);
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(wlen) = r else unsafe { unreachable }
    var wout : [WIN_MAX_PATH]u16;
    // extern func GetFullPathNameW(lpFileName : *WCHAR, nBufferLength : DWORD, lpBuffer : *WCHAR, lpFilePart : *mut *WCHAR) : DWORD;
    var res = GetFullPathNameW((&mut wtmp[0]) as LPCWSTR, WIN_MAX_PATH as u32, (&mut wout[0]) as LPWSTR, null);
    if(res == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    var conv = utf16_to_utf8(&raw mut wout[0], out, out_len);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(n) = conv else unsafe { unreachable }
    return Result.Ok(n);

        }}

// ---- from lang/libs/fs/win/permission.ch ----
public func set_permissions_native(path : path_ptr, perms : u32) : Result<UnitTy, FsError> {
        unsafe {
    // map perms -> FILE_ATTRIBUTE_READONLY
    var attrs : u32 = 0;
    if((perms & 0x200) != 0) { attrs = attrs | FILE_ATTRIBUTE_READONLY; } // owner write bit cleared -> readonly
    var ok = SetFileAttributesW(path as LPCWSTR, attrs);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(UnitTy{});

        }}

public func set_permissions(path : *char, perms : u32) : Result<UnitTy, FsError> {
        unsafe {
    // map readonly bit
    var wbuf : [WIN_MAX_PATH]u16;
    var conv = utf8_to_utf16(path, &raw mut wbuf[0], WIN_MAX_PATH as size_t);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable }
        return Result.Err(e)
    }
    return set_permissions_native(&raw mut wbuf[0], perms)

        }}

// ---- from lang/libs/fs/win/link.ch ----
// hard link / symlink
func create_hard_link_native(existing : path_ptr, newpath : path_ptr) : Result<UnitTy, FsError> {
        unsafe {
    var ok = CreateHardLinkW(newpath as LPCWSTR, existing as LPCWSTR, null);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});

        }}

func create_symlink_native(target : path_ptr, linkpath : path_ptr, dir : bool) : Result<UnitTy, FsError> {
        unsafe {
    var flags : u32 = 0;
    if(dir) { flags = 1; } // SYMBOLIC_LINK_FLAG_DIRECTORY
    var ok = CreateSymbolicLinkW(linkpath as LPCWSTR, target as LPCWSTR, flags);
    if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    return Result.Ok(UnitTy{});

        }}

func read_link_native(path : path_ptr, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
    var h = CreateFileW(path as LPCWSTR, 0, 0, null, 3 /*OPEN_EXISTING*/, (FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS) as DWORD, null);
    if(h == INVALID_HANDLE_VALUE) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    var wout : [WIN_MAX_PATH]u16;
    var n = GetFinalPathNameByHandleW(h, (&mut wout[0]) as LPWSTR, WIN_MAX_PATH as u32, 0);
    CloseHandle(h);
    if(n == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
    var conv = utf16_to_utf8(&raw mut wout[0], out, out_len);
    if(conv is Result.Err) {
        var Err(e) = conv else unsafe { unreachable };
        return Result.Err(e);
    }
    var Ok(rn) = conv else unsafe { unreachable }
    return Result.Ok(rn);

        }}

func is_symlink(path : *char) : Result<bool, FsError> {
        unsafe {
    var m = metadata(path);
    if(m is Result.Err) {
        var Err(e) = m else unsafe { unreachable };
        return Result.Err(e);
    }
    var Ok(md) = m else unsafe { unreachable };
    return Result.Ok(md.is_symlink);

        }}

func create_symlink(target : *char, linkpath : *char, dir : bool) : Result<UnitTy, FsError> {
        unsafe {
    var wtarget : [WIN_MAX_PATH]u16; var wlink : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(target, &raw mut wtarget[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unsafe { unreachable }
        return Result.Err(e)
    }
    var f2 = utf8_to_utf16(linkpath, &raw mut wlink[0], WIN_MAX_PATH as size_t)
    if(f2 is Result.Err) {
        var Err(e) = f2 else unsafe { unreachable }
        return Result.Err(e)
    }
    return create_symlink_native(&raw mut wtarget[0], &raw mut wlink[0], dir)

        }}

func read_link(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
        unsafe {
    // On Windows readlink is more involved; use DeviceIoControl or GetFinalPathNameByHandle
    // Simpler approach: open file and call GetFinalPathNameByHandleW
    var wpath : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(path, &raw mut wpath[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unsafe { unreachable };
        return Result.Err(e);
    }
    return read_link_native(&raw mut wpath[0], out, out_len)

        }}

func create_hard_link(existing : *char, newpath : *char) : Result<UnitTy, FsError> {
        unsafe {
    var wexist : [WIN_MAX_PATH]u16; var wnew : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(existing, &raw mut wexist[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unsafe { unreachable }
        return Result.Err(e)
    }
    var f2 = utf8_to_utf16(newpath, &raw mut wnew[0], WIN_MAX_PATH as size_t)
    if(f2 is Result.Err) {
        var Err(e) = f2 else unsafe { unreachable }
        return Result.Err(e)
    }
    return create_hard_link_native(&raw mut wexist[0], &raw mut wnew[0])

        }}

// ---- from lang/libs/fs/win/lock.ch ----
func lock_file_shared(path : *char) : Result<File, FsError> {
        unsafe {
    var opts : OpenOptions; opts.read = true; opts.write = false;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unsafe { unreachable };
        return Result.Err(e);
    }
    var Ok(f) = fo else unsafe { unreachable };
    // use LockFileEx with LOCKFILE_FAIL_IMMEDIATELY & LOCKFILE_SHARED_LOCK
    var ok = LockFileEx((f.handle as *mut void), LOCKFILE_FAIL_IMMEDIATELY as DWORD, 0, 0xFFFFFFFF, 0xFFFFFFFF, null);
    if(ok == 0) { var e = GetLastError(); file_close(&raw mut f); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(f);

        }}

func lock_file_exclusive(path : *char) : Result<File, FsError> {
        unsafe {
    var opts : OpenOptions; opts.read = true; opts.write = true;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unsafe { unreachable };
        return Result.Err(e);
    }
    var Ok(f) = fo else unsafe { unreachable };
    var ok = LockFileEx((f.handle as *mut void), 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, null);
    if(ok == 0) { var e = GetLastError(); file_close(&raw mut f); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(f);

        }}

// ---- from lang/libs/fs/win/disk.ch ----
func disk_space_native(path : path_ptr, total_out : *mut u64, free_out : *mut u64, avail_out : *mut u64) : Result<UnitTy, FsError> {
        unsafe {
    var ok = GetDiskFreeSpaceExW(path as LPCWSTR, free_out as PULARGE_INTEGER, total_out as PULARGE_INTEGER, avail_out as PULARGE_INTEGER);
    if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
    return Result.Ok(UnitTy{});

        }}

func disk_space(path : *char, total_out : *mut u64, free_out : *mut u64, avail_out : *mut u64) : Result<UnitTy, FsError> {
        unsafe {
    var w : [WIN_MAX_PATH]u16;
    var f1 = utf8_to_utf16(path, &raw mut w[0], WIN_MAX_PATH as size_t)
    if(f1 is Result.Err) {
        var Err(e) = f1 else unsafe { unreachable };
        return Result.Err(e);
    }
    return disk_space_native(&raw mut w[0], total_out, free_out, avail_out)

        }}

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
