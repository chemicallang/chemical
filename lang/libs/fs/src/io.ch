public namespace fs {

using std::Option;
using std::Result;
using path::Path;
using path::PathBuf;
using std::string;
using std::string_view;
using std::vector;

// Conversion into the shared I/O error used by Read/Write/network APIs.
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
        OutOfMemory() => { return std::io::IoError.Kind(std::io::IoErrorKind.OutOfMemory()) }
        Io(_, message) => {
            return std::io::IoError.Message(std::io::IoErrorKind.Other(), message.copy())
        }
        Other(message) => {
            return std::io::IoError.Message(std::io::IoErrorKind.Other(), message.copy())
        }
    }
}

// Convert a shared I/O error back into a filesystem error. Used by wrappers
// whose implementation goes through the `Read`/`Write` traits.
public func fs_error_from_io(e : std::io::IoError) : FsError {
    var kind = e.kind()
    switch(kind) {
        NotFound() => { return FsError.NotFound() }
        PermissionDenied() => { return FsError.PermissionDenied() }
        AlreadyExists() => { return FsError.AlreadyExists() }
        InvalidInput() => { return FsError.InvalidInput() }
        UnexpectedEof() => { return FsError.Io(0, string("unexpected end of file")) }
        WouldBlock() => { return FsError.WouldBlock() }
        TimedOut() => { return FsError.WouldBlock() }
        ConnectionRefused() => { return FsError.Io(-1, string("connection refused")) }
        ConnectionReset() => { return FsError.Io(-1, string("connection reset")) }
        BrokenPipe() => { return FsError.Io(-1, string("broken pipe")) }
        AddrInUse() => { return FsError.Io(-1, string("address in use")) }
        AddrNotAvailable() => { return FsError.Io(-1, string("address not available")) }
        Interrupted() => { return FsError.Io(-1, string("interrupted")) }
        OutOfMemory() => { return FsError.OutOfMemory() }
        PathTooLong() => { return FsError.PathTooLong() }
        Unsupported() => { return FsError.Unsupported() }
        Other() => {
            if(e.has_message()) {
                return FsError.Other(string(e.message()))
            }
            return FsError.Other(string("I/O error"))
        }
    }
}

// ---------------------------------------------------------------------------
// File construction
// ---------------------------------------------------------------------------

/// Open an existing file with default read-only options.
public func open_file(path : &Path) : Result<File, FsError> {
    var opts = OpenOptions.make()
    return File.open(path, &opts)
}

/// Create/truncate a file for writing.
public func create_file(path : &Path) : Result<File, FsError> {
    return File.create(path)
}

/// Explicitly close a file. Idempotent; the destructor uses the same path.
public func close_file(f : &mut File) : Result<UnitTy, FsError> {
    return raw::file_close_ref(f)
}

// ---------------------------------------------------------------------------
// Whole-file helpers
// ---------------------------------------------------------------------------

/// Read the entire file into a byte vector.
public func read(path : &Path) : Result<vector<u8>, FsError> {
    var opened = open_file(path)
    if(opened is Result.Err) {
        var Err(e) = opened else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(f) = opened else unsafe { unreachable }
    var out = vector<u8>()
    var chunk : [8192]u8
    while(true) {
        var r = raw::file_read(&raw mut f, &raw mut chunk[0], 8192 as size_t)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            var _ = raw::file_close_ref(&mut f)
            return Result.Err(e)
        }
        var Ok(n) = r else unsafe { unreachable }
        if(n == 0) {
            var _ = raw::file_close_ref(&mut f)
            return Result.Ok(out)
        }
        var i : size_t = 0
        while(i < n) {
            if(!out.try_push(chunk[i])) {
                var _ = raw::file_close_ref(&mut f)
                return Result.Err(FsError.OutOfMemory())
            }
            i += 1
        }
    }
    var _ = raw::file_close_ref(&mut f)
    return Result.Ok(out)
}

