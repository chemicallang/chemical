public namespace std {

public comptime const DEQUE_CHUNK_SIZE : size_t = 4

public struct deque_block<T> {
    var data_ptr : *mut T
    var data_size : size_t
}

public struct deque<T> {

    internal var blocks : *mut deque_block<T>
    internal var block_count : size_t
    internal var block_cap : size_t
    internal var data_size : size_t

    @constructor
    func make() {
        return deque<T> {
            blocks : null,
            block_count : 0,
            block_cap : 0,
            data_size : 0
        }
    }

    func try_reserve_blocks(&mut self, new_cap : size_t) : bool {
        if(new_cap <= block_cap) return true
        if(new_cap > ((0 as size_t) - 1) / sizeof(deque_block<T>)) {
            return false
        }
        const new_blocks = unsafe(realloc(blocks, sizeof(deque_block<T>) * new_cap)) as *mut deque_block<T>
        if(new_blocks == null) {
            return false
        }
        blocks = new_blocks
        block_cap = new_cap
        return true
    }

    func reserve_blocks(&mut self, new_cap : size_t) {
        if(!try_reserve_blocks(new_cap)) {
            unsafe(panic("failed to resize deque"))
        }
    }

    func try_add_block(&mut self) : bool {
        unsafe {
            if(block_count >= block_cap) {
                if(block_cap == 0) {
                    if(!try_reserve_blocks(2)) { return false }
                } else {
                    if(block_cap > ((0 as size_t) - 1) / 2) {
                        return false
                    }
                    if(!try_reserve_blocks(block_cap * 2)) { return false }
                }
            }
            const data = unsafe(malloc(sizeof(T) * DEQUE_CHUNK_SIZE)) as *mut T
            if(data == null) {
                return false
            }
            blocks[block_count] = deque_block<T> {
                data_ptr : data,
                data_size : 0
            }
            block_count += 1
            return true
        }
    }

    func add_block(&mut self) {
        if(!try_add_block()) {
            unsafe(panic("failed to allocate deque block"))
        }
    }

    func try_push(&mut self, value : T) : bool {
        unsafe {
            if(block_count == 0 || blocks[block_count - 1].data_size == DEQUE_CHUNK_SIZE) {
                if(!try_add_block()) { return false }
            }
            const block_index = block_count - 1
            const elem_index = blocks[block_index].data_size
            unsafe { memcpy(&raw mut blocks[block_index].data_ptr[elem_index], &raw value, sizeof(T)) }
            intrinsics::forget(value)
            blocks[block_index].data_size = elem_index + 1
            data_size += 1
            return true
        }
    }

    func push(&mut self, value : T) {
        unsafe {
            if(block_count == 0 || blocks[block_count - 1].data_size == DEQUE_CHUNK_SIZE) {
                add_block()
            }
            const block_index = block_count - 1
            const elem_index = blocks[block_index].data_size
            unsafe { memcpy(&raw mut blocks[block_index].data_ptr[elem_index], &raw value, sizeof(T)) }
            intrinsics::forget(value)
            blocks[block_index].data_size = elem_index + 1
            data_size += 1
    
        }
    }

    func push_back(&mut self, value : T) {
        push(value)
    }

    func size(&self) : size_t { return data_size }
    func empty(&self) : bool { return data_size == 0 }

    func get_ptr(&self, index : size_t) : *mut T {
        unsafe {
            if(index >= data_size) {
                panic("deque index out of bounds")
            }
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
    }

    func get_ref(&self, index : size_t) : &T {
        return unsafe(&*get_ptr(index))
    }

    func get_mut(&mut self, index : size_t) : &mut T {
        return unsafe(&mut *get_ptr(index))
    }

    func clear(&mut self) {
        unsafe {
            var block_index = 0 as size_t
            while(block_index < block_count) {
                destruct[blocks[block_index].data_size] blocks[block_index].data_ptr
                dealloc blocks[block_index].data_ptr
                block_index += 1
            }
            block_count = 0
            data_size = 0
    
        }
    }

    @delete
    func delete(&mut self) {
        unsafe {
            clear()
            if(blocks != null) {
                dealloc blocks
            }
    
        }
    }

    impl core::iterable::Chunked<T, size_t> for deque<T> {
        func begin_chunks(&self) : size_t { return 0 }
        func valid_chunk(&self, c : size_t) : bool { return c < block_count }
        func current_chunk(&self, c : size_t) : core::iterable::Chunk<T> {
            return unsafe(core::iterable::chunk_unchecked<T>(blocks[c].data_ptr, blocks[c].data_size as u64))
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


unsafe impl<T> Send for deque<T> where T: Send { }
unsafe impl<T> Sync for deque<T> where T: Sync { }
}
