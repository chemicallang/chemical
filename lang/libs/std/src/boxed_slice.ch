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
