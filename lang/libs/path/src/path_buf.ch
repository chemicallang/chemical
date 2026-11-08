// Owning path value plus the borrowed `Path` view.
//
// `Path` is a cheap (pointer, length) view over UTF-8 path bytes. `PathBuf`
// owns the text and caches the OS-native NUL-terminated form, so filesystem
// calls do not have to allocate a C string per operation.
public namespace path {

using std::Option;
using std::Result;
using std::string;
using std::string_view;

internal func path_has_nul(view : string_view) : bool {
    unsafe {
        var i : size_t = 0
        var p = view.data()
        while(i < view.size()) {
            if(p[i] == '\0') { return true }
            i += 1
        }
        return false
    }
}

/// Borrowed, UTF-8 path view.
@direct_init
public struct Path {
    internal var _view : string_view
    internal var _native : *std::native_char_t
    internal var _text : std::str

    @make
    func from_view(view : string_view, native : *std::native_char_t) : Path {
        var made = std::try_from_utf8(view)
        var Ok(text) = made else unsafe { unreachable }
        return Path {
            _view : view,
            _native : native,
            _text : text
        }
    }

    /// Create a borrowed path view over UTF-8 text.
    @make
    func new(value : &str) : Path {
        var view = value.to_view()
        var native : *std::native_char_t = null
        comptime if(!def.windows) {
            native = value.data() as *std::native_char_t
        }
        return Path.from_view(view, native)
    }

    func as_view(&self) : string_view {
        return self._view
    }

    func as_string(&self) : string {
        return string(self._view)
    }

    func size(&self) : size_t {
        return self._view.size()
    }

    func empty(&self) : bool {
        return self._view.empty()
    }

    func has_nul(&self) : bool {
        return path_has_nul(self._view)
    }

    /// UTF-8 text. A `Path` is always constructed from UTF-8, so this is
    /// `Some` for every path produced by this module; the `Option` keeps the
    /// signature lossless for future OS-string paths.
    func as_str(&self) : Option<&std::str> {
        return Option.Some<&std::str>(&self._text)
    }

    func to_path_buf(&self) : PathBuf {
        return PathBuf.from_view(self._view)
    }

    /// Native cached pointer. Returns null when the view has no NUL-terminated
    /// native form (for example a Windows `Path::new` before caching).
    func native_ptr(&self) : *std::native_char_t {
        unsafe {
        if(self._native == null) { return null }
        comptime if(def.windows) {
            if(self._native[self._view.size()] != 0) { return null }
        } else {
            var bytes = self._native as *char
            if(bytes[self._view.size()] != 0) { return null }
        }
        return self._native

        }}

    func make_slice(&self, start : size_t, len : size_t) : Path {
        var view = unsafe(string_view(self._view.data() + start, len))
        var native : *std::native_char_t = null
        if(self._native != null) {
            comptime if(def.windows) {
                native = unsafe(self._native + start)
            } else {
                native = unsafe((self._native as *char + start) as *std::native_char_t)
            }
        }
        return Path.from_view(view, native)
    }

    func is_absolute(&self) : bool {
        unsafe {
        if(self._view.size() == 0) { return false }
        var p = self._view.data()
        if(p[0] == '/') { return true }
        comptime if(def.windows) {
            var c = p[0] as u8
            if(((c >= 'a' as u8 && c <= 'z' as u8) || (c >= 'A' as u8 && c <= 'Z' as u8)) && p[1] == ':') {
                return true
            }
        }
        return false

        }}

    func has_root(&self) : bool {
        unsafe {
        if(self._view.size() == 0) { return false }
        var p = self._view.data()
        return p[0] == '/' || p[0] == '\\'

        }}

    /// Final component, ignoring trailing separators. Root and empty paths
    /// have no file name.
    func file_name(&self) : Option<Path> {
        unsafe {
        var len = self._view.size()
        if(len == 0) { return Option.None<Path>() }
        var p = self._view.data()
        var end = len
        while(end > 0 && (p[end - 1] == '/' || p[end - 1] == '\\')) { end -= 1 }
        if(end == 0) { return Option.None<Path>() }
        var start = end
        while(start > 0 && p[start - 1] != '/' && p[start - 1] != '\\') { start -= 1 }
        return Option.Some<Path>(self.make_slice(start, end - start))

        }}

    /// Parent directory. The root has no parent; relative single-component
    /// paths have the empty path as their parent.
    func parent(&self) : Option<Path> {
        unsafe {
        var len = self._view.size()
        if(len == 0) { return Option.None<Path>() }
        var p = self._view.data()
        var end = len
        while(end > 0 && (p[end - 1] == '/' || p[end - 1] == '\\')) { end -= 1 }
        if(end == 0) { return Option.None<Path>() }
        var start = end
        while(start > 0 && p[start - 1] != '/' && p[start - 1] != '\\') { start -= 1 }
        if(start == 0) {
            return Option.Some<Path>(self.make_slice(0, 0))
        }
        var parent_end = start - 1
        while(parent_end > 0 && (p[parent_end - 1] == '/' || p[parent_end - 1] == '\\')) {
            parent_end -= 1
        }
        if(parent_end == 0) {
            return Option.Some<Path>(self.make_slice(0, 1))
        }
        return Option.Some<Path>(self.make_slice(0, parent_end))

        }}

