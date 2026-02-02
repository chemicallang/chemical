public namespace std {

public struct vector<T> {

    var data_ptr : *mut T
    var data_size : size_t
    var data_cap : size_t

    @constructor
    func make() {
        data_ptr = null
        data_size = 0
        data_cap = 0
    }

    @constructor
    func make_with_capacity(init_cap : size_t) {
        data_ptr = malloc(sizeof(T) * init_cap) as *mut T
        data_size = 0
        data_cap = init_cap
    }

    func resize(&mut self, new_cap : size_t) {
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

    func reserve(&mut self, cap : size_t) {
        if(cap <= data_cap) return;
        resize(cap);
    }

    func ensure_capacity_for_one_more(&mut self) {
        if (data_cap == 0) {
            resize(2)
        } else {
            resize(data_cap * 2)
        }
    }

    func push(&mut self, value : T) {
        const s = data_size;
        if (s >= data_cap) {
            ensure_capacity_for_one_more()
        }
        memcpy(&mut data_ptr[s], &value, sizeof(T))
        intrinsics::forget(value)
        data_size = s + 1
    }

    func push_back(&mut self, value : T) {
        push(value)
    }

    func get(&self, index : size_t) : T {
        return data_ptr[index];
    }

    func get_ptr(&self, index : size_t) : *mut T {
        return &mut data_ptr[index];
    }

    func get_ref(&self, index : size_t) : &mut T {
        return data_ptr[index]
    }

    func set(&mut self, index : size_t, value : T) {
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
        return data_ptr + (data_size - 1);
    }

    func remove(&mut self, index : size_t) {
        const s = data_size;
        const last = s - 1;
        destruct get_ptr(index)
        if(index == last) {
            data_size = last;
        } else {
            memmove(&mut data_ptr[index], &data_ptr[index + 1], sizeof(T) * (last - index));
            data_size = last;
        }
    }

    func erase(&mut self, index : size_t) {
        remove(index);
    }

    func remove_last(&mut self) {
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
        const last = data_size - 1
        data_size = last;
        return *get_ptr(last);
    }

    func empty(&self) : bool {
        return data_size == 0;
    }

    func clear(&mut self) {
        destruct[data_size] data_ptr;
        data_size = 0;
    }

    func resize_unsafe(&mut self, new_size : size_t) {
        data_size = new_size
    }

    @delete
    func delete(&mut self) {
        if(data_ptr != null) {
            destruct[data_size] data_ptr;
            dealloc data_ptr;
        }
    }

}

}