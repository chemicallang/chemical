// json_cbi (#json macro) plugin tests.
//
// These exercise the full macro pipeline - parse -> symres (impl generation,
// indexing) -> codegen -> runtime round-trip - for a few struct shapes:
//   - JMPoint:    plain int fields
//   - JMChild/JMParent: nested struct (decode<T> dispatch through the macro
//     generated TypeDecoder<Child> impl)
//   - JMWide:     every scalar family the macro maps (signed/unsigned ints,
//     bool, float, double)
//   - JMTwo:      error paths (non-object root, type mismatch, missing field)
//
// Ordering rule: #json(Child) must appear before #json(Parent) - the parent's
// generated code needs the child's Serializer/Deserializer impl indexed when
// it is instantiated (see .agents/skills/json_serialization/SKILL.md).
//
// Entry points under test:
//   - encode: p.serialize(&encoder)   (the generated impl method)
//   - decode: d.decode<T>()           (generic dispatch -> TypeDecoder<T>)

struct JMPoint {
    var x : int
    var y : int
}

#json(JMPoint)

struct JMChild {
    var a : int
    var b : int
}

#json(JMChild)

struct JMParent {
    var x : int
    var c : JMChild
    var ok : bool
}

#json(JMParent)

struct JMWide {
    var i : int
    var lg : long
    var u : uint
    var b : bool
    var f : float
    var d : double
}

#json(JMWide)

struct JMTwo {
    var a : int
    var b : int
}

#json(JMTwo)

@test
public func json_macro_scalar_struct_roundtrip(env : &mut TestEnv) {
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var p = JMPoint { x : 10, y : -20 }
    var r = p.serialize(&encoder)
    if(!(r is std::Result.Ok)) { env.error("serialize returned Err"); return }
    if(!buffer.to_view().equals(std::string_view("{\"x\":10,\"y\":-20}"))) {
        env.error("scalar macro exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { env.error("scalar macro parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMPoint>()
    if(res is std::Result.Err) { env.error("decode<JMPoint> returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.x != 10 || v.y != -20) {
        env.error("JMPoint roundtrip mismatch")
    }
}

@test
public func json_macro_nested_struct_roundtrip(env : &mut TestEnv) {
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var p = JMParent { x : 1, c : JMChild { a : 3, b : 4 }, ok : true }
    var r = p.serialize(&encoder)
    if(!(r is std::Result.Ok)) { env.error("nested serialize returned Err"); return }
    if(!buffer.to_view().equals(std::string_view("{\"x\":1,\"c\":{\"a\":3,\"b\":4},\"ok\":true}"))) {
        env.error("nested macro exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { env.error("nested macro parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMParent>()
    if(res is std::Result.Err) { env.error("decode<JMParent> returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.x != 1 || v.c.a != 3 || v.c.b != 4 || !v.ok) {
        env.error("JMParent nested roundtrip mismatch")
    }
}

@test
public func json_macro_wide_scalar_struct_roundtrip(env : &mut TestEnv) {
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var p = JMWide { i : -42, lg : -7, u : 4000000000u, b : true, f : 1.5f, d : -2.25 }
    var r = p.serialize(&encoder)
    if(!(r is std::Result.Ok)) { env.error("wide serialize returned Err"); return }
    if(!buffer.to_view().equals(std::string_view("{\"i\":-42,\"lg\":-7,\"u\":4000000000,\"b\":true,\"f\":1.5,\"d\":-2.25}"))) {
        env.error("wide macro exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { env.error("wide macro parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMWide>()
    if(res is std::Result.Err) { env.error("decode<JMWide> returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.i != -42 || v.lg != -7 || v.u != 4000000000u || !v.b || v.f != 1.5f || v.d != -2.25) {
        env.error("JMWide roundtrip mismatch")
    }
}

@test
public func json_macro_decode_rejects_non_object_root(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var pr = parser.parse("5", 1, &mut ph)
    if(!pr.ok) { env.error("root number parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMTwo>()
    if(!(res is std::Result.Err)) { env.error("root number decode should be Err"); return }

    var ph2 = ASTJsonHandler()
    var parser2 = JsonParser(128, 4096)
    var pr2 = parser2.parse("[1,2]", 5, &mut ph2)
    if(!pr2.ok) { env.error("root array parse failed"); return }
    var d2 = JsonDecoder { value : &ph2.root }
    var res2 = d2.decode<JMTwo>()
    if(!(res2 is std::Result.Err)) { env.error("root array decode should be Err") }
}

@test
public func json_macro_decode_field_type_mismatch(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("{\"a\":\"not a number\",\"b\":2}")
    var pr = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!pr.ok) { env.error("type mismatch parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMTwo>()
    if(!(res is std::Result.Err)) { env.error("type mismatch decode should be Err") }
}

@test
public func json_macro_decode_missing_field(env : &mut TestEnv) {
    var ph = ASTJsonHandler()
    var parser = JsonParser(128, 4096)
    var doc = std::string_view("{\"a\":1}")
    var pr = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!pr.ok) { env.error("missing field parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMTwo>()
    if(!(res is std::Result.Err)) { env.error("missing field decode should be Err") }
}
