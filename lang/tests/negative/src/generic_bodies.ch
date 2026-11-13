// Regression tests for compiler gaps that blocked the server/transport wave.
//
// Each test compiles and runs a small program with `NEG_MOD_ASYNC_STD`, which
// imports std/core/async (the `using` import shapes below need std in scope, and
// the async interface test needs the lowering backend).
//
// These are positive tests: the programs check their own behaviour and return a
// non-zero exit code when the compiler got the shape wrong.

// ---------------------------------------------------------------------------
// `using`-imported names inside generic bodies.
//
// The generic-decl link pass skipped `using` statements, so an unqualified
// `Option`/`string`/`vector` inside a generic body was unresolved during
// instantiation.
// ---------------------------------------------------------------------------
@test
func generic_body_resolves_using_imports(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using std::Option;\nusing std::string;\nusing std::vector;\n\npublic struct Chain { var id : int }\n\npublic func <'a> opt(value : &'a Chain) : Option<&'a Chain> {\n    return Option.Some(value)\n}\n\npublic func <T> build(v : T) : string {\n    var out = string()\n    out.append_view(\"v\")\n    var vec = vector<T>()\n    vec.push_back(v)\n    if(vec.size() == 1) { out.append_view(\"!\") }\n    return out\n}\n\npublic func main() : int {\n    var c = Chain { id : 7 }\n    var o = opt(&c)\n    var code = 3\n    if(o is Option.Some) {\n        var Some(v) = o else unsafe { unreachable }\n        if(v.id == 7) { code = 0 } else { code = 2 }\n    }\n    if(code != 0) { return code }\n    var s = build(5)\n    if(s.size() != 2) { return 4 }\n    return 0\n}\n"
    expect_compile_and_exit(env, "generic_body_using_imports", ch, NEG_MOD_ASYNC_STD, 0)
}

