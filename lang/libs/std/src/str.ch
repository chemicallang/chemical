public namespace std {

// Validate one UTF-8 byte sequence starting at `data[i]`. The checks reject
// overlong forms, surrogate code points, and values above U+10FFFF.
internal func str_utf8_valid(data : *char, len : size_t) : bool { unsafe {
    var i : size_t = 0
    while(i < len) {
        var c = data[i] as u8
        if(c <= 0x7Fu) {
            i += 1
            continue
        }
        var need : size_t = 0
        var min_second : u8 = 0x80u8
        var max_second : u8 = 0xBFu8
        if(c >= 0xC2u8 && c <= 0xDFu8) {
            need = 1
        } else if(c == 0xE0u8) {
            need = 2
            min_second = 0xA0u8
        } else if(c == 0xEDu8) {
            need = 2
            max_second = 0x9Fu8
        } else if(c >= 0xE1u8 && c <= 0xEEu8) {
            need = 2
        } else if(c == 0xEFu8) {
            need = 2
        } else if(c == 0xF0u8) {
            need = 3
            min_second = 0x90u8
        } else if(c >= 0xF1u8 && c <= 0xF3u8) {
            need = 3
        } else if(c == 0xF4u8) {
            need = 3
            max_second = 0x8Fu8
        } else {
            return false
        }
        if(i + need >= len) {
            return false
        }
        var first = data[i + 1] as u8
        if(first < min_second || first > max_second) {
            return false
        }
        var j : size_t = 2
        while(j <= need) {
            var b = data[i + j] as u8
            if(b < 0x80u8 || b > 0xBFu8) {
                return false
            }
            j += 1
        }
        i += need + 1
    }
    return true
}}

// Checked UTF-8 construction; returns the invalid view on failure.
public func try_from_utf8(view : std::string_view) : std::Result<std::str, std::string_view> {
    if(!str_utf8_valid(view.data(), view.size())) {
        return std::Result.Err(view)
    }
    return std::Result.Ok(std::str { _view : view })
}

// A UTF-8 string view. Constructing one validates its bytes; `str` is a
// non-owning view over caller storage. The method surface mirrors the useful
// subset of `string_view` without exposing unchecked constructors.
@direct_init
public struct str 'a {
    internal var _view : std::string_view<'a>

    @implicit
    @constructor
    func make(value : %literal_string) : str {
        var view = unsafe(std::string_view.make_no_len(value))
        if(!str_utf8_valid(view.data(), view.size())) {
            panic("invalid UTF-8 in string literal")
        }
        return str { _view : view }
    }

    @implicit
    @constructor
    func make_view(view : std::string_view) : str {
        if(!str_utf8_valid(view.data(), view.size())) {
            panic("invalid UTF-8 in string view")
        }
        return str { _view : view }
    }

    @implicit
    @constructor
    func make_string(value : &std::string) : str {
        return make_view(std::string_view.make_view(value))
    }

    func data(&self) : *char {
        return _view.data()
    }

    func size(&self) : size_t {
        return _view.size()
    }

    func empty(&self) : bool {
        return _view.empty()
    }

    func get(&self, index : size_t) : std::Option<char> {
        return _view.get(index)
    }

    func at(&self, index : size_t) : char {
        return _view.at(index)
    }

    func to_view(&self) : std::string_view {
        return _view
    }

    // Expressive-string hook: `println(`${name}`)` renders a `str` without
    // re-validating its bytes (a `str` is already known to be UTF-8).
    func stream(&self, s : &mut StringStream) {
        unsafe { s.writeStr(_view.data(), _view.size()) }
    }

    func subview(&self, start : size_t, end : size_t) : str {
        return make_view(_view.subview(start, end))
    }

    func starts_with(&self, other : &std::str) : bool {
        return _view.starts_with(other.to_view())
    }

    func ends_with(&self, other : &std::str) : bool {
        return _view.ends_with(other.to_view())
    }
}

} // namespace std
