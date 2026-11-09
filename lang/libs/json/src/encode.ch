// ===== Encoder Structs =====

// Owning-call-site encoder state. The pointers refer to a `std::string` and a
// count vector that outlive the encoder; fields are module-internal so callers
// cannot desynchronize them, and `JsonEncoder::from_parts(buffer, counts)` is the only
// public construction path.
public struct JsonEncoder {
    internal var buffer : *mut std::string
    internal var counts : *mut std::vector<u64>

    // Controlled construction: callers provide the output buffer and count
    // stack, which must outlive the encoder.
    public func from_parts(buffer : &mut std::string, counts : &mut std::vector<u64>) : JsonEncoder {
        return JsonEncoder {
            buffer : &raw mut buffer,
            counts : &raw mut counts
        }
    }
}

// Container encoders returned by JsonEncoder.array()/object()/map(). Kept as
// three separate structs: a single struct implementing all three encoder
// interfaces breaks interface-constraint resolution for the two-generic-param
// MapEncoder.encode<K, V> (unresolved 'serialize' on the V param). The shared
// comma/bookkeeping logic lives in json::raw::container_begin_item.
public struct JsonArrayEncoder {
    var buffer : *mut std::string
    var counts : *mut std::vector<u64>

    @delete
    func destruct(&self) {
        unsafe {
            self.buffer.append(']')
            self.counts.take_last()
    
        }
    }
}

public struct JsonObjectEncoder {
    var buffer : *mut std::string
    var counts : *mut std::vector<u64>

    @delete
    func destruct(&self) {
        unsafe {
            self.buffer.append('}')
            self.counts.take_last()
    
        }
    }
}

public struct JsonMapEncoder {
    var buffer : *mut std::string
    var counts : *mut std::vector<u64>

    @delete
    func destruct(&self) {
        unsafe {
            self.buffer.append('}')
            self.counts.take_last()
    
        }
    }
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

// Append `str` as a JSON string literal (including the surrounding quotes).
// Control characters, quotes and backslashes are escaped; UTF-8 bytes pass
// through unchanged. The output owns the resulting bytes.
public func json_escape_into(output : &mut std::string, str : &str) {
    output.append('"')
    var i : size_t = 0
    while(i < str.size()) {
        append_escaped_char(output, str.at(i))
        i += 1
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
    var len = unsafe(snprintf(&raw mut buf[0], 64, "%.17g", val))
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
    var len = unsafe(snprintf(&raw mut buf[0], 64, "%.9g", val))
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

// ===== Encoder Implementation =====

impl std::Encoder<JsonValue> for JsonEncoder {
    func encode_null(&self) : std::Result<std::Unit, std::SerializationError> {
        unsafe {
            self.buffer.append_char_ptr("null")
            return std::Result.Ok(std::Unit {})
    
        }
    }

    func encode_bool(&self, v : bool) : std::Result<std::Unit, std::SerializationError> {
        unsafe {
            if(v) { self.buffer.append_char_ptr("true") }
            else { self.buffer.append_char_ptr("false") }
            return std::Result.Ok(std::Unit {})
    
        }
    }

    func encode_char(&self, c : char) : std::Result<std::Unit, std::SerializationError> { unsafe {
        // a char is a single-char JSON string; the char itself must be escaped
        // (quotes/backslashes/control chars would otherwise emit invalid JSON)
        var buf : [1]char = [c]
        json::raw::json_escape_into_raw(&mut *self.buffer, &raw mut buf[0], 1u64)
        return std::Result.Ok(std::Unit {})
     }}

    func encode_u64(&self, i : u64) : std::Result<std::Unit, std::SerializationError> { unsafe {
        write_u64(&mut *self.buffer, i)
        return std::Result.Ok(std::Unit {})
     }}

    func encode_i64(&self, i : i64) : std::Result<std::Unit, std::SerializationError> { unsafe {
        write_i64(&mut *self.buffer, i)
        return std::Result.Ok(std::Unit {})
     }}

    func encode_double(&self, d : double) : std::Result<std::Unit, std::SerializationError> { unsafe {
        write_double(&mut *self.buffer, d)
        return std::Result.Ok(std::Unit {})
     }}

    func encode_float(&self, f : float) : std::Result<std::Unit, std::SerializationError> { unsafe {
        write_float(&mut *self.buffer, f)
        return std::Result.Ok(std::Unit {})
     }}

    func encode_str_of_len(&self, c : *char, l : u64) : std::Result<std::Unit, std::SerializationError> { unsafe {
        json::raw::json_escape_into_raw(&mut *self.buffer, c, l)
        return std::Result.Ok(std::Unit {})
     }}

    func encode_str(&self, c : *char) : std::Result<std::Unit, std::SerializationError> { unsafe {
        var len : u64 = 0
        while(c[len] != '\0') { len++ }
        json::raw::json_escape_into_raw(&mut *self.buffer, c, len)
        return std::Result.Ok(std::Unit {})
     }}

    func encode_bytes(b : *u8, l : u64) : std::Result<std::Unit, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("bytes not supported for JSON encoder")
        })
    }

