public namespace std {

    // TODO: remove retained, once we have runtime magic value support
    @retained
    public struct string_view {

        var _data : *char
        var _size : size_t

        @implicit
        @constructor
        comptime func make(value : %literal_string) {
            return intrinsics::wrap(constructor(value, intrinsics::size(value)))
        }

        @constructor
        func empty_make() {
            return string_view {
                _data : "",
                _size : 0
            }
        }

        @constructor
        func constructor(value : *char, length : size_t) {
            return string_view {
                _data : value,
                _size : length
            }
        }

        @constructor
        func make_view(str : &std::string) {
            return string_view {
                _data : str.data(),
                _size : str.size()
            }
        }

        @constructor
        func make_no_len(value : *char) {
            return string_view {
                _data : value,
                _size : strlen(value)
            }
        }

        func data(&self) : *char {
            return _data;
        }

        func size(&self) : size_t {
            return _size;
        }

        func empty(&self) : bool {
            return _size == 0;
        }

        func get(&self, index : size_t) : char {
            return *(_data + index)
        }

        func subview(&self, start : size_t, end : size_t) : std::string_view {
            return std::string_view(_data + start, end - start)
        }

        func skip(&self, count : size_t) : std::string_view {
            return std::string_view(_data + count, _size - count)
        }

        func find(&self, needle : &std::string_view) : size_t {
            return internal_view_find(self, needle);
        }

        func find_last(&self, needle : &std::string_view) : size_t {
            return internal_view_find_last(self, needle);
        }

        func contains(&self, needle : &std::string_view) : bool {
            return find(needle) != NPOS
        }

        func ends_with(&self, other : &std::string_view) : bool {
            // If other_data is longer than data, data cannot end with other_data.
            if (other.size() > size()) return false;
            return memcmp(data() + size() - other.size(), other.data(), other.size()) == 0;
        }

        func starts_with(&self, other : &std::string_view) : bool {
            if (other.size() > size()) return false;
            return memcmp(data(), other.data(), other.size()) == 0;
        }

        func trim(&self) : std::string_view {
            var s = 0u;
            while (s < _size && (get(s) == ' ' || get(s) == '\t' || get(s) == '\n' || get(s) == '\r')) s++;
            if (s == _size) return std::string_view("", 0);
            var e = _size - 1;
            while (e > s && (get(e) == ' ' || get(e) == '\t' || get(e) == '\n' || get(e) == '\r')) e--;
            return subview(s, e + 1);
        }

        func to_i8(&self) : std::Result<i8, std::string_view> {
            switch(to_i64()) {
                Ok(v) => return std::Result.Ok(v as i8)
                Err(e) => return std::Result.Err(e)
            }
        }
        func to_i16(&self) : std::Result<i16, std::string_view> {
            switch(to_i64()) {
                Ok(v) => return std::Result.Ok(v as i16)
                Err(e) => return std::Result.Err(e)
            }
        }
        func to_i32(&self) : std::Result<i32, std::string_view> {
            switch(to_i64()) {
                Ok(v) => return std::Result.Ok(v as i32)
                Err(e) => return std::Result.Err(e)
            }
        }
        func to_i64(&self) : std::Result<i64, std::string_view> {
            var res : i64 = 0
            var sign : i64 = 1
            var i : size_t = 0
            while (i < _size && isspace(_data[i])) i++
            if (i == _size) return std::Result.Err(std::string_view("empty string"))
            if (_data[i] == '-') {
                sign = -1
                i++
            } else if (_data[i] == '+') {
                i++
            }
            if (i == _size || !isdigit(_data[i])) return std::Result.Err(std::string_view("invalid format"))
            while (i < _size && isdigit(_data[i])) {
                res = res * 10 + (_data[i] - '0') as i64
                i++
            }
            while (i < _size && isspace(_data[i])) i++
            if (i < _size) return std::Result.Err(std::string_view("trailing characters"))
            return std::Result.Ok(res * sign)
        }

