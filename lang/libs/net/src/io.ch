public namespace io {

    // ===== Buffer implementation =====
    public struct Buffer {
        var v: std::vector<u8>;
        var read_pos: usize; // how many bytes consumed from the front

        @constructor func constructor() {
            return Buffer {
                v : std::vector<u8>(),
                read_pos : 0u
            }
        }

        func len(&self) : usize { return if(v.size() >= read_pos) v.size() - read_pos else 0u }

        func ensure_capacity(&mut self, extra: usize) {
            var need = (v.size() - read_pos) + extra;
            if(v.capacity() < need) { v.reserve(need * 2) }
        }

        func append_bytes(&mut self, src:*u8, n: usize) {
            const old_sz = v.size();
            v.resize(old_sz + n);
            memcpy(v.get_ptr(old_sz), src, n);
            v.resize_unsafe(old_sz + n);
        }

        func consume(&mut self, n: usize) {
            // simple consume: advance read_pos, and compact vector if large
            read_pos = read_pos + n;
            if(read_pos > 4096 && read_pos * 2 > v.size()) {
                // compact in-place using memmove
                // get raw pointer to start of data
                var ptr = v.get_ptr(0) as *mut u8;
                var rem = v.size() - read_pos;
                // move remaining bytes to front
                memmove(ptr, ptr + read_pos, rem);
                // resize vector to reflect new size (just changes size, doesn't realloc/free)
                v.resize_unsafe(rem);
                read_pos = 0u;
            }
        }

        func as_ptr(&self) : *u8 {
            if(v.size() == 0) { return null }
            return &v.get_ptr(read_pos)[0]
        }

        func get_byte(&self, idx: usize) : u8 { return v.get(read_pos + idx) }
    }

}