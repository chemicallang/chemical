import "./std.ch"

struct vector<T> {

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
        if(index == last) {
            data_size = last;
        } else {
            // TODO store the value at index at last
            // this would ensure that when destructor is called, the value at current index
            // would be present inside the same memory block owned by pointer, so delete would call destructor on it
            // const temp = std::memcpy(data_ptr[i])
            for (var i = index; i < last; i++) {
                // TODO use std::memcpy instead of assignment so structs are copied properly
                data_ptr[i] = data_ptr[i + 1];
            }
            // data_ptr[s] = temp; // <--- size is not considered when destruction occurs, capacity is considered
            data_size = last;
        }
    }

    func remove_last(&self) {
        const s = data_size;
        const last = s - 1;
        data_size = last;
    }

    func clear(&self) {
        data_size = 0;
    }

    @destructor
    func delete(&self) {
        // TODO use delete[] data_ptr; instead of free(data_ptr) so destructors are called
        free(data_ptr);
        data_ptr = null;
        data_size = 0;
        data_cap = 0;
    }

}