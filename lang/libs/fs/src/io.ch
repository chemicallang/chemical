public namespace fs {

using std::Option;
using std::Result;

public func io_error(e : FsError) : std::io::IoError {
    switch(e) {
        NotFound() => { return std::io::IoError.Kind(std::io::IoErrorKind.NotFound()) }
        AlreadyExists() => { return std::io::IoError.Kind(std::io::IoErrorKind.AlreadyExists()) }
        PermissionDenied() => { return std::io::IoError.Kind(std::io::IoErrorKind.PermissionDenied()) }
        InvalidInput() => { return std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()) }
        NotADirectory() => { return std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()) }
        IsADirectory() => { return std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()) }
        WouldBlock() => { return std::io::IoError.Kind(std::io::IoErrorKind.WouldBlock()) }
        PathTooLong() => { return std::io::IoError.Kind(std::io::IoErrorKind.PathTooLong()) }
        Unsupported() => { return std::io::IoError.Kind(std::io::IoErrorKind.Unsupported()) }
        Io(_, message) => {
            var owned = unsafe(std::string(message))
            return std::io::IoError.Message(std::io::IoErrorKind.Other(), owned)
        }
        Other(message) => {
            var owned = unsafe(std::string(message))
            return std::io::IoError.Message(std::io::IoErrorKind.Other(), owned)
        }
    }
}


public func open_file(path : &str) : Result<File, std::io::IoError> {
    var opts : OpenOptions;
    opts.read = true;
    opts.write = false;
    opts.append = false;
    opts.create = false;
    opts.create_new = false;
    opts.truncate = false;
    opts.binary = true;
    return open_file_with(path, opts)
}

public func create_file(path : &str) : Result<File, std::io::IoError> {
    var opts : OpenOptions;
    opts.read = false;
    opts.write = true;
    opts.append = false;
    opts.create = true;
    opts.create_new = false;
    opts.truncate = true;
    opts.binary = true;
    return open_file_with(path, opts)
}

public func open_file_with(path : &str, opts : OpenOptions) : Result<File, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) {
        return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.InvalidInput()))
    }
    var Ok(owned) = made else unsafe { unreachable }
    var r = file_open(unsafe(owned.as_ptr()), opts)
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        return Result.Err(io_error(e))
    }
    var Ok(f) = r else unsafe { unreachable }
    return Result.Ok(f)
}


public func close_file(f : &mut File) : Result<size_t, std::io::IoError> {
    var r = file_close_ref(f)
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        return Result.Err(io_error(e))
    }
    return Result.Ok(0 as size_t)
}

public func read_file(path : &str) : Result<std::vector<u8>, std::io::IoError> {
    var opened = open_file(path)
    if(opened is Result.Err) {
        var Err(e) = opened else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(f) = opened else unsafe { unreachable }
    var out = std::vector<u8>()
    var r = f.try_read_to_end(&mut out)
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        var _ = close_file(&mut f)
        return Result.Err(e)
    }
    var cr = close_file(&mut f)
    if(cr is Result.Err) {
        var Err(e) = cr else unsafe { unreachable }
        return Result.Err(e)
    }
    return Result.Ok(out)
}

public func write_file(path : &str, data : [u8]) : Result<size_t, std::io::IoError> {
    var opened = create_file(path)
    if(opened is Result.Err) {
        var Err(e) = opened else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(f) = opened else unsafe { unreachable }
    var r = f.write_all(data)
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(n) = r else unsafe { unreachable }
    var fr = f.flush()
    if(fr is Result.Err) {
        var Err(e) = fr else unsafe { unreachable }
        var _ = close_file(&mut f)
        return Result.Err(e)
    }
    var cr = close_file(&mut f)
    if(cr is Result.Err) {
        var Err(e) = cr else unsafe { unreachable }
        return Result.Err(e)
    }
    return Result.Ok(n)
}

impl std::io::Read for File {

    func read(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        var r = file_read(self as *mut File, buf.data(), buf.size())
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(io_error(e))
        }
        var Ok(n) = r else unsafe { unreachable }
        return Result.Ok(n)
    }

    func read_exact(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        var pos : size_t = 0
        while(pos < buf.size()) {
            var r = file_read(self as *mut File, unsafe(buf.data() + pos), buf.size() - pos)
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(io_error(e))
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) {
                return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.UnexpectedEof()))
            }
            pos += n
        }
        return Result.Ok(pos)
    }

    func try_read_to_end(&mut self, out : &mut std::vector<u8>) : Result<size_t, std::io::IoError> {
        var chunk : [8192]u8
        var total : size_t = 0
        while(true) {
            var r = file_read(self as *mut File, &raw mut chunk[0], 8192 as size_t)
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(io_error(e))
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Ok(total) }
            var i : size_t = 0
            while(i < n) {
                if(!out.try_push(chunk[i])) {
                    return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.OutOfMemory()))
                }
                i += 1
            }
            total += n
        }
        return Result.Ok(total)
    }
}

