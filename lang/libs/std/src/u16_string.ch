public namespace std {

public comptime const U16_STR_BUFF_SIZE = 8;

const EMPTY_U16 : u16 = 0;

public func u16_strlen(ptr : *u16) : size_t {
    const start = ptr;
    var current = ptr
    while(*current != 0) {
        current++
    }
    return current - start;
}

public struct u16string {

    union {
        struct {
            var data : *u16;
            var length : size_t
        } constant;
        struct {
            var data : *mut u16;
            var length : size_t;
            var capacity : size_t;
        } heap;
        struct {
            var buffer : [U16_STR_BUFF_SIZE]u16;
            var length : uchar;
        } sso;
    } storage;
    var state : char

    @constructor
    func constructor(value : *u16, length : size_t) {
        var s = u16string {
            storage : {
                constant : {
                    data : value,
                    length : length
                }
            },
            state : '0'
        }
        s.ensure_mut(length)
        return s;
    }

    // the ensure parameter is added just to differentiate signature from constructor above it
    // this allows to keep literal strings as constants
    @constructor
    func constructor2(value : *u16, length : size_t, ensure : bool) {
        var s = u16string {
            storage : {
                constant : {
                    data : value,
                    length : length
                }
            },
            state : '0'
        }
        if(ensure) {
            // this branch probably will never be taken
            s.ensure_mut(length)
        }
        return s;
    }

    @constructor
    func empty_str() {
        return u16string {
            storage : {
                constant : {
                    data : &EMPTY_U16,
                    length : 0
                }
            },
            state : '0'
        }
    }

    @constructor
    func make_no_len(value : *u16) {
        const length = u16_strlen(value)
        var s = u16string {
            storage : {
                constant : {
                    data : value,
                    length : length
                }
            },
            state : '0'
        }
        s.ensure_mut(length)
        return s;
    }

    func size(&self) : size_t {
        switch(state) {
            '0' => {
                return storage.constant.length;
            }
            '1' => {
                return storage.sso.length;
            }
            '2' => {
                return storage.heap.length;
            }
            default => {
                return 0
            }
        }
    }

    func empty(&self) : bool {
        return size() == 0
    }

    func move_const_to_buffer(&mut self) {
        const d = storage.constant.data;
        const length = storage.constant.length;
        unsafe {
            for(var i = 0; i < length; i++) {
                storage.sso.buffer[i] = d[i];
            }
        }
        storage.sso.buffer[length] = 0u16
        storage.sso.length = length as uchar;
        state = '1'
    }

    func move_data_to_heap(&mut self, from_data : *u16, length : size_t, capacity : size_t) {
        // capacity multiplied by 2 because we need two bytes for a single character
        // +2 is for safety, we need to add a null terminator (always)
        var d = malloc((capacity * 2) + 2) as *mut u16
        memcpy(d, from_data, length * 2)
        d[length] = 0u16
        storage.heap.data = d;
        storage.heap.length = length;
        storage.heap.capacity = capacity
        state = '2'
    }

    // this is a private function
    // new_capacity is always > length
    func resize_heap(&mut self, new_capacity : size_t) {
        // new_capacity multiplied by 2 because we need two bytes for a single character
        // +2 for the null terminator
        var d = realloc(storage.heap.data, (new_capacity * 2) + 2) as *mut u16
        d[storage.heap.length] = 0u16
        storage.heap.data = d;
        storage.heap.capacity = new_capacity
    }

    // ensures that capacity is larger than length given and memory is mutable
    func ensure_mut(&mut self, length : size_t) {
        if((state == '0' || state == '1') && length < U16_STR_BUFF_SIZE) {
            if(state == '0') {
                move_const_to_buffer();
            }
        } else {
            if(state == '0') {
                move_data_to_heap(storage.constant.data, storage.constant.length, length);
            } else if(state == '1') {
                move_data_to_heap(&storage.sso.buffer[0], storage.sso.length, length);
            } else if(storage.heap.capacity <= length) {
                resize_heap(length);
            }
        }
    }

    func reserve(&mut self, new_capacity : size_t) {
        switch(state) {
            '0' => {
                if(new_capacity < U16_STR_BUFF_SIZE && storage.constant.length < U16_STR_BUFF_SIZE) {
                    move_const_to_buffer();
                } else {
                    const len = storage.constant.length
                    if(new_capacity > len) {
                        // user requested capacity is greater than current length of string
                        // we'll allocate the requested capacity
                        move_data_to_heap(storage.constant.data, len, new_capacity);
                    } else {
                        // user requested capacity is smaller than current length of string
                        // we'll allocate the length of current string
                        move_data_to_heap(storage.constant.data, len, len);
                    }
                }
            }
            '1' => {
                if(new_capacity >= U16_STR_BUFF_SIZE) {
                    move_data_to_heap(&storage.sso.buffer[0], storage.sso.length, new_capacity);
                }
            }
            '2' => {
                if(new_capacity > storage.heap.capacity) {
                    resize_heap(new_capacity);
                }
            }
        }
    }

    func set(&mut self, index : size_t, value : u16) {
        switch(state) {
            '0' => {
                move_const_to_buffer();
                storage.sso.buffer[index] = value;
            }
            '1' => {
                storage.sso.buffer[index] = value;
            }
            '2' => {
                storage.heap.data[index] = value;
            }
        }
    }

    func get(&self, index : size_t) : u16 {
        switch(state) {
            '0' => {
                return storage.constant.data[index];
            }
            '1' => {
                return storage.sso.buffer[index];
            }
            '2' => {
                return storage.heap.data[index]
            }
            default => {
                return 0
            }
        }
    }

    func append_with_len(&mut self, value : *u16, len : size_t) {
        // offset, where we start writing
        const offset = size()
        // the size of the string, after we've written
        const new_size = offset + len
        // +1 for null terminator
        ensure_mut(new_size + 1);
        if(state == '1') {
            // len * 2 (because len is of 2 byte characters, memcpy works for single byte pointers)
            memcpy(&mut storage.sso.buffer[offset], value, len * 2)
            storage.sso.buffer[new_size] = 0u16
            storage.sso.length = new_size
        } else {
            // state is '2', it cannot be '0'
            // len * 2 (because len is of 2 byte characters, memcpy works for single byte pointers)
            memcpy(&mut storage.heap.data[offset], value, len * 2)
            storage.heap.data[new_size] = 0u16
            storage.heap.length = new_size
        }
    }

    // append a single u16 unit (caller provides correct unit)
    func append_u16_unit(&mut self, u : u16) {
        const offset = size();
        const new_size = offset + 1;
        ensure_mut(new_size + 1);
        if(state == '1') {
            storage.sso.buffer[offset] = u;
            storage.sso.buffer[new_size] = 0u16;
            storage.sso.length = new_size;
        } else {
            storage.heap.data[offset] = u;
            storage.heap.data[new_size] = 0u16;
            storage.heap.length = new_size;
        }
    }

    // append a single char (treated as a single UTF-8 byte)
    func append_char(&mut self, c : char) {
        var tmp : char = c;
        append_utf8_view(&tmp, 1);
    }

    // append a Unicode code point (uint32), handling surrogate pairs
    func append_codepoint(&mut self, cp : u32) {
        if(cp <= 0xFFFFu32) {
            // avoid writing surrogate code points directly
            if(cp >= 0xD800u32 && cp <= 0xDFFFu32) {
                // invalid scalar — insert replacement char
                append_u16_unit(0xFFFDu16);
            } else {
                append_u16_unit(cp as u16);
            }
        } else if(cp <= 0x10FFFFu32) {
            var v = cp - 0x10000u32;
            var high = (0xD800u32 + (v >> 10)) as u16;
            var low  = (0xDC00u32 + (v & 0x3FFu32)) as u16;
            const offset = size();
            const new_size = offset + 2;
            ensure_mut(new_size + 1);
            if(state == '1') {
                storage.sso.buffer[offset] = high;
                storage.sso.buffer[offset + 1] = low;
                storage.sso.buffer[new_size] = 0u16;
                storage.sso.length = new_size;
            } else {
                storage.heap.data[offset] = high;
                storage.heap.data[offset + 1] = low;
                storage.heap.data[new_size] = 0u16;
                storage.heap.length = new_size;
            }
        } else {
            // invalid code point
            append_u16_unit(0xFFFDu16);
        }
    }

    // append UTF-8 view: convert to UTF-16 and append
    func append_utf8_view(&mut self, ptr : *char, len : size_t) {
        if(len == 0) { return; }
        // worst-case u16 units needed: len (each byte -> at most 1 u16 unit for ASCII,
        // multi-byte sequences produce <=1 or 2 units; allocating len is safe)
        var out_cap = len + 1;
        var out = malloc((out_cap * 2) + 2) as *mut u16;
        var in_i : size_t = 0;
        var out_i : size_t = 0;
        while(in_i < len) {
            var b0 = (ptr[in_i] as uchar) & 0xFF;
            var cp : u32 = 0u32;

            if((b0 & 0x80) == 0) {
                cp = b0 as u32;
                in_i = in_i + 1;
            } else if((b0 & 0xE0) == 0xC0 && (in_i + 1) < len) {
                var b1 = (ptr[in_i + 1] as uchar) & 0xFF;
                if((b1 & 0xC0) != 0x80) {
                    cp = 0xFFFDu32;
                    in_i = in_i + 1;
                } else {
                    cp = (((b0 & 0x1F) as u32) << 6) | ((b1 & 0x3F) as u32);
                    if(cp < 0x80u32) { cp = 0xFFFDu32; }
                    in_i = in_i + 2;
                }
            } else if((b0 & 0xF0) == 0xE0 && (in_i + 2) < len) {
                var b1 = (ptr[in_i + 1] as uchar) & 0xFF;
                var b2 = (ptr[in_i + 2] as uchar) & 0xFF;
                if(((b1 & 0xC0) != 0x80) || ((b2 & 0xC0) != 0x80)) {
                    cp = 0xFFFDu32;
                    in_i = in_i + 1;
                } else {
                    cp = (((b0 & 0x0F) as u32) << 12) |
                        (((b1 & 0x3F) as u32) << 6) |
                        ((b2 & 0x3F) as u32);
                    if(cp < 0x800u32 || (cp >= 0xD800u32 && cp <= 0xDFFFu32)) { cp = 0xFFFDu32; }
                    in_i = in_i + 3;
                }
            } else if((b0 & 0xF8) == 0xF0 && (in_i + 3) < len) {
                var b1 = (ptr[in_i + 1] as uchar) & 0xFF;
                var b2 = (ptr[in_i + 2] as uchar) & 0xFF;
                var b3 = (ptr[in_i + 3] as uchar) & 0xFF;
                if(((b1 & 0xC0) != 0x80) || ((b2 & 0xC0) != 0x80) || ((b3 & 0xC0) != 0x80)) {
                    cp = 0xFFFDu32;
                    in_i = in_i + 1;
                } else {
                    cp = (((b0 & 0x07) as u32) << 18) |
                        (((b1 & 0x3F) as u32) << 12) |
                        (((b2 & 0x3F) as u32) << 6) |
                        ((b3 & 0x3F) as u32);
                    if(cp < 0x10000u32 || cp > 0x10FFFFu32) { cp = 0xFFFDu32; }
                    in_i = in_i + 4;
                }
            } else {
                cp = 0xFFFDu32;
                in_i = in_i + 1;
            }

            // encode cp to UTF-16 units in out buffer
            if(cp <= 0xFFFFu32) {
                out[out_i] = cp as u16;
                out_i = out_i + 1;
            } else {
                var v = cp - 0x10000u32;
                var high = (0xD800u32 + (v >> 10)) as u16;
                var low  = (0xDC00u32 + (v & 0x3FFu32)) as u16;
                out[out_i] = high;
                out_i = out_i + 1;
                out[out_i] = low;
                out_i = out_i + 1;
            }
        }

        // append converted units
        append_with_len(out, out_i);

        // free temporary buffer
        dealloc out;
    }

    // substring copy by u16 unit indices
    func substr(&self, start : size_t, len : size_t) : u16string {
        var result = u16string.empty_str();
        const my_size = size();
        if(start >= my_size) {
            return result;
        }
        var real_len = len;
        if(start + real_len > my_size) {
            real_len = my_size - start;
        }
        if(real_len == 0) { return result; }
        const src = data();
        // create a temporary buffer to copy into result
        var buf = malloc((real_len * 2) + 2) as *mut u16;
        memcpy(buf, &src[start], real_len * 2);
        buf[real_len] = 0u16;
        // use constructor2 to keep constant/heap invariant: create as heap-backed
        result.constructor(buf, real_len);
        // Note: constructor called ensure_mut(size()) which will move to SSO/heap as needed.
        return result;
    }

    // convert from UTF-16 to UTF-8; returns newly allocated buffer and length
    func to_utf8(&self) : std::string {
        const n = size();
        if(n == 0) {
            // returning empty string
            return std::string()
        }
        // safe upper-bound: each u16 unit -> up to 3 bytes in UTF-8 (BMP),
        // surrogate pairs (2 units) -> 4 bytes, so n*3 + 1 is safe.
        var out_cap = (n * 3) + 1;
        var out = std::string();
        out.reserve(out_cap + 1);
        var i : size_t = 0;
        const src = data();
        while(i < n) {
            var w = src[i] as u32;
            if(w >= 0xD800u32 && w <= 0xDBFFu32 && (i + 1) < n) {
                // possible high surrogate
                var w2 = src[i + 1] as u32;
                if(w2 >= 0xDC00u32 && w2 <= 0xDFFFu32) {
                    var cp = (((w - 0xD800u32) << 10) | (w2 - 0xDC00u32)) + 0x10000u32;
                    // encode cp as 4 bytes
                    out.append((0xF0u32 | ((cp >> 18) & 0x07u32)) as char);
                    out.append((0x80u32 | ((cp >> 12) & 0x3Fu32)) as char);
                    out.append((0x80u32 | ((cp >> 6) & 0x3Fu32)) as char)
                    out.append((0x80u32 | (cp & 0x3Fu32)) as char);
                    i = i + 2;
                    continue;
                }
            }

            // non-surrogate or isolated surrogate: encode w to UTF-8
            if(w <= 0x7Fu32) {
                out.append((w & 0x7Fu32) as char);
            } else if(w <= 0x7FFu32) {
                out.append((0xC0u32 | ((w >> 6) & 0x1Fu32)) as char);
                out.append((0x80u32 | (w & 0x3Fu32)) as char);
            } else {
                // BMP up to U+FFFF (including possible lone surrogates treated as replacement)
                if(w >= 0xD800u32 && w <= 0xDFFFu32) {
                    // isolated surrogate -> replacement char U+FFFD
                    w = 0xFFFDu32;
                }
                out.append((0xE0u32 | ((w >> 12) & 0x0Fu32)) as char);
                out.append((0x80u32 | ((w >> 6) & 0x3Fu32)) as char);
                out.append((0x80u32 | (w & 0x3Fu32)) as char);
            }
            i = i + 1;
        }
        return out
    }

    // find substring (naive, by u16 code units). returns first index or -1
    func find(&self, sub : u16string) : int {
        const n = size();
        const m = sub.size();
        if(m == 0) { return 0; }
        if(m > n) { return -1; }
        const src = data();
        const pat = sub.data();
        var i : size_t = 0;
        while(i + m <= n) {
            var j : size_t = 0;
            while(j < m && src[i + j] == pat[j]) { j = j + 1; }
            if(j == m) { return i as int; }
            i = i + 1;
        }
        return -1;
    }

    func starts_with(&self, prefix : u16string) : bool {
        const p_len = prefix.size();
        const my_len = size();
        if(p_len > my_len) { return false; }
        const src = data();
        const pre = prefix.data();
        var i : size_t = 0;
        while(i < p_len) {
            if(src[i] != pre[i]) { return false; }
            i = i + 1;
        }
        return true;
    }

    func ends_with(&self, suffix : u16string) : bool {
        const s_len = suffix.size();
        const my_len = size();
        if(s_len > my_len) { return false; }
        const src = data();
        const suf = suffix.data();
        var i : size_t = 0;
        while(i < s_len) {
            if(src[my_len - s_len + i] != suf[i]) { return false; }
            i = i + 1;
        }
        return true;
    }

    // convenience push_back/pop_back
    func push_back(&mut self, u : u16) {
        append_u16_unit(u);
    }

    func pop_back(&mut self) {
        const sz = size();
        if(sz == 0) { return; }
        if(state == '1') {
            storage.sso.length = (sz - 1) as uchar;
            storage.sso.buffer[sz - 1] = 0u16;
        } else if(state == '2') {
            storage.heap.length = sz - 1;
            storage.heap.data[sz - 1] = 0u16;
        } else {
            // constant -> move to buffer then pop
            move_const_to_buffer();
            storage.sso.length = (sz - 1) as uchar;
            storage.sso.buffer[sz - 1] = 0u16;
        }
    }

    func capacity(&mut self) : size_t {
        switch(state) {
            '0' => {
                return storage.constant.length;
            }
            '1' => {
                return U16_STR_BUFF_SIZE as size_t;
            }
            '2' => {
                return storage.heap.capacity;
            }
            default => {
                return 0;
            }
        }
    }

    func data(&self) : *u16 {
        switch(state) {
            '0' => {
                return storage.constant.data
            }
            '1' => {
                return &storage.sso.buffer[0];
            }
            '2' => {
                return storage.heap.data;
            }
            default => {
                return &EMPTY_U16;
            }
        }
    }

    func mutable_data(&mut self) : *mut u16 {
        switch(state) {
            '0' => {
                move_const_to_buffer();
                return &mut storage.sso.buffer[0];
            }
            '1' => {
                return &mut storage.sso.buffer[0];
            }
            '2' => {
                return storage.heap.data;
            }
            default => {
                return null;
            }
        }
    }

    func clear(&mut self) {
        switch(state) {
            '0' => {
                unsafe {
                    storage.constant.data = &EMPTY_U16
                }
                storage.constant.length = 0
            }
            '1' => {
                storage.sso.buffer[0] = 0u16
                storage.sso.length = 0
            }
            '2' => {
                storage.heap.data[0] = 0u16
                storage.heap.length = 0;
            }
            default => {

            }
        }
    }

    @delete
    func delete(&mut self) {
        if(state == '2') {
            dealloc storage.heap.data;
        }
    }

}

}