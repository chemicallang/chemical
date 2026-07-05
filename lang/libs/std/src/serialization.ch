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

}
