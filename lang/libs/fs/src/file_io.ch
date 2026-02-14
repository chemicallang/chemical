public namespace fs {

using std::Result;

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

// util
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

public func read_entire_file(path : *char) : Result<std::vector<u8>, FsError> {
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


public func read_to_buffer(path : *char, buf : *mut u8, buf_len : size_t) : Result<size_t, FsError> {
    var opts : OpenOptions; 
    opts.read = true; opts.write = false; opts.append = false; opts.create = false; opts.create_new = false; opts.truncate = false; opts.binary = true;
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
    var opts : OpenOptions; 
    opts.read = false; opts.write = true; opts.append = false; opts.create = true; opts.create_new = false; opts.truncate = true; opts.binary = true;
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

}