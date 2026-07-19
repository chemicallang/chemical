// ===== HTTP ResponseWriter =====
public namespace http {

    public const DEFAULT_READ_BUF = 8192u;

    public struct ResponseWriter {
        var sock: net::Socket;
        var headers: HeaderMap;
        var status: uint;
        var sent_headers: bool;
        var is_head: bool;

        @constructor func constructor(s: net::Socket, method : &std::string) {
            return ResponseWriter {
                sock = s;
                headers = HeaderMap()
                status = 200u;
                sent_headers = false;
                is_head = method.equals_view("HEAD")
            }
        }

        func set_header(&mut self, k: std::string, v: std::string) { headers.insert(k, v) }
        func set_header_view(&mut self, k : &std::string_view, v : &std::string_view) {
            headers.insert_view(k, v)
        }

        public func set_cors(&mut self, origin : &std::string_view) {
            set_header_view("Access-Control-Allow-Origin", origin)
            set_header_view("Access-Control-Allow-Methods", "GET, POST, OPTIONS, PUT, DELETE")
            set_header_view("Access-Control-Allow-Headers", "Content-Type, Authorization")
            set_header_view("Access-Control-Allow-Credentials", "true")
        }

        func send_headers(&mut self, content_len: usize) {
            if(sent_headers) { return }

            var out = std::string();
            out.append_view(std::string_view("HTTP/1.1 "));
            out.append_uinteger(status);
            out.append_view(std::string_view(" OK\r\n"));

            out.append_view(std::string_view("Server: chemical-http\r\n"));

            out.append_view(std::string_view("Content-Length: "));
            if (is_head) {
                out.append_uinteger(0u);
            } else {
                out.append_uinteger(content_len as ubigint);
            }
            out.append_view(std::string_view("\r\n"));

            var i = 0u;
            while(i < headers.headers.size()) {
                var p = headers.headers.get_ptr(i);
                out.append_string(&p.first);
                out.append_view(std::string_view(": "));
                out.append_string(&p.second);
                out.append_view(std::string_view("\r\n"));
                i = i + 1u;
            }

            out.append_view(std::string_view("Connection: close\r\n\r\n"));

            net::send_all(sock, out.data(), out.size() as int);
            sent_headers = true;
        }

        func write_string(&mut self, s: &std::string) {
            send_headers(s.size());
            if (!is_head) {
                net::send_all(sock, s.data(), s.size() as int);
            }
        }

        func write_view(&mut self, v : &std::string_view) {
            send_headers(v.size());
            if (!is_head) {
                net::send_all(sock, v.data(), v.size() as int);
            }
        }

        func send_file(&mut self, path_view : &std::string_view, content_type : &std::string_view) : bool {
            set_header_view(std::string_view("Content-Type"), content_type);
            const path = path_view.data()
            comptime if(def.windows) {
                var fh = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, 0u, 0 as HANDLE);
                if (fh == 0u || fh == -1 as uintptr_t) { return false; }
                var filesize: longlong = 0
                if (GetFileSizeEx(fh, (&mut filesize) as *mut LARGE_INTEGER) == 0) {
                    CloseHandle(fh); return false;
                }
                self.send_headers(filesize as usize);
                var ok = net::TransmitFile(self.sock as uintptr_t, fh as uintptr_t, 0u, 0u, null, null, 0u);
                CloseHandle(fh);
                return ok != 0;
            } else {
                var fd = open(path, O_RDONLY, 0);
                if (fd < 0) { return false; }
                var size_ll = net::lseek(fd, 0 as longlong, SEEK_END);
                if (size_ll < 0) { close(fd); return false; }
                if (net::lseek(fd, 0 as longlong, SEEK_SET) < 0) { close(fd); return false; }
                self.send_headers((size_ll as usize));
                var remaining = size_ll;
                var offset: longlong = 0;
                while (remaining > 0) {
                    var to_send = if(remaining > (1 << 30)) (1 << 30) as size_t else remaining as size_t;
                    var sent = net::sendfile(self.sock as int, fd, &raw mut offset, to_send);
                    if (sent <= 0) { break; }
                    remaining = size_ll - offset;
                }
                var success = (offset == size_ll);
                close(fd);
                return success;
            }
        }

        func finish(&mut self) {}
    }
}
