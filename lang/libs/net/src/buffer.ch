// ===== Buffer implementation =====
public namespace net {

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

    public func append_bytes(&mut self, src:*u8, n: usize) {
        v.reserve(v.size() + n)
        var curr = src;
        const end = curr + n;
        while(curr != end) {
            v.push(*curr)
            curr++
        }
    }

    func consume(&mut self, n: usize) {
        read_pos = read_pos + n;
        if(read_pos > 4096 && read_pos * 2 > v.size()) {
            var ptr = v.get_ptr(0) as *mut u8;
            var rem = v.size() - read_pos;
            memmove(ptr, ptr + read_pos, rem);
            v.resize(rem);
            read_pos = 0u;
        }
    }

    func as_ptr(&self) : *u8 {
        if(v.size() == 0) { return null }
        return &raw v.get_ptr(read_pos)[0]
    }

    func get_byte(&self, idx: usize) : u8 { return v.get(read_pos + idx) }
}

}
