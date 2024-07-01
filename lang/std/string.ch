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
        return size() == other.size() && strcmp(self.data(), other.data()) == 0;
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

    func append(&self, value : char) {
        const length = size();
        if((state == '0' || state == '1') && length < 15) {
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
        if(state == '0') {
            return storage.constant.length;
        } else if(state == '1') {
            return 16;
        } else {
            return storage.heap.capacity
        }
    }

    func data(&self) : char* {
        if(state == '0') {
            return storage.constant.data
        } else if(state == '1') {
            return &storage.sso.buffer[0];
        } else {
            return storage.heap.data;
        }
    }

    @destructor
    func delete(&self) {
        if(state == '2') {
            free(storage.heap.data);
        }
    }

}