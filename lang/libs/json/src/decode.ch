// ===== Decoder (owned-copy approach) =====
// We store owned copies of JsonValue to avoid raw-pointer-to-destructible-type issues.

public struct JsonDecoder {
    var value : &JsonValue
}

// ===== Decoder Interface Implementation =====

impl std::Decoder for JsonDecoder {
    func decode_null(&self) : std::Result<std::Unit, std::SerializationError> {
        if(self.value is JsonValue.Null) {
            return std::Result.Ok(std::Unit {})
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected null")
        })
    }

    func decode_bool(&self) : std::Result<bool, std::SerializationError> {
        if(self.value is JsonValue.Bool) {
            var Bool(v) = self.value else unreachable
            return std::Result.Ok(v)
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected bool")
        })
    }

    func decode_char(&self) : std::Result<char, std::SerializationError> {
        if(self.value is JsonValue.String) {
            var String(s) = self.value else unreachable
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

    func decode_u64(&self) : std::Result<u64, std::SerializationError> {
        if(self.value is JsonValue.Number) {
            var Number(s) = self.value else unreachable
            return std::Result.Ok(parse_u64(s.data()))
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected number")
        })
    }

    func decode_i64(&self) : std::Result<i64, std::SerializationError> {
        if(self.value is JsonValue.Number) {
            var Number(s) = self.value else unreachable
            return std::Result.Ok(parse_i64(s.data()))
        }
        if(self.value is JsonValue.Bool) {
            var Bool(v) = self.value else unreachable
            return std::Result.Ok(if(v) 1i64 else 0i64)
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected number")
        })
    }

    func decode_double(&self) : std::Result<double, std::SerializationError> {
        if(self.value is JsonValue.Number) {
            var Number(s) = self.value else unreachable
            return std::Result.Ok(parse_double(s.data()))
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected number")
        })
    }

    func decode_float(&self) : std::Result<float, std::SerializationError> {
        if(self.value is JsonValue.Number) {
            var Number(s) = self.value else unreachable
            return std::Result.Ok(parse_double(s.data()) as float)
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected number")
        })
    }

    func decode_str(&self) : std::Result<std::string_view, std::SerializationError> {
        if(self.value is JsonValue.String) {
            var String(s) = self.value else unreachable
            return std::Result.Ok(s.to_view())
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected string")
        })
    }

    func decode_bytes(&self) : std::Result<std::span<u8>, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("bytes not supported for JSON decoder")
        })
    }

    func array(&self) : std::Result<JsonArrayDecoder, std::SerializationError> {
        if(self.value is JsonValue.Array) {
            var Array(v) = self.value else unreachable
            return std::Result.Ok(JsonArrayDecoder { elements : std::span<JsonValue>(v.data(), v.size()), index : 0u64 })
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected array")
        })
    }

    func object(&self) : std::Result<JsonObjectDecoder, std::SerializationError> {
        if(self.value is JsonValue.Object) {
            var Object(m) = self.value else unreachable
            return std::Result.Ok(JsonObjectDecoder { iterator : m.iterator() })
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected object")
        })
    }

    func map(&self) : std::Result<JsonMapDecoder, std::SerializationError> {
        if(self.value is JsonValue.Object) {
            var Object(m) = self.value else unreachable
            return std::Result.Ok(JsonMapDecoder { iterator : m.iterator() })
        }
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected object")
        })
    }

}

// ===== JsonArrayDecoder =====

public struct JsonArrayDecoder {
    var elements : std::span<JsonValue>
    var index : u64
}

impl std::ArrayDecoder<JsonValue> for JsonArrayDecoder {

    func item_decoder(&mut self) : std::Result<JsonDecoder, std::SerializationError> {
        if(self.index >= self.elements.size()) {
            return std::Result.Err(std::SerializationError {
                kind : std::SerializationErrorKind.Generic,
                message : std::string("array index out of bounds")
            })
        }
        const i = self.index;
        self.index++;
        return std::Result.Ok(JsonDecoder { value : &*(self.elements.data() + i) })
    }

    func remaining(&self) : u64 {
        return if(self.index >= self.elements.size()) 0u64 else self.elements.size() - self.index
    }

    func total(&self) : u64 {
        return self.elements.size();
    }

}

// ===== JsonObjectDecoder =====

public struct JsonObjectDecoder {
    var iterator : std::unordered_map_iterator<std::string, JsonValue>
}

impl std::ObjectDecoder<JsonValue> for JsonObjectDecoder {

    func item_decoder(&mut self) : std::Result<std::pair<std::string_view, JsonDecoder>, std::SerializationError> {
        if(!iterator.valid()) {
            return std::Result.Err(std::SerializationError {
                kind : std::SerializationErrorKind.Generic,
                message : std::string("array index out of bounds")
            })
        }
        var r = std::Result.Ok<std::pair<std::string_view, JsonDecoder>, std::SerializationError>(std::pair<std::string_view, JsonDecoder> {
            first : iterator.key().to_view(),
            second : JsonDecoder { value : iterator.value() }
        })
        iterator.next();
        return r;
    }

    func total(&self) : u64 {
        return self.iterator.size();
    }

}

// ===== JsonMapDecoder =====

public struct JsonMapDecoder {
    var iterator : std::unordered_map_iterator<std::string, JsonValue>
}

impl std::MapDecoder<JsonValue> for JsonMapDecoder {
    func item_decoder(&mut self) : std::Result<std::pair<JsonStringDecoder, JsonDecoder>, std::SerializationError> {
        if(!iterator.valid()) {
            return std::Result.Err(std::SerializationError {
                kind : std::SerializationErrorKind.Generic,
                message : std::string("array index out of bounds")
            })
        }
        var r = std::Result.Ok<std::pair<JsonStringDecoder, JsonDecoder>, std::SerializationError>(std::pair<JsonStringDecoder, JsonDecoder> {
            first : JsonStringDecoder { value : iterator.key().to_view() },
            second : JsonDecoder { value : iterator.value() }
        })
        iterator.next();
        return r;
    }

    func total(&self) : u64 {
        return self.iterator.size();
    }
}

// ===== JsonStringDecoder =====
// Required for JsonMapDecoder

public struct JsonStringDecoder {
    var value : std::string_view
}

impl std::Decoder for JsonStringDecoder {
    func decode_null(&self) : std::Result<std::Unit, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected null")
        })
    }

    func decode_bool(&self) : std::Result<bool, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected bool")
        })
    }

    func decode_char(&self) : std::Result<char, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected string")
        })
    }

    func decode_u64(&self) : std::Result<u64, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected number")
        })
    }

    func decode_i64(&self) : std::Result<i64, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected number")
        })
    }

    func decode_double(&self) : std::Result<double, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected number")
        })
    }

    func decode_float(&self) : std::Result<float, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected number")
        })
    }

    func decode_str(&self) : std::Result<std::string_view, std::SerializationError> {
        return std::Result.Ok(self.value)
    }

    func decode_bytes(&self) : std::Result<std::span<u8>, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("bytes not supported for JSON decoder")
        })
    }

    func array(&self) : std::Result<std::ArrayDecoder, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected array")
        })
    }

    func object(&self) : std::Result<std::ObjectDecoder, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected object")
        })
    }

    func map(&self) : std::Result<std::MapDecoder, std::SerializationError> {
        return std::Result.Err(std::SerializationError {
            kind : std::SerializationErrorKind.Generic,
            message : std::string("expected object")
        })
    }

}