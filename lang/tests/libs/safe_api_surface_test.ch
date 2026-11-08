using namespace std;

@test
func test_safe_api_surface_smoke(env : &mut TestEnv) {
    var p = path::PathBuf.from("/tmp/safe-api.txt")
    if(!p.is_absolute()) { env.error("PathBuf absolute") }
    if(!p.basename().as_view().equals(string_view("safe-api.txt"))) {
        env.error("PathBuf basename")
    }

    var data : [3]u8 = ['a','b','c']
    var view : [u8] = data
    var encoded = encoding::hex_encode(view)
    if(encoded is Result.Err) { env.error("hex_encode safe failed"); return }
    var Ok(enc) = encoded else unsafe { unreachable }
    if(!enc.equals_view(string_view("616263"))) { env.error("hex_encode safe") }
    var dec = encoding::hex_decode("616263")
    if(dec is Result.Err) { env.error("hex_decode safe") }

    var digest = crypto::sha256(view)
    if(digest[0] == 0u8 && digest[1] == 0u8 && digest[2] == 0u8 && digest[3] == 0u8) {
        env.error("sha256 safe returned all-zero prefix")
    }

    var json_text = json::stringify(JsonValue.String(string("ok")))
    var parsed = json::parse(json_text.to_view())
    if(parsed is Result.Err) { env.error("json round-trip safe") }

    var v = vector<int>()
    if(!v.try_push(1)) { env.error("vector try_push") }
    if(v.get_opt(0u) is Option.None) { env.error("vector get_opt") }
    if(v.try_pop() is Option.None) { env.error("vector try_pop") }
}
