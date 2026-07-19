// path — cross-platform path manipulation library.
// All functions operate on *char buffers (caller-supplied output buffers).
// No heap allocations.

public namespace path {

using std::Result;
using std::string_view;

public comptime const MAX_PATH = 4096;

// ---------------------------------------------------------------------------
// Path component extraction
// ---------------------------------------------------------------------------

public func basename(p : *char, out : *mut char, out_len : size_t) : Result<size_t, PathError> {
    var len : size_t = 0;
    while(p[len] != 0) { len += 1; }

    if(len == 0) {
        if(out_len < 2) { return Result.Err(PathError.BufferTooSmall()); }
        out[0] = '.'; out[1] = 0;
        return Result.Ok(1);
    }

    var end = len;
    while(end > 0 && (p[end-1] == '/' || p[end-1] == '\\')) { end -= 1; }

    if(end == 0) {
        if(out_len < 2) { return Result.Err(PathError.BufferTooSmall()); }
        out[0] = '/'; out[1] = 0;
        return Result.Ok(1);
    }

    var start = end;
    while(start > 0 && p[start-1] != '/' && p[start-1] != '\\') { start -= 1; }

    var comp_len = end - start;
    if(comp_len + 1 > out_len) { return Result.Err(PathError.BufferTooSmall()); }

    var i : size_t = 0;
    while(i < comp_len) {
        out[i] = p[start + i];
        i += 1;
    }
    out[i] = 0;
    return Result.Ok(comp_len);
}

public func dirname(p : *char, out : *mut char, out_len : size_t) : Result<size_t, PathError> {
    var len : size_t = 0;
    while(p[len] != 0) { len += 1; }

    if(len == 0) {
        if(out_len < 2) { return Result.Err(PathError.BufferTooSmall()); }
        out[0] = '.'; out[1] = 0;
        return Result.Ok(1);
    }

    var end = len;
    while(end > 0 && (p[end-1] == '/' || p[end-1] == '\\')) { end -= 1; }
    if(end == 0) {
        if(out_len < 2) { return Result.Err(PathError.BufferTooSmall()); }
        out[0] = '/'; out[1] = 0;
        return Result.Ok(1);
    }

    var start = end;
    while(start > 0 && p[start-1] != '/' && p[start-1] != '\\') { start -= 1; }
    if(start == 0) {
        if(out_len < 2) { return Result.Err(PathError.BufferTooSmall()); }
        out[0] = '.'; out[1] = 0;
        return Result.Ok(1);
    }

    var dir_end = start;
    while(dir_end > 0 && (p[dir_end-1] == '/' || p[dir_end-1] == '\\')) { dir_end -= 1; }
    if(dir_end == 0) {
        if(out_len < 2) { return Result.Err(PathError.BufferTooSmall()); }
        out[0] = '/'; out[1] = 0;
        return Result.Ok(1);
    }

    if(dir_end + 1 > out_len) { return Result.Err(PathError.BufferTooSmall()); }

    var i : size_t = 0;
    while(i < dir_end) {
        out[i] = p[i];
        i += 1;
    }
    out[i] = 0;
    return Result.Ok(dir_end);
}

public func extension(p : *char, out : *mut char, out_len : size_t) : Result<size_t, PathError> {
    var len : size_t = 0;
    while(p[len] != 0) { len += 1; }
    var end = len;
    while(end > 0 && (p[end-1] == '/' || p[end-1] == '\\')) { end -= 1; }
    var start = end;
    while(start > 0 && p[start-1] != '/' && p[start-1] != '\\') { start -= 1; }

    var dot_pos = start;
    while(dot_pos < end && p[dot_pos] != '.') { dot_pos += 1; }
    if(dot_pos >= end || dot_pos == start) {
        if(out_len < 1) { return Result.Err(PathError.BufferTooSmall()); }
        out[0] = 0;
        return Result.Ok(0);
    }

    var ext_len = end - dot_pos;
    if(ext_len + 1 > out_len) { return Result.Err(PathError.BufferTooSmall()); }
    var i : size_t = 0;
    while(i < ext_len) {
        out[i] = p[dot_pos + i];
        i += 1;
    }
    out[i] = 0;
    return Result.Ok(ext_len);
}

public func stem(p : *char, out : *mut char, out_len : size_t) : Result<size_t, PathError> {
    var len : size_t = 0;
    while(p[len] != 0) { len += 1; }
    var end = len;
    while(end > 0 && (p[end-1] == '/' || p[end-1] == '\\')) { end -= 1; }
    var start = end;
    while(start > 0 && p[start-1] != '/' && p[start-1] != '\\') { start -= 1; }

    var dot_pos = end;
    while(dot_pos > start && p[dot_pos-1] != '.') { dot_pos -= 1; }
    if(dot_pos <= start) {
        var stem_len = end - start;
        if(stem_len + 1 > out_len) { return Result.Err(PathError.BufferTooSmall()); }
        var i : size_t = 0;
        while(i < stem_len) {
            out[i] = p[start + i];
            i += 1;
        }
        out[i] = 0;
        return Result.Ok(stem_len);
    }

    var stem_len = dot_pos - start;
    if(stem_len + 1 > out_len) { return Result.Err(PathError.BufferTooSmall()); }
    var i : size_t = 0;
    while(i < stem_len) {
        out[i] = p[start + i];
        i += 1;
    }
    out[i] = 0;
    return Result.Ok(stem_len);
}

// ---------------------------------------------------------------------------
// Path joining and normalization
// ---------------------------------------------------------------------------

public func join(a : *char, b : *char, out : *mut char, out_len : size_t) : Result<size_t, PathError> {
    var a_len : size_t = 0; while(a[a_len] != 0) { a_len += 1; }
    var b_len : size_t = 0; while(b[b_len] != 0) { b_len += 1; }

    if(a_len == 0) {
        if(b_len + 1 > out_len) { return Result.Err(PathError.BufferTooSmall()); }
        var i : size_t = 0;
        while(i <= b_len) { out[i] = b[i]; i += 1; }
        return Result.Ok(b_len);
    }

    if(b_len > 0 && (b[0] == '/' || (b_len > 1 && b[1] == ':'))) {
        if(b_len + 1 > out_len) { return Result.Err(PathError.BufferTooSmall()); }
        var i : size_t = 0;
        while(i <= b_len) { out[i] = b[i]; i += 1; }
        return Result.Ok(b_len);
    }

    var need_sep = (a[a_len-1] != '/' && a[a_len-1] != '\\');
    var total = a_len + (if(need_sep) 1u else 0u) + b_len;
    if(total + 1 > out_len) { return Result.Err(PathError.BufferTooSmall()); }

    var pos : size_t = 0;
    var i : size_t = 0;
    while(i < a_len) { out[pos] = a[i]; pos += 1; i += 1; }
    if(need_sep) { out[pos] = '/'; pos += 1; }
    i = 0;
    while(i <= b_len) { out[pos] = b[i]; pos += 1; i += 1; }
    return Result.Ok(total);
}

/// Normalize a path resolving ".", ".." and double separators.
public func normalize(p : *char, out : *mut char, out_len : size_t) : Result<size_t, PathError> {
    var len : size_t = 0;
    while(p[len] != 0) { len += 1; }

    var is_absolute : bool = (len > 0 && p[0] == '/');
    var pos : size_t = 0;
    if(is_absolute) {
        if(pos + 1 >= out_len) { return Result.Err(PathError.BufferTooSmall()); }
        out[pos] = '/';
        pos += 1;
    }

    var i : size_t = 0;
    while(i < len) {
        while(i < len && (p[i] == '/' || p[i] == '\\')) { i += 1; }
        if(i >= len) { break; }

        var comp_start = i;
        while(i < len && p[i] != '/' && p[i] != '\\') { i += 1; }
        var comp_len = i - comp_start;

        if(comp_len == 1 && p[comp_start] == '.') {
            continue;
        }

        if(comp_len == 2 && p[comp_start] == '.' && p[comp_start+1] == '.') {
            if(pos == 0 || (is_absolute && pos == 1)) {
                if(!is_absolute) {
                    // Keep ".." at the start of relative paths
                    if(pos + 3 > out_len) { return Result.Err(PathError.BufferTooSmall()); }
                    if(pos > 0) { out[pos] = '/'; pos += 1; }
                    out[pos] = '.'; pos += 1;
                    out[pos] = '.'; pos += 1;
                }
                continue;
            }

            // Backtrack: go back past the previous separator
            if(pos > 0) { pos -= 1; } // past last char of component
            while(pos > 0 && out[pos-1] != '/') { pos -= 1; } // walk to separator
            if(pos > 0) { pos -= 1; } // step back to the '/' separator position
            continue;
        }

        // Regular component
        if(pos > 0 && !(is_absolute && pos == 1)) {
            if(pos + 1 >= out_len) { return Result.Err(PathError.BufferTooSmall()); }
            out[pos] = '/';
            pos += 1;
        }

        if(pos + comp_len >= out_len) { return Result.Err(PathError.BufferTooSmall()); }
        var k : size_t = 0;
        while(k < comp_len) {
            out[pos] = p[comp_start + k];
            pos += 1;
            k += 1;
        }
    }

    if(pos == 0) {
        if(out_len < 2) { return Result.Err(PathError.BufferTooSmall()); }
        if(is_absolute) {
            out[0] = '/'; out[1] = 0;
            return Result.Ok(1);
        } else {
            out[0] = '.'; out[1] = 0;
            return Result.Ok(1);
        }
    }

    if(pos >= out_len) { return Result.Err(PathError.BufferTooSmall()); }
    out[pos] = 0;
    return Result.Ok(pos);
}

// ---------------------------------------------------------------------------
// Path queries
// ---------------------------------------------------------------------------

public func is_absolute(p : *char) : bool {
    if(p[0] == '/') { return true; }
    comptime if(def.windows) {
        if((p[0] >= 'a' && p[0] <= 'z') || (p[0] >= 'A' && p[0] <= 'Z')) {
            if(p[1] == ':') { return true; }
        }
    }
    return false;
}

public func has_root(p : *char) : bool {
    return p[0] == '/' || p[0] == '\\';
}

// ---------------------------------------------------------------------------
// Parent path (string_view version for convenience)
// ---------------------------------------------------------------------------

func find_last_separator(data : *char, data_len : size_t) : int {
    if(data_len == 0) { return -1; }
    var i : int = data_len as int - 1;
    while(i >= 0) {
        if(data[i] == '/' || data[i] == '\\') { return i; }
        i -= 1;
    }
    return -1;
}

public func parent(sv : string_view) : std::string {
    var result = std::string();
    var pos = find_last_separator(sv.data(), sv.size());
    if(pos > 0) {
        result.append_with_len(sv.data(), (pos + 1) as size_t);
    }
    return result;
}

} // end namespace path
