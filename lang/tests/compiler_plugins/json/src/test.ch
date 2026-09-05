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

// namespaced struct: #json(ns::Type) must resolve through the namespace
public namespace jmns {
    public struct JMNs {
        var x : int
        var tag : std::string
    }
}

#json(jmns::JMNs)

// char / uchar fields: chars are single-char JSON strings on both sides
struct JMChar {
    var a : char
    var b : uchar
}

#json(JMChar)

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
public func json_macro_namespaced_struct_roundtrip(env : &mut TestEnv) {
    // #json(jmns::JMNs) must resolve through the namespace and generate the
    // impls for the namespaced type (mangled under its real scope).
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var p = jmns::JMNs { x : 7, tag : std::string("ns") }
    var r = p.serialize(&encoder)
    if(!(r is std::Result.Ok)) { env.error("namespaced serialize returned Err"); return }
    if(!buffer.to_view().equals(std::string_view("{\"x\":7,\"tag\":\"ns\"}"))) {
        env.error("namespaced macro exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { env.error("namespaced parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<jmns::JMNs>()
    if(res is std::Result.Err) { env.error("decode<jmns::JMNs> returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.x != 7 || !v.tag.to_view().equals(std::string_view("ns"))) {
        env.error("jmns::JMNs roundtrip mismatch")
    }
}

@test
public func json_macro_char_struct_roundtrip(env : &mut TestEnv) {
    // chars must encode as single-char JSON strings (not numbers) and decode
    // back through decode_char - including chars that need JSON escaping.
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var p = JMChar { a : '"', b : 'z' as uchar }
    var r = p.serialize(&encoder)
    if(!(r is std::Result.Ok)) { env.error("char serialize returned Err"); return }
    if(!buffer.to_view().equals(std::string_view("{\"a\":\"\\\"\",\"b\":\"z\"}"))) {
        env.error("char macro exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { env.error("char parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMChar>()
    if(res is std::Result.Err) { env.error("decode<JMChar> returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.a != '"' || v.b != 'z' as uchar) {
        env.error("JMChar roundtrip mismatch")
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

// ============================================================================
// Destructible structs (owning members + explicit @delete destructors).
//
// #json must support structs whose members (or nested member structs) own
// resources:
//   - encode passes members by REFERENCE into ObjectEncoder.field (the field
//     value must never be moved out of &self or copied), and
//   - decode MOVES the Ok payload out of each intermediate Result with
//     take_ok (leaving the Result in an Err state) so destructible values can
//     be moved into the final aggregate without copying and without the source
//     being destroyed again.
// A successful round trip here means: no "cannot move this value without
// re-initializing memory" compile errors, correct JSON, and the destructor
// counters reach EXACTLY the number of constructed objects - any extra call
// means the macro (or runtime) copied or double-destroyed a value.
// ============================================================================

// Destructor call counters. Each @test runs in its own process, so these are
// deterministic per test. A round trip must construct exactly N objects and
// run their destructors exactly N times: any EXTRA call means the macro (or
// runtime) copied or double-destroyed a value; any fewer means a leak.
var jmd_point_dtors : int = 0
var jmd_child_dtors : int = 0

struct JMDPoint {
    var x : int
    var y : int

    @delete
    func delete(&mut self) {
        // explicit destructor over trivial members - the whole struct moves
        // into/out of the Result payloads without any member copies
        jmd_point_dtors++
    }
}

#json(JMDPoint)

struct JMDChild {
    var num : int
    var tag : std::string

    @delete
    func delete(&mut self) {
        // destructor body runs; owning members (tag) are destroyed by codegen
        jmd_child_dtors++
    }
}

#json(JMDChild)

// no own @delete: destruction is the implicit member teardown, which must
// destroy `child` exactly once per JMDHolder instance (counted via JMDChild)
struct JMDHolder {
    var id : long
    var child : JMDChild
    var active : bool
}

#json(JMDHolder)

// round trips run inside helpers so every local (including the decoded value)
// is destroyed before the caller inspects the destructor counters
func run_dtor_scalar_roundtrip() : int {
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var v = JMDPoint { x : 10, y : -20 }
    var r = v.serialize(&encoder)
    if(!(r is std::Result.Ok)) { return 1 }
    if(!buffer.to_view().equals(std::string_view("{\"x\":10,\"y\":-20}"))) { return 2 }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { return 3 }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMDPoint>()
    if(res is std::Result.Err) { return 4 }
    var Ok(out) = res else unreachable
    if(out.x != 10 || out.y != -20) { return 5 }
    return 0
}

@test
public func json_macro_destructor_scalar_struct_roundtrip(env : &mut TestEnv) {
    var before = jmd_point_dtors
    var rc = run_dtor_scalar_roundtrip()
    if(rc != 0) { env.error("JMDPoint roundtrip failed"); return }
    // exactly 2 objects were constructed (the encode source + the decoded
    // value); encode borrows members by reference and decode moves payloads
    // out of their Results, so the destructor must run exactly twice
    var got = jmd_point_dtors - before
    if(got > 2) { env.error("JMDPoint destructor called MORE than expected - a copy or double-destroy happened"); return }
    if(got < 2) { env.error("JMDPoint destructor called FEWER times than expected - a value leaked") }
}

func run_dtor_owning_roundtrip() : int {
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var v = JMDChild { num : 5, tag : std::string("world") }
    var r = v.serialize(&encoder)
    if(!(r is std::Result.Ok)) { return 1 }
    if(!buffer.to_view().equals(std::string_view("{\"num\":5,\"tag\":\"world\"}"))) { return 2 }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { return 3 }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMDChild>()
    if(res is std::Result.Err) { return 4 }
    var Ok(out) = res else unreachable
    if(out.num != 5) { return 5 }
    if(!out.tag.to_view().equals(std::string_view("world"))) { return 6 }
    return 0
}

@test
public func json_macro_owning_string_struct_roundtrip(env : &mut TestEnv) {
    var before = jmd_child_dtors
    var rc = run_dtor_owning_roundtrip()
    if(rc != 0) { env.error("JMDChild owning roundtrip failed"); return }
    // 2 objects: the encode source + the decoded value. decode goes through
    // take_ok (Result left in Err state), so the owning tag is freed exactly
    // once per object - any extra call means the payload was copied/double-freed
    var got = jmd_child_dtors - before
    if(got > 2) { env.error("JMDChild destructor called MORE than expected - copy or double-destroy of the owning payload"); return }
    if(got < 2) { env.error("JMDChild destructor called FEWER times than expected - a value leaked") }
}

func run_dtor_nested_roundtrip() : int {
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var v = JMDHolder { id : 7, child : JMDChild { num : 3, tag : std::string("hello") }, active : true }
    var r = v.serialize(&encoder)
    if(!(r is std::Result.Ok)) { return 1 }
    if(!buffer.to_view().equals(std::string_view("{\"id\":7,\"child\":{\"num\":3,\"tag\":\"hello\"},\"active\":true}"))) { return 2 }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { return 3 }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMDHolder>()
    if(res is std::Result.Err) { return 4 }
    var Ok(out) = res else unreachable
    if(out.id != 7 || out.child.num != 3 || !out.active) { return 5 }
    if(!out.child.tag.to_view().equals(std::string_view("hello"))) { return 6 }
    return 0
}

// enum fields: serialize as the member NAME string ("Red") and decode back by
// matching the name hash; an unknown member name must decode to an Err
enum JMColor {
    Red,
    Green,
    Blue,
}

struct JMEnumPoint {
    var name : std::string
    var color : JMColor
    var x : int
    var y : int
}

#json(JMEnumPoint)

@test
public func json_macro_enum_struct_roundtrip(env : &mut TestEnv) {
    var buffer = std::string()
    var counts = std::vector<u64>()
    var encoder = JsonEncoder { buffer : &raw mut buffer, counts : &raw mut counts }
    var p = JMEnumPoint { name : std::string("pt"), color : JMColor.Blue, x : 3, y : -4 }
    var r = p.serialize(&encoder)
    if(!(r is std::Result.Ok)) { env.error("enum serialize returned Err"); return }
    if(!buffer.to_view().equals(std::string_view("{\"name\":\"pt\",\"color\":\"Blue\",\"x\":3,\"y\":-4}"))) {
        env.error("enum macro exact JSON text mismatch")
        return
    }

    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var pr = parser.parse(buffer.data(), buffer.size(), &mut ph)
    if(!pr.ok) { env.error("enum parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMEnumPoint>()
    if(res is std::Result.Err) { env.error("decode<JMEnumPoint> returned Err"); return }
    var Ok(v) = res else unreachable
    if(v.x != 3 || v.y != -4) { env.error("JMEnumPoint coords mismatch"); return }
    if(!(v.color == JMColor.Blue)) { env.error("JMEnumPoint color mismatch (expected Blue)"); return }
    if(!v.name.to_view().equals(std::string_view("pt"))) { env.error("JMEnumPoint name mismatch") }
}

@test
public func json_macro_enum_unknown_name_rejected(env : &mut TestEnv) {
    // a member name with no matching enum value must produce a decode error
    var ph = ASTJsonHandler()
    var parser = JsonParser(256, 8192)
    var doc = std::string_view("{\"name\":\"pt\",\"color\":\"Purple\",\"x\":1,\"y\":2}")
    var pr = parser.parse(doc.data(), doc.size(), &mut ph)
    if(!pr.ok) { env.error("unknown enum name parse failed"); return }
    var d = JsonDecoder { value : &ph.root }
    var res = d.decode<JMEnumPoint>()
    if(!(res is std::Result.Err)) { env.error("unknown enum member name decode should be Err") }
}

@test
public func json_macro_nested_destructible_roundtrip(env : &mut TestEnv) {
    var before = jmd_child_dtors
    var rc = run_dtor_nested_roundtrip()
    if(rc != 0) { env.error("JMDHolder nested roundtrip failed"); return }
    // 2 JMDChild objects: the one inside the encode source and the one inside
    // the decoded value. The nested child is moved out of its Result with
    // take_ok and moved into the parent literal - its destructor must run
    // exactly twice overall (once per parent teardown), never three times.
    var got = jmd_child_dtors - before
    if(got > 2) { env.error("JMDChild destructor called MORE than expected - nested payload copied or double-destroyed"); return }
    if(got < 2) { env.error("JMDChild destructor called FEWER times than expected - a nested value leaked") }
}
