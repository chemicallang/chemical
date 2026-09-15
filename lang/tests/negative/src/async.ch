// Negative tests for the async/await surface.

@test
public func neg_await_outside_async_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func not_async() : int {\n    var v = await 1\n    return v\n}\npublic func main() : int {\n    return not_async()\n}\n"
    expect_compile_error(env, "await_outside_async", ch, "can only be used inside")
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
