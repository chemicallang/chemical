// fs.fs    -- filesystem library for the language
// Designed: cross-platform (POSIX + Windows), no heap allocations (caller buffers / stack buffers), uses Option/Result
// You supply the OS-specific externs and structs listed below (see comment block "OS-SPECIFIC EXTERNS & STRUCTS").

// Result<UnitTy, MyError> doesn't work
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
public comptime const PATH_MAX_BUF = 4096;
public comptime const WIN_MAX_PATH = 32768;
public comptime const COPY_CHUNK = 64 * 1024; // 64 KiB
public comptime const DIR_ENT_NAME_MAX = 256;
public comptime const TEMP_NAME_MAX = 64;

// ----------------------
// Types
// ----------------------
public variant FsError {
    Io(code : int, message : *char)
    NotFound()
    AlreadyExists()
    PermissionDenied()
    InvalidInput()
    NotADirectory()
    IsADirectory()
    WouldBlock()
    PathTooLong()
    Unsupported()   // used only for extremely exotic requests
    Other(msg : *char)

    func message(&self) : std::string {
        switch(self) {
            Io(code, message) => {
                var msg = std::string("io error with code ")
                msg.append_integer(code)
                msg.append_view(std::string_view(" and message '"))
                msg.append_char_ptr(message)
                msg.append('\'')
                return msg;
            }
            NotFound() => return std::string("FsError.NotFound")
            AlreadyExists() => return std::string("FsError.AlreadyExists")
            PermissionDenied() => return std::string("FsError.PermissionDenied")
            InvalidInput() => return std::string("FsError.InvalidInput")
            NotADirectory() => return std::string("FsError.NotADirectory")
            IsADirectory() => return std::string("FsError.IsADirectory")
            WouldBlock() => return std::string("FsError.WouldBlock")
            PathTooLong() => return std::string("FsError.PathTooLong")
            Unsupported() => return std::string("FsError.Unsupported")   // used only for extremely exotic requests
            Other(msg) => return std::string(msg);
        }
    }

}

if(def.posix) {
    @extern public func realpath(path : *char, resolved : *mut char) : *char;
    @extern public func chmod(path : *char, mode : u32) : int;

    public type blksize_t = i64;
    public type fsblkcnt_t = u64;
    public type fsfilcnt_t = u64;

    const AT_FDCWD = -100;               // use current working directory
    const AT_SYMLINK_NOFOLLOW = 0x100;   // do not follow symlinks (Linux typical)
    @extern public func utimensat(dirfd : int, pathname : *char, times : *timespec, flags : int) : int;
    @extern public func fsync(fd : int) : int;

    @extern public func link(oldpath : *char, newpath : *char) : int;
    @extern public func symlink(target : *char, linkpath : *char) : int;

    // Returns file descriptor on success, -1 on error (and modifies template in-place).
    @extern public func mkstemp(template : *mut char) : int;

    struct Statvfs {
        var f_bsize   : ulong;   // file system block size
        var f_frsize  : ulong;   // fragment size
        var f_blocks  : fsblkcnt_t;      // size of fs in f_frsize units
        var f_bfree   : fsblkcnt_t;      // # free blocks
        var f_bavail  : fsblkcnt_t;      // # free blocks for unprivileged users
        var f_files   : fsfilcnt_t;      // # inodes
        var f_ffree   : fsfilcnt_t;      // # free inodes
        var f_favail  : fsfilcnt_t;      // # free inodes for unprivileged users
        var f_fsid    : ulong;   // file system ID
        var f_flag    : ulong;   // mount flags
        var f_namemax : ulong;   // maximum filename length
    }
    @extern public func statvfs(path : *char, out : *mut Statvfs) : int;

    @extern public func flock(fd : int, operation : int) : int;

    const LOCK_SH = 1;   // shared lock
    const LOCK_EX = 2;   // exclusive lock
    const LOCK_NB = 4;   // non-blocking
    const LOCK_UN = 8;   // unlock

    public type ssize_t = isize;

    // POSIX readlink declaration:
    // ssize_t readlink(const char *path, char *buf, size_t bufsiz);
    @extern public func readlink(path : *char, buf : *mut char, bufsiz : size_t) : ssize_t;

}

public struct Metadata {
    var is_file : bool;
    var is_dir : bool;
    var is_symlink : bool;
    var len : size_t;
    var modified : i64;
    var accessed : i64;
    var created : i64;
    var perms : u32; // POSIX rwx bits or Windows attributes mapping
}

public struct OpenOptions {
    var read : bool;
    var write : bool;
    var append : bool;
    var create : bool;
    var create_new : bool; // exclusive create
    var truncate : bool;
    var binary : bool;
}

public struct File {
    if(def.windows) {
        struct { var handle : void*; } win;
    } else {
        struct { var fd : int; } unix;
    }
    var valid : bool;
}

// ----------------------
// Helpers: error mapping
// ----------------------
func posix_errno_to_fs(e : int) : FsError {
    if(e == 2) { return FsError.NotFound(); }
    if(e == 13) { return FsError.PermissionDenied(); }
    if(e == 17) { return FsError.AlreadyExists(); }
    if(e == 21) { return FsError.IsADirectory(); }
    if(e == 20) { return FsError.NotADirectory(); }
    if(e == 11) { return FsError.WouldBlock(); }
    return FsError.Io(e, "posix error\0");
}

func winerr_to_fs(code : int) : FsError {
    if(code == 2) { return FsError.NotFound(); }
    if(code == 5) { return FsError.PermissionDenied(); }
    if(code == 80) { return FsError.AlreadyExists(); }
    return FsError.Io(code, "win32 error\0");
}

// ----------------------
// Path utilities (no allocations; caller buffers)
// ----------------------

