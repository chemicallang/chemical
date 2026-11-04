public namespace fs {

using std::Result;

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

}