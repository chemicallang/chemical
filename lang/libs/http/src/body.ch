// ===== HTTP bodies =====
//
// Two distinct shapes, because the two directions are different problems:
//
//   * a request body is a *reader* the framework fills from the socket, so the
//     handler can consume it without ever touching a descriptor;
//   * a response body is a *payload* the framework writes, which may be memory
//     or a file on disk.
//
// Neither touches the connection. That is what lets `Handler` be a plain
// `(Request) => Response` callback with no raw pointer and no lifetime in its
// signature.
public namespace http {

using std::Result;
using std::Option;

/// A response payload. `Chunked`/`Stream` variants are additive when streaming
/// responses land; every current variant is self-describing (the encoder can
/// always compute `Content-Length`).
public variant ResponseBody {
    Empty()
    Bytes(data : std::vector<u8>)
    Text(text : std::string)
    File(file : fs::File, len : u64)

    public func size(&self) : u64 {
        switch(self) {
            Empty() => { return 0u }
            Bytes(d) => { return d.size() as u64 }
            Text(t) => { return t.size() as u64 }
            File(_, len) => { return len }
            default => { return 0u }
        }
    }

    func is_empty(&self) : bool {
        return self.size() == 0u
    }

    /// The payload as text, when it lives in memory.
    func text(&self) : Option<std::string_view> {
        switch(self) {
            Text(t) => { return Option.Some<std::string_view>(t.to_view()) }
            Bytes(d) => { return Option.Some<std::string_view>(unsafe(std::string_view(d.data() as *char, d.size()))) }
            default => { return Option.None<std::string_view>() }
        }
    }

    /// The payload as bytes, when it lives in memory.
    func data(&self) : Option<std::span<'self, u8>> {
        switch(self) {
            Bytes(d) => { return Option.Some<std::span<'self, u8>>(d.as_span()) }
            Text(t) => { return Option.Some<std::span<'self, u8>>(unsafe(std::span<u8>(t.data() as *u8, t.size()))) }
            default => { return Option.None<std::span<'self, u8>>() }
        }
    }
}

/// Builders keep handler bodies short: `http.Response.text_view(200, v)`.
public func (b: &ResponseBody) is_text() : bool {
    switch(b) {
        Text(_) => { return true }
        default => { return false }
    }
}

/// A request payload, fully buffered by the framework before dispatch, bounded
/// by `ServerConfig.max_body_bytes`. It is a reader: reading consumes.
public struct RequestBody {
    internal var buf : net::Buffer
    internal var total : size_t

    @constructor
    func constructor() {
        return RequestBody {
            buf : net::Buffer(),
            total : 0u
        }
    }

    /// Bytes the client sent (before any consumption).
    public func size(&self) : size_t { return self.total }

    /// Bytes still unread.
    public func remaining(&self) : size_t { return self.buf.len() }

    func is_empty(&self) : bool { return self.buf.len() == 0u }

    /// The unread bytes, without consuming them.
    public func view(&self) : std::span<'self, u8> { return self.buf.view() }

    /// The unread bytes interpreted as text (no UTF-8 validation; use
    /// `read_to_string` when you need a checked `string`).
    func text(&self) : std::string_view {
        var v = self.buf.view()
        return unsafe(std::string_view(v.data() as *char, v.size()))
    }

    public func read(&mut self, out : &mut [u8]) : Result<size_t, std::io::IoError> {
        var avail = self.buf.len()
        if(avail == 0u) { return Result.Ok(0 as size_t) }
        var take = avail
        if(out.size() < take) { take = out.size() }
        if(take > 0u) {
            unsafe { memcpy(out.data(), self.buf.as_ptr(), take) }
            self.buf.consume(take)
        }
        return Result.Ok(take)
    }

    public func read_exact(&mut self, out : &mut [u8]) : Result<std::Unit, std::io::IoError> {
        if(self.buf.len() < out.size()) {
            return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::UnexpectedEof()))
        }
        var r = self.read(out)
        if(r is Result.Err) { return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::Other())) }
        return Result.Ok(std::Unit { })
    }

    public func read_to_end(&mut self, out : &mut std::vector<u8>) : Result<size_t, std::io::IoError> {
        var taken = self.buf.take()
        var n = taken.size()
        var i : size_t = 0
        while(i < n) {
            if(!out.try_push(taken.at(i))) {
                return Result.Err(std::io::IoError.Kind(std::io::IoErrorKind::OutOfMemory()))
            }
            i += 1
        }
        return Result.Ok(n)
    }

    /// Consume the rest of the body as an owned string. The bytes are copied
    /// verbatim; callers that require valid UTF-8 should validate the result.
    public func read_to_string(&mut self) : Result<std::string, std::io::IoError> {
        var out = std::string()
        var view = self.buf.view()
        var text = unsafe(std::string_view(view.data() as *char, view.size()))
        out.append_view(&text)
        self.buf.consume(view.size())
        return Result.Ok(out)
    }

    /// Framework-side append. Not part of the handler-facing API.
    func append(&mut self, data : &[u8]) {
        self.buf.append(data)
        self.total += data.size()
    }

    /// Append raw bytes. Not part of the handler-facing API.
    func append_raw(&mut self, src : *u8, n : usize) {
        unsafe {
            self.buf.append_bytes(src, n)
            self.total += n
        }
    }

    func reserve_hint(&mut self, n : usize) {
        self.buf.reserve(n)
    }

    /// Move the remaining bytes out, leaving the body empty.
    func take_all(&mut self) : std::vector<u8> {
        return self.buf.take()
    }
}

} // namespace http
