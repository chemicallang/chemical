// ===== HTTP ResponseWriter =====
public namespace http {

using std::string;
using std::string_view;

public const DEFAULT_READ_BUF = 8192u;

public struct ResponseWriter {
    internal var transport: *mut Transport;
    internal var headers: HeaderMap;
    var status: uint;
    internal var sent_headers: bool;
    internal var is_head: bool;

    @constructor func constructor(transport : *mut Transport, method : &string) {
        return ResponseWriter {
            transport = transport,
            headers = HeaderMap()
            status = 200u;
            sent_headers = false;
            is_head = method.equals_view("HEAD"),
        }
    }

    func set_header(&mut self, k: string, v: string) { headers.insert(k, v) }
    func set_header_view(&mut self, k : &string_view, v : &string_view) {
        headers.insert_view(k, v)
    }

    public func set_cors(&mut self, origin : &string_view) {
        set_header_view("Access-Control-Allow-Origin", origin)
        set_header_view("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE")
        set_header_view("Access-Control-Allow-Headers", "Content-Type, Authorization")
        set_header_view("Access-Control-Allow-Credentials", "true")
    }

    func send_data(&mut self, data : *char, len : int) {
        if(self.transport == null) { return }
        if(len <= 0) { return }
        var view = unsafe(std::span<u8>(data as *u8, len as size_t))
        var _ = unsafe { self.transport.write_all_bytes(view) }
    }

    func send_headers(&mut self, content_len: usize) {
        if(sent_headers) { return }

        var out = string();
        out.append_view(string_view("HTTP/1.1 "));
        out.append_uinteger(status);
        out.append_view(string_view(" OK\r\n"));

        out.append_view(string_view("Server: chemical-http\r\n"));

        out.append_view(string_view("Content-Length: "));
        if (is_head) {
            out.append_uinteger(0u);
        } else {
            out.append_uinteger(content_len as ubigint);
        }
        out.append_view(string_view("\r\n"));

        var i = 0u;
        while(i < headers.headers.size()) {
            var p = headers.headers.get_ptr(i);
            unsafe { out.append_string(&p.first); }
            out.append_view(string_view(": "));
            unsafe { out.append_string(&p.second); }
            out.append_view(string_view("\r\n"));
            i = i + 1u;
        }

        out.append_view(string_view("Connection: close\r\n\r\n"));

        send_data(out.data(), out.size() as int);
        sent_headers = true;
    }

    public func write_string(&mut self, s: &string) {
        send_headers(s.size());
        if (!is_head) {
            send_data(s.data(), s.size() as int);
        }
    }

    public func write_view(&mut self, v : &string_view) {
        send_headers(v.size());
        if (!is_head) {
            send_data(v.data(), v.size() as int);
        }
    }

    public func send_file(&mut self, path_view : &string_view, content_type : &string_view) : bool {
        if(path_view.data() == null || path_view.size() == 0u) { return false }
        set_header_view("Content-Type", content_type);
        // `open`/`CreateFileA` require a NUL-terminated path; a view over a
        // substring need not be terminated.
        var path = unsafe(string(path_view.data(), path_view.size()))
        var fd = unsafe(open(path.data(), O_RDONLY, 0));
        if (fd < 0) { return false; }
        var size_ll = unsafe(net::lseek(fd, 0 as longlong, SEEK_END));
        if (size_ll < 0) { unsafe { close(fd) }; return false; }
        if (unsafe(net::lseek(fd, 0 as longlong, SEEK_SET)) < 0) { unsafe { close(fd) }; return false; }
        self.send_headers((size_ll as usize));
        var remaining = size_ll;
        var buf : [65536]u8
        while (remaining > 0) {
            var want : ulong = 65536
            if(remaining < 65536) { want = remaining as ulong }
            var n = unsafe(read(fd, &raw mut buf[0] as *mut void, want))
            if (n <= 0) { break; }
            if(self.transport != null && !is_head) {
                var view = unsafe(std::span<u8>(&raw mut buf[0], n as size_t))
                var wr = unsafe { self.transport.write_all_bytes(view) }
                if(wr is std::Result.Err) { break }
            }
            remaining = remaining - n
        }
        var success = (remaining <= 0);
        unsafe { close(fd) };
        return success;
    }

    public func send_json(&mut self, value : &JsonValue) {
        var body = json::stringify(value)
        set_header_view(string_view("Content-Type"), string_view("application/json"))
        write_string(&body)
    }

    func finish(&mut self) {}
}

} // namespace http
