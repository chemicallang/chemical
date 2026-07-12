@test
func test_minimal_json_decodes_fine(env : &mut TestEnv) {
    var input = std::string_view("{\"name\": \"Alice Bob\", \"age\": 30, \"active\": true, \"tags\": [null, false, 1e-2]}");
    var output = std::string_view("{\"name\": \"Alice Bob\", \"age\": 30, \"active\": true, \"tags\": [null, false, 1e-2]}");
    test_parsed_json_equals(env, &input, &output)
}

@test
func test_array_value_decodes_fine(env : &mut TestEnv) {
    var input = std::string_view("[\"Hendrix Lua\", 29, false]");
    var output = std::string_view("[\"Hendrix Lua\",29,false]");
    test_parsed_json_equals(env, &input, &output)
}

@test
func test_escaped_strings(env : &mut TestEnv) {
    var input = std::string_view("{\"escapes\": \"\\n\\t\\\"\\\\\\/\\b\\f\\r\"}");
    var output = std::string_view("{\"escapes\":\"\\n\\t\\\"\\\\/\\b\\f\\r\"}");
    test_parsed_json_equals(env, &input, &output)
}

@test
func test_unicode_surrogates(env : &mut TestEnv) {
    var input = std::string_view("{\"emoji\": \"\\uD83D\\uDE00\"}");
    var output = std::string_view("{\"emoji\":\"\xf0\x9f\x98\x80\"}");
    test_parsed_json_equals(env, &input, &output)
}

@test
func test_complex_nesting(env : &mut TestEnv) {
    var input = std::string_view("[null,true,{\"a\":[1.0,2.0]},[[]],{}]");
    test_parsed_json_equals(env, &input, &input)
}

@test
func test_large_number(env : &mut TestEnv) {
    var input = std::string_view("123456789012345678901234567890");
    test_parsed_json_equals(env, &input, &input)
}

@test
func test_invalid_trailing_comma(env : &mut TestEnv) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(256, 8192);
    var doc = std::string_view("{\"a\":1,}");
    var r = parser.parse(doc.data(), doc.size(), &mut ph);
    if (r.ok) {
        env.error("Expected error for trailing comma");
    }
}

@test
func test_object_in_array_succeeds(env : &mut TestEnv) {
    var input = std::string_view("[{ \"name\" : \"Criminal Joe\" }, 999, true]");
    var output = std::string_view("[{\"name\":\"Criminal Joe\"},999,true]");
    test_parsed_json_equals(env, &input, &output)
}

@test
func test_number_formats(env : &mut TestEnv) {
    const tests = [
        "0",
        "-0",
        "123",
        "-123",
        "1.23",
        "-1.23",
        "1.23e4",
        "1.23E4",
        "1.23e+4",
        "1.23E-4",
        "-1.23e-4"
    ];
    const tests_view = std::span<*char>(tests)
    for (var i = 0; i < tests_view.size(); i++) {
        var input = std::string_view(tests[i]);
        test_parsed_json_equals(env, &input, &input);
    }
}

@test
func test_empty_containers(env : &mut TestEnv) {
    test_parsed_json_equals(env, std::string_view("{}"), std::string_view("{}"));
    test_parsed_json_equals(env, std::string_view("[]"), std::string_view("[]"));
}

@test
func test_whitespace_tolerance(env : &mut TestEnv) {
    var input = std::string_view("  {  \"key\"  :  [  1  ,  2  ]  }  ");
    var output = std::string_view("{\"key\":[1,2]}");
    test_parsed_json_equals(env, &input, &output);
}

@test
func test_invalid_trailing_data(env : &mut TestEnv) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(128, 4096);
    var doc = std::string_view("{\"a\":1} trailing");
    var r = parser.parse(doc.data(), doc.size(), &mut ph);
    if (r.ok) {
        env.error("Expected error for trailing data");
    }
}

@test
func test_invalid_missing_value(env : &mut TestEnv) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(128, 4096);
    var doc = std::string_view("[1, , 2]");
    var r = parser.parse(doc.data(), doc.size(), &mut ph);
    if (r.ok) {
        env.error("Expected error for missing value in array");
    }
}

@test
func test_form_urlencoded_fails_gracefully(env : &mut TestEnv) {
    var ph = ASTJsonHandler();
    var parser = JsonParser(128, 4096);
    var doc = std::string_view("name=howla&username=howla&email=howla%40gmail.com&password=howla&confirm_password=howla");
    var r = parser.parse(doc.data(), doc.size(), &mut ph);
    if (r.ok) {
        env.error("Expected error for form-urlencoded data being parsed as JSON");
    } else {
        if (!std::string_view(r.msg).equals("invalid literal")) {
             env.error(r.msg);
        }
    }
}

