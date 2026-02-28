public namespace std {

    public struct string_view : Hashable, Eq {

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

        @override
        func equals(&self, other : &std::string_view) : bool {
            const self_size = _size;
            return self_size == other.size() && strncmp(data(), other.data(), self_size) == 0;
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

        @override
        func hash(&self) : uint {
            return fnv1_hash_view(self) as uint
        }

    }

}