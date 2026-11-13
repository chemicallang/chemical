// ===== Connection stream =====
//
// One owning connection handle for every transport the library speaks. The
// previous representation was a tagged struct of three nullable pointers behind
// `*mut Transport` boxes, which every consumer (request body, response writer,
// client, server) aliased. A variant owns the stream by value, so there is one
// owner, one destructor, and no descriptor or TLS context can be duplicated or
// leaked by copying a handle.
//
// The payload methods are dispatched by reference: `switch(s)` on a `&mut
// Stream` binds a mutable reference to the live payload, so reads, writes and
// shutdown reach the real socket/TLS context.
public namespace http {

using std::Result;

public variant Stream {
    Tcp(stream : net::TcpStream)
    TlsClient(stream : tls::TlsStream)
    TlsServer(stream : tls::TlsServerStream)

    /// Read up to `buf.size()` bytes. Returns 0 at end of stream.
    public func read_bytes(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        switch(self) {
            Tcp(s) => { return s.read_bytes(buf) }
            TlsClient(s) => { return s.read_bytes(buf) }
            TlsServer(s) => { return s.read_bytes(buf) }
            default => { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
        }
    }

    public func write_bytes(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        switch(self) {
            Tcp(s) => { return s.write_bytes(buf) }
            TlsClient(s) => { return s.write_bytes(buf) }
            TlsServer(s) => { return s.write_bytes(buf) }
            default => { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::InvalidInput())) }
        }
    }

    /// Write the whole slice, retrying short writes. The underlying error is
    /// propagated as-is so callers can distinguish a slow peer from a dead one.
    public func write_all_bytes(&mut self, buf : &[u8]) : Result<std::Unit, std::io::IoError> {
        var pos : size_t = 0
        while(pos < buf.size()) {
            var sub : [u8] = buf.subview(pos, buf.size())
            var r = self.write_bytes(sub)
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(e)
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::BrokenPipe())) }
            pos += n
        }
        return Result.Ok(std::Unit { })
    }

    public func read_exact_bytes(&mut self, buf : &mut [u8]) : Result<std::Unit, std::io::IoError> {
        var pos : size_t = 0
        while(pos < buf.size()) {
            var sub = unsafe(std::mut_span<u8>(unsafe(buf.data() + pos), buf.size() - pos))
            var r = self.read_bytes(sub)
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(e)
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof())) }
            pos += n
        }
        return Result.Ok(std::Unit { })
    }

    public func set_read_timeout(&mut self, ms : u64) : Result<std::Unit, std::io::IoError> {
        switch(self) {
            Tcp(s) => { return s.set_read_timeout(ms) }
            TlsClient(s) => { return s.set_read_timeout(ms) }
            TlsServer(s) => { return s.set_read_timeout(ms) }
            default => { return Result.Ok(std::Unit { }) }
        }
    }

    /// Half-close politely: TLS sends `close_notify`, TCP shuts both directions.
    public func shutdown(&mut self) : Result<std::Unit, std::io::IoError> {
        switch(self) {
            Tcp(s) => { return s.shutdown(net::Shutdown.Both()) }
            TlsClient(s) => { return s.close_notify() }
            TlsServer(s) => { return s.close_notify() }
            default => { return Result.Ok(std::Unit { }) }
        }
    }

    public func is_tls(&self) : bool {
        switch(self) {
            Tcp(_) => { return false }
            default => { return true }
        }
    }

    /// The OS descriptor, or 0 for a TLS stream (the TLS context owns it).
    /// Only for platform event loops; not part of the safe surface.
    func raw_fd(&self) : net::raw::Socket {
        switch(self) {
            Tcp(s) => { return net::raw::stream_fd(&s) }
            default => { return 0 as net::raw::Socket }
        }
    }

    /// Give up ownership of the descriptor of a plain TCP stream (Windows IOCP
    /// hands the socket back to its completion port). Returns 0 otherwise.
    func detach(&mut self) : net::raw::Socket {
        switch(self) {
            Tcp(s) => { return unsafe(net::raw::release_tcp(&mut s)) }
            default => { return 0 as net::raw::Socket }
        }
    }
}

impl std::io::Read for Stream {
    func read(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        return self.read_bytes(buf)
    }
    func read_exact(&mut self, buf : &mut [u8]) : Result<size_t, std::io::IoError> {
        var r = self.read_exact_bytes(buf)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        return Result.Ok(buf.size())
    }
    func try_read_to_end(&mut self, out : &mut std::vector<u8>) : Result<size_t, std::io::IoError> {
        var chunk : [8192]u8
        var total : size_t = 0
        loop {
            var view = unsafe(std::mut_span<u8>(&raw mut chunk[0], 8192 as size_t))
            var r = self.read_bytes(view)
            if(r is Result.Err) {
                var Err(e) = r else unsafe { unreachable }
                return Result.Err(e)
            }
            var Ok(n) = r else unsafe { unreachable }
            if(n == 0) { return Result.Ok(total) }
            var i : size_t = 0
            while(i < n) {
                if(!out.try_push(chunk[i])) {
                    return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory()))
                }
                i += 1
            }
            total += n
        }
        return Result.Ok(total)
    }
}

impl std::io::Write for Stream {
    func write(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        return self.write_bytes(buf)
    }
    func write_all(&mut self, buf : &[u8]) : Result<size_t, std::io::IoError> {
        var r = self.write_all_bytes(buf)
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        return Result.Ok(buf.size())
    }
    func flush(&mut self) : Result<std::Unit, std::io::IoError> {
        return Result.Ok(std::Unit { })
    }
}

} // namespace http
