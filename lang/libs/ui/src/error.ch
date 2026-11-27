// P1.1 typed errors. Failure paths are allocation-free: only enum/numeric
// fields and static message/context strings are retained.

using std::Option;
using std::string_view;
using std::Result;

public enum ErrorClass { Expected, AppBug, Internal, Fatal }

public enum ErrorCode {
    Platform, Surface, Font, Image, Layout, InvalidKey, StaleNode, OutOfMemory,
    Protocol, Io, Timeout, NotImplemented, Unsupported, NotFound, PermissionDenied,
    InvalidArgument, Decode, ResourceExhausted, Cancelled, Busy, WouldBlock, DuplicateKey,
    StaleKey, ThreadAffinity, DeviceLost, Internal, Other
}

public enum RetryHint { Never, Immediate, Backoff }

@direct_init
public struct SourceLocation {
    var file : string_view
    var line : u32
    var col : u32
}

@direct_init
public struct ErrorDetail {
    var code : u32
    var message : &'static str
    var context : &'static str
}

// App-supplied codes classify as Expected or Internal. Fatal is reserved for
// the private helper below and is not exposed.
public func classify(code : ErrorCode) : ErrorClass {
    switch(code) {
        ErrorCode.ThreadAffinity, ErrorCode.StaleNode, ErrorCode.Internal, ErrorCode.Other => return ErrorClass.Internal
        default => return ErrorClass.Expected
    }
}

func default_retry(code : ErrorCode) : RetryHint {
    switch(code) {
        ErrorCode.Io, ErrorCode.Timeout, ErrorCode.Busy, ErrorCode.Cancelled, ErrorCode.WouldBlock, ErrorCode.DeviceLost => return RetryHint.Backoff
        default => return RetryHint.Never
    }
}

@direct_init
public struct UiError {
    private var code_value : ErrorCode
    private var class_value : ErrorClass
    private var detail_value : ErrorDetail
    private var source_value : Option<SourceLocation>
    private var retry_value : RetryHint

    @make
    public func make(code : ErrorCode, message : &'static str) : UiError {
        return UiError {
            code_value : code,
            class_value : classify(code),
            detail_value : ErrorDetail { code : code as u32, message : message, context : "" },
            source_value : Option.None<SourceLocation>(),
            retry_value : default_retry(code)
        }
    }

    public func code(&self) : ErrorCode { return self.code_value }
    public func class(&self) : ErrorClass { return self.class_value }
    public func retry_hint(&self) : RetryHint { return self.retry_value }
    public func detail(&self) : &ErrorDetail { return &self.detail_value }
    public func source(&self) : Option<SourceLocation> { return self.source_value }

    public func with_source(&self, loc : SourceLocation) : UiError {
        return UiError {
            code_value : self.code_value,
            class_value : self.class_value,
            detail_value : self.detail_value,
            source_value : Option.Some<SourceLocation>(loc),
            retry_value : self.retry_value
        }
    }

    public func with_retry(&self, hint : RetryHint) : UiError {
        return UiError {
            code_value : self.code_value,
            class_value : self.class_value,
            detail_value : self.detail_value,
            source_value : self.source_value,
            retry_value : hint
        }
    }

    // Not public: only the runtime may create a Fatal error. No P1.1 path calls it.
    func make_fatal(code : ErrorCode, message : &'static str) : UiError {
        return UiError {
            code_value : code,
            class_value : ErrorClass.Fatal,
            detail_value : ErrorDetail { code : code as u32, message : message, context : "" },
            source_value : Option.None<SourceLocation>(),
            retry_value : RetryHint.Never
        }
    }
}

public func <T> ok(value : T) : Result<T, UiError> {
    return Result.Ok<T, UiError>(value)
}

public func <T> err(error : UiError) : Result<T, UiError> {
    return Result.Err<T, UiError>(error)
}

public func <T, U> map(r : Result<T, UiError>, f : (x : T) => U) : Result<U, UiError> {
    if(r is Result.Err) {
        var Err(e) = r else return Result.Err<U, UiError>(UiError.make(ErrorCode.Internal, "map: corrupt result"))
        return Result.Err<U, UiError>(e)
    }
    var Ok(value) = r else return Result.Err<U, UiError>(UiError.make(ErrorCode.Internal, "map: corrupt result"))
    return Result.Ok<U, UiError>(f(value))
}