// Path utilities (join, basename, dirname, normalize)
// --------------------------
func basename(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
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
    if(comp_len + 1 > out_len) { return Result.Err(FsError.PathTooLong()); }
    var k : size_t = 0;
    while(k < comp_len) { out[k] = path[j + k]; k++ }
    out[k] = 0;
    return Result.Ok(comp_len);
}

func dirname(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
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
    if(end + 1 > out_len) { return Result.Err(FsError.PathTooLong()); }
    var k : size_t = 0;
    while(k < end) { out[k] = path[k]; k++ }
    out[k] = 0;
    return Result.Ok(end);
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
    var a_len : size_t = 0; while(a[a_len] != 0) { a_len++ }
    var b_len : size_t = 0; while(b[b_len] != 0) { b_len++ }
    if(a_len == 0) {
        if(b_len + 1 > out_len) { return Result.Err(FsError.PathTooLong()); }
        var i : size_t = 0; while(i <= b_len) { out[i] = b[i]; i++ }
        return Result.Ok(b_len);
    }
    var need_sep : bool = false; if(a[a_len-1] != '/' && a[a_len-1] != '\\') { need_sep = true }
    var total = a_len + (if(need_sep) 1 else 0) + b_len;
    if(total + 1 > out_len) { return Result.Err(FsError.PathTooLong()); }
    var pos : size_t = 0; var i : size_t = 0;
    while(i < a_len) { out[pos++] = a[i++]; }
    if(need_sep) { out[pos++] = '/'; }
    i = 0; while(i <= b_len) { out[pos++] = b[i++]; }
    return Result.Ok(total);
}

// normalize_path: resolve "." and ".." (does not resolve symlinks). Uses stack arrays for components.
// normalize path removing "." and resolving ".." relative components (no symlink resolution)
func normalize_path(path_in : *char, out_buf : *mut char, out_len : size_t) : Result<size_t, FsError> {
    var len : size_t = 0; while(path_in[len] != 0) { len++ }
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
                if(count >= MAX_COMPS) { return Result.Err(FsError.PathTooLong()); }
                offs[count] = start; lens[count] = c_len; count++;
            }
        } else {
            if(count >= MAX_COMPS) { return Result.Err(FsError.PathTooLong()); }
            offs[count] = start; lens[count] = c_len; count++;
        }
    }
    if(count == 0) {
        if(out_len < 2) { return Result.Err(FsError.PathTooLong()); }
        out_buf[0] = '.'; out_buf[1] = 0; return Result.Ok(1);
    }
    var pos : size_t = 0; var j : size_t = 0;
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

// canonicalize: platform-backed (realpath / GetFullPathNameW)
func canonicalize(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    comptime if(def.windows) {
        // Windows: GetFullPathNameW
        var wtmp : [WIN_MAX_PATH]u16;
        var r = utf8_to_utf16(path, &mut wtmp[0], WIN_MAX_PATH as size_t);
        if(r is Result.Err) {
            var Err(e) = r else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = r else unreachable
        var wout : [WIN_MAX_PATH]u16;
        // extern func GetFullPathNameW(lpFileName : *WCHAR, nBufferLength : DWORD, lpBuffer : *WCHAR, lpFilePart : *mut *WCHAR) : DWORD;
        var res = GetFullPathNameW((&mut wtmp[0]) as LPCWSTR, WIN_MAX_PATH as u32, (&mut wout[0]) as LPWSTR, null);
        if(res == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        var conv = utf16_to_utf8(&mut wout[0], out, out_len);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e)
        }
        var Ok(n) = conv else unreachable
        return Result.Ok(n);
    } else {
        // POSIX: realpath
        var resptr = fs::realpath(path, out);
        if(resptr == 0) {
            // errno must be mapped by user; assume errno accessor exists
            return Result.Err(FsError.Io(-1, "realpath failed\0"));
        }
        var plen : size_t = 0; while(out[plen] != 0) { plen++ }
        return Result.Ok(plen);
    }
}

// ----------------------
// UTF8 <-> UTF16 helpers (small, stack-based converters)
// Note: these are simple, safe converters; they avoid heap usage.
// You may replace with more optimized / complete conversions when desired.
// ----------------------
func utf8_to_utf16(in_utf8 : *char, out_w : *mut u16, out_w_len : size_t) : Result<size_t, FsError> {
    // Safe UTF-8 -> UTF-16 converter: validates continuation bytes and avoids out-of-bounds reads.
    var i : size_t = 0;
    var wpos : size_t = 0;

    while (in_utf8[i] != 0) {
        var c = in_utf8[i] as u8;

        // ASCII
        if (c < 0x80) {
            if (wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = c as u16;
            i += 1;
            continue;
        }

        // 2-byte sequence: 110xxxxx 10xxxxxx
        if ((c & 0xE0) == 0xC0) {
            // need one continuation byte
            if (in_utf8[i+1] == 0) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }
            var c2 = in_utf8[i+1] as u8;
            if ((c2 & 0xC0) != 0x80) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            // decode and check overlong (must be >= 0x80)
            var code = (((c & 0x1F) as u32) << 6) | ((c2 & 0x3F) as u32);
            if (code < 0x80u32) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            if (wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = code as u16;
            i += 2;
            continue;
        }

        // 3-byte sequence: 1110xxxx 10xxxxxx 10xxxxxx
        if ((c & 0xF0) == 0xE0) {
            // need two continuation bytes
            if (in_utf8[i+1] == 0 || in_utf8[i+2] == 0) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }
            var c2 = in_utf8[i+1] as u8;
            var c3 = in_utf8[i+2] as u8;
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            var code = (((c & 0x0F) as u32) << 12) | (((c2 & 0x3F) as u32) << 6) | ((c3 & 0x3F) as u32);

            // Reject overlong encoding and surrogate halves (U+D800..U+DFFF)
            if (code < 0x800u32) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }
            if (code >= 0xD800u32 && code <= 0xDFFFu32) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            if (wpos + 1 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = code as u16;
            i += 3;
            continue;
        }

        // 4-byte sequence: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx -> surrogate pair in UTF-16
        if ((c & 0xF8) == 0xF0) {
            // need three continuation bytes
            if (in_utf8[i+1] == 0 || in_utf8[i+2] == 0 || in_utf8[i+3] == 0) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }
            var c2 = in_utf8[i+1] as u8;
            var c3 = in_utf8[i+2] as u8;
            var c4 = in_utf8[i+3] as u8;
            if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80 || (c4 & 0xC0) != 0x80) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            var code = (((c & 0x07) as u32) << 18) | (((c2 & 0x3F) as u32) << 12) | (((c3 & 0x3F) as u32) << 6) | ((c4 & 0x3F) as u32);

            // minimal value for 4-byte is 0x10000 and max is 0x10FFFF
            if (code < 0x10000u32 || code > 0x10FFFFu32) { return Result.Err<size_t, FsError>(FsError.InvalidInput()); }

            // produce surrogate pair
            code -= 0x10000u32;
            var high = 0xD800u16 + ((code >> 10) as u16);
            var low  = 0xDC00u16 + ((code & 0x3FFu32) as u16);

            if (wpos + 2 >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
            out_w[wpos++] = high;
            out_w[wpos++] = low;
            i += 4;
            continue;
        }

        // anything else is invalid (e.g. 0x80..0xBF as a leading byte or 0xF8+)
        return Result.Err<size_t, FsError>(FsError.InvalidInput());
    }

    // null-terminate (ensure we still have space)
    if (wpos >= out_w_len) { return Result.Err<size_t, FsError>(FsError.PathTooLong()); }
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

