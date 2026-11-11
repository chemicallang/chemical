public namespace fs {

using std::Result;
using path::Path;
using path::PathBuf;
using std::string;
using std::string_view;

public variant FsError {
    Io(code : int, message : string)
    NotFound()
    AlreadyExists()
    PermissionDenied()
    InvalidInput()
    NotADirectory()
    IsADirectory()
    WouldBlock()
    PathTooLong()
    Unsupported()   // used only for extremely exotic requests
    OutOfMemory()
    Other(msg : string)

    func message(&self) : std::string {
        switch(self) {
            Io(code, message) => {
                var msg = string("io error with code ")
                msg.append_integer(code)
                msg.append_view(std::string_view(" and message '"))
                msg.append_string(&message)
                msg.append('\'')
                return msg;
            }
            NotFound() => return string("FsError.NotFound")
            AlreadyExists() => return string("FsError.AlreadyExists")
            PermissionDenied() => return string("FsError.PermissionDenied")
            InvalidInput() => return string("FsError.InvalidInput")
            NotADirectory() => return string("FsError.NotADirectory")
            IsADirectory() => return string("FsError.IsADirectory")
            WouldBlock() => return string("FsError.WouldBlock")
            PathTooLong() => return string("FsError.PathTooLong")
            Unsupported() => return string("FsError.Unsupported")   // used only for extremely exotic requests
            OutOfMemory() => return string("FsError.OutOfMemory")
            Other(msg) => return msg.copy();
        }
    }

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

@direct_init
public struct OpenOptions {
    var read : bool;
    var write : bool;
    var append : bool;
    var create : bool;
    var create_new : bool; // exclusive create
    var truncate : bool;
    var binary : bool;

    @make
    func make() : OpenOptions {
        return OpenOptions {
            read : true,
            write : false,
            append : false,
            create : false,
            create_new : false,
            truncate : false,
            binary : true
        }
    }
}

public struct File {
    // Owned native state. Only this module (and its platform implementation
    // files) may inspect or mutate these fields; callers use `is_open` and the
    // Read/Write/Seek surface.
    // Platform-neutral owned handle: a POSIX fd or a Windows HANDLE. Platform
    // implementation files cast it to their native representation; callers use
    // the safe Read/Write/Seek surface and `is_open`.
    internal var handle : i64;
    internal var valid : bool;

    /// True while this handle owns an open native file.
    public func is_open(&self) : bool {
        return self.valid
    }

    /// Open an existing file with the given options.
    func open(path : &Path, opts : &OpenOptions) : Result<File, FsError> {
        if(path.has_nul()) { return Result.Err(FsError.InvalidInput()) }
        var native = path.native_ptr()
        if(native != null) {
            return raw::file_open_native(native, unsafe(*opts))
        }
        // No cached native form: hand the raw UTF-8 bytes to the portable
        // wrapper, which converts to the platform form (UTF-16 on Windows).
        var text = path.as_string()
        return raw::file_open(text.c_str(), unsafe(*opts))
    }

    /// Create or truncate a file for writing.
    func create(path : &Path) : Result<File, FsError> {
        var opts : OpenOptions
        opts.read = false
        opts.write = true
        opts.append = false
        opts.create = true
        opts.create_new = false
        opts.truncate = true
        opts.binary = true
        return File.open(path, &opts)
    }

    /// Metadata for this open handle.
    func metadata(&self) : Result<Metadata, FsError> {
        return raw::file_metadata(unsafe(self as *mut File))
    }

    @delete
    func delete(&mut self) {
        if(self.valid) {
            var _ = raw::file_close_ref(self)
        }
    }

}

/// One directory entry produced by `read_dir`.
protected struct DirEntryImpl {
    var path : PathBuf
    var name : string
    var is_dir : bool
}

public struct DirEntry {
    internal var _impl : *mut DirEntryImpl

    @delete
    func delete(&mut self) {
        if(self._impl != null) {
            unsafe { delete self._impl }
            self._impl = null
        }
    }

    /// Full path of this entry.
    func path(&self) : PathBuf {
        return unsafe(self._impl.path.to_path_buf())
    }

    func file_name(&self) : string {
        return unsafe(self._impl.name.copy())
    }

    func is_dir(&self) : bool {
        return unsafe(self._impl.is_dir)
    }

    func metadata(&self) : Result<Metadata, FsError> {
        var p = self.path()
        return fs::metadata(&p.as_path())
    }
}

} // namespace fs
