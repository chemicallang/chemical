// Handwritten (no #json plugin) Serializer/Deserializer impls for user structs,
// plus the @test round-trip/error tests in the second half of this file.
// Structs + impls: JStructScalar (all scalar types), JChild/JParent (nested,
// string fields), JTwoInt (error paths).

// ============ wide scalar struct ============
// Field order matters: encode writes fields in this order, decode consumes the
// JSON object pairs positionally in the same order (keys are not matched by name).

struct JStructScalar {
    var i : int
    var u : uint
    var lg : long
    var ulg : ulong
    var b : bool
    var c : char
    var f : float
    var d : double
    var s : std::string
}

impl std::Serializer<JsonValue, JsonEncoder> for JStructScalar {
    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        var o = encoder.object()
        var f1 = o.field<int>(std::string_view("i"), &i)
        if(f1 is std::Result.Err) {
            var Err(error) = f1 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f2 = o.field<uint>(std::string_view("u"), &u)
        if(f2 is std::Result.Err) {
            var Err(error) = f2 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f3 = o.field<long>(std::string_view("lg"), &lg)
        if(f3 is std::Result.Err) {
            var Err(error) = f3 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f4 = o.field<ulong>(std::string_view("ulg"), &ulg)
        if(f4 is std::Result.Err) {
            var Err(error) = f4 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f5 = o.field<bool>(std::string_view("b"), &b)
        if(f5 is std::Result.Err) {
            var Err(error) = f5 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f6 = o.field<char>(std::string_view("c"), &c)
        if(f6 is std::Result.Err) {
            var Err(error) = f6 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f7 = o.field<float>(std::string_view("f"), &f)
        if(f7 is std::Result.Err) {
            var Err(error) = f7 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f8 = o.field<double>(std::string_view("d"), &d)
        if(f8 is std::Result.Err) {
            var Err(error) = f8 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f9 = o.field<std::string>(std::string_view("s"), &s)
        if(f9 is std::Result.Err) {
            var Err(error) = f9 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit {})
    }
}

impl std::Deserializer<JStructScalar> for TypeDecoder<JStructScalar> {
    func deserialize(&self) : std::Result<JStructScalar, std::SerializationError> {
        var dr = self.decoder.object()
        if(dr is std::Result.Err) {
            var Err(error) = dr else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(obj) = dr else unreachable

        var q1 = obj.item_decoder()
        if(q1 is std::Result.Err) {
            var Err(error) = q1 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p1) = q1 else unreachable
        var v1 = p1.second.decode_i64()
        if(v1 is std::Result.Err) {
            var Err(error) = v1 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r1) = v1 else unreachable

        var q2 = obj.item_decoder()
        if(q2 is std::Result.Err) {
            var Err(error) = q2 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p2) = q2 else unreachable
        var v2 = p2.second.decode_u64()
        if(v2 is std::Result.Err) {
            var Err(error) = v2 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r2) = v2 else unreachable

        var q3 = obj.item_decoder()
        if(q3 is std::Result.Err) {
            var Err(error) = q3 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p3) = q3 else unreachable
        var v3 = p3.second.decode_i64()
        if(v3 is std::Result.Err) {
            var Err(error) = v3 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r3) = v3 else unreachable

        var q4 = obj.item_decoder()
        if(q4 is std::Result.Err) {
            var Err(error) = q4 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p4) = q4 else unreachable
        var v4 = p4.second.decode_u64()
        if(v4 is std::Result.Err) {
            var Err(error) = v4 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r4) = v4 else unreachable

        var q5 = obj.item_decoder()
        if(q5 is std::Result.Err) {
            var Err(error) = q5 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p5) = q5 else unreachable
        var v5 = p5.second.decode_bool()
        if(v5 is std::Result.Err) {
            var Err(error) = v5 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r5) = v5 else unreachable

        var q6 = obj.item_decoder()
        if(q6 is std::Result.Err) {
            var Err(error) = q6 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p6) = q6 else unreachable
        var v6 = p6.second.decode_char()
        if(v6 is std::Result.Err) {
            var Err(error) = v6 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r6) = v6 else unreachable

        var q7 = obj.item_decoder()
        if(q7 is std::Result.Err) {
            var Err(error) = q7 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p7) = q7 else unreachable
        var v7 = p7.second.decode_float()
        if(v7 is std::Result.Err) {
            var Err(error) = v7 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r7) = v7 else unreachable

        var q8 = obj.item_decoder()
        if(q8 is std::Result.Err) {
            var Err(error) = q8 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p8) = q8 else unreachable
        var v8 = p8.second.decode_double()
        if(v8 is std::Result.Err) {
            var Err(error) = v8 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r8) = v8 else unreachable

        var q9 = obj.item_decoder()
        if(q9 is std::Result.Err) {
            var Err(error) = q9 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p9) = q9 else unreachable
        var v9 = p9.second.decode_str()
        if(v9 is std::Result.Err) {
            var Err(error) = v9 else unreachable
            return std::Result.Err<JStructScalar, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r9) = v9 else unreachable

        return std::Result.Ok<JStructScalar, std::SerializationError>(JStructScalar { i : r1 as int, u : r2 as uint, lg : r3 as long, ulg : r4 as ulong, b : r5, c : r6, f : r7, d : r8, s : std::string(r9) })
    }
}

// ============ nested structs ============

struct JChild {
    var num : int
    var tag : std::string

    func copy(&self) : JChild {
        return JChild { num : num, tag : tag.copy() }
    }
}

impl std::Serializer<JsonValue, JsonEncoder> for JChild {
    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        var o = encoder.object()
        var f1 = o.field<int>(std::string_view("num"), &num)
        if(f1 is std::Result.Err) {
            var Err(error) = f1 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f2 = o.field<std::string>(std::string_view("tag"), &tag)
        if(f2 is std::Result.Err) {
            var Err(error) = f2 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit {})
    }
}

impl std::Deserializer<JChild> for TypeDecoder<JChild> {
    func deserialize(&self) : std::Result<JChild, std::SerializationError> {
        var dr = self.decoder.object()
        if(dr is std::Result.Err) {
            var Err(error) = dr else unreachable
            return std::Result.Err<JChild, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(obj) = dr else unreachable

        var q1 = obj.item_decoder()
        if(q1 is std::Result.Err) {
            var Err(error) = q1 else unreachable
            return std::Result.Err<JChild, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p1) = q1 else unreachable
        var v1 = p1.second.decode_i64()
        if(v1 is std::Result.Err) {
            var Err(error) = v1 else unreachable
            return std::Result.Err<JChild, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r1) = v1 else unreachable

        var q2 = obj.item_decoder()
        if(q2 is std::Result.Err) {
            var Err(error) = q2 else unreachable
            return std::Result.Err<JChild, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p2) = q2 else unreachable
        var v2 = p2.second.decode_str()
        if(v2 is std::Result.Err) {
            var Err(error) = v2 else unreachable
            return std::Result.Err<JChild, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r2) = v2 else unreachable

        return std::Result.Ok<JChild, std::SerializationError>(JChild { num : r1 as int, tag : std::string(r2) })
    }
}

struct JParent {
    var id : long
    var child : JChild
    var active : bool
}

impl std::Serializer<JsonValue, JsonEncoder> for JParent {
    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        var o = encoder.object()
        var f1 = o.field<long>(std::string_view("id"), &id)
        if(f1 is std::Result.Err) {
            var Err(error) = f1 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f2 = o.field<JChild>(std::string_view("child"), &child)
        if(f2 is std::Result.Err) {
            var Err(error) = f2 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f3 = o.field<bool>(std::string_view("active"), &active)
        if(f3 is std::Result.Err) {
            var Err(error) = f3 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit {})
    }
}

impl std::Deserializer<JParent> for TypeDecoder<JParent> {
    func deserialize(&self) : std::Result<JParent, std::SerializationError> {
        var dr = self.decoder.object()
        if(dr is std::Result.Err) {
            var Err(error) = dr else unreachable
            return std::Result.Err<JParent, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(obj) = dr else unreachable

        var q1 = obj.item_decoder()
        if(q1 is std::Result.Err) {
            var Err(error) = q1 else unreachable
            return std::Result.Err<JParent, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p1) = q1 else unreachable
        var v1 = p1.second.decode_i64()
        if(v1 is std::Result.Err) {
            var Err(error) = v1 else unreachable
            return std::Result.Err<JParent, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r1) = v1 else unreachable

        var q2 = obj.item_decoder()
        if(q2 is std::Result.Err) {
            var Err(error) = q2 else unreachable
            return std::Result.Err<JParent, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p2) = q2 else unreachable
        var v2 = p2.second.decode<JChild>()
        if(v2 is std::Result.Err) {
            var Err(error) = v2 else unreachable
            return std::Result.Err<JParent, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r2) = v2 else unreachable

        var q3 = obj.item_decoder()
        if(q3 is std::Result.Err) {
            var Err(error) = q3 else unreachable
            return std::Result.Err<JParent, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p3) = q3 else unreachable
        var v3 = p3.second.decode_bool()
        if(v3 is std::Result.Err) {
            var Err(error) = v3 else unreachable
            return std::Result.Err<JParent, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r3) = v3 else unreachable

        return std::Result.Ok<JParent, std::SerializationError>(JParent { id : r1 as long, child : r2.copy(), active : r3 })
    }
}

// ============ 2-int struct for error-path checks ============

struct JTwoInt {
    var a : int
    var b : int
}

impl std::Serializer<JsonValue, JsonEncoder> for JTwoInt {
    func serialize(&self, encoder : &JsonEncoder) : std::Result<std::Unit, std::SerializationError> {
        var o = encoder.object()
        var f1 = o.field<int>(std::string_view("a"), &a)
        if(f1 is std::Result.Err) {
            var Err(error) = f1 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var f2 = o.field<int>(std::string_view("b"), &b)
        if(f2 is std::Result.Err) {
            var Err(error) = f2 else unreachable
            return std::Result.Err<std::Unit, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        return std::Result.Ok<std::Unit, std::SerializationError>(std::Unit {})
    }
}

impl std::Deserializer<JTwoInt> for TypeDecoder<JTwoInt> {
    func deserialize(&self) : std::Result<JTwoInt, std::SerializationError> {
        var dr = self.decoder.object()
        if(dr is std::Result.Err) {
            var Err(error) = dr else unreachable
            return std::Result.Err<JTwoInt, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(obj) = dr else unreachable

        var q1 = obj.item_decoder()
        if(q1 is std::Result.Err) {
            var Err(error) = q1 else unreachable
            return std::Result.Err<JTwoInt, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p1) = q1 else unreachable
        var v1 = p1.second.decode_i64()
        if(v1 is std::Result.Err) {
            var Err(error) = v1 else unreachable
            return std::Result.Err<JTwoInt, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r1) = v1 else unreachable

        var q2 = obj.item_decoder()
        if(q2 is std::Result.Err) {
            var Err(error) = q2 else unreachable
            return std::Result.Err<JTwoInt, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(p2) = q2 else unreachable
        var v2 = p2.second.decode_i64()
        if(v2 is std::Result.Err) {
            var Err(error) = v2 else unreachable
            return std::Result.Err<JTwoInt, std::SerializationError>(__non_gen_se_repl(&mut error, std::SerializationError()))
        }
        var Ok(r2) = v2 else unreachable

        return std::Result.Ok<JTwoInt, std::SerializationError>(JTwoInt { a : r1 as int, b : r2 as int })
    }
}

// ============================================================================
// Handwritten-impl struct serialization tests (NO #json macro).
// These structs implement std::Serializer<JsonValue, JsonEncoder> and
// std::Deserializer<T> for TypeDecoder<T> by hand, mirroring exactly what the
// json_cbi #json macro generates - so they pin down the runtime contract the
// macro (and any manual impl) must satisfy.
//
// Notes on the shape of a handwritten impl (Chemical move/copy rules):
//   - Encoding: ObjectEncoder.field takes its value BY REFERENCE
//     (std::ObjectEncoder.field is `value : &V`), so each member is passed as
//     &self.field - no copy is required even for owning/destructible members
//     (moving a member OUT of &self by value would be illegal).
//   - Decoding an owning scalar field: bind a *view* (decode_str ->
//     std::string_view) and construct the owning value inline (std::string(r));
//     a pattern-bound owning local cannot be moved into a struct literal.
//     Composite fields (decode<T>) move their Ok payload out with json's
//     take_ok helper; these manual impls instead use .copy() where the type
//     provides one - both avoid moving a pattern-bound value onward.
//   - Error propagation uses the non-generic json::__non_gen_se_repl helper
//     (generic std::replace from impl code corrupts under nested generic
//     instantiation - see .agents/skills/json_serialization/SKILL.md).
//   - Field order is positional: encode writes members in declaration order;
//     decode consumes JSON object pairs in the same order, ignoring key names.
// ============================================================================

// ===== scalar struct (all supported scalar field types) =====

@test
func test_manual_json_impl_scalar_struct_roundtrip(env : &mut TestEnv) {
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var val = JStructScalar { i : -42, u : 4000000000u, lg : -7, ulg : 99u64, b : true, c : 'A', f : 1.5f, d : -2.25, s : std::string("hello") }
    var e = encoder.encode<JStructScalar>(val)
    if(!(e is std::Result.Ok)) { env.error("scalar encode returned Err"); return }
    if(!output.to_view().equals(std::string_view("{\"i\":-42,\"u\":4000000000,\"lg\":-7,\"ulg\":99,\"b\":true,\"c\":\"A\",\"f\":1.5,\"d\":-2.25,\"s\":\"hello\"}"))) {
        env.error("scalar exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(output.data(), output.size(), &mut ph)
    if(!pr.ok) { env.error("parse failed"); return }
    var dec = JsonDecoder { value : &ph.root }
    var res = dec.decode<JStructScalar>()
    if(res is std::Result.Err) { env.error("scalar decode returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.i != -42) { env.error("scalar i roundtrip failed"); return }
    if(v.u != 4000000000u) { env.error("scalar u roundtrip failed"); return }
    if(v.lg != -7) { env.error("scalar lg roundtrip failed"); return }
    if(v.ulg != 99u64) { env.error("scalar ulg roundtrip failed"); return }
    if(!v.b) { env.error("scalar b roundtrip failed"); return }
    if(v.c != 'A') { env.error("scalar c roundtrip failed"); return }
    if(v.f != 1.5f) { env.error("scalar f roundtrip failed"); return }
    if(v.d != -2.25) { env.error("scalar d roundtrip failed"); return }
    if(!v.s.to_view().equals(std::string_view("hello"))) { env.error("scalar s roundtrip failed") }
}

@test
func test_manual_json_impl_scalar_struct_direct_serialize(env : &mut TestEnv) {
    // Call the impl method directly (val.serialize(&encoder)) instead of the
    // generic encode<T> extension entry point - same impl, different dispatch.
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var val = JStructScalar { i : 7, u : 9u, lg : 8, ulg : 10u64, b : false, c : 'Z', f : 0.5f, d : 2.5, s : std::string("direct") }
    var e = val.serialize(&encoder)
    if(!(e is std::Result.Ok)) { env.error("direct serialize returned Err"); return }
    if(!output.to_view().equals(std::string_view("{\"i\":7,\"u\":9,\"lg\":8,\"ulg\":10,\"b\":false,\"c\":\"Z\",\"f\":0.5,\"d\":2.5,\"s\":\"direct\"}"))) {
        env.error("direct serialize JSON text mismatch")
    }
}

@test
func test_manual_json_impl_nested_struct_roundtrip(env : &mut TestEnv) {
    // Nested decode goes through the generic decode<T>() dispatch chain
    // (decode -> decode_it_2 -> decode_it_1 -> TypeDecoder<T>.deserialize).
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var val = JParent { id : 7, child : JChild { num : 3, tag : std::string("x") }, active : true }
    var e = encoder.encode<JParent>(val)
    if(!(e is std::Result.Ok)) { env.error("nested encode returned Err"); return }
    if(!output.to_view().equals(std::string_view("{\"id\":7,\"child\":{\"num\":3,\"tag\":\"x\"},\"active\":true}"))) {
        env.error("nested exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(output.data(), output.size(), &mut ph)
    if(!pr.ok) { env.error("parse failed"); return }
    var dec = JsonDecoder { value : &ph.root }
    var res = dec.decode<JParent>()
    if(res is std::Result.Err) { env.error("nested decode returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.id != 7) { env.error("nested id roundtrip failed"); return }
    if(v.child.num != 3) { env.error("nested child.num roundtrip failed"); return }
    if(!v.child.tag.to_view().equals(std::string_view("x"))) { env.error("nested child.tag roundtrip failed"); return }
    if(!v.active) { env.error("nested active roundtrip failed") }
}

@test
func test_manual_json_impl_string_escapes_roundtrip(env : &mut TestEnv) {
    // Quotes, backslashes and newlines in an owned std::string field must be
    // JSON-escaped on encode and restored exactly on decode.
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var tag_val = std::string("a\"b\\c\nd")
    var val = JChild { num : 1, tag : tag_val.copy() }
    var e = encoder.encode<JChild>(val)
    if(!(e is std::Result.Ok)) { env.error("escape encode returned Err"); return }
    if(!output.to_view().equals(std::string_view("{\"num\":1,\"tag\":\"a\\\"b\\\\c\\nd\"}"))) {
        env.error("escape exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(output.data(), output.size(), &mut ph)
    if(!pr.ok) { env.error("parse failed"); return }
    var dec = JsonDecoder { value : &ph.root }
    var res = dec.decode<JChild>()
    if(res is std::Result.Err) { env.error("escape decode returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.num != 1) { env.error("escape num roundtrip failed"); return }
    if(!v.tag.to_view().equals(tag_val.to_view())) {
        env.error("escape string roundtrip equality failed")
    }
}

@test
func test_manual_json_impl_generic_string_encode_decode(env : &mut TestEnv) {
    // std::string round-trips through the lib-provided Serializer/Deserializer
    // impls (json types.ch) via the generic encode<T>/decode<T> entry points.
    var output = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut output, counts : &raw mut counts }
    var r = encoder.encode<std::string>(std::string("hi"))
    if(!(r is std::Result.Ok)) { env.error("encode<std::string> returned Err"); return }
    if(!output.to_view().equals(std::string_view("\"hi\""))) {
        env.error("encode string JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("\"hello\"")
    var pr = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!pr.ok) { env.error("decode string parse failed"); return }
    var dec = JsonDecoder { value : &ph.root }
    var res = dec.decode<std::string>()
    if(res is std::Result.Err) { env.error("decode<std::string> returned Err"); return }
    var Ok(v) = res else unreachable
    if(!v.to_view().equals(std::string_view("hello"))) {
        env.error("decode string value mismatch")
    }
}

@test
func test_manual_json_impl_decode_rejects_non_object_root(env : &mut TestEnv) {
    // Deserialize must call decoder.object() first and propagate its Err when
    // the root JSON value is not an object.
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var r = parser.parse("5", 1, &mut ph)
    if(!r.ok) { env.error("root number parse failed"); return }
    var dec = JsonDecoder { value : &ph.root }
    var res = dec.decode<JTwoInt>()
    if(!(res is std::Result.Err)) { env.error("root number decode should be Err"); return }

    var ph2 = ASTJsonHandler()
    var parser2 = JsonParser(128, 4096)
    var r2 = parser2.parse("[1,2]", 5, &mut ph2)
    if(!r2.ok) { env.error("root array parse failed"); return }
    var dec2 = JsonDecoder { value : &ph2.root }
    var res2 = dec2.decode<JTwoInt>()
    if(!(res2 is std::Result.Err)) { env.error("root array decode should be Err") }
}

@test
func test_manual_json_impl_decode_field_type_mismatch(env : &mut TestEnv) {
    // A field whose JSON value doesn't match the expected primitive decode
    // (int field given a string) must surface as Err, not garbage.
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("{\"a\":\"not a number\",\"b\":2}")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("type mismatch parse failed"); return }
    var dec = JsonDecoder { value : &ph.root }
    var res = dec.decode<JTwoInt>()
    if(!(res is std::Result.Err)) { env.error("type mismatch decode should be Err") }
}

@test
func test_manual_json_impl_decode_missing_field(env : &mut TestEnv) {
    // Fewer object pairs than struct fields: item_decoder() runs out -> Err.
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("{\"a\":1}")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("missing field parse failed"); return }
    var dec = JsonDecoder { value : &ph.root }
    var res = dec.decode<JTwoInt>()
    if(!(res is std::Result.Err)) { env.error("missing field decode should be Err") }
}

@test
func test_manual_json_impl_decode_extra_fields_ignored(env : &mut TestEnv) {
    // Decode consumes exactly the first N object pairs positionally; extra
    // trailing pairs are ignored (key names are not validated).
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("{\"a\":11,\"b\":22,\"extra\":true,\"other\":9}")
    var r = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!r.ok) { env.error("extra fields parse failed"); return }
    var dec = JsonDecoder { value : &ph.root }
    var res = dec.decode<JTwoInt>()
    if(res is std::Result.Err) { env.error("extra fields decode should be Ok"); return }
    var Ok(v) = res else unreachable
    if(v.a != 11 || v.b != 22) {
        env.error("extra fields positional decode mismatch")
    }
}
