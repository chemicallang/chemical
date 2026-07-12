public namespace std {

public struct Unit {}

public enum SerializationErrorKind {
    Generic
}

public struct SerializationError {
    var kind : SerializationErrorKind = SerializationErrorKind.Generic
    var message : std::string
}

public interface Encoder<T> {

    func encode_null(&self) : std::Result<Unit, SerializationError>

    func encode_bool(&self, v : bool) : std::Result<Unit, SerializationError>

    func encode_char(&self, c : char) : std::Result<Unit, SerializationError>

    func encode_u64(&self, i : u64) : std::Result<Unit, SerializationError>

    func encode_i64(&self, i : i64) : std::Result<Unit, SerializationError>

    func encode_double(&self, d : double) : std::Result<Unit, SerializationError>

    func encode_float(&self, f : float) : std::Result<Unit, SerializationError>

    func encode_str_of_len(&self, c : *char, l : u64) : std::Result<Unit, SerializationError>

    func encode_str(&self, c : *char) : std::Result<Unit, SerializationError>

    func encode_bytes(b : *u8, l : u64) : std::Result<Unit, SerializationError>

    func array(&self) : ArrayEncoder<T>

    func array_of_len(&self, len : u64) : ArrayEncoder<T>

    func object(&self) : ObjectEncoder<T>

    func object_of_len(&self, len : u64) : ObjectEncoder<T>

    func map(&self) : MapEncoder<T>

    func map_of_len(&self, len : u64) : MapEncoder<T>

}

public interface ArrayEncoder<T> {

    func <K : Serializer<T>> encode(&self, value : K) : std::Result<Unit, SerializationError>

}

public interface ObjectEncoder<T> {

    func <V : Serializer<T>> field(&self, name : *char, value : V) : std::Result<Unit, SerializationError>

}

public interface MapEncoder<T> {

    func <K : Serializer<T>, V : Serializer<T>> encode(&self, key : K, value : V) : std::Result<Unit, SerializationError>

}

public interface Serializer<T, E : Encoder<T>> {

    func encode(&self, encoder : &E) : std::Result<Unit, SerializationError>

}

// Decoder

public interface Decoder {

    func decode_null(&self) : std::Result<Unit, SerializationError>

    func decode_bool(&self) : std::Result<bool, SerializationError>

    func decode_char(&self) : std::Result<char, SerializationError>

    func decode_u64(&self) : std::Result<u64, SerializationError>

    func decode_i64(&self) : std::Result<i64, SerializationError>

    func decode_double(&self) : std::Result<double, SerializationError>

    func decode_float(&self) : std::Result<float, SerializationError>

    func decode_str(&self) : std::Result<std::string_view, SerializationError>

    func decode_bytes(&self) : std::Result<std::span<u8>, SerializationError>

    func array(&self) : std::Result<ArrayDecoder, SerializationError>

    func object(&self) : std::Result<ObjectDecoder, SerializationError>

    func map(&self) : std::Result<MapDecoder, SerializationError>

}

public interface ArrayDecoder {

    func item_decoder(&mut self) : std::Result<Decoder, SerializationError>

    func total(&self) : u64

    func remaining(&self) : u64

}

public interface ObjectDecoder {

    func item_decoder(&mut self) : std::Result<std::pair<std::string_view, Decoder>, SerializationError>

    func total(&self) : u64

}

public interface MapDecoder {

    func item_decoder(&mut self) : std::Result<std::pair<Decoder, Decoder>, SerializationError>

    func total(&self) : u64

}

public interface Deserializer<T> {

    func deserialize(&self) : std::Result<T, SerializationError>

}

}
