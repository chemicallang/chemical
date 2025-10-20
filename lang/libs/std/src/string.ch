public namespace std {

public comptime const STR_BUFF_SIZE = 16;

union u64_double_union {
    var u : u64
    var d : double
}

func dbl_bits(x : double) : u64 {
    var u : u64_double_union;
    u.d = x;
    return u.u;
}
func dbl_from_bits(b : u64) : double {
    var u : u64_double_union;
    u.u = b;
    return u.d;
}
comptime const DBL_EXP_MASK = 0x7FF0000000000000u64
comptime const DBL_FRAC_MASK = 0x000FFFFFFFFFFFFFu64
comptime const DBL_SIGN_MASK = 0x8000000000000000u64

func dbl_is_nan(x : double) : bool {
    var b = dbl_bits(x);
    return ((b & DBL_EXP_MASK) == DBL_EXP_MASK) && ((b & DBL_FRAC_MASK) != 0);
}
func dbl_is_inf(x : double) : bool {
    var b = dbl_bits(x);
    return ((b & DBL_EXP_MASK) == DBL_EXP_MASK) && ((b & DBL_FRAC_MASK) == 0);
}
func dbl_is_neg(x : double) : bool {
    return ((dbl_bits(x) & DBL_SIGN_MASK) != 0);
}

public struct string : Hashable, Eq {

    union {
        struct {
            var data : *char;
            var length : size_t
        } constant;
        struct {
            var data : *mut char;
            var length : size_t;
            var capacity : size_t;
        } heap;
        struct {
            var buffer : [STR_BUFF_SIZE]char;
            var length : uchar;
        } sso;
    } storage;
    var state : char

    @constructor
    comptime func make(value : %literal_string) {
        return intrinsics::wrap(constructor2(value, intrinsics::size(value), false))
    }

    @constructor
    func constructor(value : *char, length : size_t) {
        storage.constant.data = value;
        storage.constant.length = length;
        state = '0'
        ensure_mut(size())
    }

    // the ensure parameter is added just to differentiate signature from constructor above it
    // this allows to keep literal strings as constants
    @constructor
    func constructor2(value : *char, length : size_t, ensure : bool) {
        storage.constant.data = value;
        storage.constant.length = length;
        state = '0'
        if(ensure) {
            // this branch probably will never be taken
            ensure_mut(size())
        }
    }

    @constructor
    func empty_str() {
        storage.constant.data = "";
        storage.constant.length = 0;
        state = '0'
    }

    @constructor
    func make_no_len(value : *char) {
        const length = strlen(value)
        storage.constant.data = value;
        storage.constant.length = length;
        state = '0'
        ensure_mut(length)
    }

    @constructor
    func make_with_char(value : char) {
        storage.sso.buffer[0] = value;
        storage.sso.buffer[1] = '\0';
        storage.sso.length = 1;
        state = '1'
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

    func resize_unsafe(&self, value : size_t) {
        switch(state) {
            '0' => {
                storage.constant.length = value;
            }
            '1' => {
                storage.sso.length = value;
            }
            '2' => {
                storage.heap.length = value;
            }
            default => {
                return
            }
        }
    }

    func empty(&self) : bool {
        return size() == 0
    }

    func equals_with_len(&self, d : *char, l : size_t) : bool {
        const self_size = size();
        return self_size == l && strncmp(data(), d, self_size) == 0;
    }

    @override
    func equals(&self, other : &string) : bool {
        return equals_with_len(other.data(), other.size());
    }

    func equals_view(&self, other : &std::string_view) : bool {
        return equals_with_len(other.data(), other.size())
    }

    func move_const_to_buffer(&mut self) {
        const data = storage.constant.data;
        const length = storage.constant.length;
        unsafe {
            if(data != null) {
                for(var i = 0; i < length; i++) {
                    storage.sso.buffer[i] = data[i];
                }
            }
        }
        storage.sso.buffer[length] = '\0'
        storage.sso.length = length as uchar;
        state = '1'
    }

    func move_data_to_heap(&mut self, from_data : *char, length : size_t, capacity : size_t) {
        // +1 is for safety, we need to add a null terminator (always)
        var data = malloc(capacity + 1) as *mut char
        memcpy(data, from_data, length)
        data[length] = '\0'
        storage.heap.data = data;
        storage.heap.length = length;
        storage.heap.capacity = capacity
        state = '2'
    }

    // this is a private function
    // new_capacity is always > length
    func resize_heap(&mut self, new_capacity : size_t) {
        // +1 for the null terminator
        var data = realloc(storage.heap.data, new_capacity + 1) as *mut char
        data[storage.heap.length] = '\0'
        storage.heap.data = data;
        storage.heap.capacity = new_capacity
    }

    // ensures that capacity is larger than length given and memory is mutable
    func ensure_mut(&mut self, length : size_t) {
        if((state == '0' || state == '1') && length < STR_BUFF_SIZE) {
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
                if(new_capacity < STR_BUFF_SIZE && storage.constant.length < STR_BUFF_SIZE) {
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
                if(new_capacity >= STR_BUFF_SIZE) {
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

    func set(&mut self, index : size_t, value : char) {
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

    func get(&self, index : size_t) : char {
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
                return '\0'
            }
        }
    }

    func append_with_len(&mut self, value : *char, len : size_t) {
        // offset, where we start writing
        const offset = size()
        // the size of the string, after we've written
        const new_size = offset + len
        // +1 for null terminator
        ensure_mut(new_size + 1);
        if(state == '1') {
            memcpy(&mut storage.sso.buffer[offset], value, len)
            storage.sso.buffer[new_size] = '\0'
            storage.sso.length = new_size
        } else {
            // state is '2', it cannot be '0'
            memcpy(&mut storage.heap.data[offset], value, len)
            storage.heap.data[new_size] = '\0';
            storage.heap.length = new_size
        }
    }

    comptime func append_expr(&self, expr : %expressive_string) {
        return intrinsics::wrap(intrinsics::expr_str_block_value(StringStream { str : self }, expr)) as void
    }

    func append_char_ptr(&mut self, value : *char) {
        append_with_len(value, strlen(value));
    }

    func append_string(&mut self, value : &string) {
        append_with_len(value.data(), value.size())
    }

    func append_str(&mut self, value : *string) {
        append_with_len(value.data(), value.size())
    }

    func append_view(&mut self, value : &std::string_view) {
        append_with_len(value.data(), value.size())
    }

    // Append an *unsigned* 64-bit integer quickly.
    func append_uinteger(&mut self, value : ubigint) {
        // fast path for zero
        if(value == 0) {
            append('0');
            return;
        }

        // temporary buffer for digits (max 20 digits for u64)
        var buf : [20]char;
        var bi : int = 0;
        while(value != 0) {
            const digit = (value % 10) as uint;
            buf[bi] = ('0' as int + digit as int) as char;
            bi = bi + 1;
            value = value / 10;
        }

        const old_len = size();
        const add = bi as size_t;
        ensure_mut(old_len + add + 1);
        var p = mutable_data();

        var i : size_t = 0;
        while(i < add) {
            // write reversed from buf
            p[old_len + i] = buf[add - 1 - i];
            i = i + 1;
        }
        p[old_len + add] = '\0';

        if(state == '1') {
            storage.sso.length = (old_len + add) as uchar;
        } else {
            storage.heap.length = old_len + add;
        }
    }

    // Append a *signed* 64-bit integer quickly. Handles INT64_MIN safely.
    func append_integer(&mut self, value : bigint) {
        if(value < 0) {
            append('-');
            // handle INT64_MIN safely by using the trick: -(value + 1) then +1
            var tmp = value + 1; // still negative or zero
            var uv = (0 as ubigint);
            if(tmp < 0) {
                uv = (-(tmp)) as ubigint;
                uv = uv + 1;
            } else {
                // value was -1 -> tmp == 0 -> uv = 1
                uv = 1;
            }
            append_uinteger(uv);
        } else {
            append_uinteger(value as ubigint);
        }
    }

    // Simple, fast (but not fully IEEE-perfect) conversion that supports a
    // configurable precision. Uses integer rounding of fractional part.
    func append_double(&mut self, value : double, precision : int) {
        // clamp precision to reasonable bounds
        if(precision < 0) {
            precision = 6;
        } else if(precision > 18) {
            precision = 18;
        }

        // handle NaN
        if(dbl_is_nan(value)) {
            append_view(std::string_view("nan"))
            return;
        }

        // handle infinities
        if(dbl_is_inf(value)) {
            if(dbl_is_neg(value)) {
                append_view(std::string_view("-inf"))
            } else {
                append_view(std::string_view("inf"))
            }
            return;
        }

        var v = value;
        if(v < 0.0) {
            append('-');
            v = -v;
        }

        // integer part
        var int_part = (v as bigint);
        append_integer(int_part);

        if(precision == 0) {
            return;
        }

        append('.');

        // fractional part scaled and rounded
        var frac = v - (int_part as double);
        var pow10_d : double = 1.0;
        var pow10_u : ubigint = 1;
        var pi : int = 0;
        while(pi < precision) {
            pow10_d = pow10_d * 10.0;
            pow10_u = pow10_u * 10;
            pi = pi + 1;
        }

        // add rounding
        var scaled = (frac * pow10_d + 0.5) as ubigint;

        // rounding may carry into integer part
        if(scaled >= pow10_u) {
            // increment integer part by 1
            // remove previously appended integer and re-append incremented value
            // simplest approach: compute new integer and replace tail.
            // We'll compute current total length and roll back integer characters.

            // find length of integer we appended: convert int_part+1 to string in temp
            var new_int = int_part + 1;
            // remove the integer we appended before the '.'
            // compute pos where '.' is: size() currently = previous size
            var dot_pos = size();
            // backtrack: find start index of integer by scanning backwards until non-digit or start
            var si : size_t = 0;
            if(state == '1') {
                si = 0;
                // sso case: scan
                var idx = dot_pos;
                while(idx > 0 && get(idx - 1) >= '0' && get(idx - 1) <= '9') {
                    idx = idx - 1;
                }
                si = idx;
            } else {
                si = 0;
                var idx2 = dot_pos;
                while(idx2 > 0 && get(idx2 - 1) >= '0' && get(idx2 - 1) <= '9') {
                    idx2 = idx2 - 1;
                }
                si = idx2;
            }
            // truncate back to si
            if(state == '1') {
                storage.sso.buffer[si] = '\0';
                storage.sso.length = si as uchar;
            } else {
                storage.heap.data[si] = '\0';
                storage.heap.length = si;
            }
            // append new integer
            append_integer(new_int);
            // append '.' again (we removed it too)
            append('.');
            scaled = 0; // fractional part became zero after carry
        }

        // write fractional part with zero-padding to precision
        // scaled is in [0, pow10_u)
        // we need to write exactly 'precision' digits (with leading zeros)
        // convert scaled to string into temporary buffer
        var frac_buf : [20]char;
        var fbi : int = 0;
        if(scaled == 0) {
            // write zeros
            var z = 0;
            while(z < precision) {
                append('0');
                z = z + 1;
            }
            return;
        }
        var tmp = scaled;
        while(tmp != 0) {
            const d = (tmp % 10) as ubigint;
            frac_buf[fbi] = ('0' as int + d as int) as char;
            fbi = fbi + 1;
            tmp = tmp / 10;
        }
        // number of leading zeros needed
        var leading = precision - fbi;
        var zi = 0;
        while(zi < leading) {
            append('0');
            zi = zi + 1;
        }
        // append reversed frac_buf
        var fj = 0;
        while(fj < fbi) {
            append(frac_buf[fbi - 1 - fj]);
            fj = fj + 1;
        }
    }

    func append_float(&mut self, value : float, precision : int) {
        append_double(value as double, precision);
    }

    func copy(&mut self) : string {
        return substring(0, size())
    }

    func substring(&mut self, start : size_t, end : size_t) : string {
        var s : string
        const actual_len : size_t = end - start;
        if(actual_len < STR_BUFF_SIZE) {
            s.state = '1'
            s.storage.sso.length = actual_len as uchar
            const d = data()
            for(var i = 0; i < actual_len; i++) {
                s.storage.sso.buffer[i] = d[start + i]
            }
            s.storage.sso.buffer[actual_len] = '\0'
        } else {
            s.state = '2'
            const new_cap = actual_len * 2
            var new_heap = malloc(new_cap) as *mut char
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

    func append(&mut self, value : char) {
        const length = size();
        if((state == '0' || state == '1') && length < (STR_BUFF_SIZE - 1)) {
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
            } else if(storage.heap.capacity <= length + 2) {
                resize_heap(storage.heap.capacity * 2);
            }
            storage.heap.data[length] = value;
            storage.heap.data[length + 1] = '\0'
            storage.heap.length = length + 1;
        }
    }

    func capacity(&mut self) : size_t {
        switch(state) {
            '0' => {
                return storage.constant.length;
            }
            '1' => {
                return STR_BUFF_SIZE as size_t;
            }
            '2' => {
                return storage.heap.capacity;
            }
            default => {
                return 0;
            }
        }
    }

    func data(&self) : *char {
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
                return "";
            }
        }
    }

    func mutable_data(&self) : *mut char {
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
                    storage.constant.data = ""
                }
                storage.constant.length = 0
            }
            '1' => {
                storage.sso.buffer[0] = '\0'
                storage.sso.length = 0
            }
            '2' => {
                storage.heap.data[0] = '\0'
                storage.heap.length = 0;
            }
            default => {

            }
        }
    }

    @override
    func hash(&self) : uint {
        return fnv1a_hash_32(data());
    }

    func to_view(&self) : std::string_view {
        return std::string_view(data(), size())
    }

    @delete
    func delete(&mut self) {
        if(state == '2') {
            dealloc storage.heap.data;
        }
    }

}

struct StringStream : Stream {

    var str : &std::string

    @override
    func writeI8(&self, value : i8) {
        str.append(value)
    }

    @override
    func writeI16(&self, value : i16) {
        str.append_integer(value)
    }

    @override
    func writeI32(&self, value : i32) {
        str.append_integer(value)
    }

    @override
    func writeI64(&self, value : i64) {
        str.append_integer(value)
    }

    @override
    func writeU8(&self, value : u8) {
        str.append(value as char)
    }

    @override
    func writeU16(&self, value : u16) {
        str.append_uinteger(value)
    }

    @override
    func writeU32(&self, value : u32) {
        str.append_uinteger(value)
    }

    @override
    func writeU64(&self, value : u64) {
        str.append_uinteger(value)
    }

    @override
    func writeStr(&self, value : *char, length : ubigint) {
        str.append_with_len(value, length)
    }

    @override
    func writeStrNoLen(&self, value : *char) {
        str.append_with_len(value, strlen(value))
    }

    @override
    func writeChar(&self, value : char) {
        str.append(value)
    }

    @override
    func writeUChar(&self, value : uchar) {
        str.append(value as char)
    }

    @override
    func writeShort(&self, value : short) {
        str.append_integer(value)
    }

    @override
    func writeUShort(&self, value : ushort) {
        str.append_uinteger(value)
    }

    @override
    func writeInt(&self, value : int) {
        str.append_integer(value)
    }

    @override
    func writeUInt(&self, value : uint) {
        str.append_uinteger(value)
    }

    @override
    func writeLong(&self, value : long) {
        str.append_integer(value)
    }

    @override
    func writeULong(&self, value : ulong) {
        str.append_uinteger(value)
    }

    @override
    func writeLongLong(&self, value : longlong) {
        str.append_integer(value)
    }

    @override
    func writeULongLong(&self, value : ulonglong) {
        str.append_uinteger(value)
    }

    @override
    func writeFloat(&self, value : float) {
        str.append_double(value as double, 3)
    }

    @override
    func writeDouble(&self, value : double) {
        str.append_double(value as double, 3)
    }

}

}