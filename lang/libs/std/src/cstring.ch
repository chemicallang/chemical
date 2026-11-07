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

} // namespace std
