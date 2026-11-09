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

    // Internal storage invariant: only the vector implementation (same module)
    // may read or write these fields. Safe user code goes through get/push/etc.
    internal var data_ptr : *mut T
    internal var data_size : size_t
    internal var data_cap : size_t

    @constructor
    func make() {
        var ptr : *mut T = null
        if(sizeof(T) == 0) {
            // Zero-sized elements still need a non-null, aligned sentinel so
            // pointer arithmetic and the empty-element destructor are valid.
            ptr = unsafe(malloc(1)) as *mut T
            if(ptr == null) {
                panic("vector allocation failed")
            }
        }
        return vector<T> {
            data_ptr : ptr,
            data_size : 0,
            data_cap : 0
        }
    }

    public func with_capacity(init_cap : size_t) : vector<T> {
        return vector<T>(init_cap)
    }

    public func try_with_capacity(init_cap : size_t) : std::Result<vector<T>, std::alloc::AllocError> {
        var out = vector<T>()
        if(!out.try_reserve(init_cap)) {
            return std::Result.Err<vector<T>, std::alloc::AllocError>(std::alloc::AllocError.OutOfMemory())
        }
        return std::Result.Ok<vector<T>, std::alloc::AllocError>(out)
    }

    @constructor
    func make_with_capacity(init_cap : size_t) {
        const elem_size = sizeof(T)
        if(elem_size == 0) {
            var zptr = unsafe(malloc(1)) as *mut T
            if(zptr == null) {
                panic("vector allocation failed")
            }
            return vector<T> {
                data_ptr : zptr,
                data_size : 0,
                data_cap : init_cap
            }
        }
        if(init_cap > ((0 as size_t) - 1) / elem_size) {
            panic("vector capacity overflow")
        }
        var ptr = unsafe(malloc(sizeof(T) * init_cap)) as *mut T
        if(ptr == null && init_cap != 0) {
            panic("vector allocation failed")
        }
        return vector<T> {
            data_ptr : ptr,
            data_size : 0,
            data_cap : init_cap
        }
    }

    func resize(&mut self, new_size : size_t) where T: Zeroable {
        if(new_size == data_size) return;
        // shrinking
        if(new_size < data_size) {
            const to_destruct = data_size - new_size;
            const start = unsafe(data_ptr + new_size);
            unsafe { destruct[to_destruct] start; }
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
            const max_cap = ((0 as size_t) - 1);
            while (new_cap < new_size) {
                if(new_cap > max_cap / 2) {
                    panic("vector capacity overflow")
                }
                new_cap = new_cap * 2;
            }
            reserve(new_cap);
        }

        // A zero-sized vector has no bytes to initialize; growing only updates
        // the logical length.
        if(sizeof(T) == 0) {
            data_size = new_size;
            return;
        }

        // TODO: using zeroed, the type may not be trivially copyable or zero initializable
        // constraints should be used to verify the type
        var i = data_size;
        while (i < new_size) {
            unsafe { data_ptr[i] = zeroed<T>(); }
            i = i + 1;
        }

        data_size = new_size;
    }

    func try_reserve(&mut self, new_cap : size_t) : bool {
        if(new_cap <= data_cap) return true;
        const elem_size = sizeof(T)
        if(elem_size == 0) {
            if(data_ptr == null) {
                data_ptr = unsafe(malloc(1)) as *mut T
                if(data_ptr == null) {
                    return false
                }
            }
            data_cap = new_cap
            return true
        }
        const max_elems = ((0 as size_t) - 1) / elem_size
        if(new_cap > max_elems) {
            return false
        }
        var new_data = unsafe(realloc(data_ptr, (sizeof(T) * new_cap))) as *mut T;
        if(new_data == null) {
            return false
        }
        data_ptr = new_data;
        data_cap = new_cap;
        return true
    }

    func reserve(&mut self, new_cap : size_t) {
        if(!try_reserve(new_cap)) {
            panic("failed to resize vector")
        }
    }

    func try_ensure_capacity_for_one_more(&mut self) : bool {
        if (data_cap == 0) {
            return try_reserve(2)
        }
        if(data_cap > ((0 as size_t) - 1) / 2) {
            return false
        }
        return try_reserve(data_cap * 2)
    }

    func ensure_capacity_for_one_more(&mut self) {
        if(!try_ensure_capacity_for_one_more()) {
            panic("failed to resize vector")
        }
    }

    func try_push(&mut self, value : T) : bool {
        const s = data_size;
        if (s >= data_cap) {
            if(!try_ensure_capacity_for_one_more()) {
                return false
            }
        }
        unsafe { memcpy(&raw mut data_ptr[s], &raw value, sizeof(T)) }
        intrinsics::forget(value)
        data_size = s + 1
        return true
    }

    func push(&mut self, value : T) {
        if(!try_push(value)) {
            panic("failed to resize vector")
        }
    }

    func push_back(&mut self, value : T) {
        push(value)
    }

    func at(&self, index : size_t) : T where T : Copy {
        __vector_bounds_check(index, data_size)
        return unsafe(data_ptr[index]);
    }

    func get_copy(&self, index : size_t) : Option<T> where T : Copy {
        if(index >= data_size) { return Option.None<T>() }
        return Option.Some<T>(unsafe(data_ptr[index]))
    }

    func get_ptr(&self, index : size_t) : *mut T {
        __vector_bounds_check(index, data_size)
        return unsafe(&raw mut data_ptr[index]);
    }

    func get(&self, index : size_t) : Option<&T> {
        if(index >= data_size) { return Option.None<&T>() }
        return Option.Some<&T>(unsafe(&data_ptr[index]))
    }

    func get_mut(&mut self, index : size_t) : Option<&mut T> {
        if(index >= data_size) { return Option.None<&mut T>() }
        return Option.Some<&mut T>(unsafe(&mut data_ptr[index]))
    }

    func try_set(&mut self, index : size_t, value : T) : bool {
        if(index >= data_size) { return false }
        set(index, value)
        return true
    }

    func pop(&mut self) : Option<T> {
        if(data_size == 0) { return Option.None<T>() }
        var out : T = unsafe(zeroed:unsafe<T>())
        unsafe {
            var idx = data_size - 1
            memcpy(&raw mut out, &raw mut data_ptr[idx], sizeof(T))
            data_size = idx
        }
        return Option.Some<T>(out)
    }

    func pop_unchecked(&mut self) : T {
        __vector_nonempty_check(data_size)
        var out : T = unsafe(zeroed:unsafe<T>())
        unsafe {
            var idx = data_size - 1
            memcpy(&raw mut out, &raw mut data_ptr[idx], sizeof(T))
            data_size = idx
        }
        return out
    }

    func at_ref(&self, index : size_t) : &T {
        __vector_bounds_check(index, data_size)
        return unsafe(&data_ptr[index])
    }

    func at_mut(&mut self, index : size_t) : &mut T {
        __vector_bounds_check(index, data_size)
        return unsafe(&mut data_ptr[index])
    }

    func set(&mut self, index : size_t, value : T) {
        __vector_bounds_check(index, data_size)
        unsafe {
            // Replace, don't overwrite: the old element owns its resources.
            destruct get_ptr(index);
            data_ptr[index] = value;
        }
    }

    func size(&self) : size_t {
        return self.data_size;
    }

    func capacity(&self) : size_t {
        return data_cap;
    }

    func data(&self) : *T {
        return self.data_ptr;
    }

    func mutable_data(&mut self) : *mut T {
        return data_ptr;
    }

    func last_ptr(&self) : *mut T {
        __vector_nonempty_check(data_size)
        return unsafe(data_ptr + (data_size - 1));
    }

    func remove(&mut self, index : size_t) {
        __vector_bounds_check(index, data_size)
        const s = data_size;
        const last = s - 1;
        unsafe { destruct get_ptr(index) }
        if(index == last) {
            data_size = last;
        } else {
            unsafe { memmove(&raw mut data_ptr[index], &raw data_ptr[index + 1], sizeof(T) * (last - index)) };
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
        unsafe { destruct ptr; }
        data_size = last;
    }

    func pop_back(&mut self) {
        remove_last()
    }

    func empty(&self) : bool {
        return data_size == 0;
    }

    func clear(&mut self) {
        unsafe { destruct[data_size] data_ptr; }
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
            unsafe {
                destruct[data_size] data_ptr;
                dealloc data_ptr;
            }
        }
    }

    unsafe impl core::iterable::Linear<T> for vector<T> {
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

public interface SpanView<T> {
    func as_span(&self) : span<'self, T>
    func as_mut_span(&mut self) : mut_span<'self, T>
}

impl<T> SpanView<T> for vector<T> {
    public func as_span(&self) : span<'self, T> {
        return unsafe(span<T>(self.data_ptr, self.data_size))
    }

    public func as_mut_span(&mut self) : mut_span<'self, T> {
        return unsafe(mut_span<T>(self.data_ptr, self.data_size))
    }
}

impl<T : core::ops::Clone> core::ops::Clone for vector<T> {
    public func clone(&self) : vector<T> {
        var out = vector<T>()
        out.reserve(self.data_size)
        for(var i : size_t = 0; i < self.data_size; i++) {
            var ptr = unsafe(&raw self.data_ptr[i]) as *T
            out.push(__vector_clone_value(ptr))
        }
        return out
    }
}

unsafe impl<T> Send for vector<T> where T: Send { }
unsafe impl<T> Sync for vector<T> where T: Sync { }
}