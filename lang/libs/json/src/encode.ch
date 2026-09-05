// ===== Encoder Structs =====

public struct JsonEncoder {
    var buffer : *mut std::string
    var counts : *mut std::vector<u64>
}

// Container encoders returned by JsonEncoder.array()/object()/map(). Kept as
// three separate structs: a single struct implementing all three encoder
// interfaces breaks interface-constraint resolution for the two-generic-param
// MapEncoder.encode<K, V> (unresolved 'serialize' on the V param). The shared
// comma/bookkeeping logic lives in __container_begin_item.
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

// comma/bookkeeping shared by all three container impls. public because it is
// called from public generic declarations (retention rule).
public func __container_begin_item(buffer : *mut std::string, counts : *mut std::vector<u64>) {
    var cnt = counts.get_ptr(counts.size() - 1)
    if(*cnt > 0) {
        buffer.append(',')
    }
    *cnt += 1
}

// ===== Helpers =====

// appends one char to `output`, JSON-escaped if needed
func append_escaped_char(output : &mut std::string, c : char) {
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

// the single string-escaping implementation (encode + emit both use it)
public func json_escape_into(output : &mut std::string, str : *char, len : u64) {
    output.append('"')
    for(var i : u64 = 0; i < len; i++) {
        append_escaped_char(output, str[i])
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
    var buf : char[64]
    // 17 significant digits is the precision required to round-trip any
    // IEEE-754 double exactly (fewer digits silently corrupts values).
    var len = snprintf(&raw mut buf[0], 64, "%.17g", val)
    if(len > 0) {
        var i = 0
        while(i < len && i < 64) {
            output.append(buf[i])
            i++
        }
    }
}

public func write_double(output : &mut std::string, val : double) {
    // NaN (val != val) and +/- infinity (val * 0.0 is NaN, which never equals 0.0)
    if(val != val || val * 0.0 != 0.0) {
        output.append_char_ptr("null")
        return
    }
    if(val == 0.0) {
        output.append('0')
        return
    }
    write_double_raw(output, val)
}

public func write_float_raw(output : &mut std::string, val : float) {
    var buf : char[64]
    // 9 significant digits is the precision required to round-trip any
    // IEEE-754 single-precision float exactly.
    var len = snprintf(&raw mut buf[0], 64, "%.9g", val)
    if(len > 0) {
        var i = 0
        while(i < len && i < 64) {
            output.append(buf[i])
            i++
        }
    }
}

public func write_float(output : &mut std::string, val : float) {
    if(val != val || val * 0.0f != 0.0f) {
        output.append_char_ptr("null")
        return
    }
    if(val == 0.0f) {
        output.append('0')
        return
    }
    write_float_raw(output, val)
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

public func parse_u64(s : *char) : u64 {
    var i : int = 0
    var result : u64 = 0
    while(s[i] != '\0' && s[i] >= '0' && s[i] <= '9') {
        result = result * 10u64 + (s[i] - '0') as u64
        i++
    }
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

// ===== Encoder Implementation =====

impl std::Encoder<JsonValue> for JsonEncoder {
    func encode_null(&self) : std::Result<std::Unit, std::SerializationError> {
        self.buffer.append_char_ptr("null")
        return std::Result.Ok(std::Unit {})
    }

    func encode_bool(&self, v : bool) : std::Result<std::Unit, std::SerializationError> {
        if(v) { self.buffer.append_char_ptr("true") }
        else { self.buffer.append_char_ptr("false") }
        return std::Result.Ok(std::Unit {})
    }

    func encode_char(&self, c : char) : std::Result<std::Unit, std::SerializationError> {
        // a char is a single-char JSON string; the char itself must be escaped
        // (quotes/backslashes/control chars would otherwise emit invalid JSON)
        var buf : [1]char = [c]
        json_escape_into(&mut *self.buffer, &raw mut buf[0], 1u64)
        return std::Result.Ok(std::Unit {})
    }

    func encode_u64(&self, i : u64) : std::Result<std::Unit, std::SerializationError> {
        write_u64(&mut *self.buffer, i)
        return std::Result.Ok(std::Unit {})
    }

    func encode_i64(&self, i : i64) : std::Result<std::Unit, std::SerializationError> {
        write_i64(&mut *self.buffer, i)
        return std::Result.Ok(std::Unit {})
    }

    func encode_double(&self, d : double) : std::Result<std::Unit, std::SerializationError> {
        write_double(&mut *self.buffer, d)
        return std::Result.Ok(std::Unit {})
    }

    func encode_float(&self, f : float) : std::Result<std::Unit, std::SerializationError> {
        write_float(&mut *self.buffer, f)
        return std::Result.Ok(std::Unit {})
    }

    func encode_str_of_len(&self, c : *char, l : u64) : std::Result<std::Unit, std::SerializationError> {
        json_escape_into(&mut *self.buffer, c, l)
        return std::Result.Ok(std::Unit {})
    }

    func encode_str(&self, c : *char) : std::Result<std::Unit, std::SerializationError> {
        var len : u64 = 0
        while(c[len] != '\0') { len++ }
        json_escape_into(&mut *self.buffer, c, len)
        return std::Result.Ok(std::Unit {})
    }

    func encode_bytes(b : *u8, l : u64) : std::Result<std::Unit, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("bytes not supported for JSON encoder")
        })
    }

    func array(&self) : JsonArrayEncoder {
        self.buffer.append('[')
        self.counts.push(0)
        return JsonArrayEncoder { buffer : self.buffer, counts : self.counts }
    }

    func array_of_len(&self, len : u64) : JsonArrayEncoder {
        self.buffer.append('[')
        self.counts.push(0)
        return JsonArrayEncoder { buffer : self.buffer, counts : self.counts }
    }

    func object(&self) : JsonObjectEncoder {
        self.buffer.append('{')
        self.counts.push(0)
        return JsonObjectEncoder { buffer : self.buffer, counts : self.counts }
    }

    func object_of_len(&self, len : u64) : JsonObjectEncoder {
        self.buffer.append('{')
        self.counts.push(0)
        return JsonObjectEncoder { buffer : self.buffer, counts : self.counts }
    }

    func map(&self) : JsonMapEncoder {
        self.buffer.append('{')
        self.counts.push(0)
        return JsonMapEncoder { buffer : self.buffer, counts : self.counts }
    }

    func map_of_len(&self, len : u64) : JsonMapEncoder {
        self.buffer.append('{')
        self.counts.push(0)
        return JsonMapEncoder { buffer : self.buffer, counts : self.counts }
    }
}

// JsonEncoder implements std::Encoder<JsonValue>, but calling the Serializer
// interface method from generic code can't satisfy the Result return type
// directly (compiler limitation) - route through this cast + explicit cast.
// public because it is called from public generic declarations (retention rule).
public func <T> __unsafe_cast_json_encoder(e : &JsonEncoder) : &std::Encoder<T> {
    return e as &std::Encoder<T>
}

impl std::ArrayEncoder<JsonValue> for JsonArrayEncoder {
    func <K : std::Serializer<JsonValue, JsonEncoder>> encode(&self, value : &K) : std::Result<std::Unit, std::SerializationError> {
        __container_begin_item(self.buffer, self.counts)
        var encoder = JsonEncoder { buffer : self.buffer, counts : self.counts }
        // TODO: returning value.encode doesn't satisfy the result, compiler bug
        return value.serialize(__unsafe_cast_json_encoder<JsonValue>(&encoder)) as std::Result<std::Unit, std::SerializationError>
    }
}

impl std::ObjectEncoder<JsonValue> for JsonObjectEncoder {
    func <V : std::Serializer<JsonValue, JsonEncoder>> field(&self, name : std::string_view, value : &V) : std::Result<std::Unit, std::SerializationError> {
        __container_begin_item(self.buffer, self.counts)
        json_escape_into(&mut *self.buffer, name.data(), name.size())
        self.buffer.append(':')
        var encoder = JsonEncoder { buffer : self.buffer, counts : self.counts }
        // TODO: returning value.encode doesn't satisfy the result, compiler bug
        return value.serialize(__unsafe_cast_json_encoder<JsonValue>(&encoder)) as std::Result<std::Unit, std::SerializationError>
    }
}

impl std::MapEncoder<JsonValue> for JsonMapEncoder {
    func <K : std::Serializer<JsonValue, JsonEncoder>, V : std::Serializer<JsonValue, JsonEncoder>> encode(&self, key : &K, value : &V) : std::Result<std::Unit, std::SerializationError> {
        __container_begin_item(self.buffer, self.counts)
        var encoder = JsonEncoder { buffer : self.buffer, counts : self.counts }
        var r1 = key.serialize(__unsafe_cast_json_encoder<JsonValue>(&encoder))
        if(!(r1 is std::Result.Ok)) { return r1 as std::Result<std::Unit, std::SerializationError> }
        self.buffer.append(':')
        var encoder2 = JsonEncoder { buffer : self.buffer, counts : self.counts }
        // TODO: returning value.encode doesn't satisfy the result, compiler bug
        return value.serialize(__unsafe_cast_json_encoder<JsonValue>(&encoder2)) as std::Result<std::Unit, std::SerializationError>
    }
}

// Generic encode method — dispatches to Serializer<T, JsonEncoder>
func <T : std::Serializer<JsonValue, JsonEncoder>> (e : &JsonEncoder) encode(value : T) : std::Result<std::Unit, std::SerializationError> {
    // TODO: returning value.encode doesn't satisfy the result, compiler bug
    return value.serialize(__unsafe_cast_json_encoder<JsonValue>(e)) as std::Result<std::Unit, std::SerializationError>
}