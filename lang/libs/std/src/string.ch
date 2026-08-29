public namespace std {

public comptime const STR_BUFF_SIZE = 16u;

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

impl Hashable for string {
    func hash(&self) : uint {
        return fnv1a_hash_32(data());
    }
}

impl Eq for string {
    // TODO: separate implementation
    // because we want equals to take views only
    func equals(&self, other : &string) : bool {
        return equals_with_len(other.data(), other.size());
    }
}

// TODO: remove retained, once we have runtime magic val support
@retained
public struct string {

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
        return %runtime_value(constructor2(value, intrinsics::size(value), false))
    }

    @implicit
    @constructor
    comptime func make(expr : %expressive_string) {
        return %runtime_block_value {
            var str = std::string()
            str.append_expr(expr)
            str;
        }
    }

    @constructor
    func constructor(value : *char, length : size_t) {
        var s = string {
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

    @constructor
    func view_make(value : &std::string_view) {
        var s = string {
            storage : {
                constant : {
                    data : value.data(),
                    length : value.size()
                }
            },
            state : '0'
        }
        s.ensure_mut(value.size())
        return s;
    }

    @make
    func view_make2(value : std::string_view) {
        return view_make(&value)
    }

    // the ensure parameter is added just to differentiate signature from constructor above it
    // this allows to keep literal strings as constants
    @constructor
    func constructor2(value : *char, length : size_t, ensure : bool) {
        var s = string {
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
        return string {
            storage : {
                constant : {
                    data : "",
                    length : 0
                }
            },
            state : '0'
        }
    }

    @constructor
    func make_no_len(value : *char) {
        const length = strlen(value)
        var s = string {
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

    @constructor
    func make_with_char(value : char) {
        var s = string {
            storage : {
                sso : {
                    buffer : [],
                    length : 1
                }
            },
            state : '1'
        }
        s.storage.sso.buffer[0] = value;
        s.storage.sso.buffer[1] = '\0';
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

    func resize(&mut self, value : size_t) {
        ensure_mut(value + 1);
        switch(state) {
            '1' => {
                storage.sso.length = value as uchar;
                storage.sso.buffer[value] = '\0';
            }
            '2' => {
                storage.heap.length = value;
                storage.heap.data[value] = '\0';
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

    func equals_view(&self, other : &std::string_view) : bool {
        return equals_with_len(other.data(), other.size())
    }

    func move_const_to_buffer(&mut self) {
        const d = storage.constant.data;
        const length = storage.constant.length;
        unsafe {
            if(d != null) {
                for(var i = 0; i < length; i++) {
                    storage.sso.buffer[i] = d[i];
                }
            }
        }
        storage.sso.buffer[length] = '\0'
        storage.sso.length = length as uchar;
        state = '1'
    }

    func move_data_to_heap(&mut self, from_data : *char, length : size_t, capacity : size_t) {
        // +1 is for safety, we need to add a null terminator (always)
        var d = malloc(capacity + 1) as *mut char
        memcpy(d, from_data, length)
        d[length] = '\0'
        storage.heap.data = d;
        storage.heap.length = length;
        storage.heap.capacity = capacity
        state = '2'
    }

    // this is a private function
    // new_capacity is always > length
    func resize_heap(&mut self, new_capacity : size_t) : bool {
        // +1 for the null terminator
        var d = realloc(storage.heap.data, new_capacity + 1) as *mut char
        if(d == null) {
            panic("couldn't realloc in std::string -> resize_heap");
            return false;
        } else {
            d[storage.heap.length] = '\0'
            storage.heap.data = d;
            storage.heap.capacity = new_capacity
            return true;
        }
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
                var new_cap = if (length < STR_BUFF_SIZE * 2u) STR_BUFF_SIZE * 2u else length * 2u;
                move_data_to_heap(&raw storage.sso.buffer[0], storage.sso.length, new_cap);
            } else if(storage.heap.capacity <= length) {
                var new_cap = if (length < storage.heap.capacity * 2) storage.heap.capacity * 2 else length;
                // ensure at least some growth if capacity was small
                if (new_cap < length + 16) { new_cap = length + 64; }
                resize_heap(new_cap);
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
                    move_data_to_heap(&raw storage.sso.buffer[0], storage.sso.length, new_capacity);
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
            memcpy(&raw mut storage.sso.buffer[offset], value, len)
            storage.sso.buffer[new_size] = '\0'
            storage.sso.length = new_size
        } else {
            // state is '2', it cannot be '0'
            memcpy(&raw mut storage.heap.data[offset], value, len)
            storage.heap.data[new_size] = '\0';
            storage.heap.length = new_size
        }
    }

    comptime func append_expr(&mut self, expr : %expressive_string) {
        return %runtime_value(intrinsics::expr_str_block_value(StringStream { str : self }, expr)) as void
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
            append_view("nan")
            return;
        }

        // handle infinities
        if(dbl_is_inf(value)) {
            if(dbl_is_neg(value)) {
                append_view("-inf")
            } else {
                append_view("inf")
            }
            return;
        }

        var v = value;
        if(v < 0.0) {
            append('-');
            v = -v;
        }

        // Scale and round to `precision` fractional digits up front, so we never
        // have to mutate already-appended characters (which breaks the SSO layout).
        var scale : bigint = 1;
        var pi : int = 0;
        while(pi < precision) {
            scale = scale * 10;
            pi = pi + 1;
        }

        var scaled = (v * (scale as double) + 0.5) as bigint;
        var int_part = scaled / scale;
        var frac_part = scaled - int_part * scale;

        append_integer(int_part);

        if(precision > 0) {
            append('.');
            // Write frac_part zero-padded to exactly `precision` digits.
            var frac_buf : [24]char;
            var fbi : int = 0;
            while(fbi < precision) {
                frac_buf[precision - 1 - fbi] = (('0' as int) + (frac_part % 10) as int) as char;
                frac_part = frac_part / 10;
                fbi = fbi + 1;
            }
            var fj : int = 0;
            while(fj < precision) {
                append(frac_buf[fj]);
                fj = fj + 1;
            }
        }
    }

    func append_float(&mut self, value : float, precision : int) {
        append_double(value as double, precision);
    }

    func copy(&self) : string {
        return substring(0, size())
    }

    func substring(&self, start : size_t, end : size_t) : string {
        var s : string
        unsafe {
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
                move_data_to_heap(&raw storage.sso.buffer[0], length, length * 2);
            } else if(storage.heap.capacity <= length + 2) {
                resize_heap(storage.heap.capacity * 2);
            }
            storage.heap.data[length] = value;
            storage.heap.data[length + 1] = '\0'
            storage.heap.length = length + 1;
        }
    }

    func find(&self, needle : &std::string_view) : size_t {
        var view = std::string_view(data(), size())
        return internal_view_find(&view, needle);
    }

    func find_last(&self, needle : &std::string_view) : size_t {
        var view = std::string_view(data(), size())
        return internal_view_find_last(&view, needle);
    }

    func contains(&self, needle : &std::string_view) : bool {
        return find(needle) != NPOS
    }

    func starts_with(&self, other : &std::string_view) : bool {
        if (other.size() > size()) return false;
        return memcmp(data(), other.data(), other.size()) == 0;
    }

    func trim(&self) : std::string_view {
        return to_view().trim();
    }

    func split(&self, delim : char) : std::vector<std::string_view> {
        return to_view().split(delim);
    }

    func ends_with(&self, other : &std::string_view) : bool {
        // If other_data is longer than data, data cannot end with other_data.
        if (other.size() > size()) return false;
        return memcmp(data() + size() - other.size(), other.data(), other.size()) == 0;
    }

    func erase(&mut self, start : size_t, len : size_t) {
        const sz = size();

        // nothing to erase
        if(start >= sz || len == 0) {
            return;
        }

        // clamp length
        var erase_len = len;
        if(start + erase_len > sz) {
            erase_len = sz - start;
        }

        const tail_start = start + erase_len;
        const tail_len = sz - tail_start;

        // ensure mutability if needed
        // (constant → sso/heap, sso may stay sso)
        ensure_mut(sz + 1);

        if(state == '1') {
            // SSO
            if(tail_len > 0) {
                memmove(
                    &raw mut storage.sso.buffer[start],
                    &raw storage.sso.buffer[tail_start],
                    tail_len
                );
            }
            storage.sso.length = (sz - erase_len) as uchar;
            storage.sso.buffer[storage.sso.length] = '\0';
        } else {
            // heap
            if(tail_len > 0) {
                memmove(
                    &raw mut storage.heap.data[start],
                    &raw storage.heap.data[tail_start],
                    tail_len
                );
            }
            storage.heap.length = sz - erase_len;
            storage.heap.data[storage.heap.length] = '\0';
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
                return &raw storage.sso.buffer[0];
            }
            '2' => {
                return storage.heap.data;
            }
            default => {
                return "";
            }
        }
    }

    func c_str(&self) : *char {
        return data()
    }

    func mutable_data(&mut self) : *mut char {
        switch(state) {
            '0' => {
                if (storage.constant.length < STR_BUFF_SIZE) {
                    move_const_to_buffer();
                    return &raw mut storage.sso.buffer[0];
                } else {
                    move_data_to_heap(storage.constant.data, storage.constant.length, (storage.constant.length * 2));
                    return storage.heap.data;
                }
            }
            '1' => {
                return &raw mut storage.sso.buffer[0];
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

    func to_view(&self) : 'self std::string_view {
        return std::string_view(data(), size())
    }

    func to_i8(&self) : std::Result<i8, std::string_view> { return to_view().to_i8() }
    func to_i16(&self) : std::Result<i16, std::string_view> { return to_view().to_i16() }
    func to_i32(&self) : std::Result<i32, std::string_view> { return to_view().to_i32() }
    func to_i64(&self) : std::Result<i64, std::string_view> { return to_view().to_i64() }
    func to_u8(&self) : std::Result<u8, std::string_view> { return to_view().to_u8() }
    func to_u16(&self) : std::Result<u16, std::string_view> { return to_view().to_u16() }
    func to_u32(&self) : std::Result<u32, std::string_view> { return to_view().to_u32() }
    func to_u64(&self) : std::Result<u64, std::string_view> { return to_view().to_u64() }
    func to_int(&self) : std::Result<int, std::string_view> { return to_view().to_int() }
    func to_uint(&self) : std::Result<uint, std::string_view> { return to_view().to_uint() }
    func to_float(&self) : std::Result<float, std::string_view> {
        var end : *mut char = null
        var res = strtod(data() as *mut char, &raw mut end) as float
        if (end == data()) return std::Result.Err(std::string_view("invalid format"))
        while (end != null && *end != '\0' && isspace(*end as int)) end++
        if (end != null && *end != '\0') return std::Result.Err(std::string_view("trailing characters"))
        return std::Result.Ok(res)
    }
    func to_double(&self) : std::Result<double, std::string_view> {
        var end : *mut char = null
        var res = strtod(data() as *mut char, &raw mut end)
        if (end == data()) return std::Result.Err(std::string_view("invalid format"))
        while (end != null && *end != '\0' && isspace(*end as int)) end++
        if (end != null && *end != '\0') return std::Result.Err(std::string_view("trailing characters"))
        return std::Result.Ok(res)
    }

    // TODO: unstable, no interface involved, signature not stable
    // TODO: no verification of signature
    // TODO: hardcoded type StringStream, generic function support required
    func stream(&self, s : &mut StringStream) {
        s.writeStr(data(), size())
    }

    @delete
    func delete(&mut self) {
        if(state == '2') {
            dealloc storage.heap.data;
        }
    }

}

public struct StringStream {

    var str : &mut std::string

}

impl core::stream::Stream for StringStream {

    func writeChar(&mut self, value : char) {
        str.append(value)
    }

    func writeUChar(&mut self, value : uchar) {
        str.append(value as char)
    }

    func writeSigned(&mut self, value : bigint) {
        str.append_integer(value)
    }

    func writeUnsigned(&mut self, value : ubigint) {
        str.append_uinteger(value)
    }

    func writeStr(&mut self, value : *char, length : ubigint) {
        str.append_with_len(value, length)
    }

    func writeStrNoLen(&mut self, value : *char) {
        str.append_with_len(value, strlen(value))
    }

    func writeFloat(&mut self, value : float) {
        str.append_double(value as double, 3)
    }

    func writeDouble(&mut self, value : double) {
        str.append_double(value as double, 3)
    }

}

}