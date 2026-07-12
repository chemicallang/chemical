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

impl std::Deserializer<bool> for TypeDecoder<bool> {
    func deserialize(&self) : std::Result<bool, std::SerializationError> {
        return self.decoder.decode_bool()
    }
}

impl std::Deserializer<char> for TypeDecoder<char> {
    func deserialize(&self) : std::Result<char, std::SerializationError> {
        return self.decoder.decode_char()
    }
}

impl std::Deserializer<uchar> for TypeDecoder<uchar> {
    func deserialize(&self) : std::Result<uchar, std::SerializationError> {
        var r = self.decoder.decode_char()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<uchar, std::SerializationError>(v as uchar)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<uchar, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<int> for TypeDecoder<int> {
    func deserialize(&self) : std::Result<int, std::SerializationError> {
        var r = self.decoder.decode_i64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<int, std::SerializationError>(v as int)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<int, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<uint> for TypeDecoder<uint> {
    func deserialize(&self) : std::Result<uint, std::SerializationError> {
        var r = self.decoder.decode_u64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<uint, std::SerializationError>(v as uint)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<uint, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<long> for TypeDecoder<long> {
    func deserialize(&self) : std::Result<long, std::SerializationError> {
        var r = self.decoder.decode_i64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<long, std::SerializationError>(v as long)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<long, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<ulong> for TypeDecoder<ulong> {
    func deserialize(&self) : std::Result<ulong, std::SerializationError> {
        var r = self.decoder.decode_u64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<ulong, std::SerializationError>(v as ulong)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<ulong, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<bigint> for TypeDecoder<bigint> {
    func deserialize(&self) : std::Result<bigint, std::SerializationError> {
        var r = self.decoder.decode_i64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<bigint, std::SerializationError>(v as bigint)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<bigint, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<ubigint> for TypeDecoder<ubigint> {
    func deserialize(&self) : std::Result<ubigint, std::SerializationError> {
        var r = self.decoder.decode_u64()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<ubigint, std::SerializationError>(v as ubigint)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<ubigint, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<double> for TypeDecoder<double> {
    func deserialize(&self) : std::Result<double, std::SerializationError> {
        return self.decoder.decode_double()
    }
}

impl std::Deserializer<float> for TypeDecoder<float> {
    func deserialize(&self) : std::Result<float, std::SerializationError> {
        return self.decoder.decode_float()
    }
}

impl std::Deserializer<std::string_view> for TypeDecoder<std::string_view> {
    func deserialize(&self) : std::Result<std::string_view, std::SerializationError> {
        var r = self.decoder.decode_str()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<std::string_view, std::SerializationError>(v)
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<std::string_view, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}

impl std::Deserializer<std::string> for TypeDecoder<std::string> {
    func deserialize(&self) : std::Result<std::string, std::SerializationError> {
        var r = self.decoder.decode_str()
        if(r is std::Result.Ok) {
            var Ok(v) = r else unreachable
            return std::Result.Ok<std::string, std::SerializationError>(std::string(v))
        }
        var Err(e) = r else unreachable;
        return std::Result.Err<std::string, std::SerializationError>(std::replace(&mut e, std::SerializationError()))
    }
}