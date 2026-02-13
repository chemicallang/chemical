public namespace fs {

using std::Result;

public type path_ptr = if(def.windows) *u16 else *char

public type mut_path_ptr = if(def.windows) *mut u16 else *mut char

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
    var total = a_len + (if(need_sep) 1u else 0u) + b_len;
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

}