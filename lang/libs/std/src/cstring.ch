public namespace std {

public variant NulError {
    Found(position : size_t)
}

// Owned NUL-terminated byte string for C interop. Construction rejects interior
// NUL bytes; the trailing terminator is part of the storage but excluded from
// `as_bytes`.
public func cstring_make(value : &str) : std::Result<CString, NulError> {
    var out = std::vector<u8>()
    var i : size_t = 0
    while(i < value.size()) {
        var c = value.get(i) as u8
        if(c == 0) {
            return std::Result.Err<CString, NulError>(NulError.Found(i))
        }
        if(!out.try_push(c)) {
            return std::Result.Err<CString, NulError>(NulError.Found(i))
        }
        i += 1
    }
    if(!out.try_push(0)) {
        return std::Result.Err<CString, NulError>(NulError.Found(i))
    }
    return std::Result.Ok<CString, NulError>(CString { bytes : out })
}

public func <'a> cstr_from_bytes(bytes : &'a [u8]) : std::Result<CStr<'a>, NulError> {
    var i : size_t = 0
    while(i < bytes.size()) {
        if(unsafe(*bytes.get(i)) == 0) {
            return std::Result.Err<CStr<'a>, NulError>(NulError.Found(i))
        }
        i += 1
    }
    return std::Result.Ok<CStr<'a>, NulError>(CStr<'a> {
        view : unsafe(std::string_view<'a>(bytes.data() as *char, bytes.size()))
    })
}

@direct_init
public struct CString {
    internal var bytes : std::vector<u8>

    func size(&self) : size_t {
        return self.bytes.size() - 1
    }

    func empty(&self) : bool {
        return self.size() == 0
    }

    // Raw pointer access for audited C interop only.
    unsafe func as_ptr(&self) : *char {
        return self.bytes.data() as *char
    }
}

// Borrowed NUL-free C-string view. The trailing terminator is not part of the
// view; construction rejects interior NUL so the pointer form is unambiguous.
@direct_init
public struct CStr<'a> {
    internal var view : std::string_view<'a>

    func to_bytes(&self) : [u8] {
        return unsafe(std::span<u8>(self.view.data() as *u8, self.view.size()))
    }

    func to_str(&self) : std::Result<std::str, std::string_view> {
        return std::try_from_utf8(self.view)
    }

    func size(&self) : size_t {
        return self.view.size()
    }

    func empty(&self) : bool {
        return self.view.empty()
    }
}


} // namespace std
