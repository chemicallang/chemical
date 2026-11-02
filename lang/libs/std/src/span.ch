public namespace std {

@direct_init
public struct slice_pair<T> {
    public var first : span<T>
    public var second : span<T>
}

@direct_init
public struct mut_slice_pair<T> {
    public var first : mut_span<T>
    public var second : mut_span<T>
}

@direct_init
public struct span_windows<T> {
    internal var _source : span<T>
    internal var _width : size_t

    func size(&self) : size_t {
        if(_width == 0 || _width > _source.size()) return 0
        return _source.size() - _width + 1
    }

    func empty(&self) : bool {
        return size() == 0
    }

    func get(&self, index : size_t) : span<T> {
        if(index >= size()) panic("span window index out of bounds")
        return _source.subview(index, index + _width)
    }
}

@direct_init
public struct span_chunks<T> {
    internal var _source : span<T>
    internal var _width : size_t

    func size(&self) : size_t {
        if(_width == 0 || _source.size() == 0) return 0
        return (_source.size() + _width - 1) / _width
    }

    func empty(&self) : bool {
        return size() == 0
    }

    func get(&self, index : size_t) : span<T> {
        if(index >= size()) panic("span chunk index out of bounds")
        var start = index * _width
        var end = start + _width
        if(end > _source.size()) end = _source.size()
        return _source.subview(start, end)
    }
}

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

    func split_at(&self, at : size_t) : slice_pair<T> {
        if(at > _size) {
            panic("span split_at out of bounds")
        }
        var left_ptr : *T = null
        if(at != 0) {
            left_ptr = _data
        }
        var right_ptr : *T = null
        if(at < _size) {
            right_ptr = unsafe(_data + at)
        }
        return slice_pair<T> {
            first : unsafe(span<T>(left_ptr, at)),
            second : unsafe(span<T>(right_ptr, _size - at))
        }
    }

    func windows(&self, width : size_t) : span_windows<T> {
        return span_windows<T> {
            _source : unsafe(span<T>(_data, _size)),
            _width : width
        }
    }

    func chunks(&self, width : size_t) : span_chunks<T> {
        if(width == 0) panic("span chunks width must be non-zero")
        return span_chunks<T> {
            _source : unsafe(span<T>(_data, _size)),
            _width : width
        }
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

    func split_at(&mut self, at : size_t) : mut_slice_pair<T> {
        if(at > _size) {
            panic("mut_span split_at out of bounds")
        }
        var left_ptr : *mut T = null
        if(at != 0) {
            left_ptr = _data
        }
        var right_ptr : *mut T = null
        if(at < _size) {
            right_ptr = unsafe(_data + at)
        }
        return mut_slice_pair<T> {
            first : unsafe(mut_span<T>(left_ptr, at)),
            second : unsafe(mut_span<T>(right_ptr, _size - at))
        }
    }

    func windows(&self, width : size_t) : span_windows<T> {
        return as_span().windows(width)
    }

    func chunks(&self, width : size_t) : span_chunks<T> {
        return as_span().chunks(width)
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