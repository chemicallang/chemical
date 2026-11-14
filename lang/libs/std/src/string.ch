public namespace std {

public comptime const STR_BUFF_SIZE = 16u;

union u64_double_union {
    var u : u64
    var d : double
}

func dbl_bits(x : double) : u64 { unsafe {
    var u : u64_double_union;
    u.d = x;
    return u.u;
}}
func dbl_from_bits(b : u64) : double { unsafe {
    var u : u64_double_union;
    u.u = b;
    return u.d;
}}
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

    // The tagged storage is the string invariant. Only string's own methods
    // may choose a representation or mutate the tag.
    private union {
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
    private var state : char

    @constructor
    comptime func make(value : %literal_string) {
        return unsafe(constructor2(value, intrinsics::size(value), false))
    }

    @implicit
    @constructor
    @expand
    comptime func make(expr : %expressive_string) {
        var str = std::string()
        str.append_expr(expr)
        return str
    }

    @constructor
    unsafe func constructor(value : *char, length : size_t) {
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
    unsafe func constructor2(value : *char, length : size_t, ensure : bool) {
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
    unsafe func make_no_len(value : *char) {
        const length = unsafe(strlen(value))
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
        unsafe {
            if(value >= ((0 as size_t) - 1)) {
                panic("string length overflow")
            }
            const old_size = size()
            ensure_mut(value + 1);
            if(value > old_size) {
                var d = data()
                unsafe {
                    memset(&raw mut d[old_size] as *mut void, 0, value - old_size)
                }
            }
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
    }

    func empty(&self) : bool {
        return size() == 0
    }

    func equals_with_len(&self, d : *char, l : size_t) : bool {
        const self_size = size();
        return self_size == l && unsafe(strncmp(data(), d, self_size)) == 0;
    }

    func equals_view(&self, other : &std::string_view) : bool {
        return equals_with_len(other.data(), other.size())
    }

    func move_const_to_buffer(&mut self) {
        const d = storage.constant.data;
        const length = storage.constant.length;
        // The inline buffer has room for the bytes plus the terminator.
        if(length >= STR_BUFF_SIZE) {
            panic("string constant does not fit the inline buffer")
        }
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
        unsafe {
            if(capacity >= ((0 as size_t) - 1) || length > capacity) {
                panic("string capacity overflow")
            }
            // +1 is for safety, we need to add a null terminator (always)
            var d = unsafe(malloc(capacity + 1)) as *mut char
            if(d == null) {
                panic("string allocation failed")
            }
            unsafe { memcpy(d, from_data, length) }
            d[length] = '\0'
            storage.heap.data = d;
            storage.heap.length = length;
            storage.heap.capacity = capacity
            state = '2'
    
        }
    }

    // this is a private function
    // new_capacity is always > length
    func resize_heap(&mut self, new_capacity : size_t) : bool {
        unsafe {
            if(new_capacity >= ((0 as size_t) - 1)) {
                panic("string capacity overflow")
            }
            // +1 for the null terminator
            var d = unsafe(realloc(storage.heap.data, new_capacity + 1)) as *mut char
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
    }

    // ensures that capacity is larger than length given and memory is mutable
    func ensure_mut(&mut self, length : size_t) {
        if(length > ((0 as size_t) - 1) / 2) {
            panic("string capacity overflow")
        }
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
        unsafe {
            if(index >= size()) {
                panic("string index out of bounds")
            }
            switch(state) {
                '0' => {
                    // A constant may be longer than the inline buffer; let
                    // ensure_mut choose SSO or heap storage before writing.
                    ensure_mut(size());
                    if(state == '1') {
                        storage.sso.buffer[index] = value;
                    } else {
                        storage.heap.data[index] = value;
                    }
                }
                '1' => {
                    storage.sso.buffer[index] = value;
                }
                '2' => {
                    storage.heap.data[index] = value;
                }
            }
    
        }
    }

    func get(&self, index : size_t) : Option<char> {
        if(index >= size()) { return Option.None<char>() }
        return Option.Some<char>(at(index))
    }

    func at(&self, index : size_t) : char {
        unsafe {
            if(index >= size()) {
                panic("string index out of bounds")
            }
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
    }

    func append_with_len(&mut self, value : *char, len : size_t) {
        unsafe {
            if(len == 0) { return }
            if(value == null) { panic("string append from a null pointer") }
            // offset, where we start writing
            const offset = size()
            const max_size = (0 as size_t) - 1
            if(offset >= max_size || len > max_size - offset - 1) {
                panic("string length overflow")
            }
            // the size of the string, after we've written
            const new_size = offset + len
            // If the source aliases this string's own storage, growing may move
            // that storage. Remember the source offset and recompute it afterwards.
            const old_data = data()
            const old_begin = old_data as ubigint
            const old_end = old_begin + offset
            const src_addr = value as ubigint
            var src_offset : size_t = 0
            var self_source = false
            if(src_addr >= old_begin && src_addr < old_end) {
                self_source = true
                src_offset = (src_addr - old_begin) as size_t
            }
            // +1 for null terminator
            ensure_mut(new_size + 1);
            var source = value
            if(self_source) {
                source = data() + src_offset
            }
            if(state == '1') {
                unsafe { memcpy(&raw mut storage.sso.buffer[offset], source, len) }
                storage.sso.buffer[new_size] = '\0'
                storage.sso.length = new_size
            } else {
                // state is '2', it cannot be '0'
                unsafe { memcpy(&raw mut storage.heap.data[offset], source, len) }
                storage.heap.data[new_size] = '\0';
                storage.heap.length = new_size
            }
    
        }
    }

    @expand
    comptime func append_expr(&mut self, expr : %expressive_string) {
        intrinsics::expr_str_block_value(StringStream { str : self }, expr)
    }

    func append_char_ptr(&mut self, value : *char) {
        if(value == null) { panic("string append from a null pointer") }
        append_with_len(value, unsafe(strlen(value)));
    }

    func append_string(&mut self, value : &string) {
        append_with_len(value.data(), value.size())
    }

    func append_view(&mut self, value : &std::string_view) {
        append_with_len(value.data(), value.size())
    }

    // Append an *unsigned* 64-bit integer quickly.
    func append_uinteger(&mut self, value : ubigint) {
        unsafe {
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

    // Render a non-negative finite double with `frac_digits` fraction digits.
    // With `trim`, trailing zeros (and a trailing '.') are dropped, so `1.500`
    // becomes `1.5` and `1.000` becomes `1`. `v` must be below 1e15 so the
    // integer part fits a 64-bit integer; the caller picks the form.
    func append_double_fixed(&mut self, v : double, frac_digits : int, trim : bool) {
        var scale : bigint = 1
        var i : int = 0
        while(i < frac_digits) {
            scale = scale * 10
            i = i + 1
        }
        var int_part = (v as bigint)
        const frac = v - (int_part as double)
        var scaled = (frac * (scale as double) + 0.5) as bigint
        if(scaled >= scale) {
            // rounding carried into the integer part
            int_part = int_part + 1
            scaled = 0
        }
        append_integer(int_part)
        if(scaled == 0) {
            if(!trim) {
                append('.')
                var z : int = 0
                while(z < frac_digits) {
                    append('0')
                    z = z + 1
                }
            }
            return
        }
        if(trim) {
            while(frac_digits > 0 && scaled % 10 == 0) {
                scaled = scaled / 10
                frac_digits = frac_digits - 1
            }
        }
        append('.')
        var buf : [24]char
        var bi : int = frac_digits
        while(bi > 0) {
            bi = bi - 1
            buf[bi] = (('0' as int) + (scaled % 10) as int) as char
            scaled = scaled / 10
        }
        var k : int = 0
        while(k < frac_digits) {
            append(buf[k])
            k = k + 1
        }
    }

    // The decimal exponent of a positive finite double: `v = m * 10^e` with
    // `m` in [1, 10).
    internal func decimal_exponent(v : double) : int {
        var e : int = 0
        var m = v
        while(m >= 10.0) {
            m = m / 10.0
            e = e + 1
        }
        while(m < 1.0) {
            m = m * 10.0
            e = e - 1
        }
        return e
    }

    // `mantissa * 10^exponent` with `sig` significant digits, trailing zeros
    // trimmed: `1.5e30`, `4.94065645841247e-324`.
    func append_double_scientific(&mut self, mantissa : double, exponent : int, sig : int) {
        // the scale keeps `sig` significant digits in an integer
        var scale : bigint = 1
        var i = 1
        while(i < sig) {
            scale = scale * 10
            i = i + 1
        }
        var scaled = (mantissa * (scale as double) + 0.5) as bigint
        const carry = scale * 10
        if(scaled >= carry) {
            // rounding carried into the next power of ten
            scaled = scaled / 10
            exponent = exponent + 1
        }
        append_integer(scaled / scale)
        var rest = scaled % scale
        if(rest != 0) {
            append('.')
            var buf : [20]char
            var frac_digits = sig - 1
            var bi : int = frac_digits
            while(bi > 0) {
                bi = bi - 1
                buf[bi] = (('0' as int) + (rest % 10) as int) as char
                rest = rest / 10
            }
            var len : int = frac_digits
            while(len > 0 && buf[len - 1] == '0') {
                len = len - 1
            }
            var k : int = 0
            while(k < len) {
                append(buf[k])
                k = k + 1
            }
        }
        append('e')
        if(exponent < 0) {
            append('-')
            append_integer((-exponent) as bigint)
        } else {
            append_integer(exponent as bigint)
        }
    }

    // `10^k` as a double, for the exponent range a double can hold.
    internal func pow10_double(k : int) : double {
        var p : double = 1.0
        var i : int = 0
        while(i < k) {
            p = p * 10.0
            i = i + 1
        }
        return p
    }

    // Round `v` to `sig` significant decimal digits, given its decimal
    // exponent `e`.
    internal func round_significant(v : double, sig : int, e : int) : double {
        const k = sig - 1 - e
        if(k >= 0) {
            const p = pow10_double(k)
            return (((v * p + 0.5) as bigint) as double) / p
        } else {
            const p = pow10_double(-k)
            return ((((v / p + 0.5) as bigint) as double)) * p
        }
    }

    // The fewest significant digits that reproduce `v` exactly (as a `float`
    // when `as_float`, so `3.14f` renders as `3.14` rather than as the widened
    // double's digits). Never more than 15 for a double, 9 for a float.
    internal func shortest_significant(v : double, e : int, as_float : bool) : int {
        var max_sig : int = 15
        if(as_float) {
            max_sig = 9
        }
        var sig : int = 1
        while(sig < max_sig) {
            const m = round_significant(v, sig, e)
            if(as_float) {
                if((m as float) == (v as float)) {
                    return sig
                }
            } else if(m == v) {
                return sig
            }
            sig = sig + 1
        }
        return max_sig
    }

    /// Format `value` as a `double`.
    ///
    /// `precision >= 0` prints exactly that many fraction digits. `precision <
    /// 0` prints the shortest form that reproduces the value exactly (at most
    /// fifteen significant digits); scientific notation is used only outside
    /// `[1e-4, 1e15)`. `nan`, `inf` and `-inf` render as such, and a negative
    /// value keeps its sign (including `-0.0`).
    func append_double(&mut self, value : double, precision : int) {
        append_double_impl(value, precision, false)
    }

    func append_double_impl(&mut self, value : double, precision : int, as_float : bool) {
        if(dbl_is_nan(value)) {
            append_view("nan")
            return
        }

        if(dbl_is_inf(value)) {
            if(dbl_is_neg(value)) {
                append_view("-inf")
            } else {
                append_view("inf")
            }
            return
        }

        if(precision > 18) {
            precision = 18
        }

        var v = value
        if(dbl_is_neg(v)) {
            append('-')
            v = -v
        }

        if(v == 0.0) {
            append('0')
            var z : int = 0
            while(z < precision) {
                if(z == 0) {
                    append('.')
                }
                append('0')
                z = z + 1
            }
            return
        }

        if(precision >= 0) {
            append_double_fixed(v, precision, false)
            return
        }

        const e = decimal_exponent(v)
        const sig = shortest_significant(v, e, as_float)
        if(e >= 15 || e < -4) {
            // one division/multiplication keeps the mantissa as close to exact
            // as the double representation allows
            var divisor : double = 1.0
            if(e > 0) {
                var i : int = 0
                while(i < e) {
                    divisor = divisor * 10.0
                    i = i + 1
                }
                append_double_scientific(v / divisor, e, sig)
            } else {
                var j : int = 0
                while(j < -e) {
                    divisor = divisor * 10.0
                    j = j + 1
                }
                append_double_scientific(v * divisor, e, sig)
            }
        } else {
            // `sig` significant digits, minus the digits before the point
            append_double_fixed(v, sig - 1 - e, true)
        }
    }

    func append_float(&mut self, value : float, precision : int) {
        append_double_impl(value as double, precision, true)
    }

    func copy(&self) : string {
        return substring(0, size())
    }

    func substring(&self, start : size_t, end : size_t) : string {
        if(start > end || end > size()) {
            panic("string substring out of bounds")
        }
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
                if(actual_len > ((0 as size_t) - 1) / 2) {
                    panic("string capacity overflow")
                }
                const new_cap = actual_len * 2
                var new_heap = malloc(new_cap) as *mut char
                if(new_heap == null) {
                    panic("string allocation failed")
                }
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
        unsafe {
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
    }

    func find_or_npos(&self, needle : &std::string_view) : size_t {
        var view = unsafe(std::string_view(data(), size()))
        return internal_view_find(&view, needle)
    }

    func find_last_or_npos(&self, needle : &std::string_view) : size_t {
        var view = unsafe(std::string_view(data(), size()))
        return internal_view_find_last(&view, needle)
    }

    func find(&self, needle : &std::string_view) : Option<size_t> {
        var view = unsafe(std::string_view(data(), size()))
        var pos = internal_view_find(&view, needle)
        if(pos == NPOS) { return Option.None<size_t>() }
        return Option.Some<size_t>(pos)
    }

    func find_last(&self, needle : &std::string_view) : Option<size_t> {
        var view = unsafe(std::string_view(data(), size()))
        var pos = internal_view_find_last(&view, needle)
        if(pos == NPOS) { return Option.None<size_t>() }
        return Option.Some<size_t>(pos)
    }

    func contains(&self, needle : &std::string_view) : bool {
        return find(needle) is Option.Some
    }

    func starts_with(&self, other : &std::string_view) : bool {
        if (other.size() > size()) return false;
        return unsafe(memcmp(data(), other.data(), other.size())) == 0;
    }

    func trim(&self) : 'self std::string_view {
        return to_view().trim();
    }

    // Safe splitting returns owned pieces. `split_views` is the explicit
    // unsafe zero-copy form: the caller must keep this string alive for every
    // returned view.
    func split(&self, delim : char) : std::vector<std::string> {
        return to_view().split(delim);
    }

    unsafe func split_views(&self, delim : char) : std::vector<std::string_view> {
        return unsafe(to_view().split_views(delim));
    }

    func ends_with(&self, other : &std::string_view) : bool {
        // If other_data is longer than data, data cannot end with other_data.
        if (other.size() > size()) return false;
        return unsafe(memcmp(data() + size() - other.size(), other.data(), other.size())) == 0;
    }

    func erase(&mut self, start : size_t, len : size_t) {
        unsafe {
            const sz = size();

            // nothing to erase
            if(start >= sz || len == 0) {
                return;
            }

            // Clamp with subtraction: `start + len` can wrap for a huge `len` and
            // would then move/erase far past the end of the string.
            var erase_len = len;
            if(erase_len > sz - start) {
                erase_len = sz - start;
            }

            const tail_start = start + erase_len;
            const tail_len = sz - tail_start;

            // ensure mutability if needed
            // (constant → sso/heap, sso may stay sso). The string already stores a
            // NUL, so its capacity is at least `sz + 1`; asking for `sz` also avoids
            // wrapping `sz + 1` for a maximum-sized string.
            ensure_mut(sz);

            if(state == '1') {
                // SSO
                if(tail_len > 0) {
                    unsafe { memmove(
                        &raw mut storage.sso.buffer[start],
                        &raw storage.sso.buffer[tail_start],
                        tail_len
                    ) };
                }
                storage.sso.length = (sz - erase_len) as uchar;
                storage.sso.buffer[storage.sso.length] = '\0';
            } else {
                // heap
                if(tail_len > 0) {
                    unsafe { memmove(
                        &raw mut storage.heap.data[start],
                        &raw storage.heap.data[tail_start],
                        tail_len
                    ) };
                }
                storage.heap.length = sz - erase_len;
                storage.heap.data[storage.heap.length] = '\0';
            }
    
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
        unsafe {
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
    }

    func to_view(&self) : 'self std::string_view {
        return unsafe(std::string_view(data(), size()))
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
    func to_float(&self) : std::Result<float, std::string_view> { unsafe {
        var end : *mut char = null
        var res = strtod(data() as *mut char, &raw mut end) as float
        if (end == data()) return std::Result.Err(std::string_view("invalid format"))
        while (end != null && *end != '\0' && isspace(*end as int)) end++
        if (end != null && *end != '\0') return std::Result.Err(std::string_view("trailing characters"))
        return std::Result.Ok(res)
     }}
    func to_double(&self) : std::Result<double, std::string_view> { unsafe {
        var end : *mut char = null
        var res = strtod(data() as *mut char, &raw mut end)
        if (end == data()) return std::Result.Err(std::string_view("invalid format"))
        while (end != null && *end != '\0' && isspace(*end as int)) end++
        if (end != null && *end != '\0') return std::Result.Err(std::string_view("trailing characters"))
        return std::Result.Ok(res)
     }}

    // TODO: unstable, no interface involved, signature not stable
    // TODO: no verification of signature
    // TODO: hardcoded type StringStream, generic function support required
    func stream(&self, s : &mut StringStream) {
        unsafe { s.writeStr(data(), size()) }
    }

    @delete
    func delete(&mut self) {
        unsafe {
            if(state == '2') {
                dealloc storage.heap.data;
            }
    
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

    func writeBool(&mut self, value : bool) {
        if(value) {
            str.append_view("true")
        } else {
            str.append_view("false")
        }
    }

    unsafe func writeStr(&mut self, value : *char, length : ubigint) {
        str.append_with_len(value, length)
    }

    func writeFloat(&mut self, value : float) {
        str.append_float(value, -1)
    }

    func writeDouble(&mut self, value : double) {
        str.append_double(value, -1)
    }

}



// Owned byte/UTF-16 storage: moving the value is safe, and shared reads do
// not mutate the storage. // SAFETY: every &self method above is read-only.
unsafe impl Send for string { }
unsafe impl Sync for string { }
}