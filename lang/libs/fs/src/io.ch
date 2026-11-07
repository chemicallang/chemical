public namespace fs {

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

} // namespace fs
