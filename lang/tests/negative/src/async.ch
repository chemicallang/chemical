// Negative tests for the async/await surface.

@test
public func neg_await_outside_async_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func not_async() : int {\n    var v = await 1\n    return v\n}\npublic func main() : int {\n    return not_async()\n}\n"
    expect_compile_error(env, "await_outside_async", ch, "can only be used inside")
}

// Shared source for the coroutine end-to-end regression: multiple awaits and a
// destructor-bearing local across a suspension, plus a `std::string` result.
// This exercises the coroutine ramp, poll, drop (cancellation) and the
// destructible-local cleanup paths.
internal const NEG_CH_ASYNC_COROUTINE = "struct Countdown { var remaining : int; var value : int }\nfunc cd_poll(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<int> { var f = frame as *mut Countdown; if(f.remaining > 0) { f.remaining = f.remaining - 1; return core::async::Poll.Pending<int>() } return core::async::Poll.Ready<int>(f.value) }\nfunc cd_drop(frame : *mut void) { }\nfunc make_cd(remaining : int, value : int) : core::async::FutureHandle<int> { var f = malloc(sizeof(Countdown)) as *mut Countdown; f.remaining = remaining; f.value = value; var t = malloc(sizeof(core::async::FutureTable<int>)) as *mut core::async::FutureTable<int>; t.poll = cd_poll; t.drop = cd_drop; return core::async::FutureHandle<int> { frame : f as *mut void, vtbl : t } }\nasync func foo(x : int) : int { var s = std::string(\"hello\"); var a = await make_cd(2, x); var b = await make_cd(1, 1); return (s.size() as int) + a + b }\nasync func bar() : std::string { var n = await make_cd(1, 1); return std::string(\"hi\") }\npublic func main() : int { var r = async::block_on<int>(foo(36)); if(r != 42) { return 1 } var s = async::block_on<std::string>(bar()); if(s.size() != 2) { return 2 } return 0 }\n"

// Exercises the lowered state machine end-to-end. Runs on whichever backend
// built the test binary (C or LLVM).
@test
public func async_coroutine_runs(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    expect_compile_and_exit(env, "async_coroutine", NEG_CH_ASYNC_COROUTINE, NEG_MOD_ASYNC_STD, 0)
}

// B15-W regression: the same coroutine shape compiled with full debug info must
// pass the backend verifier. On LLVM, `debug_complete` runs the LLVM verifier,
// which rejects coroutine debug info attached to a non-local scope ("location
// requires a valid scope") and inlinable destructor calls without a `!dbg`
// location — both occurred when async debug info was emitted without pushing a
// DISubprogram for each generated function (ramp/poll/drop).
@test
public func async_coroutine_debug_info_compiles(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    expect_compile_and_exit_mode(env, "async_debug_info", NEG_CH_ASYNC_COROUTINE, NEG_MOD_ASYNC_STD, 0, "debug_complete")
}

@test
public func async_main_trampoline_runs(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Rd { var v : int }\nfunc rp(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<int> { var f = frame as *mut Rd; return core::async::Poll.Ready<int>(f.v) }\nfunc rd(frame : *mut void) { free(frame) }\nfunc mk(v : int) : core::async::FutureHandle<int> { var f = malloc(sizeof(Rd)) as *mut Rd; f.v = v; var t = malloc(sizeof(core::async::FutureTable<int>)) as *mut core::async::FutureTable<int>; t.poll = rp; t.drop = rd; return core::async::FutureHandle<int> { frame : f as *mut void, vtbl : t } }\nasync func main() : int { var n = await mk(40); return n + 2 }\n"
    expect_compile_and_exit(env, "async_main_trampoline", ch, NEG_MOD_ASYNC_APP, 42)
}

@test
public func neg_async_closure_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "public func main() : int {\n    var f = async ||(y : int) => { return y + 1 }\n    return f(41)\n}\n"
    expect_compile_error(env, "async_closure", ch, "async closures are not yet supported")
}

@test
public func neg_async_destructor_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "struct Foo {\n    var x : int\n    @delete\n    async func delete(&mut self) { }\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error(env, "async_destructor", ch, "cannot be a destructor")
}

@test
public func neg_async_extern_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "@extern\nasync func foreign(x : int) : int\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error(env, "async_extern", ch, "cannot be `async`")
}

// `core` is in scope (so the FutureHandle protocol is reachable) but the
// `async` library that provides the frame allocator is not, so the generated
// ramp could not link. A clear diagnostic is emitted instead.
@test
public func neg_async_missing_library_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "async func f(x : int) : int {\n    return x\n}\npublic func main() : int {\n    return 0\n}\n"
    expect_compile_error_with_mod(env, "async_missing_lib", ch, "async functions require the `async` library", NEG_MOD_CORE)
}
