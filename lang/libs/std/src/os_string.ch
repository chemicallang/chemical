public namespace std {


public comptime const CP_UTF7 = 65000u       // UTF-7 translation
public comptime const CP_UTF8 = 65001u       // UTF-8 translation

if(def.windows) {
    public type native_char_t = u16;   // native unit for OsString on Windows (UTF-16)
    public type native_string_t = u16string;
} else {
    public type native_char_t = char;       // native unit for OsString on POSIX (bytes)
    public type native_string_t = string;
}

// OsString: stores an OS-native string. Convert explicitly to/from UTF-8.
struct OsString {

    var data_ : native_string_t;

    @make
    func make(str : native_string_t) {
        data_ = str
    }

    // Construct from UTF-8. On POSIX this copies the bytes; on Windows this converts to UTF-16.
    @make
    func make2(utf8 : &std::string) {
        if(def.windows) {
            data_ = utf8_to_u16(utf8);
        } else {
            data_ = utf8;
        }
    }

    // --- factory helpers ---
    func from_utf8(utf8 : &std::string) : OsString { return OsString(utf8); }

    // --- accessors ---
    // size measured in native code units (bytes on POSIX, char16_t units on Windows)
    func size(&self) : size_t { return data_.size(); }

    func empty(&self) : bool { return data_.empty(); }

    // Get access to the native underlying string/characters
    func native(&self) : &native_string_t { return data_; }

    // Get a pointer to native data (null-terminated if you ensure it)
    func c_str_native(&self) : *native_char_t {
        if(def.windows) {
            // u16string is not guaranteed to be null-terminated by c_str in old standards,
            // but in modern C++ it is. We return data() which is fine as long as caller
            // treats the length via size().
            if(data_.empty()) {
                return null;
            } else {
                return data_.data()
            }
        } else {
            return data_.c_str();
        }
    }

    // --- modify ---
    func clear(&self) { data_.clear(); }

    func reserve(&self, n : size_t) { data_.reserve(n); }

    // TODO: support shrink_to_fit
    // func shrink_to_fit(&self) { data_.shrink_to_fit(); }

    func append_utf8(&self, data : *char, len : size_t) {
        if(def.windows) {
            data_.append_utf8_view(data, len)
        } else {
            data_.append_with_len(data, len)
        }
    }

    // Append a UTF-8 string
    func append_utf8_str(&self, utf8 : &std::string) {
        append_utf8(utf8.data(), utf8.size())
    }

    func append_utf8_view(&self, utf8 : &std::string_view) {
        append_utf8(utf8.data(), utf8.size())
    }

    // Append native data directly
    func append_native(&self, native : &native_string_t) {
        data_.append_with_len(native.data(), native.size())
    }

    // Swap
    // TODO: support swap
    // func swap(&self, other : &OsString) { data_.swap(other.data_); }

    // Convert to UTF-8. On POSIX this returns the raw bytes, on Windows it converts.
    func to_utf8(&self) : std::string {
        if(def.windows) {
            return u16_to_utf8(data_);
        } else {
            return data_;
        }
    }

    if(def.windows) {

        // Helper: convert UTF-8 -> std::u16string using Win32 API
        func utf8_to_u16(utf8 : &std::string) : std::u16string {
            if (utf8.empty()) return std::u16string();

            // Get required length (in wchar_t units)
            var req = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size() as int, NULL, 0);
            if (req == 0) {
                // TODO: return a result
                // throw std::runtime_error("utf8_to_u16: MultiByteToWideChar failed")
                return std::u16string()
            };

            // Fill a temporary std::u16string (wchar_t is 16-bit on Windows)
            var tmp = std::u16string();
            tmp.reserve(req as uint)
            var res = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), utf8.size() as int, tmp.data() as LPWSTR, req);
            if (res == 0) {
                // TODO: return a result
                // throw std::runtime_error("utf8_to_u16: MultiByteToWideChar failed second pass");
                return std::u16string()
            }

            // Convert std::u16string -> std::u16string by narrowing/widening char type
            var out = std::u16string();
            out.append_with_len(tmp.data(), tmp.size())
            return out;
        }

        // Helper: convert std::u16string -> UTF-8 using Win32 API
        func u16_to_utf8(str : &std::u16string) : std::string {
            if (str.empty()) return std::string();

            // Copy into std::u16string (wchar_t 16-bit on Windows)
            var tmp = std::u16string();
            tmp.append_with_len(str.data(), str.size())

            // Get required length in bytes
            var req = WideCharToMultiByte(CP_UTF8, 0, tmp.data(), tmp.size() as int, NULL, 0, NULL, NULL);
            if (req == 0) {
                // TODO: return a result
                // throw std::runtime_error("u16_to_utf8: WideCharToMultiByte failed")
                return std::string()
            };

            var out = std::string();
            out.reserve(req as uint)
            var res = WideCharToMultiByte(CP_UTF8, 0, tmp.data(), tmp.size() as int, out.data() as LPSTR, req, NULL, NULL);
            if (res == 0) {
                // TODO: return a result
                // throw std::runtime_error("u16_to_utf8: WideCharToMultiByte failed second pass")
                return std::string()
            }

            return out;
        }

    }
};

}