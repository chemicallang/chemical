// Convenience: encode a JsonValue to a string via append_value
public func encode_json(value : &JsonValue) : std::string {
    var buffer = std::string()
    var builder = JsonStringBuilder { ptr : &mut buffer }
    builder.append_value(value)
    return buffer
}

// ===== Encoders =====

public struct JsonEncoder {
    var buffer : *mut std::string
    var counts : *mut std::vector<u64>
}

public struct JsonArrayEncoder {
    var buffer : *mut std::string
    var counts : *mut std::vector<u64>

    @delete
    func destruct(&self) {
        self.buffer.append(']')
        self.counts.take_last()
    }
}

public struct JsonObjectEncoder {
    var buffer : *mut std::string
    var counts : *mut std::vector<u64>

    @delete
    func destruct(&self) {
        self.buffer.append('}')
        self.counts.take_last()
    }
}

public struct JsonMapEncoder {
    var buffer : *mut std::string
    var counts : *mut std::vector<u64>

    @delete
    func destruct(&self) {
        self.buffer.append('}')
        self.counts.take_last()
    }
}



// ===== Helpers =====

public func json_escape_into(output : &mut std::string, str : *char, len : u64) {
    output.append('"')
    for(var i : u64 = 0; i < len; i++) {
        var c = str[i]
        switch(c) {
            '"' => output.append_char_ptr("\\\"")
            '\\' => output.append_char_ptr("\\\\")
            '\b' => output.append_char_ptr("\\b")
            '\f' => output.append_char_ptr("\\f")
            '\n' => output.append_char_ptr("\\n")
            '\r' => output.append_char_ptr("\\r")
            '\t' => output.append_char_ptr("\\t")
            default => {
                if(c as u8 <= 0x1Fu) {
                    output.append_char_ptr("\\u00")
                    var hex = "0123456789abcdef"
                    output.append(hex[(c >> 4) & 0xF])
                    output.append(hex[c & 0xF])
                } else {
                    output.append(c)
                }
            }
        }
    }
    output.append('"')
}

public func write_i64(output : &mut std::string, val : i64) {
    output.append_integer(val)
}

public func write_u64(output : &mut std::string, val : u64) {
    output.append_uinteger(val)
}

public func write_double_raw(output : &mut std::string, val : double) {
    var v = val
    if(v < 0.0) {
        output.append('-')
        v = -v
    }
    if(v == 0.0) {
        output.append('0')
        return
    }
    var int_part = v as i64
    write_i64(output, int_part)
    var frac = v - (int_part as double)
    if(frac > 0.0) {
        output.append('.')
        var remaining = frac
        for(var i = 0; i < 12; i++) {
            if(remaining <= 0.0) break
            remaining *= 10.0
            var digit = remaining as i64
            output.append('0' + digit as char)
            remaining -= digit as double
        }
    }
}

public func write_double(output : &mut std::string, val : double) {
    if(val != val) {
        output.append_char_ptr("null")
        return
    }
    if(val == 0.0) {
        output.append('0')
        return
    }
    write_double_raw(output, val)
}

public func parse_i64(s : *char) : i64 {
    var i : int = 0
    var negative = false
    if(s[0] == '-') { negative = true; i = 1 }
    var result : i64 = 0
    while(s[i] != '\0' && s[i] >= '0' && s[i] <= '9') {
        result = result * 10i64 + (s[i] - '0') as i64
        i++
    }
    if(negative) result = -result
    return result
}