    /// Extension without the leading dot. `None` when there is no extension.
    func extension(&self) : Option<Path> {
        unsafe {
        var name = self.file_name()
        if(name is Option.None) { return Option.None<Path>() }
        var Some(n) = name else unsafe { unreachable }
        var len = n.size()
        if(len == 0) { return Option.None<Path>() }
        var p = n.as_view().data()
        var dot = len
        while(dot > 0 && p[dot - 1] != '.') { dot -= 1 }
        if(dot == 0) { return Option.None<Path>() }
        // A leading dot is a hidden file, not an extension.
        if(dot == 1) { return Option.None<Path>() }
        return Option.Some<Path>(n.make_slice(dot, len - dot))

        }}

    /// File name without its final extension.
    func stem(&self) : Option<Path> {
        unsafe {
        var name = self.file_name()
        if(name is Option.None) { return Option.None<Path>() }
        var Some(n) = name else unsafe { unreachable }
        var len = n.size()
        if(len == 0) { return Option.None<Path>() }
        var p = n.as_view().data()
        var dot = len
        while(dot > 0 && p[dot - 1] != '.') { dot -= 1 }
        if(dot <= 1) { return Option.Some<Path>(n) }
        return Option.Some<Path>(n.make_slice(0, dot - 1))

        }}

    func join(&self, other : &Path) : PathBuf {
        unsafe {
        if(other.is_absolute()) {
            return other.to_path_buf()
        }
        if(self.empty()) {
            return other.to_path_buf()
        }
        var out = self.as_string()
        var p = out.data()
        if(p[out.size() - 1] != '/' && p[out.size() - 1] != '\\') {
            out.append('/')
        }
        out.append_view(other.as_view())
        return PathBuf.from_view(out.to_view())

        }}

    func normalize(&self) : Result<PathBuf, PathError> {
        var source = self.as_string()
        var buf : [MAX_PATH]char
        var r = unsafe(raw::normalize(source.c_str(), &raw mut buf[0], MAX_PATH as size_t))
        if(r is Result.Err) {
            var Err(e) = r else unsafe { unreachable }
            return Result.Err(e)
        }
        return Result.Ok(PathBuf.from_view(unsafe(string_view.make_no_len(&raw mut buf[0]))))
    }
}

/// Owning path value. Owns both the UTF-8 text and a native NUL-terminated
/// form used by syscalls.
@direct_init
public struct PathBuf {
    internal var value : string
    internal var native : std::OsString

    @make
    func from(value : &str) {
        var text = unsafe(string(value.data(), value.size()))
        var os = std::OsString(&text)
        return PathBuf {
            value : text,
            native : os
        }
    }

    func from_view(view : string_view) : PathBuf {
        var text = unsafe(string(view.data(), view.size()))
        var os = std::OsString(&text)
        return PathBuf {
            value : text,
            native : os
        }
    }

    func as_path(&self) : Path {
        return Path.from_view(self.value.to_view(), self.native.c_str_native())
    }

    func as_view(&self) : string_view {
        return self.value.to_view()
    }

    func as_string(&self) : string {
        return self.value.copy()
    }

    func as_str(&self) : Option<std::str> {
        var made = std::try_from_utf8(self.value.to_view())
        if(made is Result.Err) { return Option.None<std::str>() }
        var Ok(text) = made else unsafe { unreachable }
        return Option.Some<std::str>(text)
    }

    func size(&self) : size_t {
        return self.value.size()
    }

    func empty(&self) : bool {
        return self.value.size() == 0
    }

    func has_nul(&self) : bool {
        return path_has_nul(self.value.to_view())
    }

    func native_ptr(&self) : *std::native_char_t {
        return self.native.c_str_native()
    }

    func to_path_buf(&self) : PathBuf {
        return PathBuf.from_view(self.value.to_view())
    }

    func file_name(&self) : Option<Path> {
        return self.as_path().file_name()
    }

    func parent(&self) : Option<Path> {
        return self.as_path().parent()
    }

    func extension(&self) : Option<Path> {
        return self.as_path().extension()
    }

    func stem(&self) : Option<Path> {
        return self.as_path().stem()
    }

    func join(&self, other : &Path) : PathBuf {
        return self.as_path().join(other)
    }

    func normalize(&self) : Result<PathBuf, PathError> {
        return self.as_path().normalize()
    }

    func is_absolute(&self) : bool {
        return self.as_path().is_absolute()
    }

    func has_root(&self) : bool {
        return self.as_path().has_root()
    }

    // Compatibility aliases. These do not collide with the canonical names.
    func basename(&self) : PathBuf {
        var name = self.file_name()
        if(name is Option.None) { return PathBuf.from("") }
        var Some(n) = name else unsafe { unreachable }
        return n.to_path_buf()
    }

    func dirname(&self) : PathBuf {
        var dir = self.parent()
        if(dir is Option.None) { return PathBuf.from("") }
        var Some(d) = dir else unsafe { unreachable }
        return d.to_path_buf()
    }
}

// `Path` is a real borrowed view; this alias keeps old `Path` spelling valid.
public type PathRef = Path;

} // namespace path
