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

// public func <T, E : std::Encoder<T>, S : std::Serializer<T, E>> encode(e : E, s : S) : std::Result<std::Unit, std::SerializationError> {
//     return s.encode(e)
// }