impl std::io::Write for File {

    func write(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        var r = file_write(self as *mut File, buf.data(), buf.size())
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(io_error(e))
        }
        var Ok(n) = r else unsafe { unreachable }
        return Result.Ok(n)
    }

    func write_all(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        var pos : size_t = 0
        while(pos < buf.size()) {
            var r = file_write(self as *mut File, unsafe(buf.data() + pos), buf.size() - pos)
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(io_error(e))
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) {
                return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind.Other()))
            }
            pos += n
        }
        return Result.Ok(pos)
    }

    func flush(&mut self) : Result<size_t, std::io::IoError> {
        var r = file_flush(self as *mut File)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(io_error(e))
        }
        return Result.Ok(0 as size_t)
    }
}


// ---------------------------------------------------------------------------
// Safe directory/metadata helpers
// ---------------------------------------------------------------------------

public func create_dir_safe(path : &str) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = create_dir(unsafe(c.as_ptr()))
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func create_dir_all_safe(path : &str) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = create_dir_all(unsafe(c.as_ptr()))
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func remove_dir_safe(path : &str) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = remove_dir(unsafe(c.as_ptr()))
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func remove_dir_all_safe(path : &str) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = remove_dir_all_recursive(unsafe(c.as_ptr()))
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func remove_file_safe(path : &str) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = remove_file(unsafe(c.as_ptr()))
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func copy_file_safe(src : &str, dst : &str) : Result<size_t, std::io::IoError> {
    var m1 = std::cstring_make(src)
    var m2 = std::cstring_make(dst)
    if(m1 is Result.Err || m2 is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c1) = m1 else unsafe { unreachable }
    var Ok(c2) = m2 else unsafe { unreachable }
    var r = copy_file(unsafe(c1.as_ptr()), unsafe(c2.as_ptr()))
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func move_path_safe(src : &str, dst : &str) : Result<size_t, std::io::IoError> {
    var m1 = std::cstring_make(src)
    var m2 = std::cstring_make(dst)
    if(m1 is Result.Err || m2 is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c1) = m1 else unsafe { unreachable }
    var Ok(c2) = m2 else unsafe { unreachable }
    var r = move_path(unsafe(c1.as_ptr()), unsafe(c2.as_ptr()))
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func metadata_safe(path : &str) : Result<Metadata, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = metadata(unsafe(c.as_ptr()))
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    var Ok(m) = r else unsafe { unreachable }
    return Result.Ok(m)
}

public func exists_safe(path : &str) : bool {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return false }
    var Ok(c) = made else unsafe { unreachable }
    return exists(unsafe(c.as_ptr()))
}

public func is_file_safe(path : &str) : bool {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return false }
    var Ok(c) = made else unsafe { unreachable }
    var r = is_file(unsafe(c.as_ptr()))
    if(r is Result.Err) { return false }
    var Ok(v) = r else unsafe { unreachable }
    return v
}

public func is_dir_safe(path : &str) : bool {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return false }
    var Ok(c) = made else unsafe { unreachable }
    var r = is_dir(unsafe(c.as_ptr()))
    if(r is Result.Err) { return false }
    var Ok(v) = r else unsafe { unreachable }
    return v
}

public func atomic_write_safe(path : &str, data : [u8]) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = atomic_write(unsafe(c.as_ptr()), data.data(), data.size())
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(data.size())
}

public func read_dir_safe(path : &str, callback : std::function<(name : &std::string_view, is_dir : bool) => bool>) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var adapted : std::function<(name : *char, name_len : size_t, is_dir : bool) => bool> = |callback|(name, name_len, is_dir) => {
        var view = unsafe(std::string_view(name, name_len))
        return callback(&view, is_dir)
    }
    var r = read_dir(unsafe(c.as_ptr()), adapted)
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func temp_dir_path() : Option<std::string> {
    var buf : [4096]char
    var r = temp_dir(&raw mut buf[0], 4096)
    if(r is Result.Err) { return Option.None<std::string>() }
    return Option.Some<std::string>(unsafe(std::string.make_no_len(&raw mut buf[0])))
}

public func set_times_safe(path : &str, atime : i64, mtime : i64) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = set_times(unsafe(c.as_ptr()), atime, mtime)
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

public func set_permissions_safe(path : &str, perms : u32) : Result<size_t, std::io::IoError> {
    var made = std::cstring_make(path)
    if(made is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
    var Ok(c) = made else unsafe { unreachable }
    var r = set_permissions(unsafe(c.as_ptr()), perms)
    if(r is Result.Err) { var Err(e) = r else unsafe { unreachable }; return Result.Err(io_error(e)) }
    return Result.Ok(0 as size_t)
}

} // namespace fs