public func parse_double(s : *char) : double {
    var i : int = 0
    var negative = false
    if(s[0] == '-') { negative = true; i = 1 }
    var result : double = 0.0
    while(s[i] >= '0' && s[i] <= '9') {
        result = result * 10.0 + (s[i] - '0') as double
        i++
    }
    if(s[i] == '.') {
        i++
        var divisor : double = 10.0
        while(s[i] >= '0' && s[i] <= '9') {
            result += (s[i] - '0') as double / divisor
            divisor *= 10.0
            i++
        }
    }
    if(s[i] == 'e' || s[i] == 'E') {
        i++
        var exp_sign = 1
        if(s[i] == '+') i++
        else if(s[i] == '-') { exp_sign = -1; i++ }
        var exp_val : int = 0
        while(s[i] >= '0' && s[i] <= '9') {
            exp_val = exp_val * 10 + (s[i] - '0') as int
            i++
        }
        var multiplier : double = 1.0
        for(var j = 0; j < exp_val; j++) { multiplier *= 10.0 }
        if(exp_sign > 0) result *= multiplier
        else result /= multiplier
    }
    if(negative) result = -result
    return result
}

public func is_float_str(s : *char) : bool {
    var i : int = 0
    if(s[0] == '-') i = 1
    while(s[i] != '\0') {
        if(s[i] == '.' || s[i] == 'e' || s[i] == 'E') return true
        i++
    }
    return false
}

// Helper to copy a JsonValue by variant dispatch
public func copy_json_value(src : &JsonValue) : JsonValue {
    switch(src) {
        Null() => return JsonValue.Null()
        Bool(v) => return JsonValue.Bool(v)
        Number(v) => return JsonValue.Number(v.copy())
        String(v) => return JsonValue.String(v.copy())
        Object(m) => {
            var out = std::unordered_map<std::string, JsonValue>()
            var it = m.iterator()
            while(it.valid()) {
                var key = it.key().copy()
                var val = it.value()
                out.insert(key, copy_json_value(val))
                it.next()
            }
            return JsonValue.Object(out)
        }
        Array(v) => {
            var out = std::vector<JsonValue>()
            var current = v.data()
            var end = current + v.size()
            while(current != end) {
                out.push(copy_json_value(&*current))
                current++
            }
            return JsonValue.Array(out)
        }
    }
}

// ===== Encoder Implementation =====

impl core::Encoder<JsonValue> for JsonEncoder {
    func encode_null(&self) : std::Result<core::Unit, core::SerializationError> {
        self.buffer.append_char_ptr("null")
        return std::Result.Ok(core::Unit {})
    }

    func encode_bool(&self, v : bool) : std::Result<core::Unit, core::SerializationError> {
        if(v) { self.buffer.append_char_ptr("true") }
        else { self.buffer.append_char_ptr("false") }
        return std::Result.Ok(core::Unit {})
    }

    func encode_char(&self, c : char) : std::Result<core::Unit, core::SerializationError> {
        self.buffer.append('"')
        self.buffer.append(c)
        self.buffer.append('"')
        return std::Result.Ok(core::Unit {})
    }

    func encode_u64(&self, i : u64) : std::Result<core::Unit, core::SerializationError> {
        write_u64(&mut *self.buffer, i)
        return std::Result.Ok(core::Unit {})
    }

    func encode_i64(&self, i : i64) : std::Result<core::Unit, core::SerializationError> {
        write_i64(&mut *self.buffer, i)
        return std::Result.Ok(core::Unit {})
    }

    func encode_double(&self, d : double) : std::Result<core::Unit, core::SerializationError> {
        write_double(&mut *self.buffer, d)
        return std::Result.Ok(core::Unit {})
    }

    func encode_float(&self, f : float) : std::Result<core::Unit, core::SerializationError> {
        write_double(&mut *self.buffer, f as double)
        return std::Result.Ok(core::Unit {})
    }

    func encode_str_of_len(&self, c : *char, l : u64) : std::Result<core::Unit, core::SerializationError> {
        json_escape_into(&mut *self.buffer, c, l)
        return std::Result.Ok(core::Unit {})
    }

    func encode_str(&self, c : *char) : std::Result<core::Unit, core::SerializationError> {
        var len : u64 = 0
        while(c[len] != '\0') { len++ }
        json_escape_into(&mut *self.buffer, c, len)
        return std::Result.Ok(core::Unit {})
    }

    func encode_bytes(b : *u8, l : u64) : std::Result<core::Unit, core::SerializationError> {
        return std::Result.Err(core::SerializationError {
            kind : core::SerializationErrorKind.Generic,
            message : std::string("bytes not supported for JSON encoder")
        })
    }

