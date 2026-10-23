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
