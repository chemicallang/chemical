// Regression tests for B26: a *large struct variant* carried through a
// `FutureHandle` must survive the LLVM async result store/load.
//
// Historically `block_on<Result<Response, std::string>>(http::get_async(...))`
// read back a garbage payload (the `Response` contains strings + a vector +
// pointer/bool state). The result-store bug was the same one fixed for B23
// (`Codegen::writeReturnStmtFor`'s coroutine branch stored an alloca pointer
// instead of copying the aggregate). These cover the shapes that were affected:
// eager, suspended, `return await`, and a bare `spawn_blocking<...>` future.

using namespace std;

public struct B26Body {
    var data : string
    var len : size_t
    var cap : size_t
    var tls : *mut void
    var closed : bool
    var owns : bool
    var remaining : long
}

public struct B26Headers {
    var names : vector<string>
    var values : vector<string>
}

public struct B26Response {
    var status : int
    var status_text : string
    var proto : string
    var headers : B26Headers
    var body_len : size_t
    var body : B26Body
}

public variant B26Result {
    Ok(value : B26Response)
    Err(error : string)
}

func b26_mk(status : int, text : string) : B26Response {
    return B26Response {
        status : status,
        status_text : text,
        proto : string("HTTP/1.1"),
        headers : B26Headers { names : vector<string>(), values : vector<string>() },
        body_len : 11u,
        body : B26Body {
            data : string("hello world"),
            len : 11u,
            cap : 16u,
            tls : null,
            closed : false,
            owns : false,
            remaining : -1
        }
    }
}

func b26_mk_hr(status : int, text : string) : B26Result {
    return B26Result.Ok(b26_mk(status, text))
}

async func b26_eager() : B26Result {
    return B26Result.Ok(b26_mk(200, string("OK")))
}

async func b26_suspend() : B26Result {
    var u = await async::yield_now()
    return B26Result.Ok(b26_mk(201, string("Created")))
}

async func b26_forward() : B26Result {
    return await b26_eager()
}

func b26_check(env : &mut TestEnv, r : B26Result, status : int, body : int, text : int) {
    if(r is B26Result.Ok) {
        var Ok(resp) = r else unreachable
        if(resp.status != status || resp.body.data.size() as int != body || resp.status_text.size() as int != text) {
            env.error("large-variant payload mismatch")
        }
    } else {
        env.error("expected Ok")
    }
}

@test
func test_async_large_variant_eager(env : &mut TestEnv) {
    b26_check(env, async::block_on<B26Result>(b26_eager()), 200, 11, 2)
}

@test
func test_async_large_variant_suspend(env : &mut TestEnv) {
    b26_check(env, async::block_on<B26Result>(b26_suspend()), 201, 11, 7)
}

@test
func test_async_large_variant_return_await(env : &mut TestEnv) {
    b26_check(env, async::block_on<B26Result>(b26_forward()), 200, 11, 2)
}

@test
func test_async_large_variant_spawn_blocking(env : &mut TestEnv) {
    var z = 1
    var r = async::block_on<B26Result>(async::spawn_blocking<B26Result>(|z|() => b26_mk_hr(204, string("No Content"))))
    b26_check(env, r, 204, 11, 10)
}
