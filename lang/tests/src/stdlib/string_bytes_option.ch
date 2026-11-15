using namespace std;

// `string`/`string_view` expose a byte view so the common "hand this text to a
// byte API" path does not need `unsafe`.
@test
func string_as_bytes_matches_storage(env : &mut TestEnv) {
    var s = string("hello")
    var b = s.as_bytes()
    if(b.size() != 5) { env.error("string.as_bytes size"); return }
    if(*b.at_ref(0) != 'h' as u8) { env.error("string.as_bytes first byte"); return }

    var v = s.to_view()
    var vb = v.as_bytes()
    if(vb.size() != 5) { env.error("string_view.as_bytes size"); return }

    var i : size_t = 0
    while(i < 5) {
        if(*b.at_ref(i) != *vb.at_ref(i)) {
            env.error("string and string_view byte views disagree")
            return
        }
        i = i + 1
    }
}

// `Option::unwrap_or` moves the payload out (no `Clone` bound).
@test
func option_unwrap_or_moves_payload(env : &mut TestEnv) {
    var some = Option.Some<string>(string("payload"))
    var got : string = some.unwrap_or(string("fallback"))
    if(!got.equals_view(string_view("payload"))) {
        env.error("Option::unwrap_or Some")
    }

    var none = Option.None<string>()
    var got2 : string = none.unwrap_or(string("fallback"))
    if(!got2.equals_view(string_view("fallback"))) {
        env.error("Option::unwrap_or None")
    }
}
