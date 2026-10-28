public namespace std {

public struct span<T> {

    var _data : *T
    var _size : size_t

    @implicit
    @constructor
    @expand
    comptime func make2(vec : %maybe_runtime<&vector<T>>) {
        return constructor<T>(vec.data(), vec.size())
    }

    @implicit
    @constructor
    @expand
    comptime func make2(array : []%maybe_runtime<T>) {
        return constructor<T>(array, intrinsics::size(array))
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
        if(loc >= _size) {
            panic("span index out of bounds")
        }
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


impl<T> Copy for span<T> {}
}