// --------------------------
// File open/close/read/write/seek/flush
// --------------------------
func file_open(path : *char, opts : OpenOptions) : Result<File, FsError> {
    comptime if(def.windows) {
        // Convert path to UTF-16 and call CreateFileW
        var wbuf : [WIN_MAX_PATH]u16;
        var r = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
        if(r is Result.Err) {
            var Err(e) = r else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = r else unreachable
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
        var handle = CreateFileW((&mut wbuf[0]) as LPCWSTR, access, share, null, create_disp, flags, null);
        if(handle == INVALID_HANDLE_VALUE) {
            var err = GetLastError();
            return Result.Err(winerr_to_fs(err as int));
        }
        var f : File; f.win.handle = handle; f.valid = true;
        return Result.Ok(f);
    } else {
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
}

func file_close(f : *mut File) : Result<UnitTy, FsError> {
    if(!f.valid) { return Result.Ok(UnitTy{}); }
    comptime if(def.windows) {
        var ok = CloseHandle(f.win.handle);
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        f.valid = false;
        return Result.Ok(UnitTy{});
    } else {
        var r = close(f.unix.fd);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        f.valid = false;
        return Result.Ok(UnitTy{});
    }
}

func file_read(f : *mut File, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
    comptime if(def.windows) {
        var read_out : u32 = 0;
        var ok = ReadFile(f.win.handle, buf as *mut void, buf_len as u32, (&mut read_out) as *mut DWORD, null);
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(read_out as size_t);
    } else {
        var n = read(f.unix.fd, buf as *mut void, buf_len);
        if(n < 0) { return Result.Err(posix_errno_to_fs(-n)); }
        return Result.Ok(n as size_t);
    }
}

func file_write(f : *mut File, buf : *u8, buf_len : size_t) : Result<size_t, FsError> {
    comptime if(def.windows) {
        var written : u32 = 0;
        var ok = WriteFile(f.win.handle, buf as *void, buf_len as u32, (&mut written) as *mut DWORD, null);
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(written as size_t);
    } else {
        var n = write(f.unix.fd, buf as *void, buf_len);
        if(n < 0) { return Result.Err(posix_errno_to_fs(-n)); }
        return Result.Ok(n as size_t);
    }
}

// read_exact: read until buf_len bytes filled or error
func file_read_exact(f : *mut File, buf : *mut u8, buf_len : size_t) : Result<UnitTy, FsError> {
    var pos : size_t = 0;
    while(pos < buf_len) {
        var r = file_read(f, buf + pos, buf_len - pos);
        if(r is Result.Err) {
            var Err(e) = r else unreachable
            return Result.Err(e)
        }
        var Ok(n) = r else unreachable
        if(n == 0) { return Result.Err(FsError.Io(0, "unexpected EOF\0")); }
        pos += n;
    }
    return Result.Ok(UnitTy{});
}

// write_all: loop till all bytes written
func file_write_all(f : *mut File, buf : *u8, buf_len : size_t) : Result<UnitTy, FsError> {
    var pos : size_t = 0;
    while(pos < buf_len) {
        var r = file_write(f, buf + pos, buf_len - pos);
        if(r is Result.Err) {
            var Err(e) = r else unreachable
            return Result.Err(e)
        }
        var Ok(n) = r else unreachable
        if(n == 0) { return Result.Err(FsError.Io(0, "write returned 0\0")); }
        pos += n;
    }
    return Result.Ok(UnitTy{});
}

func file_flush(f : *mut File) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        var ok = FlushFileBuffers(f.win.handle);
        if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var r = fsync(f.unix.fd);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

if(def.windows) {
public func filetime_to_unix(ft : FILETIME) : i64 {
    const SECS_BETWEEN_EPOCHS : i64 = 11644473600;
    const HUNDRED_NANOSECONDS_PER_SEC : i64 = 10000000;

    // Reconstruct 64-bit Windows time from FILETIME parts
    var windows_time : u64 = ((ft.dwHighDateTime as u64) << 32) | (ft.dwLowDateTime as u64);

    // Convert to seconds and adjust epoch
    var unix_time : i64 = (windows_time / HUNDRED_NANOSECONDS_PER_SEC) - SECS_BETWEEN_EPOCHS;

    return unix_time;
}
}

// --------------------------
// Metadata, timestamps, permissions
// --------------------------
func metadata(path : *char) : Result<Metadata, FsError> {
    comptime if(def.windows) {
        var wbuf : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = conv else unreachable
        var fi : WIN32_FILE_ATTRIBUTE_DATA;
        var ok = GetFileAttributesExW((&mut wbuf[0]) as LPCWSTR, GET_FILEEX_INFO_LEVELS.GetFileExInfoStandard, &mut fi);
        if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        var m : Metadata;
        var attrs = fi.dwFileAttributes;
        m.is_dir = ((attrs & FILE_ATTRIBUTE_DIRECTORY) != 0);
        m.is_file = !m.is_dir;
        m.is_symlink = ((attrs & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
        // convert LARGE_INTEGER FILETIME to epoch seconds (simplified)
        m.len = (fi.nFileSizeHigh as size_t) << 32 | (fi.nFileSizeLow as size_t);
        m.modified = filetime_to_unix(fi.ftLastWriteTime);
        m.accessed = filetime_to_unix(fi.ftLastAccessTime);
        m.created = filetime_to_unix(fi.ftCreationTime);
        m.perms = attrs as u32;
        return Result.Ok(m);
    } else {
        var st : Stat;
        var r = lstat(path, &mut st);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        var m : Metadata;
        var mode : int = st.st_mode as int;
        m.is_dir = ((mode & 0xF000) == 0x4000);
        m.is_file = ((mode & 0xF000) == 0x8000);
        m.is_symlink = ((mode & 0xF000) == 0xA000);
        m.len = st.st_size as size_t;
        m.modified = st.st_mtime as i64;
        m.accessed = st.st_atime as i64;
        m.created = st.st_ctime as i64;
        m.perms = (st.st_mode & 0x1FF) as u32;
        return Result.Ok(m);
    }
}

func set_permissions(path : *char, perms : u32) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        // map readonly bit
        var wbuf : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = conv else unreachable
        // map perms -> FILE_ATTRIBUTE_READONLY
        var attrs : u32 = 0;
        if((perms & 0x200) != 0) { attrs = attrs | FILE_ATTRIBUTE_READONLY; } // owner write bit cleared -> readonly
        var ok = SetFileAttributesW((&mut wbuf[0]) as LPCWSTR, attrs);
        if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var r = fs::chmod(path, perms as u32);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

if(def.windows) {
public func unix_to_filetime(unix_time : i64) : FILETIME {
    const SECS_BETWEEN_EPOCHS : i64 = 11644473600;   // seconds between 1601 and 1970
    const HUNDRED_NANOSECONDS_PER_SEC : i64 = 10000000;

    var ft : FILETIME;

    // Convert unix seconds to Windows ticks
    var windows_time : u64 = ((unix_time + SECS_BETWEEN_EPOCHS) * HUNDRED_NANOSECONDS_PER_SEC) as u64;

    ft.dwLowDateTime = (windows_time & 0xFFFFFFFF) as u32;
    ft.dwHighDateTime = ((windows_time >> 32) & 0xFFFFFFFF) as u32;

    return ft;
}
}

func set_times(path : *char, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        var wbuf : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = conv else unreachable;
        var h = CreateFileW((&mut wbuf[0]) as LPCWSTR, 0x40000000 /*GENERIC_WRITE*/, 0, null, 3 /*OPEN_EXISTING*/, 0, null);
        if(h == INVALID_HANDLE_VALUE) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        var cft = unix_to_filetime(0); // unused creation
        var aft = unix_to_filetime(atime);
        var mft = unix_to_filetime(mtime);
        var ok = SetFileTime(h, &cft, &aft, &mft);
        CloseHandle(h);
        if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(UnitTy{});
    } else {
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

// --------------------------
// Directory functions
// --------------------------
public func create_dir(path : *char) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        var w : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16(path, &mut w[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = conv else unreachable;
        var ok = CreateDirectoryW((&mut w[0]) as LPCWSTR, null);
        if(ok == 0) {
            var e = GetLastError();
            return Result.Err(winerr_to_fs(e as int));
        }
        return Result.Ok(UnitTy{});
    } else {
        var r = posix_mkdir(path, 0o777);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

// create_dir_all (recursive)
func create_dir_all(path : *char) : Result<UnitTy, FsError> {
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
public func remove_dir(path : *char) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        var w : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16(path, &mut w[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = conv else unreachable
        var ok = RemoveDirectoryW((&mut w[0]) as LPCWSTR);
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var r = rmdir(path);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

const PATH_MAX_BUF = 4096;
const WIN_MAX_PATH = 32768;

const S_IFMT  = 0xF000;
const S_IFDIR = 0x4000;

const FILE_ATTRIBUTE_DIRECTORY = 0x10;

// -----------------------------
// Platform APIs & helpers
// -----------------------------
if(def.windows) {
    struct WIN32_FIND_DATAW {
        var dwFileAttributes : DWORD;
        // additional fields omitted
        var cFileName : [WIN_MAX_PATH]u16;
    }
    @extern @dllimport @stdcall public func FindFirstFileW(pattern : LPCWSTR, out : *mut WIN32_FIND_DATAW) : HANDLE;
    @extern @dllimport @stdcall public func FindNextFileW(h : HANDLE, out : *mut WIN32_FIND_DATAW) : BOOL;
    @extern @dllimport @stdcall public func FindClose(h : HANDLE) : BOOL;
    @extern @dllimport @stdcall public func DeleteFileW(path : LPCWSTR) : BOOL;
    @extern @dllimport @stdcall public func RemoveDirectoryW(path : LPCWSTR) : BOOL;

    const INVALID_HANDLE_VALUE : HANDLE = -1 as HANDLE;
    const CP_UTF8 = 65001u32;
} else  {
    // POSIX externs
    // type DIR = *void;
    struct dirent { var d_name : [PATH_MAX_BUF]char; }
    struct stat { var st_mode : u32; }

    // @extern public func opendir(path : *char) : DIR;
    // @extern public func readdir(dir : DIR) : *mut dirent;
    // @extern public func closedir(dir : DIR) : int;
    // @extern public func lstat(path : *char, out : *mut Stat) : int;
    @extern public func unlink(path : *char) : int;
    @extern public func rmdir(path : *char) : int;


    @extern public func openat(dirfd : int, path : *char, flags : int, mode : u32) : int;
    @extern public func dup(fd : int) : int;
    @extern public func fdopendir(fd : int) : *mut DIR;

    @extern public func fstatat(dirfd : int, pathname : *char, out : *mut Stat, flags : int) : int;

    @extern public func unlinkat(dirfd : int, pathname : *char, flags : int) : int;

    // common flags (Linux values)
    const O_RDONLY = 0;
    const O_DIRECTORY = 0x20000;           // typical Linux value; adjust if your platform differs
    const AT_REMOVEDIR = 0x200;            // unlinkat flag to remove a directory (Linux)

}

if(def.windows) {
    public func utf8_to_utf16_inplace(src : *char, out : *mut u16, out_len : size_t) : Result<size_t, FsError> {
        var n = MultiByteToWideChar(CP_UTF8, 0u32, src, -1, out, (out_len as i32));
        if(n == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        // return length excluding null
        return Result.Ok((n as size_t) - 1);
    }

    public func utf16_to_utf8_str(src : *u16) : std::string {
        var out = std::string();
        var needed = WideCharToMultiByte(CP_UTF8, 0u32, src, -1, null, 0, null, null);
        if(needed <= 0) { return out; }
        out.reserve((needed as size_t) - 1);
        out.resize_unsafe((needed as size_t) - 1);
        var n = WideCharToMultiByte(CP_UTF8, 0u32, src, -1, out.mutable_data(), needed, null, null);
        if(n <= 0) { out.resize_unsafe(0); return out; }
        return out;
    }
}

public func remove_file_platform(path : *char) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        var w : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16_inplace(path, &mut w[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) { var Err(e) = conv else unreachable; return Result.Err(e); }
        var deleted = DeleteFileW((&mut w[0]) as LPCWSTR);
        if(deleted == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(UnitTy{});
    } else {
        if(unlink(path) != 0) { return Result.Err(posix_errno_to_fs(-get_errno())); }
        return Result.Ok(UnitTy{});
    }
}

public func remove_dir_platform(path : *char) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        var w : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16_inplace(path, &mut w[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) { var Err(e) = conv else unreachable; return Result.Err(e); }
        var ok = RemoveDirectoryW((&mut w[0]) as LPCWSTR);
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var r = rmdir(path);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

// ---------------------------
// Helper: remove_dir_all_at(fd)
// Remove contents of directory referenced by an open directory fd.
// Does not remove the directory itself; caller should remove it by name (rmdir) or handle separately.
// ---------------------------
func remove_dir_all_at(dirfd : int) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        // nothing here — this helper is POSIX-only
        return Result.Ok(UnitTy{});
    } else {
        // Duplicate fd for fdopendir (fdopendir usually consumes the fd)
        var ddup = dup(dirfd);
        if(ddup < 0) {
            var e = get_errno();
            // printf("dup(%d) failed: errno=%d (%s)\n", dirfd, e, strerror(e));
            return Result.Err(posix_errno_to_fs(-e));
        }

        var dir = fdopendir(ddup);
        if(dir == null) {
            var e = get_errno();
            // printf("fdopendir(%d) failed: errno=%d (%s)\n", ddup, e, strerror(e));
            close(ddup);
            return Result.Err(posix_errno_to_fs(-e));
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
                    return Result.Err(posix_errno_to_fs(-re));
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
                    return Result.Err(posix_errno_to_fs(-e));
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
                        return Result.Err(posix_errno_to_fs(-e2));
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
                        return Result.Err(posix_errno_to_fs(-e3));
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
                        return Result.Err(posix_errno_to_fs(-e4));
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
}

// -----------------------------
// Recursive remove_dir_all (public)
// -----------------------------
public func remove_dir_all_recursive(path : *char) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        // Convert path to wide
        var wbuf : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16_inplace(path, &mut wbuf[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) { var Err(e) = conv else unreachable; return Result.Err(e); }
        // Build pattern path\* for enumeration
        var search : [WIN_MAX_PATH]u16;
        var p : size_t = 0;
        while(wbuf[p] != 0) { search[p] = wbuf[p]; p += 1; }
        if(p > 0) {
            var last = search[p - 1];
            if(last != '\\' as u16 && last != '/' as u16) { search[p] = '\\' as u16; p += 1; }
        }
        search[p] = '*' as u16; p += 1;
        search[p] = 0;

        var finddata : WIN32_FIND_DATAW;
        var h = FindFirstFileW((&mut search[0]) as LPCWSTR, &mut finddata);
        if(h == INVALID_HANDLE_VALUE) {
            // If directory cannot be opened, return mapped error
            var err = GetLastError();
            return Result.Err(winerr_to_fs(err as int));
        }

        loop {
            // skip "." and ".."
            var name_w = &mut finddata.cFileName[0];
            if(!(name_w[0] == '.' as u16 && name_w[1] == 0) &&
               !(name_w[0] == '.' as u16 && name_w[1] == '.' as u16 && name_w[2] == 0)) {
                // build child wide path: original path + '\' + name
                var child : [WIN_MAX_PATH]u16;
                var q : size_t = 0;
                // copy original wide path
                var i : size_t = 0;
                while(wbuf[i] != 0) { child[i] = wbuf[i]; i += 1; }
                if(i > 0) {
                    var last = child[i - 1];
                    if(last != '\\' as u16 && last != '/' as u16) { child[i] = '\\' as u16; i += 1; }
                }
                // append name_w
                var j : size_t = 0;
                while(name_w[j] != 0) { child[i + j] = name_w[j]; j += 1; }
                child[i + j] = 0;

                var is_dir = (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                if(is_dir) {
                    // convert child to utf-8 and recurse
                    var child_utf = utf16_to_utf8_str(&mut child[0]);
                    var rem = remove_dir_all_recursive(child_utf.data());
                    if(rem is Result.Err) { var Err(e) = rem else unreachable; FindClose(h); return Result.Err(e); }
                } else {
                    // delete file
                    var del = DeleteFileW((&mut child[0]) as LPCWSTR);
                    if(del == 0) { var err = GetLastError(); FindClose(h); return Result.Err(winerr_to_fs(err as int)); }
                }
            }

            var more = FindNextFileW(h, &mut finddata);
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
        var remd = RemoveDirectoryW((&mut wbuf[0]) as LPCWSTR);
        if(remd == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(UnitTy{});

    } else {
        // POSIX
        // printf("remove_dir_all_recursive: opening '%s'\n", path);
        var dirfd = open(path, O_RDONLY | O_DIRECTORY, 0);
        if(dirfd < 0) {
            var e = get_errno();
            // printf("open('%s') -> errno=%d (%s)\n", path, e, strerror(e));
            return Result.Err(posix_errno_to_fs(-e));
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
            return Result.Err(posix_errno_to_fs(-er));
        }
        // printf("remove_dir_all_recursive: removed directory '%s' OK\n", path);
        return Result.Ok(UnitTy{});
    }
}

func copy_directory(src : *char, dst : *char, preserve_metadata : bool) : Result<UnitTy, FsError> {
    // recursive copy: create dest dir then iterate src and copy files/dirs
    var st_res = metadata(src);
    if(st_res is Result.Err) {
        var Err(e) = st_res else unreachable
        return Result.Err(e)
    }
    var Ok(st) = st_res else unreachable;
    if(!st.is_dir) { return Result.Err(FsError.NotADirectory()); }
    // create dst
    var cd = create_dir_all(dst);
    if(cd is Result.Err) {
        var Err(e) = cd else unreachable
        return Result.Err(e)
    }
    // iterate src
    var res = read_dir(src, |src, dst, preserve_metadata|(name : *char, name_len : size_t, is_dir : bool) => {
        // skip . and ..
        if(name_len == 1 && name[0] == '.') { return true; }
        if(name_len == 2 && name[0] == '.' && name[1] == '.') { return true; }
        var srcchild : [PATH_MAX_BUF]char;
        var dstchild : [PATH_MAX_BUF]char;
        var p : size_t = 0; while(src[p] != 0) { srcchild[p] = src[p]; p++ }
        if(p > 0 && srcchild[p-1] != '/') { srcchild[p++] = '/'; }
        var q : size_t = 0; while(q <= name_len) { srcchild[p + q] = name[q]; q++ }
        var r : size_t = 0; while(dst[r] != 0) { dstchild[r] = dst[r]; r++ }
        if(r > 0 && dstchild[r-1] != '/') { dstchild[r++] = '/'; }
        q = 0; while(q <= name_len) { dstchild[r + q] = name[q]; q++ }
        if(is_dir) {
            var c = copy_directory(&mut srcchild[0], &mut dstchild[0], preserve_metadata);
            if(c is Result.Err) { var Err(e) = c else unreachable; return false; }
        } else {
            var c = copy_file(&mut srcchild[0], &mut dstchild[0]);
            if(c is Result.Err) { var Err(e) = c else unreachable; return false; }
            if(preserve_metadata) {
                var meta = metadata(&mut srcchild[0]);
                if(meta is Result.Err) {
                    var Err(e) = meta else unreachable
                    // TODO: cannot return the result
                    // return Result.Err<UnitTy, FsError>(e)
                    return false;
                }
                var Ok(m) = meta else unreachable;
                var setp = set_permissions(&mut dstchild[0], m.perms);
                if(setp is Result.Err) { var Err(e) = setp else unreachable; /* non-fatal? */ }
                var stimes = set_times(&mut dstchild[0], m.accessed, m.modified);
                if(stimes is Result.Err) { var Err(e) = stimes else unreachable; }
            }
        }
        return true;
    });
    if(res is Result.Err) {
        var Err(e) = res else unreachable
        return Result.Err(e)
    }
    var Ok(_) = res else unreachable;
    return Result.Ok(UnitTy{});
}

// read_dir: callback style to avoid allocations. Callback signature: fn(name : *char, name_len : size_t, is_dir : bool) -> bool
func read_dir(path : *char, callback : std::function<(name : *char, name_len : size_t, is_dir : bool) => bool>) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        // Windows implementation using FindFirstFileW / FindNextFileW
        var wpath : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16(path, &mut wpath[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = conv else unreachable
        // append \* pattern
        var pattern : [WIN_MAX_PATH]u16;
        var p : size_t = 0; while(wpath[p] != 0) { pattern[p] = wpath[p]; p++ }
        if(p == 0) { pattern[p++] = '.' as u16; }
        if(p > 0 && pattern[p-1] != '\\' && pattern[p-1] != '/') { pattern[p++] = '\\'; }
        pattern[p] = '*'; pattern[p+1] = 0;
        var findData : WIN32_FIND_DATAW;
        var h = FindFirstFileW((&mut pattern[0]) as LPCWSTR, &mut findData);
        if(h == INVALID_HANDLE_VALUE) {
            var e = GetLastError();
            if(e == ERROR_FILE_NOT_FOUND) { return Result.Ok(UnitTy{}); } // empty directory
            return Result.Err(winerr_to_fs(e as int));
        }
        while(true) {
            // convert name to utf8
            var name_utf8 : [DIR_ENT_NAME_MAX]char;
            var conv2 = utf16_to_utf8(&mut findData.cFileName[0], &mut name_utf8[0], DIR_ENT_NAME_MAX as size_t);
            if(conv2 is Result.Err) {
                FindClose(h);
                var Err(e) = conv2 else unreachable
                return Result.Err(e)
            }
            var Ok(nlen) = conv2 else unreachable
            var is_dir : bool = ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0);
            var cont = callback(&mut name_utf8[0], nlen, is_dir);
            if(!cont) { FindClose(h); return Result.Ok(UnitTy{}); }
            var ok = FindNextFileW(h, &mut findData);
            if(ok == 0) { var err = GetLastError(); if(err == ERROR_NO_MORE_FILES) { break; } FindClose(h); return Result.Err(winerr_to_fs(err as int)); }
        }
        FindClose(h);
        return Result.Ok(UnitTy{});
    } else {
        var d = opendir(path);
        if(d == null) { return Result.Err(FsError.Io(-1, "opendir failed\0")); }
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
}

// remove_file / unlink
func remove_file(path : *char) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        var w : [WIN_MAX_PATH]u16;
        var conv = utf8_to_utf16(path, &mut w[0], WIN_MAX_PATH as size_t);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e)
        }
        var Ok(wlen) = conv else unreachable;
        var ok = DeleteFileW((&mut w[0]) as LPCWSTR);
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var r = unlink(path);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

// --------------------------
// Copy & move (files and directories)
// --------------------------
func copy_file(src : *char, dst : *char) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
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
        var ok = CopyFileW((&mut wsrc[0]) as LPCWSTR, (&mut wdst[0]) as LPCWSTR, 0); // overwrite
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(UnitTy{});
    } else {
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
}

func move_path(src : *char, dst : *char) : Result<UnitTy, FsError> {
    var r = rename(src, dst);
    if(r == 0) { return Result.Ok(UnitTy{}); }
    // if rename fails (e.g. cross-device) fallback to copy+remove
    var src_meta = metadata(src);
    if(src_meta is Result.Err) {
        var Err(e) = src_meta else unreachable
        return Result.Err(e)
    }
    var Ok(m) = src_meta else unreachable
    if(m.is_dir) {
        var c = copy_directory(src, dst, true);
        if(c is Result.Err) {
            var Err(e) = c else unreachable
            return Result.Err(e)
        }
        var rem = remove_dir_all_recursive(src);
        if(rem is Result.Err) {
            var Err(e) = rem else unreachable
            return Result.Err(e)
        }
        return Result.Ok(UnitTy{});
    } else {
        var c = copy_file(src, dst);
        if(c is Result.Err) {
            var Err(e) = c else unreachable
            return Result.Err(e)
        }
        var rem = remove_file(src);
        if(rem is Result.Err) {
            var Err(e) = rem else unreachable
            return Result.Err(e)
        }
        return Result.Ok(UnitTy{});
    }
}

// hard link / symlink
func create_hard_link(existing : *char, newpath : *char) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
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
        var ok = CreateHardLinkW((&mut wnew[0]) as LPCWSTR, (&mut wexist[0]) as LPCWSTR, null);
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var r = link(existing, newpath);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

func create_symlink(target : *char, linkpath : *char, dir : bool) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
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
        var flags : u32 = 0;
        if(dir) { flags = 1; } // SYMBOLIC_LINK_FLAG_DIRECTORY
        var ok = CreateSymbolicLinkW((&mut wlink[0]) as LPCWSTR, (&mut wtarget[0]) as LPCWSTR, flags);
        if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var r = symlink(target, linkpath);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

func read_link(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    comptime if(def.windows) {
        // On Windows readlink is more involved; use DeviceIoControl or GetFinalPathNameByHandle
        // Simpler approach: open file and call GetFinalPathNameByHandleW
        var wpath : [WIN_MAX_PATH]u16;
        var f1 = utf8_to_utf16(path, &mut wpath[0], WIN_MAX_PATH as size_t)
        if(f1 is Result.Err) {
            var Err(e) = f1 else unreachable;
            return Result.Err(e);
        }
        var h = CreateFileW((&mut wpath[0]) as LPCWSTR, 0, 0, null, 3 /*OPEN_EXISTING*/, (FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS) as DWORD, null);
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
    } else {
        var r = readlink(path, out, out_len);
        if(r < 0) { return Result.Err(posix_errno_to_fs(-r)); }
        if((r as size_t) < out_len) { out[r] = 0; }
        return Result.Ok(r as size_t);
    }
}

func is_symlink(path : *char) : Result<bool, FsError> {
    comptime if(def.windows) {
        var m = metadata(path);
        if(m is Result.Err) {
            var Err(e) = m else unreachable;
            return Result.Err(e);
        }
        var Ok(md) = m else unreachable;
        return Result.Ok(md.is_symlink);
    } else {
        var st : Stat;
        var r = lstat(path, &mut st);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        var islnk = ((st.st_mode & 0xF000) == 0xA000);
        return Result.Ok(islnk);
    }
}

// ----------------------
// Temp file helpers & atomic write
// ----------------------
func temp_dir(out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    comptime if(def.windows) {
        var wbuf : [WIN_MAX_PATH]u16;
        var n = GetTempPathW(WIN_MAX_PATH as u32, (&mut wbuf[0]) as LPWSTR);
        if(n == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        var conv = utf16_to_utf8(&mut wbuf[0], out, out_len);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable;
            return Result.Err(e);
        }
        var Ok(len) = conv else unreachable;
        return Result.Ok(len);
    } else {
        var tmp : *char = "/tmp\0";
        var i : size_t = 0; while(tmp[i] != 0) { out[i] = tmp[i]; i++ }
        out[i] = 0;
        return Result.Ok(i);
    }
}

func create_temp_file_in(dir : *char, prefix : *char, out_path : *mut char, out_len : size_t, fh : *mut File) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
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
        var res = GetTempFileNameW((&mut wdir[0]) as LPCWSTR, (&mut wprefix[0]) as LPCWSTR, 0, (&mut wout[0]) as LPWSTR);
        if(res == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        var conv = utf16_to_utf8(&mut wout[0], out_path, out_len);
        if(conv is Result.Err) {
            var Err(e) = conv else unreachable
            return Result.Err(e);
        }
        var Ok(len) = conv else unreachable
        // open file
        var opts : OpenOptions; opts.read = true; opts.write = true; opts.create = false;
        var fo = file_open(out_path, opts);
        if(fo is Result.Err) {
            var Err(e) = fo else unreachable
            return Result.Err(e)
        }
        var Ok(f) = fo else unreachable;
        fh.valid = f.valid; fh.win.handle = f.win.handle;
        return Result.Ok(UnitTy{});
    } else {
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
}

// --------------------------
// Disk capacity
// --------------------------
func disk_space(path : *char, total_out : *mut u64, free_out : *mut u64, avail_out : *mut u64) : Result<UnitTy, FsError> {
    comptime if(def.windows) {
        var w : [WIN_MAX_PATH]u16;
        var f1 = utf8_to_utf16(path, &mut w[0], WIN_MAX_PATH as size_t)
        if(f1 is Result.Err) {
            var Err(e) = f1 else unreachable;
            return Result.Err(e);
        }
        var ok = GetDiskFreeSpaceExW((&mut w[0]) as LPCWSTR, free_out as PULARGE_INTEGER, total_out as PULARGE_INTEGER, avail_out as PULARGE_INTEGER);
        if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var st : Statvfs;
        var r = statvfs(path, &mut st);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        *total_out = (st.f_blocks as u64) * (st.f_frsize as u64);
        *free_out = (st.f_bfree as u64) * (st.f_frsize as u64);
        *avail_out = (st.f_bavail as u64) * (st.f_frsize as u64);
        return Result.Ok(UnitTy{});
    }
}

// --------------------------
// File locking (shared/exclusive) - advisory on POSIX
// --------------------------
func lock_file_shared(path : *char) : Result<File, FsError> {
    var opts : OpenOptions; opts.read = true; opts.write = false;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unreachable;
        return Result.Err(e);
    }
    var Ok(f) = fo else unreachable;
    comptime if(def.windows) {
        // use LockFileEx with LOCKFILE_FAIL_IMMEDIATELY & LOCKFILE_SHARED_LOCK
        var ok = LockFileEx(f.win.handle, LOCKFILE_FAIL_IMMEDIATELY as DWORD, 0, 0xFFFFFFFF, 0xFFFFFFFF, null);
        if(ok == 0) { var e = GetLastError(); file_close(&mut f); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(f);
    } else {
        var r = flock(f.unix.fd, LOCK_SH);
        if(r != 0) { file_close(&mut f); return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(f);
    }
}

func lock_file_exclusive(path : *char) : Result<File, FsError> {
    var opts : OpenOptions; opts.read = true; opts.write = true;
    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unreachable;
        return Result.Err(e);
    }
    var Ok(f) = fo else unreachable;
    comptime if(def.windows) {
        var ok = LockFileEx(f.win.handle, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, null);
        if(ok == 0) { var e = GetLastError(); file_close(&mut f); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(f);
    } else {
        var r = flock(f.unix.fd, LOCK_EX);
        if(r != 0) { file_close(&mut f); return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(f);
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
    var rnm = move_path(&mut tmpbuf[0], path);
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
// Read entire file by growing the buffer as needed.
// Does NOT call metadata; works even when file grows while reading.
public func read_entire_file(path : *char) : Result<std::vector<u8>, FsError> {
    // open file for reading
    var opts : OpenOptions;
    opts.read = true;
    opts.write = false;
    opts.create = false;
    opts.truncate = false;
    opts.append = false;

    var fo = file_open(path, opts);
    if(fo is Result.Err) {
        var Err(e) = fo else unreachable;
        return Result.Err(e);
    }
    var Ok(f) = fo else unreachable;

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
                file_close(&mut f);
                return Result.Err(FsError.Other("file too large to read"));
            }
            cap = newcap;
            vec.reserve(cap);
        }

        // pointer to writable region in the string's buffer
        var ptr = vec.data() + pos as isize; // cast to pointer arithmetic if needed

        // read up to (cap - pos) bytes
        var want = cap - pos;
        var r = file_read(&mut f, ptr as *mut u8, want);
        if(r is Result.Err) {
            var Err(e) = r else unreachable;
            file_close(&mut f);
            return Result.Err(e);
        }
        var Ok(n) = r else unreachable;

        if(n == 0) {
            // EOF
            break;
        }

        // advance write position
        pos += n;
        // loop continues; if pos == cap, we'll grow next iteration
    }

    // set the string's size to the number of bytes read
    vec.resize_unsafe(pos);

    file_close(&mut f);
    return Result.Ok(vec);
}


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

public func write_text_file(path : *char, data : *u8, data_len : size_t) : Result<UnitTy, FsError> {
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