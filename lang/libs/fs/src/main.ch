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
    if(def.windows) {
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
        // extern "c" { func realpath(path : *char, resolved : *mut char) : *char; }
        var resptr = realpath(path, out);
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

// --------------------------
// File open/close/read/write/seek/flush
// --------------------------
func file_open(path : *char, opts : OpenOptions) : Result<File, FsError> {
    if(def.windows) {
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
        if(opts.read && !opts.write) { flags = 0; } // O_RDONLY
        else if(opts.read && opts.write) { flags = 2; } // O_RDWR
        else if(!opts.read && opts.write) { flags = 1; } // O_WRONLY
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
    if(def.windows) {
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
    if(def.windows) {
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
    if(def.windows) {
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
    if(def.windows) {
        var ok = FlushFileBuffers(f.win.handle);
        if(ok == 0) { var e = GetLastError(); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(UnitTy{});
    } else {
        var r = fsync(f.unix.fd);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

// --------------------------
// Metadata, timestamps, permissions
// --------------------------
func metadata(path : *char) : Result<Metadata, FsError> {
    if(def.windows) {
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
        // TODO: filetime_to_unix not available
        // m.modified = filetime_to_unix(fi.ftLastWriteTime);
        // m.accessed = filetime_to_unix(fi.ftLastAccessTime);
        // m.created = filetime_to_unix(fi.ftCreationTime);
        m.perms = attrs as u32;
        return Result.Ok(m);
    } else {
        var st : stat;
        var r = lstat(path, &st);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        var m : Metadata;
        var mode : int = st.st_mode;
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
    if(def.windows) {
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
        var r = chmod(path, perms as int);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        return Result.Ok(UnitTy{});
    }
}

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

func set_times(path : *char, atime : i64, mtime : i64) : Result<UnitTy, FsError> {
    if(def.windows) {
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
        var r = utimensat(0, path, &times, 0);
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
func create_dir(path : *char) : Result<UnitTy, FsError> {
    if(def.windows) {
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
        var r = mkdir(path, 0o777);
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
func remove_dir(path : *char) : Result<UnitTy, FsError> {
    if(def.windows) {
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

// remove_dir_all : recursive
func remove_dir_all(path : *char) : Result<UnitTy, FsError> {
    // Use manual stack to avoid recursion depth issues
    const MAX_STACK = 1024;
    var stack : [MAX_STACK][PATH_MAX_BUF]char;
    var top : int = 0;
    // push initial path
    var i : size_t = 0;
    while(path[i] != 0) { stack[0][i] = path[i]; i++ }
    stack[0][i] = 0; top = 1;
    while(top > 0) {
        top -= 1;
        var curr = &mut stack[top][0];
        // iterate entries
        var res = read_dir(curr, |&mut stack, curr, top, MAX_STACK|(name : *char, name_len : size_t, is_dir : bool) => {
            // skip "." and ".."
            if(name_len == 1 && name[0] == '.') { return true; }
            if(name_len == 2 && name[0] == '.' && name[1] == '.') { return true; }
            var child : [PATH_MAX_BUF]char;
            var p : size_t = 0;
            while(curr[p] != 0) { child[p] = curr[p]; p++ }
            if(p > 0 && child[p-1] != '/') { child[p++] = '/'; }
            var q : size_t = 0; while(q <= name_len) { child[p + q] = name[q]; q++ } // includes null
            if(is_dir) {
                // push this directory
                if(top + 1 >= MAX_STACK) { return false; } // overflow
                var r : size_t = 0; while(child[r] != 0) { stack[top][r] = child[r]; r++ } stack[top][r] = 0;
                top += 1;
            } else {
                var rem = remove_file(&mut child[0]);
                if(rem is Result.Err) { var Err(e) = rem else unreachable; return false; }
            }
            return true;
        });
        if(res is Result.Err) {
            var Err(e) = res else unreachable
            return Result.Err(e)
        }
        // after children removed, remove directory
        var rr = remove_dir(curr);
        if(rr is Result.Err) { var Err(e) = rr else unreachable; return Result.Err(e); }
    }
    return Result.Ok(UnitTy{});
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
func read_dir(path : *char, callback : (name : *char, name_len : size_t, is_dir : bool) => bool) : Result<UnitTy, FsError> {
    if(def.windows) {
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
            var name_ptr = ent.d_name;
            var nl : size_t = 0; while(name_ptr[nl] != 0) { nl++ }
            var isdir : bool = false;
            // TODO:
            // if(def.HAVE_DIRENT_D_TYPE)
            isdir = (ent.d_type == DT_DIR);
            // else {
            // fallback: stat child
            var child : [PATH_MAX_BUF]char;
            var p : size_t = 0; while(path[p] != 0) { child[p] = path[p]; p++ }
            if(p > 0 && child[p-1] != '/') { child[p++] = '/'; }
            var q : size_t = 0; while(q <= nl) { child[p + q] = name_ptr[q]; q++ }
            var st : stat;
            var r = lstat(child.ptr(), &st);
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
    if(def.windows) {
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
    if(def.windows) {
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
            file_close(&sf)
            var Err(e) = dres else unreachable
            return Result.Err(e)
        }
        var Ok(df) = dres else unreachable
        var buf : [COPY_CHUNK]u8;
        while(true) {
            var r = file_read(&sf, buf.ptr(), COPY_CHUNK);
            if(r is Result.Err) {
                file_close(&sf);
                file_close(&df)
                var Err(e) = r else unreachable
                return Result.Err(e)
            }
            var Ok(n) = r else unreachable
            if(n == 0) { break; }
            var w = file_write_all(&df, buf.ptr(), n);
            if(w is Result.Err) {
                file_close(&sf)
                file_close(&df)
                var Err(e) = w else unreachable
                return Result.Err(e)
            }
        }
        file_close(&sf); file_close(&df);
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
        var rem = remove_dir_all(src);
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


// TODO: methods for create_hard link are present somewhere else
// hard link / symlink
// func create_hard_link(existing : *char, newpath : *char) : Result<UnitTy, FsError> {
//     if(def.windows) {
//         var wexist : [WIN_MAX_PATH]u16; var wnew : [WIN_MAX_PATH]u16;
//         var f1 = utf8_to_utf16(existing, &mut wexist[0], WIN_MAX_PATH as size_t)
//         if(f1 is Result.Err) {
//             var Err(e) = f1 else unreachable
//             return Result.Err(e)
//         }
//         var f2 = utf8_to_utf16(newpath, &mut wnew[0], WIN_MAX_PATH as size_t)
//         if(f2 is Result.Err) {
//             var Err(e) = f2 else unreachable
//             return Result.Err(e)
//         }
//         var ok = CreateHardLinkW((&mut wnew[0]) as LPCWSTR, (&mut wexist[0]) as LPCWSTR, null);
//         if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
//         return Result.Ok(UnitTy{});
//     } else {
//         var r = link(existing, newpath);
//         if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
//         return Result.Ok(UnitTy{});
//     }
// }
//
// func create_symlink(target : *char, linkpath : *char, dir : bool) : Result<UnitTy, FsError> {
//     if(def.windows) {
//         var wtarget : [WIN_MAX_PATH]u16; var wlink : [WIN_MAX_PATH]u16;
//         var f1 = utf8_to_utf16(target, &mut wtarget[0], WIN_MAX_PATH as size_t)
//         if(f1 is Result.Err) {
//             var Err(e) = f1 else unreachable
//             return Result.Err(e)
//         }
//         var f2 = utf8_to_utf16(linkpath, &mut wlink[0], WIN_MAX_PATH as size_t)
//         if(f2 is Result.Err) {
//             var Err(e) = f2 else unreachable
//             return Result.Err(e)
//         }
//         var flags : u32 = 0;
//         if(dir) { flags = 1; } // SYMBOLIC_LINK_FLAG_DIRECTORY
//         var ok = CreateSymbolicLinkW((&mut wlink[0]) as LPCWSTR, (&mut wtarget[0]) as LPCWSTR, flags);
//         if(ok == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
//         return Result.Ok(UnitTy{});
//     } else {
//         var r = symlink(target, linkpath);
//         if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
//         return Result.Ok(UnitTy{});
//     }
// }
//
// func read_link(path : *char, out : *mut char, out_len : size_t) : Result<size_t, FsError> {
//     if(def.windows) {
//         // On Windows readlink is more involved; use DeviceIoControl or GetFinalPathNameByHandle
//         // Simpler approach: open file and call GetFinalPathNameByHandleW
//         var wpath : [WIN_MAX_PATH]u16;
//         var f1 = utf8_to_utf16(path, &mut wpath[0], WIN_MAX_PATH as size_t)
//         if(f1 is Result.Err) {
//             var Err(e) = f1 else unreachable;
//             return Result.Err(e);
//         }
//         var h = CreateFileW((&mut wpath[0]) as LPCWSTR, 0, 0, null, 3 /*OPEN_EXISTING*/, (FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS) as DWORD, null);
//         if(h == INVALID_HANDLE_VALUE) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
//         var wout : [WIN_MAX_PATH]u16;
//         var n = GetFinalPathNameByHandleW(h, (&mut wout[0]) as LPWSTR, WIN_MAX_PATH as u32, 0);
//         CloseHandle(h);
//         if(n == 0) { var err = GetLastError(); return Result.Err(winerr_to_fs(err as int)); }
//         var conv = utf16_to_utf8(&mut wout[0], out, out_len);
//         if(conv is Result.Err) {
//             var Err(e) = conv else unreachable;
//             return Result.Err(e);
//         }
//         var Ok(rn) = conv else unreachable
//         return Result.Ok(rn);
//     } else {
//         var r = readlink(path, out, out_len);
//         if(r < 0) { return Result.Err(posix_errno_to_fs(-r)); }
//         if((r as size_t) < out_len) { out[r] = 0; }
//         return Result.Ok(r as size_t);
//     }
// }

func is_symlink(path : *char) : Result<bool, FsError> {
    if(def.windows) {
        var m = metadata(path);
        if(m is Result.Err) {
            var Err(e) = m else unreachable;
            return Result.Err(e);
        }
        var Ok(md) = m else unreachable;
        return Result.Ok(md.is_symlink);
    } else {
        var st : stat;
        var r = lstat(path, &st);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        var islnk = ((st.st_mode & 0xF000) == 0xA000);
        return Result.Ok(islnk);
    }
}

// ----------------------
// Temp file helpers & atomic write
// ----------------------
func temp_dir(out : *mut char, out_len : size_t) : Result<size_t, FsError> {
    if(def.windows) {
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
    if(def.windows) {
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
        var fd = mkstemp(tmpl.ptr()); // user-provided extern
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
    if(def.windows) {
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
        var st : statvfs;
        var r = statvfs(path, &st);
        if(r != 0) { return Result.Err(posix_errno_to_fs(-r)); }
        *total_out = st.f_blocks as u64 * st.f_frsize as u64;
        *free_out = st.f_bfree as u64 * st.f_frsize as u64;
        *avail_out = st.f_bavail as u64 * st.f_frsize as u64;
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
    if(def.windows) {
        // use LockFileEx with LOCKFILE_FAIL_IMMEDIATELY & LOCKFILE_SHARED_LOCK
        var ok = LockFileEx(f.win.handle, LOCKFILE_FAIL_IMMEDIATELY as DWORD, 0, 0xFFFFFFFF, 0xFFFFFFFF, null);
        if(ok == 0) { var e = GetLastError(); file_close(&mut f); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(f);
    } else {
        var r = flock(f.unix.fd, LOCK_SH);
        if(r != 0) { file_close(&f); return Result.Err(posix_errno_to_fs(-r)); }
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
    if(def.windows) {
        var ok = LockFileEx(f.win.handle, 0, 0, 0xFFFFFFFF, 0xFFFFFFFF, null);
        if(ok == 0) { var e = GetLastError(); file_close(&mut f); return Result.Err(winerr_to_fs(e as int)); }
        return Result.Ok(f);
    } else {
        var r = flock(f.unix.fd, LOCK_EX);
        if(r != 0) { file_close(&f); return Result.Err(posix_errno_to_fs(-r)); }
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