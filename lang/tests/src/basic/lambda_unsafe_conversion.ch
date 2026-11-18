// A capturing lambda behind an unsafe boundary is still a lambda.
//
// Closures cross `unsafe(...)` whenever the capture is legal only under a
// contract the checker cannot see (a `&'static` referent out of a global, for
// one). MIR lowering used to test `val_kind() == LambdaFunc` on the *argument*
// itself, so an `unsafe(...)`-wrapped (or `as`-cast) lambda fell through to a
// plain function-pointer cast of the capture environment and the C backend
// rejected the result with
//
//   cannot convert 'int (*)(void*, int)' to
//   'struct std_stddefault_function_instance'
//
// `MIRLowerCall.cpp::lambda_arg` now looks through `UnsafeValue`/`CastedValue`
// to the lambda underneath, so the instance struct is constructed from the
// real capture environment.

/// Named so the call sites read clearly and nothing spells `>>`.
type IntHandler = std::function<(x : int) => int>

/// A callee that only accepts the erased instance type — the shape of every
/// API that stores a handler (a router table, a subscription list).
func invoke(handler : IntHandler, argument : int) : int {
    return handler(argument)
}

public struct Board {
    var title : std::string
}

// A `&'static` referent, the way a server keeps a shared table alive.
@never_destructed var shared_board : Board

func board() : &Board {
    return unsafe(&shared_board)
}

@test
func unsafe_wrapped_capturing_lambda_is_callable(env : &mut TestEnv) {
    var base = 41
    var callback = unsafe((|base|(x : int) => { return x + base }))
    if(callback(1) != 42) {
        env.error("the unsafe-wrapped capturing lambda did not call")
    }
}

@test
func unsafe_wrapped_lambda_capturing_a_referent_is_callable(env : &mut TestEnv) {
    // The router shape: the capture is a reference to a referent the checker
    // cannot place, so the closure is built under an unsafe boundary.
    var handle = board()
    var callback = unsafe((|&handle|(x : int) => { return x + 1 }))
    if(callback(41) != 42) {
        env.error("the unsafe-wrapped referent-capturing lambda did not call")
    }
}

@test
func unsafe_wrapped_capturing_lambda_converts_to_the_instance_type(env : &mut TestEnv) {
    // The conversion at the parameter boundary is what used to emit a raw
    // pointer cast instead of a function instance.
    var base = 21
    var result = invoke(unsafe((|base|(x : int) => { return x + base })), 21)
    if(result != 42) {
        env.error("the unsafe-wrapped lambda was not converted to its instance struct")
    }
}

@test
func an_unwrapped_capturing_lambda_still_converts(env : &mut TestEnv) {
    // The control: the same closure without the unsafe wrapper took the
    // `LambdaFunc` path all along and must keep working.
    var base = 40
    var result = invoke(|base|(x : int) => { return x + base }, 2)
    if(result != 42) {
        env.error("the plain capturing lambda stopped converting")
    }
}