// ===== Encoding Tests (encode_json) =====

@test
func test_encode_json_null(env : &mut TestEnv) {
    var value = JsonValue.Null()
    var result = encode_json(&value)
    if(!result.to_view().equals(std::string_view("null"))) {
        env.error("encode_json(Null) failed")
    }
}

@test
func test_encode_json_bool(env : &mut TestEnv) {
    var t = JsonValue.Bool(true)
    var result_t = encode_json(&t)
    if(!result_t.to_view().equals(std::string_view("true"))) {
        env.error("encode_json(true) failed")
    }
    var f = JsonValue.Bool(false)
    var result_f = encode_json(&f)
    if(!result_f.to_view().equals(std::string_view("false"))) {
        env.error("encode_json(false) failed")
    }
}

@test
func test_encode_json_number(env : &mut TestEnv) {
    var n = JsonValue.Number(std::string("42"))
    var result = encode_json(&n)
    if(!result.to_view().equals(std::string_view("42"))) {
        env.error("encode_json(Number 42) failed")
    }
    var neg = JsonValue.Number(std::string("-7"))
    var result_neg = encode_json(&neg)
    if(!result_neg.to_view().equals(std::string_view("-7"))) {
        env.error("encode_json(Number -7) failed")
    }
    var flt = JsonValue.Number(std::string("3.14"))
    var result_flt = encode_json(&flt)
    if(!result_flt.to_view().equals(std::string_view("3.14"))) {
        env.error("encode_json(Number 3.14) failed")
    }
}

@test
func test_encode_json_string(env : &mut TestEnv) {
    var s = JsonValue.String(std::string("hello"))
    var result = encode_json(&s)
    if(!result.to_view().equals(std::string_view("\"hello\""))) {
        env.error("encode_json(\"hello\") failed")
    }
}

@test
func test_encode_json_string_escaped(env : &mut TestEnv) {
    var s = JsonValue.String(std::string("a\"b\\c"))
    var result = encode_json(&s)
    if(!result.to_view().equals(std::string_view("\"a\\\"b\\\\c\""))) {
        env.error("encode_json(escaped string) failed")
    }
}

@test
func test_encode_json_array(env : &mut TestEnv) {
    var arr = std::vector<JsonValue>()
    arr.push(JsonValue.Null())
    arr.push(JsonValue.Bool(true))
    arr.push(JsonValue.Number(std::string("123")))
    var v = JsonValue.Array(arr)
    var result = encode_json(&v)
    if(!result.to_view().equals(std::string_view("[null,true,123]"))) {
        env.error("encode_json([null,true,123]) failed")
    }
}

@test
func test_encode_json_empty_array(env : &mut TestEnv) {
    var arr = std::vector<JsonValue>()
    var v = JsonValue.Array(arr)
    var result = encode_json(&v)
    if(!result.to_view().equals(std::string_view("[]"))) {
        env.error("encode_json([]) failed")
    }
}

@test
func test_encode_json_object(env : &mut TestEnv) {
    var map = std::unordered_map<std::string, JsonValue>()
    map.insert(std::string("a"), JsonValue.Number(std::string("1")))
    map.insert(std::string("b"), JsonValue.String(std::string("two")))
    var v = JsonValue.Object(map)
    var result = encode_json(&v)
    var view = result.to_view()
    if(!view.equals(std::string_view("{\"a\":1,\"b\":\"two\"}")) &&
       !view.equals(std::string_view("{\"b\":\"two\",\"a\":1}"))) {
        env.error("encode_json(object) failed")
    }
}

@test
func test_encode_json_empty_object(env : &mut TestEnv) {
    var map = std::unordered_map<std::string, JsonValue>()
    var v = JsonValue.Object(map)
    var result = encode_json(&v)
    if(!result.to_view().equals(std::string_view("{}"))) {
        env.error("encode_json({}) failed")
    }
}

@test
func test_encode_json_nested(env : &mut TestEnv) {
    var inner_arr = std::vector<JsonValue>()
    inner_arr.push(JsonValue.Bool(false))
    inner_arr.push(JsonValue.Null())
    var inner = JsonValue.Array(inner_arr)

    var map = std::unordered_map<std::string, JsonValue>()
    map.insert(std::string("data"), inner)
    map.insert(std::string("count"), JsonValue.Number(std::string("3")))

    var v = JsonValue.Object(map)
    var result = encode_json(&v)
    var view = result.to_view()
    if(!view.equals(std::string_view("{\"data\":[false,null],\"count\":3}")) &&
       !view.equals(std::string_view("{\"count\":3,\"data\":[false,null]}"))) {
        env.error("encode_json(nested) failed")
    }
}

