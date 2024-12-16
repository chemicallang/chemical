import "./std.ch"

public namespace std {

    struct string_view {

        var _data : *char
        var _size : size_t

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

    }

}