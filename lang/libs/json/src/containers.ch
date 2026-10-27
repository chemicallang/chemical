// Generic std-container serialization.
//
// These are top-level generic impls (proposal 0003), so a container only gets a
// concrete impl when a serializer/deserializer is actually demanded for that
// element type. Unused instantiations emit nothing.

// ---------------------------------------------------------------------------
// vector<T> — JSON array
// ---------------------------------------------------------------------------

impl<T : std::Serializer<JsonValue, JsonEncoder>> std::Serializer<JsonValue, JsonEncoder> for std::vector<T> {

    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        // opens '[' and pushes the item counter; the destructor closes it
        var arr = encoder.array()
        var i = 0u
        while(i < self.size()) {
            // `get_ptr` is a method of vector<T>. Its return type carries the
            // vector's own `T`, not this impl's `T`, so bridge it explicitly:
            // the cast also gives the pointer the Serializer bound.
            var item = self.get_ptr(i) as *mut T
            __container_begin_item(encoder.buffer, encoder.counts)
            var itemenc = JsonEncoder { buffer : encoder.buffer, counts : encoder.counts }
            var r = item.serialize(__unsafe_cast_json_encoder<JsonValue>(&itemenc))
            if(r is std::Result.Err) {
                return r
            }
            i = i + 1u
        }
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit {})
    }
}

impl<T> std::Deserializer<std::vector<T>> for TypeDecoder<std::vector<T>> {

    func deserialize(&self) : std::Result<std::vector<T>, std::SerializationError> {
        var arr_r = self.decoder.array()
        if(arr_r is std::Result.Err) {
            return std::Result.Err<std::vector<T>, std::SerializationError>(se_err("expected array"))
        }
        // positional destructuring (the payload parameter is named `value`)
        var Ok(arr) = arr_r else unreachable
        var out = std::vector<T>()
        var total = arr.total()
        var i = 0u
        while(i < total) {
            var item_r = arr.item_decoder()
            if(item_r is std::Result.Err) {
                return std::Result.Err<std::vector<T>, std::SerializationError>(se_err("expected array item"))
            }
            var Ok(dec) = item_r else unreachable
            var val_r = dec.decode<T>()
            if(val_r is std::Result.Err) {
                return std::Result.Err<std::vector<T>, std::SerializationError>(se_err("invalid array element"))
            }
            // take_ok moves the payload out of the Result so it can be pushed
            out.push(take_ok<T>(&mut val_r))
            i = i + 1u
        }
        return std::Result.Ok<std::vector<T>, std::SerializationError>(out)
    }
}

// ---------------------------------------------------------------------------
// span<T> — encode only (a span borrows; decoding cannot produce one)
// ---------------------------------------------------------------------------

impl<T : std::Serializer<JsonValue, JsonEncoder>> std::Serializer<JsonValue, JsonEncoder> for std::span<T> {

    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        var arr = encoder.array()
        var i = 0u
        while(i < self.size()) {
            var item = self.get(i) as *mut T
            __container_begin_item(encoder.buffer, encoder.counts)
            var itemenc = JsonEncoder { buffer : encoder.buffer, counts : encoder.counts }
            var r = item.serialize(__unsafe_cast_json_encoder<JsonValue>(&itemenc))
            if(r is std::Result.Err) {
                return r
            }
            i = i + 1u
        }
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit {})
    }
}

// ---------------------------------------------------------------------------
// ordered_map<std::string, V> — JSON object, insertion order preserved
// ---------------------------------------------------------------------------

impl<V : std::Serializer<JsonValue, JsonEncoder>> std::Serializer<JsonValue, JsonEncoder> for std::ordered_map<std::string, V> {
    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> { unsafe {
        var obj = encoder.object()
        var it = self.iterator()
        while(it.valid()) {
            var key = it.key() as *std::string
            var value = it.value() as *mut V
            __container_begin_item(encoder.buffer, encoder.counts)
            json_escape_into(&mut *encoder.buffer, key.data(), key.size() as u64)
            encoder.buffer.append(':')
            var venc = JsonEncoder { buffer : encoder.buffer, counts : encoder.counts }
            var r = value.serialize(__unsafe_cast_json_encoder<JsonValue>(&venc))
            if(r is std::Result.Err) {
                return r
            }
            it.next()
        }
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit {})
     }}
}

