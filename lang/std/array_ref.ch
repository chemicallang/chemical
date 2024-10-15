import "./vector.ch"
import "./std.ch"

public struct ArrayRef<T> {

    var data : *T
    var _size : size_t

    @implicit
    @comptime
    @constructor
    func make(array : T[]) {
        return compiler::wrap(constructor(array, compiler::size(array)))
    }

    @implicit
    @comptime
    @constructor
    func make2(vec : &vector<T>) {
        return compiler::wrap(constructor(vec.data(), vec.size()))
    }

    @constructor
    func constructor(array_ptr : *T, array_size : size_t) {
        init {
            data(array_ptr)
            _size(array_size)
        }
    }

    func get(&self, loc : size_t) : *T {
        return data + loc;
    }

    func size(&self) : size_t {
        return _size;
    }

}