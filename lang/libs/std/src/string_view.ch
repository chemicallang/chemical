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
            _data = ""
            _size = 0
        }

        @constructor
        func constructor(value : *char, length : size_t) {
            _data = value;
            _size = length;
        }

        @constructor
        func make_view(str : &std::string) {
            _data = str.data()
            _size = str.size()
        }

        @constructor
        func make_no_len(value : *char) {
            _data = value;
            _size = strlen(value)
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

        func contains(&self, needle : &std::string_view) : bool {
            return find(needle) != NPOS
        }

        @override
        func equals(&self, other : &std::string_view) : bool {
            const self_size = _size;
            return self_size == other.size() && strncmp(data(), other.data(), self_size) == 0;
        }

        @override
        func hash(&self) : uint {
            return fnv1_hash_view(self) as uint
        }

    }

}