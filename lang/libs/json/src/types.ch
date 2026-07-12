impl std::Serializer<JsonValue, JsonEncoder> for bool {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_bool(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for char {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_char(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for uchar {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_char((*self) as char)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for short {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_i64(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for ushort {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_u64(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for int {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_i64(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for uint {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_u64(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for long {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_i64(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for ulong {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_u64(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for bigint {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_i64(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for ubigint {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_u64(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for float {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_float(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for double {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_double(*self)
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for std::string_view {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_str_of_len(self.data(), self.size())
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for std::string {
    func encode(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        return encoder.encode_str_of_len(self.data(), self.size())
    }
}

// ===== Deserializer Implementations =====

impl std::Deserializer<bool> for JsonDecoder {
    func deserialize(&self) : std::Result<bool, std::SerializationError> {
        return self.decode_bool()
    }
}

impl std::Deserializer<char> for JsonDecoder {
    func deserialize(&self) : std::Result<char, std::SerializationError> {
        return self.decode_char()
    }
}

impl std::Deserializer<uchar> for JsonDecoder {
    func deserialize(&self) : std::Result<uchar, std::SerializationError> {
        var r = self.decode_char()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<uchar, std::SerializationError>(v as uchar)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<uchar, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<int> for JsonDecoder {
    func deserialize(&self) : std::Result<int, std::SerializationError> {
        var r = self.decode_i64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<int, std::SerializationError>(v as int)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<int, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<uint> for JsonDecoder {
    func deserialize(&self) : std::Result<uint, std::SerializationError> {
        var r = self.decode_u64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<uint, std::SerializationError>(v as uint)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<uint, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<long> for JsonDecoder {
    func deserialize(&self) : std::Result<long, std::SerializationError> {
        var r = self.decode_i64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<long, std::SerializationError>(v as long)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<long, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<ulong> for JsonDecoder {
    func deserialize(&self) : std::Result<ulong, std::SerializationError> {
        var r = self.decode_u64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<ulong, std::SerializationError>(v as ulong)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<ulong, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<bigint> for JsonDecoder {
    func deserialize(&self) : std::Result<bigint, std::SerializationError> {
        var r = self.decode_i64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<bigint, std::SerializationError>(v as bigint)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<bigint, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<ubigint> for JsonDecoder {
    func deserialize(&self) : std::Result<ubigint, std::SerializationError> {
        var r = self.decode_u64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<ubigint, std::SerializationError>(v as ubigint)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<ubigint, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<double> for JsonDecoder {
    func deserialize(&self) : std::Result<double, std::SerializationError> {
        return self.decode_double()
    }
}

impl std::Deserializer<float> for JsonDecoder {
    func deserialize(&self) : std::Result<float, std::SerializationError> {
        return self.decode_float()
    }
}

impl std::Deserializer<std::string_view> for JsonDecoder {
    func deserialize(&self) : std::Result<std::string_view, std::SerializationError> {
        var r = self.decode_str()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<std::string_view, std::SerializationError>(v)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<std::string_view, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<std::string> for JsonDecoder {
    func deserialize(&self) : std::Result<std::string, std::SerializationError> {
        var r = self.decode_str()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<std::string, std::SerializationError>(std::string(v))
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<std::string, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}