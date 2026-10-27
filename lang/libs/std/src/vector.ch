public namespace std {

@retained
public func __vector_bounds_check(index : size_t, size : size_t) {
    if(index >= size) {
        panic("vector index out of bounds")
    }
}

@retained
public func __vector_nonempty_check(size : size_t) {
    if(size == 0) {
        panic("vector is empty")
    }
}

public struct vector<T> {

    var data_ptr : *mut T
    var data_size : size_t
    var data_cap : size_t

    @constructor
    func make() {
        return vector<T> {
            data_ptr : null,
            data_size : 0,
            data_cap : 0
        }
    }

    @constructor
    func make_with_capacity(init_cap : size_t) {
        const elem_size = sizeof(T)
        if(elem_size != 0 && init_cap > ((0 as size_t) - 1) / elem_size) {
            panic("vector capacity overflow")
        }
        var ptr = malloc(sizeof(T) * init_cap) as *mut T
        if(ptr == null && init_cap != 0) {
            panic("vector allocation failed")
        }
        return vector<T> {
            data_ptr : ptr,
            data_size : 0,
            data_cap : init_cap
        }
    }

    func resize(&mut self, new_size : size_t) {
        if(new_size == data_size) return;
        // shrinking
        if(new_size < data_size) {
            const to_destruct = data_size - new_size;
            const start = data_ptr + new_size;
            destruct[to_destruct] start;
            data_size = new_size;
            return;
        }
        // TODO: support construction in place, either T should be zeroable or T should be default constructable
        // reserving size for growth
        if (new_size > self.data_cap) {
            var new_cap = 2 as size_t;
            if(data_cap != 0) {
                new_cap = data_cap;
            }
            while (new_cap < new_size) {
                new_cap = new_cap * 2;
            }
            reserve(new_cap);
        }

        // TODO: using zeroed, the type may not be trivially copyable or zero initializable
        // constraints should be used to verify the type
        var i = data_size;
        while (i < new_size) {
            data_ptr[i] = zeroed<T>();
            i = i + 1;
        }

        data_size = new_size;
    }

    func reserve(&mut self, new_cap : size_t) {
        if(new_cap <= data_cap) return;
        const elem_size = sizeof(T)
        if(elem_size != 0) {
            const max_elems = ((0 as size_t) - 1) / elem_size
            if(new_cap > max_elems) {
                panic("vector capacity overflow")
            }
        }
        var new_data = realloc(data_ptr, (sizeof(T) * new_cap)) as *mut T;
        unsafe {
            if (new_data != null) {
                data_ptr = new_data;
                data_cap = new_cap;
            } else {
                panic("failed to resize vector");
            }
        }
    }

    func ensure_capacity_for_one_more(&mut self) {
        if (data_cap == 0) {
            reserve(2)
        } else {
            if(data_cap > ((0 as size_t) - 1) / 2) {
                panic("vector capacity overflow")
            }
            reserve(data_cap * 2)
        }
    }

    func push(&mut self, value : T) {
        const s = data_size;
        if (s >= data_cap) {
            ensure_capacity_for_one_more()
        }
        memcpy(&raw mut data_ptr[s], &raw value, sizeof(T))
        intrinsics::forget(value)
        data_size = s + 1
    }

    func push_back(&mut self, value : T) {
        push(value)
    }

    func get(&self, index : size_t) : T where T : Copy {
        __vector_bounds_check(index, data_size)
        return data_ptr[index];
    }

    func get_ptr(&self, index : size_t) : *mut T {
        __vector_bounds_check(index, data_size)
        return &raw mut data_ptr[index];
    }

    func get_ref(&self, index : size_t) : &T {
        __vector_bounds_check(index, data_size)
        return &data_ptr[index]
    }

    func get_mut(&mut self, index : size_t) : &mut T {
        __vector_bounds_check(index, data_size)
        return &mut data_ptr[index]
    }

    func set(&mut self, index : size_t, value : T) {
        __vector_bounds_check(index, data_size)
        data_ptr[index] = value;
    }

    func size(&self) : size_t {
        return data_size;
    }

    func capacity(&self) : size_t {
        return data_cap;
    }

    func data(&self) : *T {
        return data_ptr;
    }

    func last_ptr(&self) : *mut T {
        __vector_nonempty_check(data_size)
        return data_ptr + (data_size - 1);
    }

    func remove(&mut self, index : size_t) {
        __vector_bounds_check(index, data_size)
        const s = data_size;
        const last = s - 1;
        destruct get_ptr(index)
        if(index == last) {
            data_size = last;
        } else {
            memmove(&raw mut data_ptr[index], &raw data_ptr[index + 1], sizeof(T) * (last - index));
            data_size = last;
        }
    }

    func erase(&mut self, index : size_t) {
        remove(index);
    }

    func remove_last(&mut self) {
        __vector_nonempty_check(data_size)
        const s = data_size;
        const last = s - 1;
        const ptr = get_ptr(last);
        destruct ptr;
        data_size = last;
    }

    func pop_back(&mut self) {
        remove_last()
    }

    func take_last(&mut self) : T {
        __vector_nonempty_check(data_size)
        const last = data_size - 1
        const ptr = get_ptr(last)
        data_size = last;
        unsafe {
            return *ptr;
        }
    }

    func empty(&self) : bool {
        return data_size == 0;
    }

    func clear(&mut self) {
        destruct[data_size] data_ptr;
        data_size = 0;
    }

    // Overwrites the length without initializing or destroying elements.
    // Growing past the capacity exposes uninitialized memory to `get`, and
    // shrinking skips the destructors of the dropped tail, so the caller must
    // uphold the vector's invariant.
    unsafe func set_len(&mut self, new_size : size_t) {
        data_size = new_size
    }

    @delete
    func delete(&mut self) {
        if(data_ptr != null) {
            destruct[data_size] data_ptr;
            dealloc data_ptr;
        }
    }

    impl core::iterable::Linear<T> for vector<T> {
        func data(&self) : *T {
            return data_ptr;
        }
        func size(&self) : size_t {
            return data_size
        }
    }

}

func <T : core::ops::Clone> __vector_clone_value(value : *T) : T {
    var elem = zeroed:unsafe<T>()
    unsafe {
        memcpy(&raw mut elem, value as *void, sizeof(T))
    }
    var out = elem.clone()
    intrinsics::forget(elem)
    return out
}

impl<T : core::ops::Clone> core::ops::Clone for vector<T> {
    public func clone(&self) : vector<T> {
        var out = vector<T>()
        out.reserve(self.data_size)
        for(var i : size_t = 0; i < self.data_size; i++) {
            var ptr = &raw self.data_ptr[i] as *T
            out.push(__vector_clone_value(ptr))
        }
        return out
    }
}
}