    func array(&self) : core::ArrayEncoder<JsonValue> {
        self.buffer.append('[')
        self.counts.push(0)
        return JsonArrayEncoder { buffer : self.buffer, counts : self.counts }
    }

    func array_of_len(&self, len : u64) : core::ArrayEncoder<JsonValue> {
        self.buffer.append('[')
        self.counts.push(0)
        return JsonArrayEncoder { buffer : self.buffer, counts : self.counts }
    }

    func object(&self) : core::ObjectEncoder<JsonValue> {
        self.buffer.append('{')
        self.counts.push(0)
        return JsonObjectEncoder { buffer : self.buffer, counts : self.counts }
    }

    func object_of_len(&self, len : u64) : core::ObjectEncoder<JsonValue> {
        self.buffer.append('{')
        self.counts.push(0)
        return JsonObjectEncoder { buffer : self.buffer, counts : self.counts }
    }

    func map(&self) : core::MapEncoder<JsonValue> {
        self.buffer.append('{')
        self.counts.push(0)
        return JsonMapEncoder { buffer : self.buffer, counts : self.counts }
    }

    func map_of_len(&self, len : u64) : core::MapEncoder<JsonValue> {
        self.buffer.append('{')
        self.counts.push(0)
        return JsonMapEncoder { buffer : self.buffer, counts : self.counts }
    }
}

impl core::ArrayEncoder<JsonValue> for JsonArrayEncoder {
    func <K : core::Serializer<JsonValue>> encode(&self, value : K) : std::Result<core::Unit, core::SerializationError> {
        var cnt = self.counts.get_ptr(self.counts.size() - 1)
        if(*cnt > 0) {
            self.buffer.append(',')
        }
        *cnt += 1
        var encoder = JsonEncoder { buffer : self.buffer, counts : self.counts }
        var r = value.encode(encoder)
        return r as std::Result<core::Unit, core::SerializationError>
    }
}

impl core::ObjectEncoder<JsonValue> for JsonObjectEncoder {
    func <V : core::Serializer<JsonValue>> field(&self, name : *char, value : V) : std::Result<core::Unit, core::SerializationError> {
        var cnt = self.counts.get_ptr(self.counts.size() - 1)
        if(*cnt > 0) {
            self.buffer.append(',')
        }
        *cnt += 1
        var len : u64 = 0
        while(name[len] != '\0') { len++ }
        json_escape_into(&mut *self.buffer, name, len)
        self.buffer.append(':')
        var encoder = JsonEncoder { buffer : self.buffer, counts : self.counts }
        var r = value.encode(encoder)
        return r as std::Result<core::Unit, core::SerializationError>
    }
}

impl core::MapEncoder<JsonValue> for JsonMapEncoder {
    func <K : core::Serializer<JsonValue>, V : core::Serializer<JsonValue>> encode(&self, key : K, value : V) : std::Result<core::Unit, core::SerializationError> {
        var cnt = self.counts.get_ptr(self.counts.size() - 1)
        if(*cnt > 0) {
            self.buffer.append(',')
        }
        *cnt += 1
        var encoder = JsonEncoder { buffer : self.buffer, counts : self.counts }
        var r1 = key.encode(encoder)
        if(!(r1 is std::Result.Ok)) { return r1 as std::Result<core::Unit, core::SerializationError> }
        self.buffer.append(':')
        var encoder2 = JsonEncoder { buffer : self.buffer, counts : self.counts }
        var r2 = value.encode(encoder2)
        return r2 as std::Result<core::Unit, core::SerializationError>
    }
}

// ===== Serializer for JsonValue =====
// Note: Implementing core::Serializer<JsonValue> for JsonValue requires
// accessing individual elements of vector<JsonValue> by value, which is
// currently blocked by the type system (cannot dereference raw pointer to
// destructible variant type). The encoder structs work correctly for
// programmatic JSON building; use encode_json() for JsonValue-to-string.