impl<V> std::Deserializer<std::ordered_map<std::string, V>> for TypeDecoder<std::ordered_map<std::string, V>> {
    func deserialize(&self) : std::Result<std::ordered_map<std::string, V>, std::SerializationError> {
        var map_r = self.decoder.map()
        if(map_r is std::Result.Err) {
            return std::Result.Err<std::ordered_map<std::string, V>, std::SerializationError>(se_err("expected object"))
        }
        var Ok(mapdec) = map_r else unreachable
        var out = std::ordered_map<std::string, V>()
        var total = mapdec.total()
        var i = 0u
        while(i < total) {
            var pair_r = mapdec.item_decoder()
            if(pair_r is std::Result.Err) {
                return std::Result.Err<std::ordered_map<std::string, V>, std::SerializationError>(se_err("expected object item"))
            }
            var Ok(pair) = pair_r else unreachable
            var key_r = pair.first.decode_str()
            if(key_r is std::Result.Err) {
                return std::Result.Err<std::ordered_map<std::string, V>, std::SerializationError>(se_err("expected object key"))
            }
            var Ok(key_view) = key_r else unreachable
            var val_r = pair.second.decode<V>()
            if(val_r is std::Result.Err) {
                return std::Result.Err<std::ordered_map<std::string, V>, std::SerializationError>(se_err("invalid object value"))
            }
            out.insert(std::string(key_view), take_ok<V>(&mut val_r))
            i = i + 1u
        }
        return std::Result.Ok<std::ordered_map<std::string, V>, std::SerializationError>(out)
    }
}

// ---------------------------------------------------------------------------
// unordered_map<std::string, V> — JSON object, order unspecified
// ---------------------------------------------------------------------------

impl<V : std::Serializer<JsonValue, JsonEncoder>> std::Serializer<JsonValue, JsonEncoder> for std::unordered_map<std::string, V> {
    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> { unsafe {
        var obj = encoder.object()
        var it = self.iterator()
        while(it.valid()) {
            var key = it.key() as *std::string
            var value = it.value() as *mut V
            __container_begin_item(encoder.buffer, encoder.counts)
            json_escape_into(&mut *encoder.buffer, key.data(), key.size() as u64)
            encoder.buffer.append(':')
            var venc = JsonEncoder { buffer : encoder.buffer, counts : encoder.counts }
            var r = value.serialize(__unsafe_cast_json_encoder<JsonValue>(&venc))
            if(r is std::Result.Err) {
                return r
            }
            it.next()
        }
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit {})
     }}
}

impl<V> std::Deserializer<std::unordered_map<std::string, V>> for TypeDecoder<std::unordered_map<std::string, V>> {
    func deserialize(&self) : std::Result<std::unordered_map<std::string, V>, std::SerializationError> {
        var map_r = self.decoder.map()
        if(map_r is std::Result.Err) {
            return std::Result.Err<std::unordered_map<std::string, V>, std::SerializationError>(se_err("expected object"))
        }
        var Ok(mapdec) = map_r else unreachable
        var out = std::unordered_map<std::string, V>()
        var total = mapdec.total()
        var i = 0u
        while(i < total) {
            var pair_r = mapdec.item_decoder()
            if(pair_r is std::Result.Err) {
                return std::Result.Err<std::unordered_map<std::string, V>, std::SerializationError>(se_err("expected object item"))
            }
            var Ok(pair) = pair_r else unreachable
            var key_r = pair.first.decode_str()
            if(key_r is std::Result.Err) {
                return std::Result.Err<std::unordered_map<std::string, V>, std::SerializationError>(se_err("expected object key"))
            }
            var Ok(key_view) = key_r else unreachable
            var val_r = pair.second.decode<V>()
            if(val_r is std::Result.Err) {
                return std::Result.Err<std::unordered_map<std::string, V>, std::SerializationError>(se_err("invalid object value"))
            }
            out.insert(std::string(key_view), take_ok<V>(&mut val_r))
            i = i + 1u
        }
        return std::Result.Ok<std::unordered_map<std::string, V>, std::SerializationError>(out)
    }
}
