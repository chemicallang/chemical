// ===== Static file serving =====
public namespace http {

using std::Option;
using std::Result;
using std::string;
using std::string_view;

/// A weak validator for an on-disk file: its size and modification time, which is
/// what the file's bytes are a function of for any writer that replaces the file
/// (which is how a deploy ships one). Quoted, as RFC 9110 requires.
func file_etag(meta : &fs::Metadata) : string {
    var out = string()
    out.append('"')
    out.append_uinteger(meta.len as ubigint)
    out.append('-')
    out.append_integer(meta.modified)
    out.append('"')
    return out
}

/// True when `if_none_match` names `etag`. Handles `*`, a comma-separated list and
/// the weak `W/` prefix: RFC 9110 §13.1.2 requires the weak comparison here, so a
/// proxy that weakened our tag still gets its `304`.
func etag_list_matches(if_none_match : string_view, etag : string_view) : bool {
    var start : usize = 0u
    var i : usize = 0u
    while(i <= if_none_match.size()) {
        var at_end = i == if_none_match.size()
        if(at_end || if_none_match.at(i) == ',') {
            var token = if_none_match.subview(start, i)
            token = trim_ascii_space(token)
            if(text_equals(token, string_view("*"))) { return true }
            if(token.size() > 2u && token.at(0u) == 'W' && token.at(1u) == '/') {
                token = token.subview(2u, token.size())
            }
            if(text_equals(token, etag)) { return true }
            start = i + 1u
        }
        i = i + 1u
    }
    return false
}

func trim_ascii_space(value : string_view) : string_view {
    var first : usize = 0u
    var last = value.size()
    while(first < last) {
        var c = value.at(first)
        if(c != ' ' && c != '\t') { break }
        first = first + 1u
    }
    while(last > first) {
        var c = value.at(last - 1u)
        if(c != ' ' && c != '\t') { break }
        last = last - 1u
    }
    return value.subview(first, last)
}

/// True when `name`'s value in `req` lists `etag` (see `etag_list_matches`).
func request_matches_etag(req : &http.Request, name : &str, etag : string_view) : bool {
    var got = req.headers.get(name)
    if(got is Option.None) { return false }
    var Some(value) = got else unsafe { unreachable }
    return etag_list_matches(value, etag)
}

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

        // Validators first: a conditional GET is answered without opening the
        // file, which is the common case for a browser revisiting a page.
        var etag = file_etag(&meta)
        var etag_view = etag.to_view()
        // A zero mtime means the platform would not tell us one; an epoch date would
        // be a lie, so the header is omitted rather than faked.
        var last_modified = string()
        if(meta.modified > 0) { last_modified = http::format_http_date(meta.modified) }
        var last_modified_view = last_modified.to_view()
        if(request_matches_etag(req, "If-None-Match", etag_view)) {
            var cached = http::Response()
            cached.status = 304u
            cached.set_header_view("ETag", &etag_view)
            if(!last_modified_view.empty()) {
                cached.set_header_view("Last-Modified", &last_modified_view)
            }
            return cached
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
        res.set_header_view("ETag", &etag_view)
        if(!last_modified_view.empty()) {
            res.set_header_view("Last-Modified", &last_modified_view)
        }
        // A file body is rangeable (see `apply_byte_range`): the server seeks and
        // streams the slice, so a resumed download never reads the whole file.
        res.body = ResponseBody.File(file, meta.len as u64)
        return res
    }
}

public func create_file_server(root : std::str) : FileServer {
    return FileServer(root);
}

} // namespace http
