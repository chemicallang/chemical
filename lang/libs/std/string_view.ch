import "./std.ch"
import "./hashing/fnv1.ch"

public namespace std {

    struct string_view {

        var _data : *char
        var _size : size_t

        @implicit
        @comptime
        @constructor
        func make(value : literal<string>) {
            return compiler::wrap(constructor(value, compiler::size(value)))
        }

        @constructor
        func constructor(value : *char, length : size_t) {
            _data = value;
            _size = length;
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

        func unordered_map_compare(view1 : &string_view, view2 : &string_view) : bool {
            return strcmp(view1.data(), view2.data()) == 0;
        }

        func unordered_map_hash(view : &string_view) : uint {
            return fnv1a_hash_32(view.data());
        }

    }

}