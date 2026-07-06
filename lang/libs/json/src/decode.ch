// ===== Decoder (owned-copy approach) =====
// We store owned copies of JsonValue to avoid raw-pointer-to-destructible-type issues.

public struct JsonDecoder {
    var value : JsonValue
}

public func make_json_decoder(value : &JsonValue) : JsonDecoder {
    return JsonDecoder { value : copy_json_value(value) }
}

public func (d : &JsonDecoder) decode_null() : std::Result<std::Unit, std::SerializationError> {
    if(d.value is JsonValue.Null) {
        return std::Result.Ok(std::Unit {})
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected null")
    })
}

public func (d : &JsonDecoder) decode_bool() : std::Result<bool, std::SerializationError> {
    if(d.value is JsonValue.Bool) {
        var Bool(v) = d.value else unreachable
        return std::Result.Ok(v)
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected bool")
    })
}

public func (d : &JsonDecoder) decode_i64() : std::Result<i64, std::SerializationError> {
    if(d.value is JsonValue.Number) {
        var Number(s) = d.value else unreachable
        return std::Result.Ok(parse_i64(s.data()))
    }
    if(d.value is JsonValue.Bool) {
        var Bool(v) = d.value else unreachable
        return std::Result.Ok(if(v) 1i64 else 0i64)
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected number")
    })
}

public func (d : &JsonDecoder) decode_u64() : std::Result<u64, std::SerializationError> {
    if(d.value is JsonValue.Number) {
        var Number(s) = d.value else unreachable
        return std::Result.Ok(parse_u64(s.data()))
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected number")
    })
}

public func (d : &JsonDecoder) decode_double() : std::Result<double, std::SerializationError> {
    if(d.value is JsonValue.Number) {
        var Number(s) = d.value else unreachable
        return std::Result.Ok(parse_double(s.data()))
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected number")
    })
}

public func (d : &JsonDecoder) decode_float() : std::Result<float, std::SerializationError> {
    if(d.value is JsonValue.Number) {
        var Number(s) = d.value else unreachable
        return std::Result.Ok(parse_double(s.data()) as float)
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected number")
    })
}

public func (d : &JsonDecoder) decode_char() : std::Result<char, std::SerializationError> {
    if(d.value is JsonValue.String) {
        var String(s) = d.value else unreachable
        if(s.size() == 1) {
            return std::Result.Ok(s.get(0))
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected single-char string")
        })
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected string")
    })
}

public func (d : &JsonDecoder) decode_str() : std::Result<std::string, std::SerializationError> {
    if(d.value is JsonValue.String) {
        var String(s) = d.value else unreachable
        return std::Result.Ok(s.copy())
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected string")
    })
}

public func (d : &JsonDecoder) decode_str_of_len(len : u64) : std::Result<std::string, std::SerializationError> {
    if(d.value is JsonValue.String) {
        var String(s) = d.value else unreachable
        if(s.size() >= len) {
            var out = std::string()
            out.append_with_len(s.data(), len)
            return std::Result.Ok(out)
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("string too short")
        })
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected string")
    })
}

// ===== Array Iteration =====

public struct JsonArrayIter {
    var elements : std::vector<JsonValue>
    var index : u64
}

public func (d : &JsonDecoder) array() : std::Result<JsonArrayIter, std::SerializationError> {
    if(d.value is JsonValue.Array) {
        var Array(v) = d.value else unreachable
        var copy = std::vector<JsonValue>()
        var ptr = v.data()
        var end = ptr + v.size()
        while(ptr != end) {
            copy.push(copy_json_value(&*ptr))
            ptr++
        }
        return std::Result.Ok(JsonArrayIter { elements : copy, index : 0 })
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected array")
    })
}

public func (iter : &JsonArrayIter) remaining() : u64 {
    return if(iter.index >= iter.elements.size()) 0u64 else iter.elements.size() - iter.index
}

public func (iter : &mut JsonArrayIter) next() : std::Result<JsonDecoder, std::SerializationError> {
    if(iter.index >= iter.elements.size()) {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("array index out of bounds")
        })
    }
    // Use data() to get base pointer, then index to element
    var ptr = iter.elements.data() + iter.index
    var item = copy_json_value(&*ptr)
    iter.index++
    return std::Result.Ok(JsonDecoder { value : item })
}

// ===== Object Iteration =====

public struct JsonObjectEntry {
    var key : std::string
    var value : JsonValue
}

public struct JsonObjectIter {
    var entries : std::vector<JsonObjectEntry>
    var index : u64
}

public func (d : &JsonDecoder) object() : std::Result<JsonObjectIter, std::SerializationError> {
    if(d.value is JsonValue.Object) {
        var Object(m) = d.value else unreachable
        var entries = std::vector<JsonObjectEntry>()
        var it = m.iterator()
        while(it.valid()) {
            var key = it.key().copy()
            var val = copy_json_value(it.value())
            entries.push(JsonObjectEntry { key : key, value : val })
            it.next()
        }
        return std::Result.Ok(JsonObjectIter { entries : entries, index : 0 })
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("expected object")
    })
}

public func (o : &JsonObjectIter) remaining() : u64 {
    return if(o.index >= o.entries.size()) 0u64 else o.entries.size() - o.index
}

public func (o : &mut JsonObjectIter) next_key() : std::string {
    var entry_ptr = o.entries.data() + o.index
    var result = entry_ptr.key.copy()
    o.index++
    return result
}

public func (o : &mut JsonObjectIter) next_value() : std::Result<JsonDecoder, std::SerializationError> {
    if(o.index >= o.entries.size()) {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("object index out of bounds")
        })
    }
    var entry_ptr = o.entries.data() + o.index
    var val = copy_json_value(&entry_ptr.value)
    o.index++
    return std::Result.Ok(JsonDecoder { value : val })
}

public func (o : &JsonObjectIter) field(name : *char) : std::Result<JsonDecoder, std::SerializationError> {
    var key = std::string(name)
    for(var i = 0u64; i < o.entries.size(); i++) {
        var entry_ptr = o.entries.data() + i
        if(entry_ptr.key.equals(&key)) {
            var val = copy_json_value(&entry_ptr.value)
            return std::Result.Ok(JsonDecoder { value : val })
        }
    }
    return std::Result.Err(std::SerializationError {
        kind : std::SerializationErrorKind.Generic,
        message : std::string("field not found")
    })
}