// ===== Direct Encoder Method Tests =====

@test
func test_encoder_encode_null(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_null()
    if(!(r is std::Result.Ok)) { env.error("encode_null returned error"); return }
    if(!output.to_view().equals(std::string_view("null"))) {
        env.error("encoder.encode_null() failed")
    }
}

@test
func test_encoder_encode_bool(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_bool(true)
    if(!(r is std::Result.Ok)) { env.error("encode_bool returned error"); return }
    if(!output.to_view().equals(std::string_view("true"))) {
        env.error("encoder.encode_bool(true) failed")
    }
}

@test
func test_encoder_encode_u64(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_u64(42u64)
    if(!(r is std::Result.Ok)) { env.error("encode_u64 returned error"); return }
    if(!output.to_view().equals(std::string_view("42"))) {
        env.error("encoder.encode_u64(42) failed")
    }
}

@test
func test_encoder_encode_u64_zero(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_u64(0u64)
    if(!(r is std::Result.Ok)) { env.error("encode_u64 returned error"); return }
    if(!output.to_view().equals(std::string_view("0"))) {
        env.error("encoder.encode_u64(0) failed")
    }
}

@test
func test_encoder_encode_i64(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_i64(-42i64)
    if(!(r is std::Result.Ok)) { env.error("encode_i64 returned error"); return }
    if(!output.to_view().equals(std::string_view("-42"))) {
        env.error("encoder.encode_i64(-42) failed")
    }
}

@test
func test_encoder_encode_i64_negative_zero(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_i64(0i64)
    if(!(r is std::Result.Ok)) { env.error("encode_i64 returned error"); return }
    if(!output.to_view().equals(std::string_view("0"))) {
        env.error("encoder.encode_i64(0) failed")
    }
}

@test
func test_encoder_encode_i64_min(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_i64(-9223372036854775807i64 - 1i64)
    if(!(r is std::Result.Ok)) { env.error("encode_i64 returned error"); return }
    if(!output.to_view().equals(std::string_view("-9223372036854775808"))) {
        env.error("encoder.encode_i64(INT64_MIN) failed")
    }
}

@test
func test_encoder_encode_char(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_char('A')
    if(!(r is std::Result.Ok)) { env.error("encode_char returned error"); return }
    if(!output.to_view().equals(std::string_view("\"A\""))) {
        env.error("encoder.encode_char('A') failed")
    }
}

@test
func test_encoder_encode_str(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_str("hello world")
    if(!(r is std::Result.Ok)) { env.error("encode_str returned error"); return }
    if(!output.to_view().equals(std::string_view("\"hello world\""))) {
        env.error("encoder.encode_str(\"hello world\") failed")
    }
}

@test
func test_encoder_encode_str_escaped(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_str("a\"b\\c\nd")
    if(!(r is std::Result.Ok)) { env.error("encode_str returned error"); return }
    if(!output.to_view().equals(std::string_view("\"a\\\"b\\\\c\\nd\""))) {
        env.error("encoder.encode_str(escaped) failed")
    }
}

@test
func test_encoder_encode_str_of_len(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode_str_of_len("abc", 3u64)
    if(!(r is std::Result.Ok)) { env.error("encode_str_of_len returned error"); return }
    if(!output.to_view().equals(std::string_view("\"abc\""))) {
        env.error("encoder.encode_str_of_len(\"abc\", 3) failed")
    }
}

// ===== Generic Encoder Method Tests =====

@test
func test_encoder_encode_generic_bool(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode<bool>(true)
    if(!(r is std::Result.Ok)) { env.error("encoder.encode returned error"); return }
    if(!output.to_view().equals(std::string_view("true"))) {
        env.error("encoder.encode(true) failed")
    }
}

@test
func test_encoder_encode_generic_uint(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode<uint>(42u)
    if(!(r is std::Result.Ok)) { env.error("encoder.encode returned error"); return }
    if(!output.to_view().equals(std::string_view("42"))) {
        env.error("encoder.encode(42) failed")
    }
}

@test
func test_encoder_encode_generic_float(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode<float>(3.14f)
    if(!(r is std::Result.Ok)) { env.error("encoder.encode returned error"); return }
    if(!output.to_view().equals(std::string_view("3.14"))) {
        env.error("encoder.encode(3.14f) failed")
    }
}

@test
func test_encoder_encode_generic_double(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode<double>(3.14)
    if(!(r is std::Result.Ok)) { env.error("encoder.encode returned error"); return }
    if(!output.to_view().equals(std::string_view("3.14"))) {
        env.error("encoder.encode(3.14) failed")
    }
}

