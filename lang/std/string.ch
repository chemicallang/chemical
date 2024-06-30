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
        var len = compiler::strlen(value)
        return compiler::wrap(constructor(value, len))
    }

    @constructor
    func constructor(value : char*, length : size_t) {
        var s : string
        s.storage.constant.data = value;
        s.storage.constant.length = length;
        s.state = '0'
        return s;
    }

    func data(&self) : char* {
        return storage.constant.data
    }

}