/// Write all bytes to a file, creating/truncating it.
public func write(path : &Path, data : &[u8]) : Result<UnitTy, FsError> {
    var opened = File.create(path)
    if(opened is Result.Err) {
        var Err(e) = opened else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(f) = opened else unsafe { unreachable }
    var r = raw::file_write_all(&raw mut f, data.data(), data.size())
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        var _ = raw::file_close_ref(&mut f)
        return Result.Err(e)
    }
    var fr = raw::file_flush(&raw mut f)
    if(fr is Result.Err) {
        var Err(e) = fr else unsafe { unreachable }
        var _ = raw::file_close_ref(&mut f)
        return Result.Err(e)
    }
    return raw::file_close_ref(&mut f)
}

/// Read a file and validate it as UTF-8.
public func read_to_string(path : &Path) : Result<string, FsError> {
    var r = read(path)
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        return Result.Err(e)
    }
    var Ok(bytes) = r else unsafe { unreachable }
    var view = unsafe(string_view(bytes.data() as *char, bytes.size()))
    var made = std::try_from_utf8(view)
    if(made is Result.Err) {
        return Result.Err(FsError.InvalidInput())
    }
    var Ok(text) = made else unsafe { unreachable }
    return Result.Ok(string(text.to_view()))
}

// ---------------------------------------------------------------------------
// Directory and metadata helpers
// ---------------------------------------------------------------------------

public func create_dir(path : &Path) : Result<UnitTy, FsError> {
    if(path.has_nul()) { return Result.Err(FsError.InvalidInput()) }
    var native = path.native_ptr()
    if(native != null) { return raw::create_dir_native(native) }
    var text = path.as_string()
    return raw::create_dir(text.c_str())
}

public func create_dir_all(path : &Path) : Result<UnitTy, FsError> {
    if(path.has_nul()) { return Result.Err(FsError.InvalidInput()) }
    var text = path.as_string()
    return raw::create_dir_all(text.c_str())
}

public func remove_dir(path : &Path) : Result<UnitTy, FsError> {
    if(path.has_nul()) { return Result.Err(FsError.InvalidInput()) }
    var native = path.native_ptr()
    if(native != null) { return raw::remove_dir_native(native) }
    var text = path.as_string()
    return raw::remove_dir(text.c_str())
}

public func remove_dir_all(path : &Path) : Result<UnitTy, FsError> {
    if(path.has_nul()) { return Result.Err(FsError.InvalidInput()) }
    var native = path.native_ptr()
    if(native != null) { return raw::remove_dir_all_recursive_native(native) }
    var text = path.as_string()
    return raw::remove_dir_all_recursive(text.c_str())
}

public func remove_file(path : &Path) : Result<UnitTy, FsError> {
    if(path.has_nul()) { return Result.Err(FsError.InvalidInput()) }
    var native = path.native_ptr()
    if(native != null) { return raw::remove_file_native(native) }
    var text = path.as_string()
    return raw::remove_file(text.c_str())
}

public func copy_file(src : &Path, dst : &Path) : Result<UnitTy, FsError> {
    if(src.has_nul() || dst.has_nul()) { return Result.Err(FsError.InvalidInput()) }
    var snative = src.native_ptr()
    var dnative = dst.native_ptr()
    if(snative != null && dnative != null) {
        return raw::copy_file_native(snative, dnative)
    }
    var stext = src.as_string()
    var dtext = dst.as_string()
    return raw::copy_file(stext.c_str(), dtext.c_str())
}

public func move_path(src : &Path, dst : &Path) : Result<UnitTy, FsError> {
    if(src.has_nul() || dst.has_nul()) { return Result.Err(FsError.InvalidInput()) }
    var stext = src.as_string()
    var dtext = dst.as_string()
    return raw::move_path(stext.c_str(), dtext.c_str())
}

public func metadata(path : &Path) : Result<Metadata, FsError> {
    if(path.has_nul()) { return Result.Err(FsError.InvalidInput()) }
    var text = path.as_string()
    return raw::metadata(text.c_str())
}

public func exists(path : &Path) : bool {
    var r = metadata(path)
    return !(r is Result.Err)
}

