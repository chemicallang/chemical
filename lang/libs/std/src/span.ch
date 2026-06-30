public namespace std {

public struct span<T> {

    var _data : *T
    var _size : size_t

    @implicit
    @constructor
    comptime func make2(array : []%maybe_runtime<T>) {
        return %runtime_value(constructor<T>(array, intrinsics::size(array)))
    }

    @implicit
    @constructor
    comptime func make2(vec : %maybe_runtime<&vector<T>>) {
        return %runtime_value(constructor<T>(vec.data(), vec.size()))
    }

    @constructor
    func constructor(array_ptr : *T, array_size : size_t) {
        return span<T> {
            _data : array_ptr,
            _size : array_size
        }
    }

    @constructor
    func empty_make() {
        return span<T> {
            _data : null,
            _size : 0
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

    impl core::iterable::Linear<T> for span<T> {
        func data(&self) : *T {
            return _data;
        }
        func size(&self) : size_t {
            return _size
        }
    }

}

}