@test
func test_encoder_encode_generic_string_view(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode<std::string_view>(std::string_view("hello world"))
    if(!(r is std::Result.Ok)) { env.error("encoder.encode returned error"); return }
    if(!output.to_view().equals(std::string_view("\"hello world\""))) {
        env.error("encoder.encode(\"hello world\") failed")
    }
}

// ===== Direct Decoder Method Tests =====

@test
func test_decoder_decode_bool(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("true")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_bool()
    if(!(val is std::Result.Ok)) { env.error("decode_bool error"); return }
    var Ok(v) = val else unreachable
    if(!v) { env.error("expected true") }
}

@test
func test_decoder_decode_i64(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("42")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_i64()
    if(!(val is std::Result.Ok)) { env.error("decode_i64 error"); return }
    var Ok(v) = val else unreachable
    if(v != 42i64) { env.error("expected 42") }
}

@test
func test_decoder_decode_i64_negative(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("-7")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_i64()
    if(!(val is std::Result.Ok)) { env.error("decode_i64 error"); return }
    var Ok(v) = val else unreachable
    if(v != -7i64) { env.error("expected -7") }
}

@test
func test_decoder_decode_u64(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("99")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_u64()
    if(!(val is std::Result.Ok)) { env.error("decode_u64 error"); return }
    var Ok(v) = val else unreachable
    if(v != 99u64) { env.error("expected 99") }
}

@test
func test_decoder_decode_double(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("3.14")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_double()
    if(!(val is std::Result.Ok)) { env.error("decode_double error"); return }
    var Ok(v) = val else unreachable
    if(v < 3.13 || v > 3.15) { env.error("expected ~3.14") }
}

@test
func test_decoder_decode_float(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("2.5")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_float()
    if(!(val is std::Result.Ok)) { env.error("decode_float error"); return }
    var Ok(v) = val else unreachable
    if(v < 2.49f || v > 2.51f) { env.error("expected ~2.5") }
}

@test
func test_decoder_decode_char(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("\"A\"")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_char()
    if(!(val is std::Result.Ok)) { env.error("decode_char error"); return }
    var Ok(v) = val else unreachable
    if(v != 'A') { env.error("expected 'A'") }
}

@test
func test_decoder_decode_str(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("\"hello\"")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_str()
    if(!(val is std::Result.Ok)) { env.error("decode_str error"); return }
    var Ok(v) = val else unreachable
    if(!v.equals(std::string_view("hello"))) { env.error("expected hello") }
}

@test
func test_decoder_decode_null(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("null")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var val = d.decode_null()
    if(!(val is std::Result.Ok)) { env.error("decode_null error") }
}

// ===== Roundtrip Tests =====

@test
func test_encode_json_roundtrip_simple(env : &mut TestEnv) {
    var input = std::string_view("{\"name\":\"Alice\",\"age\":30,\"tags\":[1,2,3]}")
    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var r = parser.parse(input.data(), input.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var encoded = encode_json(&ph.root)
    var ph2 = ASTJsonHandler()
    var parser2 = JsonParser(256, 8192)
    var r2 = parser2.parse(encoded.data(), encoded.size(), &mut ph2)
    if(!r2.ok) { env.error("roundtrip re-parse failed"); return }
    var encoded2 = encode_json(&ph2.root)
    if(!encoded.to_view().equals(encoded2.to_view())) {
        env.error("roundtrip mismatch")
    }
}

@test
func test_encode_json_roundtrip_nested(env : &mut TestEnv) {
    var input = std::string_view("{\"a\":[1,null,{\"b\":false}],\"c\":\"d\"}")
    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var r = parser.parse(input.data(), input.size(), &mut ph)
    if(!r.ok) { env.error("parse failed"); return }
    var encoded = encode_json(&ph.root)
    var ph2 = ASTJsonHandler()
    var parser2 = JsonParser(256, 8192)
    var r2 = parser2.parse(encoded.data(), encoded.size(), &mut ph2)
    if(!r2.ok) { env.error("roundtrip re-parse failed"); return }
    var encoded2 = encode_json(&ph2.root)
    if(!encoded.to_view().equals(encoded2.to_view())) {
        env.error("roundtrip mismatch for nested")
    }
}

@test
func test_double_encode_decode_number(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("42")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("parse 42 failed"); return }
    if(!(ph.root is JsonValue.Number)) { env.error("expected Number variant"); return }
    var encoded = encode_json(&ph.root)

    // also check via direct encoder
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    encoder.encode_i64(42i64)
    if(!output.to_view().equals(std::string_view("42"))) {
        env.error("encode_i64(42) via encoder failed")
    }
}
