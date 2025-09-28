// fs.fs    -- filesystem library for the language
// Designed: cross-platform (POSIX + Windows), no heap allocations (caller buffers / stack buffers), uses Option/Result
// You supply the OS-specific externs and structs listed below (see comment block "OS-SPECIFIC EXTERNS & STRUCTS").

// Result<void, MyError> doesn't work
// because we can't contain the void type in our struct
// so we are going to use a Unit Type
public struct UnitTy {}
// TODO: top level struct values in constants not yet supported
// public comptime const Unit = UnitTy {}

// TODO: not yet supported to be inside namespace fs
public comptime const PATH_MAX_BUF = 4096;     // max path buffer for POSIX-style
public comptime const WIN_MAX_PATH = 32768;    // wide path limit for Windows (extended)
public comptime const SMALL_STACK_BUF = 4096;  // general-purpose stack buffer
public comptime const COPY_CHUNK = 64 * 1024;  // 64 KB copy chunk

public namespace fs {

using std::Result;

// ----------------------
// Configuration constants
// ----------------------
public comptime const PATH_MAX_BUF = 4096;     // max path buffer for POSIX-style
public comptime const WIN_MAX_PATH = 32768;    // wide path limit for Windows (extended)
public comptime const SMALL_STACK_BUF = 4096;  // general-purpose stack buffer
public comptime const COPY_CHUNK = 64 * 1024;  // 64 KB copy chunk

// ----------------------
// OS-SPECIFIC EXTERNS & STRUCTS YOU MUST ADD (fill these in for your platform)
//
// POSIX (examples you should provide)
// -----------------------------------
// // types & functions:
// struct stat { ... }                 // POSIX stat struct
// func stat(path : *char, buf : *mut stat) : int;
// func lstat(path : *char, buf : *mut stat) : int;
// func fstat(fd : int, buf : *mut stat) : int;
// func open(path : *char, flags : int, mode : int) : int;
// func close(fd : int) : int;
// func read(fd : int, buf : *mut void, count : size_t) : isize;
// func write(fd : int, buf : *const void, count : size_t) : isize;
// func pread(fd : int, buf : *mut void, count : size_t, offset : long) : isize; // optional
// func pwrite(fd : int, buf : *const void, count : size_t, offset : long) : isize; // optional
// func mkdir(path : *char, mode : int) : int;
// func rmdir(path : *char) : int;
// func unlink(path : *char) : int;
// func rename(oldp : *char, newp : *char) : int;
// func opendir(path : *char) : *DIR;            // return DIR*
// func readdir(d : *DIR) : *dirent;             // return dirent*
// func closedir(d : *DIR) : int;
// func symlink(target : *char, linkpath : *char) : int;
// func link(existing : *char, newpath : *char) : int;
// func chmod(path : *char, mode : int) : int;
// func chown(path : *char, uid : int, gid : int) : int;
// func utimensat(...) or utimes(...) etc.
// // errno access:
// extern int errno; or func __errno_location() : *int;
//
// Windows (examples you should provide)
// -----------------------------------
// // types:
// public type WCHAR = u16;
// public type HANDLE = void*;
// public type DWORD = u32;
// public type BOOL = int;
// // functions:
// @dllimport @stdcall @extern func CreateFileW(name : *WCHAR, access : DWORD, share : DWORD, sa : void*, creation : DWORD, flags : DWORD, template : void*) : HANDLE;
// @dllimport @stdcall @extern func ReadFile(h : HANDLE, buf : *mut void, n : DWORD, read : *mut DWORD, overlapped : void*) : BOOL;
// @dllimport @stdcall @extern func WriteFile(h : HANDLE, buf : *const void, n : DWORD, written : *mut DWORD, overlapped : void*) : BOOL;
// @dllimport @stdcall @extern func CloseHandle(h : HANDLE) : BOOL;
// @dllimport @stdcall @extern func GetLastError() : DWORD;
// @dllimport @stdcall @extern func CreateDirectoryW(path : *WCHAR, sa : void*) : BOOL;
// @dllimport @stdcall @extern func RemoveDirectoryW(path : *WCHAR) : BOOL;
// @dllimport @stdcall @extern func DeleteFileW(path : *WCHAR) : BOOL;
// @dllimport @stdcall @extern func MoveFileExW(old : *WCHAR, new : *WCHAR, flags : DWORD) : BOOL;
// @dllimport @stdcall @extern func CopyFileExW(src : *WCHAR, dst : *WCHAR, progress : void*, data : void*, cancel : *bool, flags : DWORD) : BOOL;
// @dllimport @stdcall @extern func GetFileAttributesExW(name : *WCHAR, infoLevelId : DWORD, out : void*) : BOOL;
// @dllimport @stdcall @extern func GetFullPathNameW(name : *WCHAR, bufSize : DWORD, buf : *WCHAR, filePart : *mut *WCHAR) : DWORD;
// @dllimport @stdcall @extern func GetTempPathW(nBufferLength : DWORD, lpBuffer : *WCHAR) : DWORD;
// @dllimport @stdcall @extern func GetTempFileNameW(lpPathName : *WCHAR, lpPrefixString : *WCHAR, uUnique : u32, lpTempFileName : *WCHAR) : u32;
// @dllimport @stdcall @extern func CreateSymbolicLinkW(lpSymlinkFileName : *WCHAR, lpTargetFileName : *WCHAR, dwFlags : DWORD) : BOOL;
//
// Note: add any platform constants (O_RDONLY, O_WRONLY, O_CREAT, etc.) as needed.
//
// ----------------------
// End OS-specific list
// ----------------------

// ----------------------
// Types
// ----------------------
public variant FsError {
    IoError(code : int, msg : *char)
    NotFound()
    AlreadyExists()
    PermissionDenied()
    InvalidInput()
    UnsupportedPlatform()
    WouldBlock()
    PathTooLong()
    Other(msg : *char)
}

public struct Metadata {
    var is_file : bool;
    var is_dir : bool;
    var is_symlink : bool;
    var len : size_t;
    var modified : i64; // seconds since epoch
    var accessed : i64;
    var created : i64;
    var perms : u32;    // platform-specific permission bits
}

public struct OpenOptions {
    var read : bool;
    var write : bool;
    var append : bool;
    var create : bool;
    var truncate : bool;
    var binary : bool; // hint
}

public struct File {
    union {
        struct { var fd : int; } unix;
        struct { var handle : void*; } win;
    }
    var is_valid : bool;
}

// ----------------------
// Helpers: error mapping
// ----------------------
func errno_to_fs(errno_val : int) : FsError {
    if(errno_val == 2) { return FsError.NotFound(); }        // ENOENT
    if(errno_val == 17) { return FsError.AlreadyExists(); } // EEXIST
    if(errno_val == 13) { return FsError.PermissionDenied(); } // EACCES
    if(errno_val == 11) { return FsError.WouldBlock(); }    // EAGAIN/EWOULDBLOCK
    return FsError.IoError(errno_val, "posix io error\0");
}

func winerr_to_fs(code : int) : FsError {
    // Map common Win32 error codes (user can extend)
    if(code == 2) { return FsError.NotFound(); }        // ERROR_FILE_NOT_FOUND
    if(code == 5) { return FsError.PermissionDenied(); } // ERROR_ACCESS_DENIED
    if(code == 80) { return FsError.AlreadyExists(); }  // ERROR_FILE_EXISTS
    return FsError.IoError(code, "win32 error\0");
}

// ----------------------
// Path utilities (no allocations; caller buffers)
// ----------------------
// basename: gets last component
func basename(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var len : size_t = 0;
    while(path[len] != 0) { len++ }
    if(len == 0) {
        if(out_len < 2) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        out[0] = '.'; out[1] = 0; return Result.Ok<size_t, FsError>(1);
    }
    // skip trailing separators
    var i : size_t = len;
    while(i > 0 && (path[i-1] == '/' || path[i-1] == '\\')) { i-- }
    if(i == 0) {
        // path was all separators -> return "/"
        if(out_len < 2) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        out[0] = '/'; out[1] = 0; return Result.Ok<size_t, FsError>(1);
    }
    // find last separator before i
    var j : size_t = i;
    while(j > 0 && path[j-1] != '/' && path[j-1] != '\\') { j-- }
    var comp_len = i - j;
    if(comp_len + 1 > out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    var k : size_t = 0;
    while(k < comp_len) { out[k] = path[j + k]; k++ }
    out[k] = 0;
    return Result.Ok<size_t, FsError>(comp_len);
}

// dirname: everything before last component (or "." if none)
func dirname(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var len : size_t = 0;
    while(path[len] != 0) { len++ }
    if(len == 0) {
        if(out_len < 2) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        out[0] = '.'; out[1] = 0; return Result.Ok<size_t, FsError>(1);
    }
    // remove trailing separators
    var i : size_t = len;
    while(i > 0 && (path[i-1] == '/' || path[i-1] == '\\')) { i-- }
    if(i == 0) {
        // all separators -> "/"
        if(out_len < 2) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        out[0] = '/'; out[1] = 0; return Result.Ok<size_t, FsError>(1);
    }
    // find last separator before i
    var j : size_t = i;
    while(j > 0 && path[j-1] != '/' && path[j-1] != '\\') { j-- }
    if(j == 0) {
        // no separator
        if(out_len < 2) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        out[0] = '.'; out[1] = 0; return Result.Ok<size_t, FsError>(1);
    }
    // remove trailing separators from the left part
    var end = j;
    while(end > 0 && (path[end-1] == '/' || path[end-1] == '\\')) { end-- }
    if(end == 0) { // root
        if(out_len < 2) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        out[0] = '/'; out[1] = 0; return Result.Ok<size_t, FsError>(1);
    }
    // copy [0..end)
    if(end + 1 > out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    var k : size_t = 0;
    while(k < end) { out[k] = path[k]; k++ }
    out[k] = 0;
    return Result.Ok<size_t, FsError>(end);
}

// extension (returns extension starting at '.' or empty)
func extension(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var base_buf : [PATH_MAX_BUF]char;
    var r = basename(path, &mut base_buf[0], PATH_MAX_BUF as size_t);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err<size_t, FsError>(e); }
    var Ok(base_len) = r else unreachable;
    var i : size_t = base_len;
    while(i > 0 && base_buf[i-1] != '.') { i-- }
    if(i == 0) {
        if(out_len < 1) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        out[0] = 0; return Result.Ok<size_t, FsError>(0);
    }
    var ext_len = base_len - i;
    if(ext_len + 1 > out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    var k : size_t = 0;
    while(k < ext_len) { out[k] = base_buf[i + k]; k++ }
    out[k] = 0;
    return Result.Ok<size_t, FsError>(ext_len);
}

// join path a + b -> out (normalizes separators). Caller must provide out buffer.
func join_path(a : *char, b : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var a_len : size_t = 0;
    while(a[a_len] != 0) { a_len++ }
    var b_len : size_t = 0;
    while(b[b_len] != 0) { b_len++ }
    // handle empty a
    if(a_len == 0) {
        if(b_len + 1 > out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        var i : size_t = 0;
        while(i <= b_len) { out[i] = b[i]; i++ } // include null
        return Result.Ok<size_t, FsError>(b_len);
    }
    var need_sep : bool = false;
    if(a[a_len - 1] != '/' && a[a_len - 1] != '\\') { need_sep = true; }
    var total = a_len + (if(need_sep) 1 else 0) + b_len;
    if(total + 1 > out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    var pos : size_t = 0;
    var i : size_t = 0;
    while(i < a_len) { out[pos++] = a[i++]; }
    if(need_sep) { out[pos++] = '/'; } // normalize to '/'
    i = 0;
    while(i <= b_len) { out[pos++] = b[i++]; } // copy including null
    return Result.Ok<size_t, FsError>(total);
}

// normalize_path: resolve "." and ".." (does not resolve symlinks). Uses stack arrays for components.
func normalize_path(path_in : *char, out_buf : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var len : size_t = 0;
    while(path_in[len] != 0) { len++ }
    // component arrays (stack-allocated)
    const MAX_COMPS = 256;
    var comp_offs : [MAX_COMPS]size_t;
    var comp_lens : [MAX_COMPS]size_t;
    var comps_count : size_t = 0;
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
            if(comps_count > 0) { comps_count -= 1; }
            else {
                if(comps_count >= MAX_COMPS) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
                comp_offs[comps_count] = start;
                comp_lens[comps_count] = c_len;
                comps_count++;
            }
        } else {
            if(comps_count >= MAX_COMPS) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            comp_offs[comps_count] = start;
            comp_lens[comps_count] = c_len;
            comps_count++;
        }
    }
    if(comps_count == 0) {
        // empty -> "."
        if(out_len < 2) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        out_buf[0] = '.';
        out_buf[1] = 0;
        return Result.Ok<size_t, FsError>(1);
    }
    var pos : size_t = 0;
    var j : size_t = 0;
    while(j < comps_count) {
        if(j > 0) {
            if(pos + 1 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_buf[pos] = '/'; pos++;
        }
        var off = comp_offs[j];
        var c_l = comp_lens[j];
        if(pos + c_l >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
        var k : size_t = 0;
        while(k < c_l) { out_buf[pos + k] = path_in[off + k]; k++ }
        pos += c_l; j++
    }
    if(pos >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    out_buf[pos] = 0;
    return Result.Ok<size_t, FsError>(pos);
}

// canonicalize (platform-backed). Uses OS APIs; we provide interfaces and use compile-time branches.
func canonicalize(path_in : *char, out_buf : *mut char, out_len : size_t) : Result<size_t, FsError> {
    if(def.windows) {
        // Windows: use GetFullPathNameW -> user must provide extern above.
        // Convert UTF-8 path_in -> WCHAR stack buffer, call GetFullPathNameW, convert back.
        var wbuf : [WIN_MAX_PATH]u16;
        var wlen_r = utf8_to_utf16(path_in, &mut wbuf[0], WIN_MAX_PATH as size_t);
        if(wlen_r is Result.Err) { var Err(e) = wlen_r else unreachable; return Result.Err<size_t, FsError>(e); }
        var Ok(wlen) = wlen_r else unreachable;
        // call GetFullPathNameW (extern must be provided)
        // var out_w : [WIN_MAX_PATH]u16;
        // var res = GetFullPathNameW(wbuf.ptr(), WIN_MAX_PATH, out_w.ptr(), null);
        // if(res == 0) return Result.Err(winerr_to_fs(GetLastError()));
        // var ret_r = utf16_to_utf8(out_w.ptr(), out_buf, out_len);
        // return ret_r;
        return Result.Err<size_t, FsError>(FsError.UnsupportedPlatform()); // placeholder; wiring requires GetFullPathNameW extern
    } else {
        // POSIX: use realpath(3) if available or fallback to manual resolve (we will call realpath if present)
        // extern char* realpath(const char *path, char *resolved_path);
        // var res_ptr = realpath(path_in, out_buf); // requires proper extern
        // if(res_ptr == null) return Result.Err(errno_to_fs(errno));
        // return Result.Ok(strlen(out_buf));
        return Result.Err(FsError.UnsupportedPlatform()); // user should supply realpath extern
    }
}

// ----------------------
// UTF8 <-> UTF16 helpers (small, stack-based converters)
// Note: these are simple, safe converters; they avoid heap usage.
// You may replace with more optimized / complete conversions when desired.
// ----------------------
func utf8_to_utf16(in_utf8 : *char, out_w : *mut u16, out_w_len : size_t) : Result<size_t, FsError> {
    // Simple conversion assuming valid UTF-8; no heap; returns number of WCHAR written (excluding null)
    var i : size_t = 0;
    var wpos : size_t = 0;
    while(in_utf8[i] != 0) {
        var c = in_utf8[i] as u8;
        if(c < 0x80) {
            if(wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = c as u16;
            i++;
        } else if((c & 0xE0) == 0xC0) {
            // 2-byte
            var c2 = in_utf8[i+1] as u8;
            var ch = ((c & 0x1F) as u16) << 6 | ((c2 & 0x3F) as u16);
            if(wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = ch;
            i += 2;
        } else if((c & 0xF0) == 0xE0) {
            // 3-byte
            var c2 = in_utf8[i+1] as u8;
            var c3 = in_utf8[i+2] as u8;
            var ch = ((c & 0x0F) as u16) << 12 | ((c2 & 0x3F) as u16) << 6 | ((c3 & 0x3F) as u16);
            if(wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = ch;
            i += 3;
        } else {
            // 4-byte -> surrogate pair
            var c2 = in_utf8[i+1] as u8;
            var c3 = in_utf8[i+2] as u8;
            var c4 = in_utf8[i+3] as u8;
            var code = ((c & 0x07) as u32) << 18 | ((c2 & 0x3F) as u32) << 12 | ((c3 & 0x3F) as u32) << 6 | ((c4 & 0x3F) as u32);
            code -= 0x10000;
            var high = 0xD800u16 + ((code >> 10) as u16);
            var low  = 0xDC00u16 + ((code & 0x3FFu32) as u16);
            if(wpos + 2 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = high;
            out_w[wpos++] = low;
            i += 4;
        }
    }
    if(wpos >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    out_w[wpos] = 0;
    return Result.Ok<size_t, FsError>(wpos);
}

func utf16_to_utf8(in_w : *u16, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var i : size_t = 0;
    var pos : size_t = 0;
    while(true) {
        var w = in_w[i];
        if(w == 0) { break; }
        if(w < 0x80) {
            if(pos + 1 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[pos++] = (w as char);
        } else if(w < 0x800) {
            if(pos + 2 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[pos++] = (0xC0 | ((w >> 6) & 0x1F)) as char;
            out[pos++] = (0x80 | (w & 0x3F)) as char;
        } else if(w >= 0xD800 && w <= 0xDBFF) {
            // surrogate pair
            var w2 = in_w[i+1];
            var code = 0x10000 + ((((w & 0x3FF) as u32) << 10) | ((w2 & 0x3FF) as u32));
            if(pos + 4 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[pos++] = (0xF0 | ((code >> 18) & 0x07)) as char;
            out[pos++] = (0x80 | ((code >> 12) & 0x3F)) as char;
            out[pos++] = (0x80 | ((code >> 6) & 0x3F)) as char;
            out[pos++] = (0x80 | (code & 0x3F)) as char;
            i += 1; // consumed extra
        } else {
            if(pos + 3 >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out[pos++] = (0xE0 | ((w >> 12) & 0x0F)) as char;
            out[pos++] = (0x80 | ((w >> 6) & 0x3F)) as char;
            out[pos++] = (0x80 | (w & 0x3F)) as char;
        }
        i++;
    }
    if(pos >= out_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    out[pos] = 0;
    return Result.Ok<size_t, FsError>(pos);
}

// ----------------------
// File operations
// ----------------------
func file_open(path : *char, opts : OpenOptions) : Result<File, FsError> {
    if(def.windows) {
        // Convert to UTF-16, call CreateFileW (extern)
        var wbuf : [WIN_MAX_PATH]u16;
        var r = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
        if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
        var Ok(wlen) = r else unreachable;
        // Build desired access & creation flags (user to supply constants)
        var access : u32 = 0;
        if(opts.read) { access = access | 0x80000000; } // GENERIC_READ
        if(opts.write) { access = access | 0x40000000; } // GENERIC_WRITE
        var creation : u32 = 0;
        if(opts.create && opts.truncate) {
            creation = 2 /*CREATE_ALWAYS*/;
        } else if(opts.create && !opts.truncate) {
            creation = 1 /*CREATE_NEW or OPEN_ALWAYS*/; creation = 4 /*OPEN_ALWAYS*/;
        } else if(!opts.create && opts.truncate) {
            creation = 3 /*TRUNCATE_EXISTING*/;
        } else {
            creation = 3 /*OPEN_EXISTING*/; creation = 3;
        } // placeholder; user can adjust
        // Share flags: allow read/write sharing
        var share : u32 = 1u | 2u; // FILE_SHARE_READ | FILE_SHARE_WRITE
        // call CreateFileW (extern must be provided)
        // var h = CreateFileW(wbuf.ptr(), access, share, null, creation, 0, null);
        // if(h == INVALID_HANDLE_VALUE) { return Result.Err(winerr_to_fs(GetLastError() as int)); }
        return Result.Err(FsError.UnsupportedPlatform()); // placeholder until CreateFileW extern is added
    } else {
        // POSIX open
        var flags : int = 0;
        // O_RDONLY = 0, O_WRONLY = 1, O_RDWR = 2
        if(opts.read && !opts.write) { flags = 0; }
        else if(opts.read && opts.write) { flags = 2; }
        else if(!opts.read && opts.write) { flags = 1; }
        // O_CREAT etc: user must supply constants; we approximate common values but you should replace with your system constants
        const O_CREAT = 0x40;
        const O_TRUNC = 0x200;
        const O_APPEND = 0x400;
        if(opts.create) { flags = flags | O_CREAT; }
        if(opts.truncate) { flags = flags | O_TRUNC; }
        if(opts.append) { flags = flags | O_APPEND; }
        var fd = open(path, flags, 0o666);
        if(fd < 0) {
            // map errno (user must supply errno binding; here assume fd == -1 and errno available)
            // we expect the user to add an errno accessor; we fallback to returning IoError(-1)
            return Result.Err(FsError.IoError(-1, "open failed\0"));
        }
        var f : File;
        f.unix.fd = fd;
        f.is_valid = true;
        return Result.Ok(f);
    }
}

func file_close(file : *mut File) : Result<UnitTy, FsError> {
    if(!file.is_valid) { return Result.Ok(UnitTy{}); }
    if(def.windows) {
        // if(!CloseHandle(file.win.handle)) { return Result.Err(winerr_to_fs(GetLastError() as int)); }
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        var r = close(file.unix.fd);
        if(r != 0) { return Result.Err(errno_to_fs(r)); }
        file.is_valid = false;
        return Result.Ok();
    }
}

func file_read(file : *mut File, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
    if(def.windows) {
        return Result.Err<size_t, FsError>(FsError.UnsupportedPlatform());
    } else {
        var n = read(file.unix.fd, buf as *mut void, buf_len);
        if(n < 0) { return Result.Err(errno_to_fs(n as int)); }
        return Result.Ok(n as size_t);
    }
}

func file_write(file : *mut File, buf : *u8, buf_len : size_t) : Result<size_t, FsError> {
    if(def.windows) {
        return Result.Err<size_t, FsError>(FsError.UnsupportedPlatform());
    } else {
        var n = write(file.unix.fd, buf as *void, buf_len);
        if(n < 0) { return Result.Err(errno_to_fs(n as int)); }
        return Result.Ok(n as size_t);
    }
}

// read_exact: read until buf_len bytes filled or error
func file_read_exact(file : *mut File, buf : *mut u8, buf_len : size_t) : Result<UnitTy, FsError> {
    var pos : size_t = 0;
    while(pos < buf_len) {
        var r = file_read(file, buf + pos, buf_len - pos);
        if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
        var Ok(n) = r else unreachable;
        if(n == 0) { return Result.Err(FsError.IoError(0, "unexpected EOF\0")); }
        pos += n;
    }
    return Result.Ok(UnitTy{});
}

// write_all: loop till all bytes written
func file_write_all(file : *mut File, buf : *u8, buf_len : size_t) : Result<UnitTy, FsError> {
    var pos : size_t = 0;
    while(pos < buf_len) {
        var r = file_write(file, buf + pos, buf_len - pos);
        if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
        var Ok(n) = r else unreachable;
        if(n == 0) { return Result.Err(FsError.IoError(0, "write returned 0\0")); }
        pos += n;
    }
    return Result.Ok(UnitTy{});
}

// seek / tell / truncate: platform dependent. Provide POSIX implementation using lseek/ftruncate
func file_seek(file : *mut File, offset : i64, whence : int) : Result<i64, FsError> {
    if(def.windows) {
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        // extern long lseek(int fd, long offset, int whence);
        // var r = lseek(file.unix.fd, offset, whence);
        // if(r < 0) return Result.Err(errno_to_fs(errno));
        return Result.Err(FsError.UnsupportedPlatform()); // add lseek extern to enable
    }
}

func file_truncate(path : *char, size : i64) : Result<UnitTy, FsError> {
    if(def.windows) {
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        // extern int truncate(const char *path, off_t length);
        // var r = truncate(path, size);
        // if(r != 0) return Result.Err(errno_to_fs(errno));
        return Result.Err(FsError.UnsupportedPlatform()); // user to add truncate extern
    }
}

func file_fsync(file : *mut File) : Result<UnitTy, FsError> {
    if(def.windows) {
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        // extern int fsync(int fd);
        // var r = fsync(file.unix.fd);
        // if(r != 0) return Result.Err(errno_to_fs(errno));
        return Result.Err(FsError.UnsupportedPlatform());
    }
}

// ----------------------
// Metadata & attributes
// ----------------------
func metadata(path : *char) : Result<Metadata, FsError> {
    if(def.windows) {
        // Use GetFileAttributesExW and GetFileInformationByHandleEx etc.
        // Convert path -> WCHAR, call GetFileAttributesExW, transform into Metadata
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        // POSIX: call lstat() to not follow symlinks
        var st : stat; // user must define struct stat in OS-specific section
        var r = lstat(path, &st);
        if(r != 0) { return Result.Err(errno_to_fs(r)); }
        var m : Metadata;
        // The following fields assume struct stat layout:
        // user must ensure stat has st_mode, st_size, st_mtime, st_atime, st_ctime
        m.is_dir = ((st.st_mode & 0xF000) == 0x4000);  // S_IFDIR
        m.is_file = ((st.st_mode & 0xF000) == 0x8000); // S_IFREG
        m.is_symlink = ((st.st_mode & 0xF000) == 0xA000); // S_IFLNK
        m.len = st.st_size as size_t;
        m.modified = (st.st_mtime) as i64;
        m.accessed = (st.st_atime) as i64;
        m.created = (st.st_ctime) as i64;
        m.perms = (st.st_mode & 0x1FF) as u32; // lower 9 bits (rwx)
        return Result.Ok(m);
    }
}

// is_file / is_dir convenience
func is_file(path : *char) : Result<bool, FsError> {
    var r = metadata(path);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
    var Ok(m) = r else unreachable;
    return Result.Ok(m.is_file);
}

func is_dir(path : *char) : Result<bool, FsError> {
    var r = metadata(path);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
    var Ok(m) = r else unreachable;
    return Result.Ok(m.is_dir);
}

// ----------------------
// Directory operations
// ----------------------
// create_dir
func create_dir(path : *char) : Result<UnitTy, FsError> {
    if(def.windows) {
        // CreateDirectoryW
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        var r = mkdir(path, 0o777);
        if(r != 0) { return Result.Err(errno_to_fs(r)); }
        return Result.Ok(UnitTy {});
    }
}

// create_dir_all (recursive)
func create_dir_all(path : *char) : Result<UnitTy, FsError> {
    // Create directories recursively without heap; use a temporary buffer on stack
    var buf : [PATH_MAX_BUF]char;
    var r = normalize_path(path, &mut buf[0], PATH_MAX_BUF as size_t);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
    var Ok(len) = r else unreachable;
    // iterate components and mkdir each prefix
    var i : size_t = 0;
    while(i < len) {
        // find next separator
        var j : size_t = i;
        while(j < len && buf[j] != '/') { j++ }
        // create prefix [0..j)
        var prefix_buf : [PATH_MAX_BUF]char;
        var p : size_t = 0;
        while(p < j) { prefix_buf[p] = buf[p]; p++ }
        prefix_buf[p] = 0;
        // attempt mkdir - if exists continue
        var rmk = create_dir(&mut prefix_buf[0]);
        if(rmk is Result.Err) {
            var Err(e) = rmk else unreachable;
            // if AlreadyExists or NotFound? for now just check and continue on AlreadyExists
            // For simplicity assume errno mapping; user can refine
            // If error is AlreadyExists ignore, else return error
            if(e is FsError.AlreadyExists) {
                // continue
            } else {
                return Result.Err(e);
            }
        }
        if(j >= len) { break; }
        i = j + 1;
    }
    return Result.Ok(UnitTy{});
}

// remove_dir (non-recursive) and remove_dir_all (recursive)
func remove_dir(path : *char) : Result<UnitTy, FsError> {
    if(def.windows) {
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        var r = rmdir(path);
        if(r != 0) { return Result.Err(errno_to_fs(r)); }
        return Result.Ok(UnitTy {});
    }
}

// remove_dir_all : recursive
func remove_dir_all(path : *char) : Result<UnitTy, FsError> {
    // We'll implement a simple stackless recursive walker using a small stack of paths on the stack (bounded).
    // For large trees user should use a streaming walker. This is a convenience.
    const MAX_STACK = 256;
    var stack_paths : [MAX_STACK][PATH_MAX_BUF]char;
    var stack_top : int = 0;
    // push starting path
    var i : size_t = 0;
    while(path[i] != 0) { stack_paths[0][i] = path[i]; i++ }
    stack_paths[0][i] = 0;
    stack_top = 1;
    // loop
    while(stack_top > 0) {
        // pop
        stack_top -= 1;
        var curr_ptr = &mut stack_paths[stack_top][0];
        // read dir entries
        var read_res = read_dir(curr_ptr, |curr_ptr, &mut stack_top, MAX_STACK, &mut stack_paths|(name : *char, name_len : size_t, is_dir : bool) => {
            // skip "." and ".."
            if(name_len == 1 && name[0] == '.') { return true; }
            if(name_len == 2 && name[0] == '.' && name[1] == '.') { return true; }
            // build child path
            var child : [PATH_MAX_BUF]char;
            var j : size_t = 0;
            while(curr_ptr[j] != 0) { child[j] = curr_ptr[j]; j++ }
            if(j > 0 && child[j-1] != '/') { child[j++] = '/'; }
            var k : size_t = 0;
            while(k <= name_len) { child[j + k] = name[k]; k++ } // includes null
            // if directory push to stack else unlink
            if(is_dir) {
                if(stack_top + 1 >= MAX_STACK) { /* stack overflow */ return false; }
                var p : size_t = 0;
                while(child[p] != 0) { stack_paths[stack_top][p] = child[p]; p++ }
                stack_paths[stack_top][p] = 0;
                stack_top += 1;
            } else {
                var rem = remove_file(&mut child[0]);
                if(rem is Result.Err) { var Err(e) = rem else unreachable; return false; }
            }
            return true;
        });
        if(read_res is Result.Err) { var Err(e) = read_res else unreachable; return Result.Err(e); }
        // after children removed, remove dir itself
        var remd = remove_dir(curr_ptr);
        if(remd is Result.Err) { var Err(e) = remd else unreachable; return Result.Err(e); }
    }
    return Result.Ok(UnitTy{});
}

// read_dir: callback style to avoid allocations. Callback signature: fn(name : *char, name_len : size_t, is_dir : bool) -> bool
func read_dir(path : *char, callback : std::function<(name : *char, name_len : size_t, is_dir : bool) => bool>) : Result<UnitTy, FsError> {
    if(def.windows) {
        // Use FindFirstFileW / FindNextFileW; convert path to search pattern path\*.
        // Implementation requires Windows externs; return UnsupportedPlatform placeholder until externs are wired.
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        var d = opendir(path);
        if(d == 0) { return Result.Err(errno_to_fs(-1)); }
        while(true) {
            var ent = readdir(d);
            if(ent == 0) { break; }
            // User must provide struct dirent; assume dirent has d_name and d_type if supported.
            // We'll extract name pointer and decide is_dir based on d_type or stat fallback.
            var name_ptr : *char = ent.d_name; // requires dirent typed
            // compute length
            var nl : size_t = 0;
            while(name_ptr[nl] != 0) { nl++ }
            var is_dir : bool = false;
            // TODO: HAVE_DIRENT_D_TYPE
            // #if defined(HAVE_DIRENT_D_TYPE)
            is_dir = (ent.d_type == 4) // DT_DIR
            // #else
            // fallback: stat the entry
            var child : [PATH_MAX_BUF]char;
            var p : size_t = 0;
            while(path[p] != 0) { child[p] = path[p]; p++ }
            if(p > 0 && child[p-1] != '/') { child[p++] = '/'; }
            var q : size_t = 0;
            while(q <= nl) { child[p + q] = name_ptr[q]; q++ }
            var st : stat;
            var r = lstat(child.ptr(), &st);
            if(r == 0) {
                is_dir = ((st.st_mode & 0xF000) == 0x4000);
            }
            // #endif
            var cont = callback(name_ptr, nl, is_dir);
            if(!cont) { break; }
        }
        closedir(d);
        return Result.Ok(UnitTy {});
    }
}

// remove_file / unlink
func remove_file(path : *char) : Result<UnitTy, FsError> {
    if(def.windows) {
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        var r = unlink(path);
        if(r != 0) { return Result.Err(errno_to_fs(r)); }
        return Result.Ok(UnitTy {});
    }
}

// ----------------------
// Copy, rename, move
// ----------------------
func copy_file(src : *char, dst : *char) : Result<UnitTy, FsError> {
    if(def.windows) {
        // Use CopyFileExW; requires UTF16 convert + extern
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        // try efficient syscall (sendfile) if available else fallback to read/write chunks
        // We'll implement pure read/write fallback (portable).
        var src_opts : OpenOptions; src_opts.read = true; src_opts.write = false; src_opts.create = false; src_opts.truncate = false; src_opts.append = false;
        var src_o = file_open(src, src_opts);
        if(src_o is Result.Err) {
            var Err(e) = src_o else unreachable
            return Result.Err(e)
        }
        var Ok(sfile) = src_o else unreachable
        var dst_opts : OpenOptions; dst_opts.read = false; dst_opts.write = true; dst_opts.create = true; dst_opts.truncate = true; dst_opts.append = false;
        var dst_o = file_open(dst, dst_opts);
        if(dst_o is Result.Err) {
            file_close(&sfile)
            var Err(e) = dst_o else unreachable
            return e
        }
        var Ok(dfile) = dst_o else unreachable
        var buf : [COPY_CHUNK]u8;
        while(true) {
            var r = file_read(&sfile, buf.ptr(), COPY_CHUNK);
            if(r is Result.Err) { var Err(e) = r else unreachable; file_close(&sfile); file_close(&dfile); return Result.Err(e); }
            var Ok(n) = r else unreachable;
            if(n == 0) { break; }
            var wres = file_write_all(&dfile, buf.ptr(), n);
            if(wres is Result.Err) { var Err(e) = wres else unreachable; file_close(&sfile); file_close(&dfile); return Result.Err(e); }
        }
        file_close(&sfile);
        file_close(&dfile);
        return Result.Ok(UnitTy {});
    }
}

func rename_path(oldp : *char, newp : *char) : Result<UnitTy, FsError> {
    if(def.windows) {
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        var r = rename(oldp, newp);
        if(r != 0) { return Result.Err(errno_to_fs(r)); }
        return Result.Ok(UnitTy {});
    }
}

// hard link / symlink
func create_hard_link(existing : *char, newpath : *char) : Result<UnitTy, FsError> {
    if(def.windows) {
        // CreateHardLinkW
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        var r = link(existing, newpath);
        if(r != 0) { return Result.Err(errno_to_fs(r)); }
        return Result.Ok(UnitTy {});
    }
}

func create_symlink(target : *char, linkpath : *char, is_dir : bool) : Result<UnitTy, FsError> {
    if(def.windows) {
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        var r = symlink(target, linkpath);
        if(r != 0) { return Result.Err(errno_to_fs(r)); }
        return Result.Ok(UnitTy {});
    }
}

func read_link(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    if(def.windows) {
        return Result.Err<size_t, FsError>(FsError.UnsupportedPlatform());
    } else {
        // ssize_t readlink(const char *pathname, char *buf, size_t bufsiz);
        var r = readlink(path, out, out_len); // user must add readlink extern
        if(r < 0) { return Result.Err(errno_to_fs(r as int)); }
        // ensure null terminate if space
        if((r as size_t) < out_len) { out[r] = 0; }
        return Result.Ok(r as size_t);
    }
}

// ----------------------
// Temp file helpers & atomic write
// ----------------------
func temp_dir(out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    if(def.windows) {
        var wbuf : [WIN_MAX_PATH]u16;
        // call GetTempPathW and convert back
        return Result.Err<size_t, FsError>(FsError.UnsupportedPlatform());
    } else {
        // typically /tmp
        var tmp : *char = "/tmp\0";
        var i : size_t = 0;
        while(i <= 4) { out[i] = tmp[i]; i++ }
        return Result.Ok(4);
    }
}

func create_temp_file(prefix : *char, out_path : *mut char, out_len : size_t, fh_out : *mut File) : Result<UnitTy, FsError> {
    if(def.windows) {
        return Result.Err(FsError.UnsupportedPlatform());
    } else {
        // create unique tmp file under /tmp using mkstemp - user to provide mkstemp extern
        // Example: char tmpl[] = "/tmp/prefixXXXXXX"; int fd = mkstemp(tmpl);
        return Result.Err(FsError.UnsupportedPlatform());
    }
}

func atomic_write(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
    // write to temp in same dir then rename
    // compose tmp path: path + ".tmpXXXX"
    var dir_buf : [PATH_MAX_BUF]char;
    var r = dirname(path, &mut dir_buf[0], PATH_MAX_BUF as size_t);
    if(r is Result.Err) { var Err(e) = r else unreachable; return Result.Err(e); }
    var Ok(dir_len) = r else unreachable;
    var tmpbuf : [PATH_MAX_BUF]char;
    var tmpname : [64]char;
    var p : size_t = 0;
    while(p < dir_len) { tmpbuf[p] = dir_buf[p]; p++ }
    if(p > 0 && tmpbuf[p-1] != '/') { tmpbuf[p++] = '/'; }
    // append prefix + pid + random (we'll do pid)
    var pref : *char = ".tmpfs.";
    var pi : size_t = 0;
    while(pref[pi] != 0) { tmpbuf[p + pi] = pref[pi]; pi++ }
    p += pi;
    // naive PID -> decimal (user can replace with syscall)
    var PID : int = 12345; // user should replace with getpid()
    var tlen : size_t = int_to_str(PID, &mut tmpname[0], 64);
    if(tlen == 0) { return Result.Err(FsError.Other("pid conversion failed\0")); }
    var q : size_t = 0;
    while(q <= tlen) { tmpbuf[p + q] = tmpname[q]; q++ }
    // ensure null
    tmpbuf[p + tlen] = 0;
    // write file
    var wr = write_text_file(&mut tmpbuf[0], data, data_len);
    if(wr is Result.Err) { var Err(e) = wr else unreachable; return Result.Err(e); }
    // rename temp to final
    var rnm = rename_path(&mut tmpbuf[0], path);
    if(rnm is Result.Err) { var Err(e) = rnm else unreachable; return Result.Err(e); }
    return Result.Ok(UnitTy{});
}

// helper: integer -> decimal string (unsigned)
func int_to_str(v : int, out : *mut char, out_len : size_t) : size_t {
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

// ----------------------
// High-level convenience: read/write small files to caller buffer
// ----------------------
func read_to_buffer(path : *char, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
    var opts : OpenOptions; opts.read = true; opts.write = false; opts.create = false; opts.truncate = false; opts.append = false;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unreachable;
        return Result.Err<size_t, FsError>(e);
    }
    var Ok(f) = fo else unreachable
    var pos : size_t = 0;
    while(true) {
        var r = file_read(&mut f, buf + pos, buf_len - pos);
        if(r is Result.Err) { var Err(e) = r else unreachable; file_close(&mut f); return Result.Err<size_t, FsError>(e); }
        var Ok(n) = r else unreachable;
        if(n == 0) { break; } // EOF
        pos += n;
        if(pos >= buf_len) { file_close(&mut f); return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
    }
    file_close(&mut f);
    return Result.Ok<size_t, FsError>(pos);
}

func write_text_file(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
    var opts : OpenOptions; opts.read = false; opts.write = true; opts.create = true; opts.truncate = true; opts.append = false;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unreachable; return Result.Err(e);
    }
    var Ok(f) = fo else unreachable;
    var r = file_write_all(&mut f, data, data_len);
    if(r is Result.Err) { var Err(e) = r else unreachable; file_close(&mut f); return Result.Err(e); }
    file_close(&mut f);
    return Result.Ok(UnitTy{});
}

} // end namespace fs