    func array(&self) : JsonArrayEncoder {
        unsafe {
            self.buffer.append('[')
            self.counts.push(0)
            return JsonArrayEncoder { buffer : self.buffer, counts : self.counts }
    
        }
    }

    func array_of_len(&self, len : u64) : JsonArrayEncoder {
        unsafe {
            self.buffer.append('[')
            self.counts.push(0)
            return JsonArrayEncoder { buffer : self.buffer, counts : self.counts }
    
        }
    }

    func object(&self) : JsonObjectEncoder {
        unsafe {
            self.buffer.append('{')
            self.counts.push(0)
            return JsonObjectEncoder { buffer : self.buffer, counts : self.counts }
    
        }
    }

    func object_of_len(&self, len : u64) : JsonObjectEncoder {
        unsafe {
            self.buffer.append('{')
            self.counts.push(0)
            return JsonObjectEncoder { buffer : self.buffer, counts : self.counts }
    
        }
    }

    func map(&self) : JsonMapEncoder {
        unsafe {
            self.buffer.append('{')
            self.counts.push(0)
            return JsonMapEncoder { buffer : self.buffer, counts : self.counts }
    
        }
    }

    func map_of_len(&self, len : u64) : JsonMapEncoder {
        unsafe {
            self.buffer.append('{')
            self.counts.push(0)
            return JsonMapEncoder { buffer : self.buffer, counts : self.counts }
    
        }
    }
}

impl std::ArrayEncoder<JsonValue> for JsonArrayEncoder {
    func <K : std::Serializer<JsonValue, JsonEncoder>> encode(&self, value : &K) : std::Result<std::Unit, std::SerializationError> { unsafe {
        json::raw::container_begin_item(self.buffer, self.counts)
        var encoder = JsonEncoder::from_parts(&mut *self.buffer, &mut *self.counts)
        // TODO: returning value.encode doesn't satisfy the result, compiler bug
        return value.serialize(json::raw::cast_encoder<JsonValue>(&encoder)) as std::Result<std::Unit, std::SerializationError>
     }}
}

impl std::ObjectEncoder<JsonValue> for JsonObjectEncoder {
    func <V : std::Serializer<JsonValue, JsonEncoder>> field(&self, name : std::string_view, value : &V) : std::Result<std::Unit, std::SerializationError> { unsafe {
        json::raw::container_begin_item(self.buffer, self.counts)
        json::raw::json_escape_into_raw(&mut *self.buffer, name.data(), name.size())
        self.buffer.append(':')
        var encoder = JsonEncoder::from_parts(&mut *self.buffer, &mut *self.counts)
        // TODO: returning value.encode doesn't satisfy the result, compiler bug
        return value.serialize(json::raw::cast_encoder<JsonValue>(&encoder)) as std::Result<std::Unit, std::SerializationError>
     }}
}

impl std::MapEncoder<JsonValue> for JsonMapEncoder {
    func <K : std::Serializer<JsonValue, JsonEncoder>, V : std::Serializer<JsonValue, JsonEncoder>> encode(&self, key : &K, value : &V) : std::Result<std::Unit, std::SerializationError> {
        unsafe {
            json::raw::container_begin_item(self.buffer, self.counts)
            var encoder = JsonEncoder::from_parts(&mut *self.buffer, &mut *self.counts)
            var r1 = key.serialize(json::raw::cast_encoder<JsonValue>(&encoder))
            if(!(r1 is std::Result.Ok)) { return r1 as std::Result<std::Unit, std::SerializationError> }
            self.buffer.append(':')
            var encoder2 = JsonEncoder::from_parts(&mut *self.buffer, &mut *self.counts)
            // TODO: returning value.encode doesn't satisfy the result, compiler bug
            return value.serialize(json::raw::cast_encoder<JsonValue>(&encoder2)) as std::Result<std::Unit, std::SerializationError>
    
        }
    }
}

// Generic encode method — dispatches to Serializer<T, JsonEncoder>
func <T : std::Serializer<JsonValue, JsonEncoder>> (e : &JsonEncoder) encode(value : T) : std::Result<std::Unit, std::SerializationError> {
    // TODO: returning value.encode doesn't satisfy the result, compiler bug
    return value.serialize(json::raw::cast_encoder<JsonValue>(e)) as std::Result<std::Unit, std::SerializationError>
}