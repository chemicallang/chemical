using namespace std;

@test
func test_option_result_safe_methods(env : &mut TestEnv) {
    var o = Option.Some<int>(7)
    if(!o.is_some() || o.is_none()) { env.error("Option is_some/is_none") }
    var taken = o.take_or_none()
    if(taken is Option.None) { env.error("Option.take_or_none Some") }
    var again = o.take_or_none()
    if(again is Option.Some) { env.error("Option.take_or_none None") }

    var ok : Result<int, int> = Result.Ok<int, int>(1)
    var err : Result<int, int> = Result.Err<int, int>(2)
    if(!ok.is_ok() || ok.is_err()) { env.error("Result.Ok classification") }
    if(!err.is_err() || err.is_ok()) { env.error("Result.Err classification") }
}

@test
func test_vector_span_safe_getters(env : &mut TestEnv) {
    var v = vector<int>()
    v.push(1)
    v.push(2)
    var a = v.get(0u)
    if(a is Option.None) { env.error("vector.get_opt in range") }
    var miss = v.get(9u)
    if(miss is Option.Some) { env.error("vector.get_opt out of range") }
    var m = v.get_mut(1u)
    if(m is Option.None) { env.error("vector.get_mut in range") }
    if(v.at(0u) != 1) { env.error("vector.at") }
    if(*v.at_ref(0u) != 1) { env.error("vector.at_ref") }
    *v.at_mut(0u) = 6
    if(v.at(0u) != 6) { env.error("vector.at_mut") }
    var copied = v.get_copy(0u)
    if(copied is Option.None) { env.error("vector.get_copy") }
    if(!v.try_set(0u, 5)) { env.error("vector.try_set in range") }
    if(v.try_set(9u, 1)) { env.error("vector.try_set out of range") }
    var popped = v.pop()
    if(popped is Option.None) { env.error("vector.try_pop") }

    var arr : [3]int = [3, 4, 5]
    var sp : [int] = arr
    var s0 = sp.get(0u)
    if(s0 is Option.None) { env.error("span.get in range") }
    if(sp.get(8u) is Option.Some) { env.error("span.get out of range") }
    if(unsafe(*sp.at(0u)) != 3) { env.error("span.at") }
    if(*sp.at_ref(0u) != 3) { env.error("span.at_ref") }
}

@test
func test_cstr_safe_view(env : &mut TestEnv) {
    var bytes : [3]u8 = ['a', 'b', 'c']
    var view : [u8] = bytes
    var made = cstr_from_bytes(view)
    if(made is Result.Err) { env.error("cstr_from_bytes valid") ; return }
    var Ok(c) = made else unsafe { unreachable }
    if(c.size() != 3) { env.error("CStr size") }
    var bad : [2]u8 = ['a', 0u8]
    var bview : [u8] = bad
    if(cstr_from_bytes(bview) is Result.Ok) { env.error("cstr_from_bytes rejects NUL") }
}

@test
func test_vector_try_reserve_overflow(env : &mut TestEnv) {
    var v = vector<u8>()
    var huge = ((0 as size_t) - 1) / 2
    if(v.try_reserve(huge)) {
        env.error("try_reserve must reject an impossible capacity")
    }
}
