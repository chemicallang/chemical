public namespace core {

struct Unit {}

enum SerializationErrorKind {
    Generic
}

struct SerializationError {
    var kind : SerializationErrorKind
    var message : std::string
}

interface Encoder<T> {

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

interface ArrayEncoder<T> {

    func <K : Serializer<T>> encode(&self, value : K) : std::Result<Unit, SerializationError>

}

interface ObjectEncoder<T> {

    func <V : Serializer<T>> field(&self, name : *char, value : V) : std::Result<Unit, SerializationError>

}

interface MapEncoder<T> {

    func <K : Serializer<T>, V : Serializer<T>> encode(&self, key : K, value : V) : std::Result<Unit, SerializationError>

}

interface Serializer<T> {

    func encode(&self, encoder : Encoder<T>) : std::Result<Unit, SerializationError>

}

// Decoder

interface Decoder<T> {

    func decode_null(&self) : std::Result<Unit, SerializationError>

    func decode_bool(&self) : std::Result<bool, SerializationError>

    func decode_char(&self) : std::Result<char, SerializationError>

    func decode_u64(&self) : std::Result<u64, SerializationError>

    func decode_i64(&self) : std::Result<i64, SerializationError>

    func decode_double(&self) : std::Result<double, SerializationError>

    func decode_float(&self) : std::Result<float, SerializationError>

    func decode_str_of_len(&self, l : u64) : std::Result<std::string, SerializationError>

    func decode_str(&self) : std::Result<std::string, SerializationError>

    func decode_bytes(&self) : std::Result<std::vector<u8>, SerializationError>

    func array(&self) : std::Result<ArrayDecoder<T>, SerializationError>

    func object(&self) : std::Result<ObjectDecoder<T>, SerializationError>

    func map(&self) : std::Result<MapDecoder<T>, SerializationError>

}

interface ArrayDecoder<T> {

    func <K : Deserializer<T>> decode(&self) : std::Result<K, SerializationError>

    func remaining(&self) : std::Result<u64, SerializationError>

}

interface ObjectDecoder<T> {

    func <V : Deserializer<T>> field(&self, name : *char) : std::Result<V, SerializationError>

    func remaining(&self) : std::Result<u64, SerializationError>

}

interface MapDecoder<T> {

    func remaining(&self) : std::Result<u64, SerializationError>

    func decode_null(&self) : std::Result<Unit, SerializationError>

    func decode_bool(&self) : std::Result<bool, SerializationError>

    func decode_char(&self) : std::Result<char, SerializationError>

    func decode_u64(&self) : std::Result<u64, SerializationError>

    func decode_i64(&self) : std::Result<i64, SerializationError>

    func decode_float(&self) : std::Result<float, SerializationError>

    func decode_double(&self) : std::Result<double, SerializationError>

    func decode_str(&self) : std::Result<std::string, SerializationError>

    func decode_str_of_len(&self, l : u64) : std::Result<std::string, SerializationError>

    func decode_bytes(&self) : std::Result<std::vector<u8>, SerializationError>

}

interface Deserializer<T> {

    func deserialize(&self, decoder : Decoder<T>) : std::Result<Unit, SerializationError>

}

}
