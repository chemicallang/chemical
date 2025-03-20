import "./std.ch"
import "./hashing/fnv1.ch"
import "./hashing/hash.ch"
import "./string.ch"

public namespace std {

    struct string_view : Hashable, Eq {

        var _data : *char
        var _size : size_t

        @implicit
        @comptime
        @constructor
        func make(value : literal<string>) {
            return compiler::wrap(constructor(value, compiler::size(value)))
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

        @override
        func equals(&self, other : &std::string_view) : bool {
            const self_size = _size;
            return self_size == other.size() && strncmp(data(), other.data(), self_size) == 0;
        }

        @override
        func hash(&self) : uint {
            return fnv1a_hash_32(_data);
        }

    }

}