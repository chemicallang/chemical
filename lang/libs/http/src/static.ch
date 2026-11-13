// ===== Static file serving =====
public namespace http {

using std::Option;
using std::Result;
using std::string;
using std::string_view;

public struct FileServer {
    var root: string;

    @constructor func constructor(root_path : std::str) {
        var root = string()
        root.append_view(root_path.to_view())
        return FileServer {
            root : root
        }
    }

    func get_mime_type(ext: &string) : string_view {
        return mime::get_type(std::str.make_string(ext))
    }

    /// Map a request path to a response. Directories serve `index.html`;
    /// anything unreadable is a 404, traversal is a 403.
    public func serve_http(&self, req : &http.Request) : http::Response {
        var rel = req.path.to_view()
        var dots = string_view("..")
        if(rel.find_or_npos(&dots) != std::NPOS) {
            return http::response_text(403u, "Forbidden\n")
        }

        var full_path = self.root.copy()
        comptime if (def.windows) {
            if (!full_path.empty() && full_path.at(full_path.size() - 1u) != '\\' && full_path.at(full_path.size() - 1u) != '/') {
                full_path.append('\\')
            }
        } else {
            if (!full_path.empty() && full_path.at(full_path.size() - 1u) != '/') {
                full_path.append('/')
            }
        }
        if(rel.size() > 0u && rel.at(0u) == '/') {
            rel = rel.subview(1u, rel.size())
        }
        full_path.append_view(&rel)

        // Resolve the target: a directory means its index.html.
        var target = std::str.make_view(full_path.to_view())
        var p = path::Path.new(target)
        var md = fs::metadata(&p)
        if(md is Result.Err) {
            return http::response_text(404u, "Not Found\n")
        }
        var Ok(meta) = md else unsafe { unreachable }

        if(meta.is_dir) {
            comptime if (def.windows) {
                if (!full_path.empty() && full_path.at(full_path.size() - 1u) != '\\') {
                    full_path.append('\\')
                }
            } else {
                if (!full_path.empty() && full_path.at(full_path.size() - 1u) != '/') {
                    full_path.append('/')
                }
            }
            full_path.append_view("index.html")
            var target2 = std::str.make_view(full_path.to_view())
            var p2 = path::Path.new(target2)
            md = fs::metadata(&p2)
            if(md is Result.Err) {
                return http::response_text(404u, "Not Found\n")
            }
            var Ok(meta2) = md else unsafe { unreachable }
            meta = meta2
        }

        var ext = string()
        var i = full_path.size();
        while (i > 0u) {
            i = i - 1u;
            if (full_path.at(i) == '.') {
                ext = full_path.substring(i, full_path.size());
                break;
            }
            if (full_path.at(i) == '/' || full_path.at(i) == '\\') break;
        }

        var path_text = std::str.make_view(full_path.to_view())
        var path_ref = path::Path.new(path_text)
        var opened = fs::open_file(&path_ref)
        if(opened is Result.Err) {
            return http::response_text(404u, "Not Found\n")
        }
        var Ok(file) = opened else unsafe { unreachable }

        var res = http::Response()
        res.status = 200u
        var mime = self.get_mime_type(&ext)
        res.set_header_view("Content-Type", &mime)
        res.body = ResponseBody.File(file, meta.len as u64)
        return res
    }
}

public func create_file_server(root : std::str) : FileServer {
    return FileServer(root);
}

} // namespace http
