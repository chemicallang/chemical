public namespace std {

// Shared I/O error taxonomy. Library-specific errors convert into this at the
// Read/Write/Seek boundary; rich library errors remain available on their own
// APIs. The success path never allocates.
public namespace io {

public variant IoErrorKind {
    NotFound()
    PermissionDenied()
    AlreadyExists()
    InvalidInput()
    UnexpectedEof()
    WouldBlock()
    TimedOut()
    ConnectionRefused()
    ConnectionReset()
    BrokenPipe()
    AddrInUse()
    AddrNotAvailable()
    Interrupted()
    OutOfMemory()
    PathTooLong()
    Unsupported()
    Other()
}

public variant IoError {
    Kind(kind : IoErrorKind)
    Message(kind : IoErrorKind, message : std::string)

    func kind(&self) : IoErrorKind {
        switch(self) {
            Kind(k) => { return k; }
            Message(k, _) => { return k; }
        }
    }

    func message(&self) : std::string_view {
        switch(self) {
            Kind(_) => { return std::string_view(""); }
            Message(_, message) => { return message.to_view(); }
        }
    }

    func has_message(&self) : bool {
        switch(self) {
            Kind(_) => { return false; }
            Message(_, _) => { return true; }
        }
    }
}

public variant SeekFrom {
    Start(offset : i64)
    Current(offset : i64)
    End(offset : i64)
}

/// Map a platform `errno` value onto the shared taxonomy, so a failed write is
/// reportable instead of silently dropped. Socket-specific codes (`WSA*`) go
/// through `net::io_error_from`, which delegates here for POSIX.
public func error_from_errno(code : int) : IoError {
    switch(code) {
        EAGAIN => { return IoError.Kind(IoErrorKind.WouldBlock()) }
        EWOULDBLOCK => { return IoError.Kind(IoErrorKind.WouldBlock()) }
        EINTR => { return IoError.Kind(IoErrorKind.Interrupted()) }
        EPIPE => { return IoError.Kind(IoErrorKind.BrokenPipe()) }
        ECONNRESET => { return IoError.Kind(IoErrorKind.ConnectionReset()) }
        ECONNREFUSED => { return IoError.Kind(IoErrorKind.ConnectionRefused()) }
        ETIMEDOUT => { return IoError.Kind(IoErrorKind.TimedOut()) }
        ENOENT => { return IoError.Kind(IoErrorKind.NotFound()) }
        EACCES => { return IoError.Kind(IoErrorKind.PermissionDenied()) }
        EEXIST => { return IoError.Kind(IoErrorKind.AlreadyExists()) }
        ENAMETOOLONG => { return IoError.Kind(IoErrorKind.PathTooLong()) }
        ENOSPC => { return IoError.Kind(IoErrorKind.Other()) }
        EIO => { return IoError.Kind(IoErrorKind.Other()) }
        EBADF => { return IoError.Kind(IoErrorKind.InvalidInput()) }
        EINVAL => { return IoError.Kind(IoErrorKind.InvalidInput()) }
        default => { return IoError.Kind(IoErrorKind.Other()) }
    }
}

public interface Read {
    func read(&mut self, buf : &mut [u8]) : Result<size_t, IoError>
    func read_exact(&mut self, buf : &mut [u8]) : Result<size_t, IoError>
    func try_read_to_end(&mut self, out : &mut vector<u8>) : Result<size_t, IoError>
}

public interface Write {
    func write(&mut self, buf : &[u8]) : Result<size_t, IoError>
    func write_all(&mut self, buf : &[u8]) : Result<size_t, IoError>
    func flush(&mut self) : Result<size_t, IoError>
}

public interface Seek {
    func seek(&mut self, pos : SeekFrom) : Result<u64, IoError>
}

} // namespace io
} // namespace std
