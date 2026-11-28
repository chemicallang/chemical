using std::string_view;
// P1.1 error taxonomy tests. Allocation-free values; no panic/unwrap.
using std::Option;
using std::Result;

public namespace error_tests {

func fail(name : *char) : bool {
    unsafe { printf("error FAIL %s\n", name) }
    return false
}

func expected_class(code : ErrorCode) : ErrorClass {
    switch(code) {
        ErrorCode.ThreadAffinity, ErrorCode.StaleNode, ErrorCode.Internal, ErrorCode.Other => return ErrorClass.Internal
        default => return ErrorClass.Expected
    }
}

func expected_retry(code : ErrorCode) : RetryHint {
    switch(code) {
        ErrorCode.Io, ErrorCode.Timeout, ErrorCode.Busy, ErrorCode.Cancelled, ErrorCode.WouldBlock, ErrorCode.DeviceLost => return RetryHint.Backoff
        default => return RetryHint.Never
    }
}

func add1(x : int) : int { return x + 1 }

public func run() : bool {
    var passed = true
    var codes : [27]ErrorCode = [
        ErrorCode.Platform, ErrorCode.Surface, ErrorCode.Font, ErrorCode.Image,
        ErrorCode.Layout, ErrorCode.InvalidKey, ErrorCode.StaleNode, ErrorCode.OutOfMemory,
        ErrorCode.Protocol, ErrorCode.Io, ErrorCode.Timeout, ErrorCode.NotImplemented,
        ErrorCode.Unsupported, ErrorCode.NotFound, ErrorCode.PermissionDenied,
        ErrorCode.InvalidArgument, ErrorCode.Decode, ErrorCode.ResourceExhausted,
        ErrorCode.Cancelled, ErrorCode.Busy, ErrorCode.WouldBlock, ErrorCode.DuplicateKey,
        ErrorCode.StaleKey, ErrorCode.ThreadAffinity, ErrorCode.DeviceLost,
        ErrorCode.Internal, ErrorCode.Other
    ]
    var i = 0u32
    while(i < 27u32) {
        var e = UiError.make(codes[i as size_t], "static detail")
        if(e.class() != expected_class(codes[i as size_t])) { passed = fail("class-map") }
        if(e.class() == ErrorClass.Fatal) { passed = fail("app-cannot-make-fatal") }
        if(classify(codes[i as size_t]) != expected_class(codes[i as size_t])) { passed = fail("classify-free") }
        if(e.retry_hint() != expected_retry(codes[i as size_t])) { passed = fail("retry-map") }
        if(e.detail().code != (codes[i as size_t] as u32)) { passed = fail("detail-code") }
        i = i + 1u32
    }
    if(sizeof(UiError) > 96u) { passed = fail("error-size") }
    if(sizeof(ErrorDetail) > 64u) { passed = fail("detail-size") }

    var e : UiError
    {
        var local : string_view = "temporary source scope"
        var loc = SourceLocation { file : "error_test.ch", line : 12u32, col : 7u32 }
        e = UiError.make(ErrorCode.Io, "static message").with_source(loc)
        if(local.size() == 0u) { passed = fail("scope-local") }
    }
    var d = e.detail()
    if(d.message.size() != 14u) { passed = fail("static-detail-retained") }
    var src = e.source()
    if(src is Option.Some) {
        var Some(s) = src else return false
        if(s.line != 12u32 || s.col != 7u32) { passed = fail("source-copy") }
    } else {
        passed = fail("source-present")
    }
    var retried = e.with_retry(RetryHint.Immediate)
    if(retried.retry_hint() != RetryHint.Immediate) { passed = fail("with-retry") }
    if(e.retry_hint() != RetryHint.Backoff) { passed = fail("io-default-backoff") }

    var good : Result<int, UiError> = ok(41)
    if(good is Result.Ok) {
        var Ok(v) = good else return false
        if(v != 41) { passed = fail("result-passed") }
    } else {
        passed = fail("result-passed-shape")
    }
    var mapped : Result<int, UiError> = map(ok(41), add1)
    if(mapped is Result.Ok) {
        var Ok(m) = mapped else return false
        if(m != 42) { passed = fail("result-map") }
    } else {
        passed = fail("result-map-shape")
    }
    var bad : Result<int, UiError> = err(UiError.make(ErrorCode.NotFound, "missing"))
    if(bad is Result.Err) {
        var Err(be) = bad else return false
        if(be.code() != ErrorCode.NotFound) { passed = fail("result-err") }
    } else {
        passed = fail("result-err-shape")
    }
    return passed
}

} // namespace error_tests
