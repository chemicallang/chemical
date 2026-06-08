public namespace std {

public comptime const DEQUE_CHUNK_SIZE : size_t = 4

public struct deque_block<T> {
    var data_ptr : *mut T
    var data_size : size_t
}

public struct deque<T> {

    var blocks : *mut deque_block<T>
    var block_count : size_t
    var block_cap : size_t
    var data_size : size_t

    @constructor
    func make() {
        return deque<T> {
            blocks : null,
            block_count : 0,
            block_cap : 0,
            data_size : 0
        }
    }

    func reserve_blocks(&mut self, new_cap : size_t) {
        if(new_cap <= block_cap) return
        const new_blocks = realloc(blocks, sizeof(deque_block<T>) * new_cap) as *mut deque_block<T>
        unsafe {
            if(new_blocks != null) {
                blocks = new_blocks
                block_cap = new_cap
            } else {
                panic("failed to resize deque")
            }
        }
    }

    func add_block(&mut self) {
        if(block_count >= block_cap) {
            if(block_cap == 0) {
                reserve_blocks(2)
            } else {
                reserve_blocks(block_cap * 2)
            }
        }
        const data = malloc(sizeof(T) * DEQUE_CHUNK_SIZE) as *mut T
        blocks[block_count] = deque_block<T> {
            data_ptr : data,
            data_size : 0
        }
        block_count += 1
    }

    func push(&mut self, value : T) {
        if(block_count == 0 || blocks[block_count - 1].data_size == DEQUE_CHUNK_SIZE) {
            add_block()
        }
        const block_index = block_count - 1
        const elem_index = blocks[block_index].data_size
        memcpy(&raw mut blocks[block_index].data_ptr[elem_index], &raw value, sizeof(T))
        intrinsics::forget(value)
        blocks[block_index].data_size = elem_index + 1
        data_size += 1
    }

    func push_back(&mut self, value : T) {
        push(value)
    }

    func size(&self) : size_t { return data_size }
    func empty(&self) : bool { return data_size == 0 }

    func get_ptr(&self, index : size_t) : *mut T {
        var remaining = index
        var block_index = 0 as size_t
        while(block_index < block_count) {
            const block_size = blocks[block_index].data_size
            if(remaining < block_size) {
                return &raw mut blocks[block_index].data_ptr[remaining]
            }
            remaining = remaining - block_size
            block_index += 1
        }
        return null
    }

    func get_ref(&self, index : size_t) : &mut T {
        return &mut *get_ptr(index)
    }

    func clear(&mut self) {
        var block_index = 0 as size_t
        while(block_index < block_count) {
            destruct[blocks[block_index].data_size] blocks[block_index].data_ptr
            dealloc blocks[block_index].data_ptr
            block_index += 1
        }
        block_count = 0
        data_size = 0
    }

    @delete
    func delete(&mut self) {
        clear()
        if(blocks != null) {
            dealloc blocks
        }
    }

    impl core::iterable::Chunked<T, size_t> for deque<T> {
        func begin_chunks(&self) : size_t { return 0 }
        func valid_chunk(&self, c : size_t) : bool { return c < block_count }
        func current_chunk(&self, c : size_t) : core::iterable::Chunk<T> {
            return core::iterable::Chunk<T> { ptr : blocks[c].data_ptr, len : blocks[c].data_size }
        }
        func next_chunk(&self, c : size_t) : size_t { return c + 1 }
        func rbegin_chunks(&self) : size_t {
            if(block_count == 0) {
                return 0
            }
            return block_count - 1
        }
        func previous_chunk(&self, c : size_t) : size_t { return c - 1 }
        func total_size(&self) : size_t { return data_size }
    }

}

}
