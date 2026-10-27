// mime — Content-type detection by file extension.
// Extracted from http::FileServer.get_mime_type for standalone reuse.

public namespace mime {

    // Look up a MIME content-type string by file extension (including the dot).
    // Returns "application/octet-stream" for unknown extensions.
    public func get_type(ext : *char) : std.string_view {
        if(unsafe(strcmp(ext, ".html")) == 0) return std.string_view("text/html")
        if(unsafe(strcmp(ext, ".htm")) == 0) return std.string_view("text/html")
        if(unsafe(strcmp(ext, ".css")) == 0) return std.string_view("text/css")
        if(unsafe(strcmp(ext, ".js")) == 0) return std.string_view("application/javascript")
        if(unsafe(strcmp(ext, ".json")) == 0) return std.string_view("application/json")
        if(unsafe(strcmp(ext, ".xml")) == 0) return std.string_view("application/xml")
        if(unsafe(strcmp(ext, ".svg")) == 0) return std.string_view("image/svg+xml")
        if(unsafe(strcmp(ext, ".png")) == 0) return std.string_view("image/png")
        if(unsafe(strcmp(ext, ".jpg")) == 0) return std.string_view("image/jpeg")
        if(unsafe(strcmp(ext, ".jpeg")) == 0) return std.string_view("image/jpeg")
        if(unsafe(strcmp(ext, ".gif")) == 0) return std.string_view("image/gif")
        if(unsafe(strcmp(ext, ".webp")) == 0) return std.string_view("image/webp")
        if(unsafe(strcmp(ext, ".ico")) == 0) return std.string_view("image/x-icon")
        if(unsafe(strcmp(ext, ".bmp")) == 0) return std.string_view("image/bmp")
        if(unsafe(strcmp(ext, ".ppm")) == 0) return std.string_view("image/x-portable-pixmap")
        if(unsafe(strcmp(ext, ".wav")) == 0) return std.string_view("audio/wav")
        if(unsafe(strcmp(ext, ".mp3")) == 0) return std.string_view("audio/mpeg")
        if(unsafe(strcmp(ext, ".ogg")) == 0) return std.string_view("audio/ogg")
        if(unsafe(strcmp(ext, ".mp4")) == 0) return std.string_view("video/mp4")
        if(unsafe(strcmp(ext, ".webm")) == 0) return std.string_view("video/webm")
        if(unsafe(strcmp(ext, ".ttf")) == 0) return std.string_view("font/ttf")
        if(unsafe(strcmp(ext, ".otf")) == 0) return std.string_view("font/otf")
        if(unsafe(strcmp(ext, ".woff")) == 0) return std.string_view("font/woff")
        if(unsafe(strcmp(ext, ".woff2")) == 0) return std.string_view("font/woff2")
        if(unsafe(strcmp(ext, ".txt")) == 0) return std.string_view("text/plain")
        if(unsafe(strcmp(ext, ".csv")) == 0) return std.string_view("text/csv")
        if(unsafe(strcmp(ext, ".md")) == 0) return std.string_view("text/markdown")
        if(unsafe(strcmp(ext, ".pdf")) == 0) return std.string_view("application/pdf")
        if(unsafe(strcmp(ext, ".zip")) == 0) return std.string_view("application/zip")
        if(unsafe(strcmp(ext, ".gz")) == 0) return std.string_view("application/gzip")
        if(unsafe(strcmp(ext, ".tar")) == 0) return std.string_view("application/x-tar")
        if(unsafe(strcmp(ext, ".wasm")) == 0) return std.string_view("application/wasm")
        return std.string_view("application/octet-stream")
    }

    // Extract the file extension from a path (including the dot), or "" if none.
    // Example: "/foo/bar.css" -> ".css"
    public func extension(path : *char) : std.string {
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
            var result = std.string()
            var j = last_dot
            while(j < len) {
                result.append(path[j] as char)
                j += 1
            }
            return result
        }
        return std.string()
    }

    // Check if a MIME type is a text type.
    public func is_text(content_type : *char) : bool {
        if(unsafe(strcmp(content_type, "text/plain")) == 0) return true
        if(unsafe(strcmp(content_type, "text/html")) == 0) return true
        if(unsafe(strcmp(content_type, "text/css")) == 0) return true
        if(unsafe(strcmp(content_type, "text/csv")) == 0) return true
        if(unsafe(strcmp(content_type, "text/markdown")) == 0) return true
        if(unsafe(strcmp(content_type, "application/javascript")) == 0) return true
        if(unsafe(strcmp(content_type, "application/json")) == 0) return true
        if(unsafe(strcmp(content_type, "application/xml")) == 0) return true
        return false
    }

    // Check if a MIME type is an image type.
    public func is_image(content_type : *char) : bool {
        if(unsafe(strcmp(content_type, "image/png")) == 0) return true
        if(unsafe(strcmp(content_type, "image/jpeg")) == 0) return true
        if(unsafe(strcmp(content_type, "image/gif")) == 0) return true
        if(unsafe(strcmp(content_type, "image/webp")) == 0) return true
        if(unsafe(strcmp(content_type, "image/svg+xml")) == 0) return true
        if(unsafe(strcmp(content_type, "image/x-icon")) == 0) return true
        if(unsafe(strcmp(content_type, "image/bmp")) == 0) return true
        return false
    }

} // end namespace mime
