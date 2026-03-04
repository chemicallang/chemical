// ===== Static file serving =====
public namespace http {

    public struct FileServer {
        var root: std::string;

        @constructor func constructor(root_path: *char) {
            return FileServer {
                root = std::string(root_path)
            }
        }

        func get_mime_type(ext: &std::string) : std::string_view {
            if (ext.equals_view(".html")) return std::string_view("text/html");
            if (ext.equals_view(".htm")) return std::string_view("text/html");
            if (ext.equals_view(".css")) return std::string_view("text/css");
            if (ext.equals_view(".js")) return std::string_view("application/javascript");
            if (ext.equals_view(".json")) return std::string_view("application/json");
            if (ext.equals_view(".png")) return std::string_view("image/png");
            if (ext.equals_view(".jpg")) return std::string_view("image/jpeg");
            if (ext.equals_view(".jpeg")) return std::string_view("image/jpeg");
            if (ext.equals_view(".gif")) return std::string_view("image/gif");
            if (ext.equals_view(".svg")) return std::string_view("image/svg+xml");
            if (ext.equals_view(".txt")) return std::string_view("text/plain");
            return std::string_view("application/octet-stream");
        }

        public func serve_http(&self, req: http.Request, res: http.ResponseWriter) {
            var path = req.path.copy();
            // Basic security: prevent traversal
            if (path.find(std::string_view("..")) != -1u) {
                res.status = 403u;
                res.write_view("Forbidden\n");
                return;
            }

            var full_path = self.root.copy();
            comptime if (def.windows) {
                if (!full_path.empty() && full_path.get(full_path.size() - 1u) != '\\' && full_path.get(full_path.size() - 1u) != '/') {
                    full_path.append('\\');
                }
            } else {
                if (!full_path.empty() && full_path.get(full_path.size() - 1u) != '/') {
                    full_path.append('/');
                }
            }

            // Strip leading slash from request path
            var req_path_view = path.to_view();
            if (req_path_view.size() > 0 && req_path_view.get(0) == '/') {
                req_path_view = req_path_view.subview(1, req_path_view.size());
            }
            full_path.append_view(req_path_view);

            // Check if it's a directory
            var is_dir = false;
            comptime if (def.windows) {
                var attrs = GetFileAttributesA(full_path.c_str());
                if (attrs != 0xFFFFFFFFu && (attrs & 0x10u) != 0u) {
                    is_dir = true;
                }
            } else {
                // We don't have Stat struct easily accessible without more plumbing, 
                // but we can try opendir as a proxy for "is directory"
                var d = opendir(full_path.c_str());
                if (d != null) {
                    is_dir = true;
                    closedir(d);
                }
            }

            if (is_dir) {
                comptime if (def.windows) {
                    if (full_path.get(full_path.size() - 1u) != '\\' && full_path.get(full_path.size() - 1u) != '/') {
                        full_path.append('\\');
                    }
                } else {
                    if (full_path.get(full_path.size() - 1u) != '/') {
                        full_path.append('/');
                    }
                }
                full_path.append_view(std::string_view("index.html"));
            }

            // Get extension for MIME type
            var ext = std::string();
            var i = full_path.size();
            while (i > 0u) {
                i = i - 1u;
                if (full_path.get(i) == '.') {
                    ext = full_path.substring(i, full_path.size());
                    break;
                }
                if (full_path.get(i) == '/' || full_path.get(i) == '\\') break;
            }

            var mime = self.get_mime_type(ext);
            var ok = res.send_file(full_path.to_view(), mime);
            if (!ok) {
                res.status = 404u;
                res.set_header_view("Content-Type", "text/plain; charset=utf-8");
                res.write_view("Not Found\n");
            }
        }
    }

    public func create_file_server(root: *char) : FileServer {
        return FileServer(root);
    }
}
