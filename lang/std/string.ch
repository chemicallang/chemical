import "./std.ch"

struct string {

    union {
        struct {
            var data : char*;
            var length : size_t
        } constant;
        struct {
            var data : char*;
            var length : size_t;
            var capacity : size_t;
        } heap;
        struct {
            var buffer : char[16];
            var length : uchar;
        } sso;
    } storage;
    var state : char

    @comptime
    @constructor
    func make(value : literal::string) {
        return compiler::wrap(constructor(value, compiler::strlen(value)))
    }

    @constructor
    func constructor(value : char*, length : size_t) {
        var s : string
        s.storage.constant.data = value;
        s.storage.constant.length = length;
        s.state = '0'
        return s;
    }

    func size(&self) : size_t {
        if(state == '0') {
            return storage.constant.length;
        } else if(state == '1') {
            return storage.sso.length;
        } else {
            return storage.heap.length;
        }
    }

    func equals(&self, other : string*) : bool {
        const self_size = size();
        return self_size == other.size() && memcmp(self.data(), other.data(), self_size) == 0;
    }

    func move_const_to_buffer(&self) {
        const data = storage.constant.data;
        const length = storage.constant.length;
        for(var i = 0; i < length; i++) {
            storage.sso.buffer[i] = data[i];
        }
        storage.sso.length = length;
        state = '1'
    }

    func move_data_to_heap(&self, from_data : char*, length : size_t, capacity : size_t) {
        var data = malloc(capacity) as char*
        var i = 0
        while(i < length) {
            data[i] = from_data[i]
            i++
        }
        data[i] = '\0'
        storage.heap.data = data;
        storage.heap.length = length;
        storage.heap.capacity = capacity
        state = '2'
    }

    func resize(&self, new_capacity : size_t) {
        var data = realloc(storage.heap.data, new_capacity) as char*
        var i = 0
        while(i < storage.heap.length) {
            data[i] = storage.heap.data[i]
            i++
        }
        data[i] = '\0'
        storage.heap.data = data;
        storage.heap.capacity = new_capacity
    }

    // ensures that capacity is larger than length given and memory is mutable
    func ensure_mut(&self, length : size_t) {
        if((state == '0' || state == '1') && length <= 16) {
            if(state == '0') {
                move_const_to_buffer();
            }
        } else {
            if(state == '0') {
                move_data_to_heap(storage.constant.data, storage.constant.length, length);
            } else if(state == '1') {
                move_data_to_heap(&storage.sso.buffer[0], storage.sso.length, length);
            } else if(storage.heap.capacity <= length) {
                resize(length);
            }
        }
    }

    func set(&self, index : size_t, value : char) {
        if(state == '0') {
            move_const_to_buffer();
        }
        switch(state) {
            case '1' -> {
                storage.sso.buffer[index] = value;
            }
            case '2' -> {
                storage.heap.data[index] = value;
            }
        }
    }

    func get(&self, index : size_t) : char {
        switch(state) {
            case '0' -> {
                return storage.constant.data[index];
            }
            case '1' -> {
                return storage.sso.buffer[index];
            }
            case '2' -> {
                return storage.heap.data[index]
            }
            default -> {
                return '\0'
            }
        }
    }

    func append_with_len(&self, value : char*, len : size_t) {
        ensure_mut(size() + len);
        var i : size_t = 0;
        while(i < len) {
            append(value[i]);
            i++;
        }
    }

    func append_char_ptr(&self, value : char*) {
        append_with_len(value, strlen(value));
    }

    func append_str(&self, value : string*) {
        append_with_len(value.data(), value.size())
    }

    func copy(&self) : string {
        var s : string
        s.state = state;
        switch(state) {
            case '0' -> {
                s.storage.constant.data = storage.constant.data
                s.storage.constant.length = storage.constant.length
            }
            case '1' -> {
                s.storage.sso.length = storage.sso.length
                memcpy(s.storage.sso.buffer, storage.sso.buffer, storage.sso.length)
            }
            case '2' -> {
                var new_heap = malloc(storage.heap.capacity) as char*
                memcpy(new_heap, storage.heap.data, storage.heap.length)
                s.storage.heap.data = new_heap
                s.storage.heap.length = storage.heap.length
                s.storage.heap.capacity = storage.heap.capacity
            }
        }
        return s;
    }

    func substring(&self, start : size_t, end : size_t) : string {
        var s : string
        const actual_len : size_t = end - start;
        if(actual_len <= 16) {
            s.state = '1'
            s.storage.sso.length = actual_len
            const d = data()
            for(var i = 0; i < actual_len; i++) {
                s.storage.sso.buffer[i] = d[start + i]
            }
            s.storage.sso.buffer[actual_len] = '\0'
        } else {
            s.state = '2'
            const new_cap = actual_len * 2
            var new_heap = malloc(new_cap) as char*
            const d = data()
            for(var i = 0; i < actual_len; i++) {
                new_heap[i] = d[start + i]
            }
            s.storage.heap.data = new_heap
            s.storage.heap.data[actual_len] = '\0'
            s.storage.heap.length = actual_len
            s.storage.heap.capacity = new_cap
        }
        return s;
    }

    func append(&self, value : char) {
        const length = size();
        if((state == '0' || state == '1') && length < 16) {
            if(state == '0') {
                move_const_to_buffer();
            }
            storage.sso.buffer[length] = value;
            storage.sso.buffer[length + 1] = '\0'
            storage.sso.length = length + 1;
        } else {
            if(state == '0') {
                move_data_to_heap(storage.constant.data, length, length * 2);
            } else if(state == '1') {
                move_data_to_heap(&storage.sso.buffer[0], length, length * 2);
            } else if(storage.heap.capacity <= length + 1) {
                resize(storage.heap.capacity * 2);
            }
            storage.heap.data[length] = value;
            storage.heap.data[length + 1] = '\0'
            storage.heap.length = length + 1;
        }
    }

    func capacity(&self) : size_t {
        switch(state) {
            case '0' -> {
                return storage.constant.length;
            }
            case '1' -> {
                return 16;
            }
            case '2' -> {
                return storage.heap.capacity;
            }
            default -> {
                return 0;
            }
        }
    }

    func data(&self) : char* {
        switch(state) {
            case '0' -> {
                return storage.constant.data
            }
            case '1' -> {
                return &storage.sso.buffer[0];
            }
            case '2' -> {
                return storage.heap.data;
            }
            default -> {
                return null;
            }
        }
    }

    @destructor
    func delete(&self) {
        if(state == '2') {
            free(storage.heap.data);
        }
    }

}