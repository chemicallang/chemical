// Safe CBOR accessors: owned Option values, no raw pointers.
//
// Covers empty containers, non-container receivers, out-of-range indices,
// unicode text keys, and malformed input (rejected by the decoder before any
// accessor is involved).

using std::Option;
using std::Result;
using std::string_view;
using std::vector;

@test
func test_safe_map_value_by_int_owned(env : &mut TestEnv) {
    // {1: 2, 3: 4}
    var d = try_decode("a201020304")
    if(d is Result.Err) {
        env.error("safe map fixture did not decode")
        return
    }
    var Ok(v) = d else unsafe { unreachable }

    var got = cbor::map_value_by_int(&v, 1)
    if(got is Option.None) {
        env.error("safe integer-key lookup should find key 1")
        return
    }
    var Some(val) = got else unsafe { unreachable }
    var n = cbor::as_u64(val)
    if(n is Option.None) {
        env.error("key 1 should hold an unsigned integer")
    } else {
        var Some(count) = n else unsafe { unreachable }
        if(count != 2u64) { env.error("key 1 should be 2") }
    }

    if(cbor::map_value_by_int(&v, 9) is Option.Some) {
        env.error("absent integer key should return None")
    }
}

@test
func test_safe_map_value_by_text_and_unicode_key(env : &mut TestEnv) {
    // {"水": 7}
    var d = try_decode("a163e6b0b407")
    if(d is Result.Err) {
        env.error("unicode-key map fixture did not decode")
        return
    }
    var Ok(v) = d else unsafe { unreachable }

    var got = cbor::map_value_by_text(&v, string_view("水"))
    if(got is Option.None) {
        env.error("safe text-key lookup should find the unicode key")
        return
    }
    var Some(val) = got else unsafe { unreachable }
    var n = cbor::as_u64(val)
    if(n is Option.None) {
        env.error("unicode-key value should be an integer")
    } else {
        var Some(count) = n else unsafe { unreachable }
        if(count != 7u64) { env.error("unicode-key value should be 7") }
    }

    if(cbor::map_value_by_text(&v, string_view("missing")) is Option.Some) {
        env.error("absent text key should return None")
    }
}

@test
func test_safe_map_value_rejects_non_map_and_empty_map(env : &mut TestEnv) {
    // An unsigned integer is not a map.
    var d = try_decode("01")
    if(d is Result.Err) {
        env.error("scalar fixture did not decode")
        return
    }
    var Ok(v) = d else unsafe { unreachable }
    if(cbor::map_value_by_int(&v, 1) is Option.Some) {
        env.error("a scalar receiver must return None")
    }
    if(cbor::map_value_by_text(&v, string_view("k")) is Option.Some) {
        env.error("a scalar receiver must return None for text keys")
    }

    // An empty map has no pairs.
    var e = try_decode("a0")
    if(e is Result.Err) {
        env.error("empty map fixture did not decode")
        return
    }
    var Ok(empty) = e else unsafe { unreachable }
    if(cbor::map_value_by_int(&empty, 1) is Option.Some) {
        env.error("an empty map must return None")
    }
}

@test
func test_safe_array_value_at_owned(env : &mut TestEnv) {
    // [1, 2, 3]
    var d = try_decode("83010203")
    if(d is Result.Err) {
        env.error("array fixture did not decode")
        return
    }
    var Ok(v) = d else unsafe { unreachable }

    var first = cbor::array_value_at(&v, 0u)
    if(first is Option.None) {
        env.error("array index 0 should be present")
        return
    }
    var Some(val) = first else unsafe { unreachable }
    var n = cbor::as_u64(val)
    if(n is Option.None) {
        env.error("array element 0 should be an integer")
    } else {
        var Some(one) = n else unsafe { unreachable }
        if(one != 1u64) { env.error("array element 0 should be 1") }
    }

    if(cbor::array_value_at(&v, 3u) is Option.Some) {
        env.error("out-of-range array index should return None")
    }
    if(cbor::array_value_at(&v, 99u) is Option.Some) {
        env.error("far out-of-range array index should return None")
    }
}

@test
func test_safe_array_value_rejects_non_array(env : &mut TestEnv) {
    var d = try_decode("a0")
    if(d is Result.Err) {
        env.error("map fixture did not decode")
        return
    }
    var Ok(v) = d else unsafe { unreachable }
    if(cbor::array_value_at(&v, 0u) is Option.Some) {
        env.error("a non-array receiver must return None")
    }
}

@test
func test_safe_accessors_after_malformed_decode_are_not_reachable(env : &mut TestEnv) {
    // Malformed input is rejected by `decode`; there is no CborValue to pass to
    // an accessor. This pins the boundary: safety starts at successful decode.
    var bad = try_decode("ff")
    if(bad is Result.Ok) {
        env.error("a stray break must be rejected")
    }
    var truncated = try_decode("4401")
    if(truncated is Result.Ok) {
        env.error("a truncated byte string must be rejected")
    }
}
