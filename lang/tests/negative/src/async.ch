// Negative tests for the async/await surface.

@test
public func neg_await_outside_async_errors(env : &mut TestEnv) {
    mkdir(NEG_WORK_DIR, 0o777 as uint)
    var ch = "func not_async() : int {\n    var v = await 1\n    return v\n}\npublic func main() : int {\n    return not_async()\n}\n"
    expect_compile_error(env, "await_outside_async", ch, "can only be used inside")
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
