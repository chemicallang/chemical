import "./vector.ch"
import "./std.ch"

public struct ArrayRef<T> {

    var data : T*
    var size : size_t

    @implicit
    @comptime
    @constructor
    func make(array : T[]) {
        return compiler::wrap(constructor(array, compiler::size(array)))
    }

    @implicit
    @comptime
    @constructor
    func make2(vec : vector<int>) {
        return compiler::wrap(constructor(vec.data(), vec.size()))
    }

    @constructor
    func constructor(array_ptr : T*, array_size : size_t) {
        data = array_ptr;
        size = array_size;
    }

}