public func is_file(path : &Path) : bool {
    var r = metadata(path)
    if(r is Result.Err) { return false }
    var Ok(m) = r else unsafe { unreachable }
    return m.is_file
}

public func is_dir(path : &Path) : bool {
    var r = metadata(path)
    if(r is Result.Err) { return false }
    var Ok(m) = r else unsafe { unreachable }
    return m.is_dir
}

// State shared with the callback-style raw directory walker.
struct ReadDirState {
    var base : path::PathBuf
    var out : vector<DirEntry>
    var failed : bool
    var err : FsError
}

/// Read all directory entries (excluding `.` and `..`) as owned values.
public func read_dir(path : &Path) : Result<vector<DirEntry>, FsError> {
    var state = ReadDirState {
        base : path.to_path_buf(),
        out : vector<DirEntry>(),
        failed : false,
        err : FsError.InvalidInput()
    }
    var statep = &raw mut state
    var adapted : std::function<(name : *char, name_len : size_t, is_dir : bool) => bool> = |statep|(name, name_len, is_dir) => {
        var state = statep
        var nview = unsafe(string_view(name, name_len))
        if(nview.size() == 1 && nview.at(0) == '.') { return true }
        if(nview.size() == 2 && nview.at(0) == '.' && nview.at(1) == '.') { return true }
        var made = std::try_from_utf8(nview)
        if(made is Result.Err) {
            unsafe { state.failed = true; state.err = FsError.InvalidInput() }
            return false
        }
        var Ok(text) = made else unsafe { unreachable }
        var child = unsafe(state.base.join(&Path.new(text)))
        var name_str = string(text.to_view())
        var impl = new DirEntryImpl {
            path : PathBuf.from(""),
            name : string(),
            is_dir : is_dir
        }
        unsafe {
            impl.path = std::replace<PathBuf>(&mut child, std::mem.zeroed_unchecked<PathBuf>())
            impl.name = std::replace<string>(&mut name_str, std::mem.zeroed_unchecked<string>())
        }
        if(!unsafe(state.out.try_push(DirEntry { _impl : impl }))) {
            unsafe { state.failed = true; state.err = FsError.OutOfMemory() }
            return false
        }
        return true
    }
    var text = path.as_string()
    var r = raw::read_dir(text.c_str(), adapted)
    if(r is Result.Err) {
        var Err(e) = r else unsafe { unreachable }
        return Result.Err(e)
    }
    if(state.failed) {
        return Result.Err(state.err)
    }
    var result = std::replace<std::vector<DirEntry>>(&mut state.out, unsafe(std::mem.zeroed_unchecked<std::vector<DirEntry>>()))
    return Result.Ok(result)
}

// ---------------------------------------------------------------------------
// Read / Write / Seek implementations for File
// ---------------------------------------------------------------------------

impl std::io::Read for File {

    func read(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        var r = raw::file_read(self as *mut File, buf.data(), buf.size())
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
            var r = raw::file_read(self as *mut File, unsafe(buf.data() + pos), buf.size() - pos)
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
            var r = raw::file_read(self as *mut File, &raw mut chunk[0], 8192 as size_t)
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
        var r = raw::file_write(self as *mut File, buf.data(), buf.size())
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
            var r = raw::file_write(self as *mut File, unsafe(buf.data() + pos), buf.size() - pos)
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
        var r = raw::file_flush(self as *mut File)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(io_error(e))
        }
        return Result.Ok(0 as size_t)
    }
}

impl std::io::Seek for File {

    func seek(&mut self, pos : std::io::SeekFrom) : Result<u64, std::io::IoError> {
        var offset : i64 = 0
        var whence : int = 0
        switch(pos) {
            Start(o) => { offset = o; whence = 0 }
            Current(o) => { offset = o; whence = 1 }
            End(o) => { offset = o; whence = 2 }
        }
        var r = raw::file_seek(self as *mut File, offset, whence)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(io_error(e))
        }
        var Ok(n) = r else unsafe { unreachable }
        return Result.Ok(n)
    }
}

} // namespace fs
