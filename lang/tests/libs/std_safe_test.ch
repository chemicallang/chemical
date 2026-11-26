using namespace std;

struct MapBox {
    var n : int
}

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

@test
func test_string_try_append_and_reserve(env : &mut TestEnv) {
    var s = string()
    if(!s.try_append('a')) { env.error("string.try_append small") }
    if(!s.try_append_string(&s)) { env.error("string.try_append_string self") }
    if(s.size() != 2) { env.error("string.try_append self-alias size") }
    var view : std::string_view = "xyz"
    if(!s.try_append_view(&view)) { env.error("string.try_append_view") }
    if(s.size() != 5) { env.error("string.try_append_view size") }
    var huge = ((0 as size_t) - 1) / 2
    if(s.try_reserve(huge)) { env.error("string.try_reserve impossible capacity") }
    // A failed reserve/append leaves the string usable and unchanged.
    if(!s.try_append('!')) { env.error("string.try_append after failed reserve") }
    if(s.size() != 6) { env.error("string.try_append after failed reserve size") }
}

@test
func test_rc_arc_try_new(env : &mut TestEnv) {
    var r = std::try_rc(7)
    if(!r.is_ok()) { env.error("Rc.try_new") }
    var a = std::try_arc(9)
    if(!a.is_ok()) { env.error("Arc.try_new") }
}

@test
func test_deque_try_push(env : &mut TestEnv) {
    var d = std::deque<int>()
    if(!d.try_push(1)) { env.error("deque.try_push first") }
    var i = 0
    while(i < 20) {
        if(!d.try_push(i)) { env.error("deque.try_push growth") ; return }
        i = i + 1
    }
    if(d.size() != 21) { env.error("deque.try_push size") }
    if(*d.get_ref(0) != 1) { env.error("deque.try_push value") }
    var huge = ((0 as size_t) - 1)
    if(d.try_reserve_blocks(huge)) { env.error("deque.try_reserve_blocks impossible") }
}

@test
func test_map_try_insert(env : &mut TestEnv) {
    var m = std::unordered_map<int, int>()
    if(!m.try_insert(1, 10)) { env.error("unordered_map.try_insert first") }
    if(!m.try_insert(2, 20)) { env.error("unordered_map.try_insert second") }
    if(!m.try_insert(1, 30)) { env.error("unordered_map.try_insert update") }
    if(m.size() != 2) { env.error("unordered_map.try_insert size") }
    var o = std::ordered_map<int, int>()
    if(!o.try_insert(1, 10)) { env.error("ordered_map.try_insert first") }
    if(!o.try_insert(2, 20)) { env.error("ordered_map.try_insert second") }
    if(o.size() != 2) { env.error("ordered_map.try_insert size") }
}

@test
func test_map_get_mut(env : &mut TestEnv) {
    var key = 1
    var m = std::unordered_map<int, MapBox>()
    m.insert(1, MapBox { n : 10 })
    var slot = m.get_mut(&key)
    if(slot is Option.None) {
        env.error("unordered_map.get_mut present")
    } else {
        m.at_mut(&key).n = 20
        if(m.at(&key).n != 20) { env.error("unordered_map.get_mut write") }
    }
    var missing = 9
    if(m.get_mut(&missing) is Option.Some) { env.error("unordered_map.get_mut missing") }

    var o = std::ordered_map<int, MapBox>()
    o.insert(1, MapBox { n : 10 })
    var oslot = o.get_mut(&key)
    if(oslot is Option.None) {
        env.error("ordered_map.get_mut present")
    } else {
        o.at_mut(&key).n = 30
        if(o.at(&key).n != 30) { env.error("ordered_map.get_mut write") }
    }
    if(o.get_mut(&missing) is Option.Some) { env.error("ordered_map.get_mut missing") }
}

@test
func test_async_try_channel(env : &mut TestEnv) {
    var made = async::try_channel<int>()
    if(made is Result.Err) { env.error("try_channel allocation") ; return }
    var Ok(ch) = made else unsafe { unreachable }
    var sent = ch.sender.send(7)
    if(sent is Result.Err) { env.error("try_channel send") }
    var got = ch.receiver.try_recv()
    if(got is Result.Err) {
        env.error("try_channel try_recv")
    } else {
        var Ok(value) = got else unsafe { unreachable }
        if(value != 7) { env.error("try_channel value") }
    }
}
