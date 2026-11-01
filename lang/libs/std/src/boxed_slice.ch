public namespace std {

// `Box<[T]>` is the owning counterpart of a borrowed `[T]`/`&[T]` view.
// It currently owns a contiguous `vector<T>`; the vector's drop glue destroys
// every element and frees the allocation exactly once.
@direct_init
public struct boxed_slice<T> {

    internal var _values : vector<T>

    @implicit
    @constructor
    func make(values : vector<T>) : boxed_slice<T> {
        return boxed_slice<T> { _values : values }
    }

    @constructor
    func make_empty() : boxed_slice<T> {
        return boxed_slice<T> { _values : vector<T>() }
    }

    func size(&self) : size_t {
        return _values.size()
    }

    func empty(&self) : bool {
        return _values.size() == 0
    }

    func data(&self) : *T {
        return _values.data()
    }

    func get_ref(&self, index : size_t) : &T {
        return _values.get_ref(index)
    }

    func get_mut(&mut self, index : size_t) : &mut T {
        return _values.get_mut(index)
    }

    func set(&mut self, index : size_t, value : T) {
        _values.set(index, value)
    }

    func as_span(&self) : span<T> {
        return unsafe(span<T>(_values.data() as *T, _values.size()))
    }

    func subview(&self, start : size_t, end : size_t) : span<T> {
        return as_span().subview(start, end)
    }

    func split_at(&self, at : size_t) : slice_pair<T> {
        return as_span().split_at(at)
    }

    func subview_mut(&mut self, start : size_t, end : size_t) : mut_span<T> {
        if(start > end || end > _values.size()) {
            panic("boxed_slice subview out of bounds")
        }
        var ptr : *mut T = null
        if(start < _values.size()) {
            ptr = unsafe(_values.mutable_data() + start)
        }
        return unsafe(mut_span<T>(ptr, end - start))
    }

}



impl<T> core::ops::Index<size_t, T> for boxed_slice<T> {
    func index(&self, index : size_t) : &T {
        return self.get_ref(index)
    }
}

impl<T> core::ops::IndexMut<size_t, T> for boxed_slice<T> {
    func index_mut(&mut self, index : size_t) : &mut T {
        return self.get_mut(index)
    }
}

}
