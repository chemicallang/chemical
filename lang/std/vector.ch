import "./std.ch"

public struct vector<T> {

    var data_ptr : T*
    var data_size : size_t
    var data_cap : size_t

    @constructor
    func make() {
        data_ptr = malloc(#sizeof { T } * 2)
        data_size = 0
        data_cap = 2
    }

    @constructor
    func make_with_capacity(init_cap : size_t) {
        data_ptr = malloc(#sizeof { T } * init_cap)
        data_size = 0
        data_cap = init_cap
    }

    func resize(&self, new_cap : size_t) {
        var new_data = realloc(data_ptr, (#sizeof { T } * new_cap)) as T*;
        unsafe {
            if (new_data != null) {
                data_ptr = new_data;
                data_cap = new_cap;
            } else {
                // Handle allocation failure
                // fprintf(stderr, "Failed to resize vector\n");
                printf("failed to resize vector to a capacity of %d\n", new_cap);
                // exit(1);
            }
        }
    }

    func push(&self, value : T) {
        const s = data_size;
        if (s >= data_cap) {
            // Double the capacity if needed
            resize(data_cap * 2);
        }
        data_ptr[s] = value;
        data_size = s + 1
    }

    func get(&self, index : size_t) : T {
        return data_ptr[index];
    }

    func get_ptr(&self, index : size_t) : T* {
        return &data_ptr[index];
    }

    func set(&self, index : size_t, value : T) {
        data_ptr[index] = value;
    }

    func size(&self) : size_t {
        return data_size;
    }

    func capacity(&self) : size_t {
        return data_cap;
    }

    func data(&self) : T* {
        return data_ptr;
    }

    func remove(&self, index : size_t) {
        const s = data_size;
        const last = s - 1;
        destruct get_ptr(index)
        if(index == last) {
            data_size = last;
        } else {
            for (var i = index; i < last; i++) {
                data_ptr[i] = data_ptr[i + 1];
            }
            data_size = last;
        }
    }

    func remove_last(&self) {
        const s = data_size;
        const last = s - 1;
        const ptr = get_ptr(last);
        destruct ptr;
        data_size = last;
    }

    func clear(&self) {
        destruct[data_size] data_ptr;
        data_size = 0;
    }

    @delete
    func delete(&self) {
        destruct[data_size] data_ptr;
        free(data_ptr);
        unsafe {
            data_ptr = null;
        }
        data_size = 0;
        data_cap = 0;
    }

}