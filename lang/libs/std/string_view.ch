import "./std.ch"

namespace std {

    struct string_view {

        var _data : *char
        var _size : size_t

        func data(&self) : *char {
            return _data;
        }

        func size(&self) : size_t {
            return _size;
        }

    }

}