        func to_u8(&self) : std::Result<u8, std::string_view> {
            switch(to_u64()) {
                Ok(v) => return std::Result.Ok(v as u8)
                Err(e) => return std::Result.Err(e)
            }
        }
        func to_u16(&self) : std::Result<u16, std::string_view> {
            switch(to_u64()) {
                Ok(v) => return std::Result.Ok(v as u16)
                Err(e) => return std::Result.Err(e)
            }
        }
        func to_u32(&self) : std::Result<u32, std::string_view> {
            switch(to_u64()) {
                Ok(v) => return std::Result.Ok(v as u32)
                Err(e) => return std::Result.Err(e)
            }
        }
        func to_u64(&self) : std::Result<u64, std::string_view> {
            var res : u64 = 0
            var i : size_t = 0
            while (i < _size && isspace(_data[i])) i++
            if (i == _size) return std::Result.Err(std::string_view("empty string"))
            if (_data[i] == '+') i++
            if (i == _size || !isdigit(_data[i])) return std::Result.Err(std::string_view("invalid format"))
            while (i < _size && isdigit(_data[i])) {
                res = res * 10 + (_data[i] - '0') as u64
                i++
            }
            while (i < _size && isspace(_data[i])) i++
            if (i < _size) return std::Result.Err(std::string_view("trailing characters"))
            return std::Result.Ok(res)
        }

        func to_int(&self) : std::Result<int, std::string_view> {
            switch(to_i32()) {
                Ok(v) => return std::Result.Ok(v as int)
                Err(e) => return std::Result.Err(e)
            }
        }
        func to_uint(&self) : std::Result<uint, std::string_view> {
            switch(to_u32()) {
                Ok(v) => return std::Result.Ok(v as uint)
                Err(e) => return std::Result.Err(e)
            }
        }

        func to_float(&self) : std::Result<float, std::string_view> {
            if (_size == 0) return std::Result.Err(std::string_view("empty string"))
            if (_size < 128) {
                var buf : [128]char
                unsafe {
                    memcpy(&mut buf[0], _data, _size)
                    buf[_size] = '\0'
                    var end : *mut char = null
                    var res = strtod(&mut buf[0] as *mut char, &mut end) as float
                    if (end == &buf[0] as *mut char) return std::Result.Err(std::string_view("invalid format"))
                    while (end != null && *end != '\0' && isspace(*end as int)) end++
                    if (end != null && *end != '\0') return std::Result.Err(std::string_view("trailing characters"))
                    return std::Result.Ok(res)
                }
            }
            var s = std::string(self)
            return s.to_float()
        }

        func to_double(&self) : std::Result<double, std::string_view> {
            if (_size == 0) return std::Result.Err(std::string_view("empty string"))
            if (_size < 128) {
                var buf : [128]char
                unsafe {
                    memcpy(&mut buf[0], _data, _size)
                    buf[_size] = '\0'
                    var end : *mut char = null
                    var res = strtod(&mut buf[0] as *mut char, &mut end)
                    if (end == &buf[0] as *mut char) return std::Result.Err(std::string_view("invalid format"))
                    while (end != null && *end != '\0' && isspace(*end as int)) end++
                    if (end != null && *end != '\0') return std::Result.Err(std::string_view("trailing characters"))
                    return std::Result.Ok(res)
                }
            }
            var s = std::string(self)
            return s.to_double()
        }

        func split(&self, delim : char) : std::vector<std::string_view> {
            var res = std::vector<std::string_view>();
            var start = 0u;
            for (var i = 0u; i < _size; i++) {
                if (get(i) == delim) {
                    res.push(subview(start, i));
                    start = i + 1;
                }
            }
            if (start <= _size) {
                res.push(subview(start, _size));
            }
            return res;
        }

        func to_string(&self) : std::string {
            return std::string::view_make(self);
        }

        // TODO: unstable, no interface involved, signature not stable
        // TODO: no verification of signature
        // TODO: hardcoded type StringStream, generic function support required
        func stream(&self, s : &mut StringStream) {
            s.writeStr(_data, _size)
        }

    }

    impl Hashable for string_view {
        func hash(&self) : uint {
            return fnv1_hash_view(self) as uint
        }
    }

    impl Eq for string_view {
        func equals(&self, other : &std::string_view) : bool {
            const self_size = _size;
            return self_size == other.size() && strncmp(data(), other.data(), self_size) == 0;
        }
    }

}