public namespace core {

struct Unit {}

enum SerializationErrorKind {

}

struct SerializationError {
    var kind : SerializationErrorKind
    var message : std::string
}

interface Encoder<T> {

    func encode_null(&self) : Result<Unit, SerializationError>

    func encode_bool(&self, v : bool) : Result<Unit, SerializationError>

    func encode_char(&self, c : char) : Result<Unit, SerializationError>

    func encode_u64(&self, i : u64) : Result<Unit, SerializationError>

    func encode_i64(&self, i : i64) : Result<Unit, SerializationError>

    func encode_double(&self, d : double) : Result<Unit, SerializationError>

    func encode_float(&self, f : float) : Result<Unit, SerializationError>

    func encode_str_of_len(&self, c : *char, l : u64) : Result<Unit, SerializationError>

    func encode_str(&self, c : *char) : Result<Unit, SerializationError>

    func encode_bytes(b : *u8, l : u64) : Result<Unit, SerializationError>

    func array(&self) : ArrayEncoder<T>

    func array_of_len(&self, len : u64) : ArrayEncoder<T>

    func object(&self) : ObjectEncoder<T>

    func object_of_len(&self, len : u64) : ObjectEncoder<T>

    func map(&self) : MapEncoder<T>

    func map_of_len(&self, len : u64) : MapEncoder<T>

}

interface ArrayEncoder<T> {

    func <K : Serializer<T>> encode(&self, value : K) : Result<Unit, SerializationError>

}

interface ObjectEncoder<T> {

    func <V : Serializer<T>> field(&self, name : *char, value : V) : Result<Unit, SerializationError>

}

interface MapEncoder<T> {

    func <K : Serializer<T>, V : Serializer<T>> encode(&self, key : K, value : V) : Result<Unit, SerializationError>

}

interface Serializer<T> {

    func encode(&self, encoder : Encoder<T>) : Result<Unit, SerializationError>

}

// Decoder

interface Decoder<T> {

    func decode_null(&self) : Result<Unit, SerializationError>

    func decode_bool(&self) : Result<bool, SerializationError>

    func decode_char(&self) : Result<char, SerializationError>

    func decode_u64(&self) : Result<u64, SerializationError>

    func decode_i64(&self) : Result<i64, SerializationError>

    func decode_double(&self) : Result<double, SerializationError>

    func decode_float(&self) : Result<float, SerializationError>

    func decode_str_of_len(&self, l : u64) : Result<std::string, SerializationError>

    func decode_str(&self) : Result<std::string, SerializationError>

    func decode_bytes(&self) : Result<vector<u8>, SerializationError>

    func array(&self) : Result<ArrayDecoder<T>, SerializationError>

    func object(&self) : Result<ObjectDecoder<T>, SerializationError>

    func map(&self) : Result<MapDecoder<T>, SerializationError>

}

interface ArrayDecoder<T> {

    func <K : Deserializer<T>> decode(&self) : Result<K, SerializationError>

    func remaining(&self) : Result<u64, SerializationError>

}

interface ObjectDecoder<T> {

    func <V : Deserializer<T>> field(&self, name : *char) : Result<V, SerializationError>

    func remaining(&self) : Result<u64, SerializationError>

}

interface MapDecoder<T> {

    func <K : Deserializer<T>, V : Deserializer<T>> decode(&self) : Result<(K, V), SerializationError>

    func remaining(&self) : Result<u64, SerializationError>

}

interface Deserializer<T> {

    func deserialize(&self, decoder : Decoder<T>) : Result<Unit, SerializationError>

}

}
