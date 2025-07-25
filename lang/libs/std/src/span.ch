public namespace std {

public struct span<T> {

    var data : *T
    var _size : size_t

    @implicit
    @comptime
    @constructor
    func make2(array : T[]) {
        return intrinsics::wrap(constructor<T>(array, intrinsics::size(array)))
    }

    @implicit
    @comptime
    @constructor
    func make2(vec : &vector<T>) {
        return intrinsics::wrap(constructor<T>(vec.data(), vec.size()))
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

    func empty(&self) : bool {
        return _size == 0;
    }

}

}