// ---------------------------------------------------------------------------
// Generic variant payloads inside generic bodies.
//
// A constructor call inside a generic body was left linked to the declaration,
// so a destructuring pattern bound the declaration's own parameter (`T`)
// instead of the argument, and `v.field` / `*v` failed (or was typed as `T`).
// ---------------------------------------------------------------------------
@test
func generic_variant_payload_is_substituted(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public variant Maybe2<T> {\n    Here(value : T)\n    Gone()\n}\n\npublic func <'a> explicit_arg(x : &'a int) : int {\n    var o = Maybe2.Here<&'a int>(x)\n    if(o is Maybe2.Here) {\n        var Here(v) = o else unsafe { unreachable }\n        return *v\n    }\n    return -1\n}\n\npublic func <'b> inferred_arg(x : &'b int) : int {\n    var o = Maybe2.Here(x)\n    if(o is Maybe2.Here) {\n        var Here(v) = o else unsafe { unreachable }\n        return *v\n    }\n    return -2\n}\n\npublic struct Holder<T> {\n    var item : Maybe2<T>\n\n    @constructor\n    func constructor(item_ : Maybe2<T>) {\n        return Holder<T> { item : item_ }\n    }\n\n    public func unwrap_or(&self, fallback : T) : T {\n        if(self.item is Maybe2.Here) {\n            var Here(v) = self.item else unsafe { unreachable }\n            return v\n        }\n        return fallback\n    }\n}\n\npublic func main() : int {\n    var x : int = 41\n    var a = explicit_arg(&x)\n    if(a != 41) { return 1 }\n    var b = inferred_arg(&x)\n    if(b != 41) { return 2 }\n    var h = Holder<int>(Maybe2.Here<int>(5))\n    if(h.unwrap_or(0) != 5) { return 3 }\n    return 0\n}\n"
    expect_compile_and_exit(env, "generic_variant_payload", ch, NEG_MOD_ASYNC_STD, 0)
}

// a general generic function still infers its argument inside a generic body
// (regression for the `Option<&'a Struct>` return shape)
@test
func generic_function_infers_payload_inside_generic_body(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using std::Option;\n\npublic struct Chain { var id : int }\n\npublic func <'a> option_of_ref(value : &'a Chain) : Option<&'a Chain> {\n    return Option.Some(value)\n}\n\npublic func <'a> read_it(value : &'a Chain) : int {\n    var o = option_of_ref(value)\n    if(o is Option.Some) {\n        var Some(v) = o else unsafe { unreachable }\n        return v.id\n    }\n    return -1\n}\n\npublic func main() : int {\n    var c = Chain { id : 7 }\n    var n = read_it(&c)\n    if(n != 7) { return 1 }\n    return 0\n}\n"
    expect_compile_and_exit(env, "generic_function_payload", ch, NEG_MOD_ASYNC_STD, 0)
}

// ---------------------------------------------------------------------------
// `&'a self` receivers.
//
// The parser rejected an explicit lifetime on the receiver, so a
// lifetime-generic method could not return or accept values borrowing `self`.
// ---------------------------------------------------------------------------
@test
func lifetime_generic_self_receiver(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using std::Option;\n\npublic struct Chain { var id : int }\n\n@direct_init\npublic struct Cfg<'a> {\n    internal var slot : Option<&'a Chain>\n\n    public func slot_ref(&'a self, value : &int) : int {\n        if(self.slot is Option.Some) {\n            var Some(v) = self.slot else unsafe { unreachable }\n            return v.id + *value\n        }\n        return -1\n    }\n\n    public func slot_value(&'a self) : Option<&'a Chain> {\n        return self.slot\n    }\n\n    public func set_slot(&'a mut self, value : Option<&'a Chain>) {\n        self.slot = value\n    }\n}\n\npublic func <'b> run(c : &'b Chain) : int {\n    var cfg = Cfg<'b> { slot : Option.None<&'b Chain>() }\n    cfg.set_slot(Option.Some<&'b Chain>(c))\n    var x : int = 1\n    var a = cfg.slot_ref(&x)\n    if(a != 8) { return 1 }\n    var o = cfg.slot_value()\n    var code = 4\n    if(o is Option.Some) {\n        var Some(v) = o else unsafe { unreachable }\n        if(v.id == 7) { code = 0 } else { code = 3 }\n    }\n    return code\n}\n\npublic func main() : int {\n    var c = Chain { id : 7 }\n    return run(&c)\n}\n"
    expect_compile_and_exit(env, "lifetime_self_receiver", ch, NEG_MOD_ASYNC_STD, 0)
}

// ---------------------------------------------------------------------------
// Async interface methods keep their future through a generic bound.
//
// An `async func` interface member's return type was not rewritten to
// `FutureHandle<T>` when reached through a generic `R : ARead` receiver, so
// `await r.read_it(...)` saw the inner `int` and failed to type check. This
// blocks `AsyncRead`/`AsyncWrite`.
// ---------------------------------------------------------------------------
@test
func async_interface_method_awaits_through_generic_bound(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "using std::vector;\n\npublic interface ARead {\n    async func read_it(&mut self, out : &mut vector<u8>) : int\n}\n\npublic struct Reader {\n    var total : int\n\n    @constructor\n    func constructor() {\n        return Reader { total : 0 }\n    }\n}\n\nimpl ARead for Reader {\n    async func read_it(&mut self, out : &mut vector<u8>) : int {\n        out.push_back(65u8)\n        self.total = self.total + 1\n        return self.total\n    }\n}\n\npublic async func <R> read_through(r : &mut R, out : &mut vector<u8>) : int where R : ARead {\n    var n = await r.read_it(out)\n    return n\n}\n\npublic func main() : int {\n    var r = Reader()\n    var out = vector<u8>()\n    var n = async::block_on<int>(read_through<Reader>(&mut r, &mut out))\n    if(n != 1) { return 1 }\n    if(out.size() != 1) { return 2 }\n    return 0\n}\n"
    expect_compile_and_exit(env, "async_interface_method", ch, NEG_MOD_ASYNC_STD, 0)
}

// a generic variant with a default type parameter keeps the default when the
// caller does not name a type (inference from a literal must not override it)
@test
func generic_variant_default_type_wins_over_literal_inference(env : &mut TestEnv) {
    unsafe { mkdir(NEG_WORK_DIR, 0o777 as uint) }
    var ch = "public variant DefVar<T = ulong> {\n    Some(value : T)\n    None()\n}\n\nfunc <T> is_ulong(value : T) : bool {\n    return T is ulong\n}\n\npublic func main() : int {\n    var v = DefVar.Some<>(10)\n    switch(v) {\n        Some(value) => {\n            if(!is_ulong(value)) { return 1 }\n        }\n        None => { return 2 }\n    }\n    return 0\n}\n"
    expect_compile_and_exit(env, "generic_variant_default_type", ch, NEG_MOD_ASYNC_STD, 0)
}
