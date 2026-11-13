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

    /// Bytes still readable (everything past the consumed prefix).
    public func len(&self) : usize { return if(v.size() >= read_pos) v.size() - read_pos else 0u }

    /// Total bytes ever written to this buffer.
    public func written(&self) : usize { return v.size() }

    public func empty(&self) : bool { return self.len() == 0u }

    func ensure_capacity(&mut self, extra: usize) {
        var need = (v.size() - read_pos) + extra;
        if(v.capacity() < need) { v.reserve(need * 2) }
    }

    /// Make room for at least `extra` more bytes without growing again.
    public func reserve(&mut self, extra: usize) {
        ensure_capacity(extra)
    }

    /// Append a byte slice. Bulk copy, not a per-byte loop.
    public func append(&mut self, data : &[u8]) {
        var n = data.size()
        if(n == 0u) { return }
        v.reserve(v.size() + n)
        unsafe {
            var dst = v.mutable_data() + v.size()
            memcpy(dst, data.data(), n)
            v.set_len(v.size() + n)
        }
    }

    /// Raw append. Prefer `append(&[u8])`; kept for C-side callers that already
    /// hold a pointer/length pair.
    public func append_bytes(&mut self, src:*u8, n: usize) {
        if(n == 0u) { return }
        var view = unsafe(std::span<u8>(src, n))
        append(view)
    }

    public func consume(&mut self, n: usize) {
        read_pos = read_pos + n;
        if(read_pos > 4096 && read_pos * 2 > v.size()) {
            var ptr = v.mutable_data();
            var rem = v.size() - read_pos;
            unsafe { memmove(ptr, ptr + read_pos, rem) };
            v.resize(rem);
            read_pos = 0u;
        }
    }

    /// The remaining readable bytes, as a borrowed view.
    public func view(&self) : std::span<'self, u8> {
        if(read_pos >= v.size()) { return std::span<u8>() }
        return unsafe(std::span<u8>(v.data() + read_pos, v.size() - read_pos))
    }

    public func as_ptr(&self) : *u8 {
        if(read_pos >= v.size()) { return null }
        return unsafe(v.data() + read_pos)
    }

    public func get_byte(&self, idx: usize) : u8 { return v.at(read_pos + idx) }

    public func clear(&mut self) {
        v.clear()
        read_pos = 0u
    }

    /// Move the remaining bytes out, leaving the buffer empty.
    public func take(&mut self) : std::vector<u8> {
        var out = std::vector<u8>()
        var remaining = self.len()
        if(remaining == 0u) { return out }
        out.reserve(remaining)
        unsafe {
            memcpy(out.mutable_data(), self.as_ptr(), remaining)
            out.set_len(remaining)
        }
        self.clear()
        return out
    }
}

}
