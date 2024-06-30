import "./std.ch"

struct string {

    union {
        struct {
            var data : char*;
            var length : size_t
        } constant;
        struct {
            var data : char*;
            var length : size_t;
            var capacity : size_t;
        } heap;
        struct {
            var buffer : char[16];
            var length : uchar;
        } sso;
    } storage;
    var state : char

    @comptime
    @constructor
    func make(value : literal::string) {
        return compiler::wrap(constructor(value, compiler::strlen(value)))
    }

    @constructor
    func constructor(value : char*, length : size_t) {
        var s : string
        s.storage.constant.data = value;
        s.storage.constant.length = length;
        s.state = '0'
        return s;
    }

    func size(&self) : size_t {
        return storage.constant.length;
    }

    func equals(&self, other : string*) : bool {
        return size() == other.size() && strcmp(self, other) == 0;
    }

    func data(&self) : char* {
        return storage.constant.data
    }

}