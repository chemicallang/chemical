// std::NonNull tests.

@test
func non_null_wraps_and_unwraps(env : &mut TestEnv) {
    var value = 41
    var p = unsafe(&raw mut value)
    var nn = std::non_null(p)
    if(nn.is_null()) {
        env.error("NonNull::is_null reported a non-null pointer as null")
        return
    }
    if(unsafe(*nn.as_ptr()) != 41) {
        env.error("NonNull::as_ptr returned the wrong pointer")
        return
    }
    unsafe { *nn.as_ptr() = 42 }
    if(value != 42) {
        env.error("NonNull write through as_ptr was not visible")
    }
}

using std::Option;

@test
func non_null_try_rejects_null(env : &mut TestEnv) {
    var p : *mut int = null
    var maybe = std::non_null_try(p)
    if(maybe is Option.Some) {
        env.error("non_null_try accepted a null pointer")
    }
}

@test
func non_null_try_accepts_non_null(env : &mut TestEnv) {
    var value = 7
    var p = unsafe(&raw mut value)
    var maybe = std::non_null_try(p)
    if(maybe is Option.None) {
        env.error("non_null_try rejected a non-null pointer")
        return
    }
    var Some(nn) = maybe else unreachable
    if(nn.as_ptr() != p) {
        env.error("non_null_try changed the pointer")
    }
}
