// mime — Content-type detection by file extension.
// Extracted from http::FileServer.get_mime_type for standalone reuse.
//
// The normal API takes `&str` (string literals, string views, and `std::str`
// values all work) and never exposes a raw pointer. The `*_cstr` functions are
// the legacy C-string entry points kept for existing C-style callers.

public namespace mime {

    // Look up a MIME content-type string by file extension (including the dot).
    // Returns "application/octet-stream" for unknown extensions.
    public func get_type(ext : &str) : std::string_view {
        var v = ext.to_view()
        if(v.equals(std::string_view(".html"))) return std::string_view("text/html")
        if(v.equals(std::string_view(".htm"))) return std::string_view("text/html")
        if(v.equals(std::string_view(".css"))) return std::string_view("text/css")
        if(v.equals(std::string_view(".js"))) return std::string_view("application/javascript")
        if(v.equals(std::string_view(".json"))) return std::string_view("application/json")
        if(v.equals(std::string_view(".xml"))) return std::string_view("application/xml")
        if(v.equals(std::string_view(".svg"))) return std::string_view("image/svg+xml")
        if(v.equals(std::string_view(".png"))) return std::string_view("image/png")
        if(v.equals(std::string_view(".jpg"))) return std::string_view("image/jpeg")
        if(v.equals(std::string_view(".jpeg"))) return std::string_view("image/jpeg")
        if(v.equals(std::string_view(".gif"))) return std::string_view("image/gif")
        if(v.equals(std::string_view(".webp"))) return std::string_view("image/webp")
        if(v.equals(std::string_view(".ico"))) return std::string_view("image/x-icon")
        if(v.equals(std::string_view(".bmp"))) return std::string_view("image/bmp")
        if(v.equals(std::string_view(".ppm"))) return std::string_view("image/x-portable-pixmap")
        if(v.equals(std::string_view(".wav"))) return std::string_view("audio/wav")
        if(v.equals(std::string_view(".mp3"))) return std::string_view("audio/mpeg")
        if(v.equals(std::string_view(".ogg"))) return std::string_view("audio/ogg")
        if(v.equals(std::string_view(".mp4"))) return std::string_view("video/mp4")
        if(v.equals(std::string_view(".webm"))) return std::string_view("video/webm")
        if(v.equals(std::string_view(".ttf"))) return std::string_view("font/ttf")
        if(v.equals(std::string_view(".otf"))) return std::string_view("font/otf")
        if(v.equals(std::string_view(".woff"))) return std::string_view("font/woff")
        if(v.equals(std::string_view(".woff2"))) return std::string_view("font/woff2")
        if(v.equals(std::string_view(".txt"))) return std::string_view("text/plain")
        if(v.equals(std::string_view(".csv"))) return std::string_view("text/csv")
        if(v.equals(std::string_view(".md"))) return std::string_view("text/markdown")
        if(v.equals(std::string_view(".pdf"))) return std::string_view("application/pdf")
        if(v.equals(std::string_view(".zip"))) return std::string_view("application/zip")
        if(v.equals(std::string_view(".gz"))) return std::string_view("application/gzip")
        if(v.equals(std::string_view(".tar"))) return std::string_view("application/x-tar")
        if(v.equals(std::string_view(".wasm"))) return std::string_view("application/wasm")
        return std::string_view("application/octet-stream")
    }

    // Legacy C-string form of `get_type`. New code should pass a `&str`.
    public func get_type_cstr(ext : *char) : std::string_view {
        return get_type(std::str.make_view(unsafe(std::string_view.make_no_len(ext))))
    }

    // Extract the file extension from a path (including the dot), or "" if none.
    // Example: "/foo/bar.css" -> ".css". The result owns its bytes.
    public func extension(path : &str) : std::string {
        var view = path.to_view()
        var last_dot : size_t = 0
        var found_dot = false
        var last_slash : size_t = 0
        var found_slash = false
        var i = view.size()
        while(i > 0) {
            i -= 1
            var c = view.get(i)
            if(c == '.' && !found_dot) { last_dot = i; found_dot = true }
            else if(c == '/' || c == '\\') { last_slash = i; found_slash = true; break }
        }
        if(found_dot && (!found_slash || last_dot > last_slash)) {
            var result = std::string()
            var j = last_dot
            while(j < view.size()) {
                result.append(view.get(j))
                j += 1
            }
            return result
        }
        return std::string()
    }

    // Legacy C-string form of `extension`. New code should pass a `&str`.
    public func extension_cstr(path : *char) : std::string {
        unsafe {
            var last_dot : size_t = 0
            var found_dot = false
            var last_slash : size_t = 0
            var found_slash = false
            var i : size_t = 0
            // find end of string
            while(path[i] != 0) { i += 1 }
            var len = i
            i = len
            while(i > 0) {
                i -= 1
                var c = path[i]
                if(c == '.' && !found_dot) { last_dot = i; found_dot = true }
                else if(c == '/' || c == '\\') { last_slash = i; found_slash = true; break }
            }
            if(found_dot && (!found_slash || last_dot > last_slash)) {
                // Create a string from the dot onwards
                var result = std::string()
                var j = last_dot
                while(j < len) {
                    result.append(path[j] as char)
                    j += 1
                }
                return result
            }
            return std::string()
        }
    }

    // Check if a MIME type is a text type.
    public func is_text(content_type : &str) : bool {
        var v = content_type.to_view()
        if(v.equals(std::string_view("text/plain"))) return true
        if(v.equals(std::string_view("text/html"))) return true
        if(v.equals(std::string_view("text/css"))) return true
        if(v.equals(std::string_view("text/csv"))) return true
        if(v.equals(std::string_view("text/markdown"))) return true
        if(v.equals(std::string_view("application/javascript"))) return true
        if(v.equals(std::string_view("application/json"))) return true
        if(v.equals(std::string_view("application/xml"))) return true
        return false
    }

    // Legacy C-string form of `is_text`. New code should pass a `&str`.
    public func is_text_cstr(content_type : *char) : bool {
        return is_text(std::str.make_view(unsafe(std::string_view.make_no_len(content_type))))
    }

    // Check if a MIME type is an image type.
    public func is_image(content_type : &str) : bool {
        var v = content_type.to_view()
        if(v.equals(std::string_view("image/png"))) return true
        if(v.equals(std::string_view("image/jpeg"))) return true
        if(v.equals(std::string_view("image/gif"))) return true
        if(v.equals(std::string_view("image/webp"))) return true
        if(v.equals(std::string_view("image/svg+xml"))) return true
        if(v.equals(std::string_view("image/x-icon"))) return true
        if(v.equals(std::string_view("image/bmp"))) return true
        return false
    }

    // Legacy C-string form of `is_image`. New code should pass a `&str`.
    public func is_image_cstr(content_type : *char) : bool {
        return is_image(std::str.make_view(unsafe(std::string_view.make_no_len(content_type))))
    }

} // end namespace mime
