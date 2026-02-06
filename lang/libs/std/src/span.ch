public namespace std {

public struct span<T> {

    var _data : *T
    var _size : size_t

    @implicit
    @constructor
    comptime func make2(array : []%maybe_runtime<T>) {
        return intrinsics::wrap(constructor<T>(array, intrinsics::size(array)))
    }

    @implicit
    @constructor
    comptime func make2(vec : %maybe_runtime<&vector<T>>) {
        return intrinsics::wrap(constructor<T>(vec.data(), vec.size()))
    }

    @constructor
    func constructor(array_ptr : *T, array_size : size_t) {
        return span<T> {
            _data : array_ptr,
            _size : array_size
        }
    }

    func data(&self) : *T {
        return _data;
    }

    func get(&self, loc : size_t) : *T {
        return _data + loc;
    }

    func size(&self) : size_t {
        return _size;
    }

    func empty(&self) : bool {
        return _size == 0;
    }

}

}