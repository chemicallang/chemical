public namespace std {

public struct span<T> {

    // The pointer/length pair is the span invariant; user code uses get/size.
    internal var _data : *T
    internal var _size : size_t

    @implicit
    @constructor
    @expand
    comptime func make2(vec : %maybe_runtime<&vector<T>>) {
        return unsafe(constructor<T>(vec.data(), vec.size()))
    }

    @implicit
    @constructor
    @expand
    comptime func make2(array : []%maybe_runtime<T>) {
        return unsafe(constructor<T>(array, intrinsics::size(array)))
    }

    @constructor
    unsafe func constructor(array_ptr : *T, array_size : size_t) {
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
        return self._data;
    }

    func get(&self, loc : size_t) : *T {
        if(loc >= _size) {
            panic("span index out of bounds")
        }
        return unsafe(_data + loc);
    }

    func size(&self) : size_t {
        return self._size;
    }

    func empty(&self) : bool {
        return _size == 0;
    }

    func subview(&self, start : size_t, end : size_t) : span<T> {
        if(start > end || end > _size) {
            panic("span subview out of bounds")
        }
        var ptr : *T = null
        if(start < _size) {
            ptr = unsafe(_data + start)
        }
        return unsafe(span<T>(ptr, end - start))
    }

    unsafe impl core::iterable::Linear<T> for span<T> {
        func data(&self) : *T {
            return _data;
        }
        func size(&self) : size_t {
            return _size
        }
    }

}


impl<T> Copy for span<T> {}

impl<T> core::ops::Index<size_t, T> for span<T> {
    func index(&self, loc : size_t) : &T {
        return unsafe(&*self.get(loc))
    }
}

@direct_init
public struct mut_span<T> {

    internal var _data : *mut T
    internal var _size : size_t

    @implicit
    @constructor
    @expand
    comptime func make2(vec : %maybe_runtime<&mut vector<T>>) {
        return unsafe(constructor<T>(vec.mutable_data(), vec.size()))
    }

    @implicit
    @constructor
    @expand
    comptime func make2(array : &mut []%maybe_runtime<T>) {
        return unsafe(constructor<T>(array as *mut T, intrinsics::size(array)))
    }


    @constructor
    unsafe func constructor(array_ptr : *mut T, array_size : size_t) {
        return mut_span<T> {
            _data : array_ptr,
            _size : array_size
        }
    }

    @constructor
    func empty_make() {
        return mut_span<T> {
            _data : null,
            _size : 0
        }
    }

    func data(&self) : *mut T {
        return self._data;
    }

    func get(&self, loc : size_t) : *mut T {
        if(loc >= _size) {
            panic("mut_span index out of bounds")
        }
        return unsafe(_data + loc);
    }

    func get_ref(&self, loc : size_t) : &T {
        return unsafe(&*self.get(loc))
    }

    func get_mut(&mut self, loc : size_t) : &mut T {
        return unsafe(&mut *self.get(loc))
    }

    func set(&mut self, loc : size_t, value : T) {
        if(loc >= _size) {
            panic("mut_span index out of bounds")
        }
        unsafe { _data[loc] = value; }
    }

    func size(&self) : size_t {
        return _size;
    }

    func empty(&self) : bool {
        return _size == 0;
    }

    func subview(&mut self, start : size_t, end : size_t) : mut_span<T> {
        if(start > end || end > _size) {
            panic("mut_span subview out of bounds")
        }
        var ptr : *mut T = null
        if(start < _size) {
            ptr = unsafe(_data + start)
        }
        return unsafe(mut_span<T>(ptr, end - start))
    }

    func as_span(&self) : span<T> {
        return unsafe(span<T>(_data as *T, _size))
    }

    unsafe impl core::iterable::Linear<T> for mut_span<T> {
        func data(&self) : *T {
            return _data;
        }
        func size(&self) : size_t {
            return _size
        }
    }

}

impl<T> core::ops::Index<size_t, T> for mut_span<T> {
    func index(&self, loc : size_t) : &T {
        return self.get_ref(loc)
    }
}

impl<T> core::ops::IndexMut<size_t, T> for mut_span<T> {
    func index_mut(&mut self, loc : size_t) : &mut T {
        return self.get_mut(loc)
    }
}

}