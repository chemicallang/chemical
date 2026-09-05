// ===== High-Level API =====
// The beginner-friendly entry points for the json library.
//
//   var value = json::parse(text)              // Result<JsonValue, JsonParseError>
//   var text  = json::stringify(&value)        // compact JSON
//   var text  = json::stringify_pretty(&value) // indented JSON
//
//   // typed (works with structs annotated #json)
//   var s = json::encode<Point>(&point)        // Result<string, SerializationError>
//   var p = json::decode<Point>(&value)        // Result<Point, SerializationError>
//
// The lower-level machinery (JsonParser/JsonSaxHandler, JsonEncoder/JsonDecoder,
// TypeDecoder, ASTJsonHandler) remains available for streaming / manual work.
//
// The functions below live in `public namespace json` (like archive::), so they
// are called as json::parse / json::stringify / json::encode<T> / json::decode<T>.
// The pre-existing types (JsonValue, JsonEncoder, ...) stay module-top-level and
// are used unqualified for backwards compatibility.
public namespace json {

public struct JsonParseError {
    var pos : size_t
    var message : std::string
}

// parse JSON text into a JsonValue tree
public func parse(text : std::string_view) : std::Result<JsonValue, JsonParseError> {
    var handler = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var r = parser.parse(text.data(), text.size(), &mut handler)
    if(!r.ok) {
        return std::Result.Err<JsonValue, JsonParseError>(JsonParseError {
            pos : r.pos,
            message : std::string(r.msg)
        })
    }
    // move the root out of the handler, leaving it as Null
    var root = std::replace(&mut handler.root, JsonValue.Null())
    return std::Result.Ok<JsonValue, JsonParseError>(root)
}

// serialize a JsonValue to compact JSON text
public func stringify(value : &JsonValue) : std::string {
    var output = std::string()
    output.append_value(value)
    return output
}

// serialize a JsonValue to pretty-printed JSON text
public func stringify_pretty(value : &JsonValue) : std::string {
    var output = std::string()
    output.append_value_pretty(value)
    return output
}

// typed encode: serialize any Serializer type (including #json structs) to JSON text
public func <T : std::Serializer<JsonValue, JsonEncoder>> encode(value : &T) : std::Result<std::string, std::SerializationError> {
    var output = std::string()
    var counts = __new_counts()
    var encoder = __new_encoder(&mut output, &mut counts)
    var r = value.serialize(__unsafe_cast_json_encoder<JsonValue>(&encoder)) as std::Result<std::Unit, std::SerializationError>
    if(r is std::Result.Ok) {
        // move the buffer out (leave an empty string behind)
        var out = std::replace(&mut output, std::string())
        return std::Result.Ok<std::string, std::SerializationError>(out)
    }
    // NOTE: extracting the Err payload of a Result inside a generic function is
    // not possible (the payload binds as the generic E and can't be passed to
    // non-generic code), so the message is static here. The lower-level
    // container API (ObjectEncoder.field etc.) still propagates the real error.
    return std::Result.Err<std::string, std::SerializationError>(se_err("encode failed"))
}

// non-generic helpers: create the encoder state outside generic functions -
// &raw mut of generic-instantiation locals (vector<u64>) inside a generic
// function hits a type-alias quirk, so the creation lives in a plain function
public func __new_counts() : std::vector<u64> {
    return std::vector<u64>()
}

public func __new_encoder(buffer : &mut std::string, counts : &mut std::vector<u64>) : JsonEncoder {
    return JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
}

// typed decode: deserialize any Deserializer type (including #json structs) from a JsonValue
public func <T> decode(value : &JsonValue) : std::Result<T, std::SerializationError> {
    var d = JsonDecoder { value : value }
    return d.decode<T>()
}

// parse + typed decode in one call:
//   var p = json::decode_str<Point>(text) // Result<Point, SerializationError>
public func <T> decode_str(text : std::string_view) : std::Result<T, std::SerializationError> {
    // Pattern-matching a Result inside a generic function mis-types the bound
    // payload (the compiler can't link the concrete variant member), so the
    // parse + payload extraction happens in the NON-generic __try_parse helper,
    // which returns an owned Option<JsonValue>.
    var opt = __try_parse(text)
    if(opt is std::Option.None) {
        return std::Result.Err<T, std::SerializationError>(se_err("json parse error"))
    }
    // Option.take moves the parsed JsonValue out (opt -> None), giving an owned
    // value that stays alive for the decode call below
    var value = opt.take()
    var d = JsonDecoder { value : &value }
    return d.decode<T>()
}

} // end namespace json

// non-generic parse helper used by json::decode_str: parses the text and moves
// the root JsonValue out of the parse Result on success (None on parse error,
// whose detail is dropped at this layer). Lives OUTSIDE the generic decode_str
// so its pattern binds happen in a non-generic context (where they link).
public func __try_parse(text : std::string_view) : std::Option<JsonValue> {
    var pr = json::parse(text)
    if(pr is std::Result.Err) {
        return std::Option.None<JsonValue>()
    }
    var Ok(v) = pr else unreachable
    return std::Option.Some<JsonValue>(std::replace(&mut v, JsonValue.Null()))
}
