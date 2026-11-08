using namespace std;

// Fixed Phase 2 malformed-input corpus. The same bytes live under
// lang/tests/fuzz/corpus/ for external fuzz drivers.
@test
func fuzz_encoding_malformed(env : &mut TestEnv) {
    var b1 = encoding::safe::base64_decode("!")
    if(b1 is Result.Ok) { env.error("base64 '!' accepted") }
    var b2 = encoding::safe::base64_decode("A")
    if(b2 is Result.Ok) { env.error("base64 length 1 accepted") }
    var b3 = encoding::safe::base64_decode("AA=A")
    if(b3 is Result.Ok) { env.error("base64 interior padding accepted") }
    var h1 = encoding::safe::hex_decode("zz")
    if(h1 is Result.Ok) { env.error("hex 'zz' accepted") }
    var u1 = encoding::safe::url_decode("%%")
    if(u1 is Result.Ok) { env.error("url '%%' accepted") }
    var u2 = encoding::safe::url_decode("%FF")
    if(u2 is Result.Ok) { env.error("url invalid UTF-8 accepted") }
    var bad : [3]u8 = [0xF0u8, 0x28u8, 0x8Cu8]
    var view : [u8] = bad
    if(encoding::safe::utf8_to_string(view) is Result.Ok) {
        env.error("malformed UTF-8 accepted")
    }
}

@test
func fuzz_json_cbor_malformed(env : &mut TestEnv) {
    var js = json::parse(string_view("{\"a\": [1,2,"))
    if(js is Result.Ok) { env.error("truncated JSON accepted") }
    var esc = json::parse(string_view("\"\\uZZZZ\""))
    if(esc is Result.Ok) { env.error("bad JSON escape accepted") }
    var bad_cbor = vector<u8>()
    bad_cbor.push(0x82u8)
    bad_cbor.push(0x01u8)
    var cv = cbor::decode(bad_cbor)
    if(cv is Result.Ok) { env.error("truncated CBOR accepted") }
}

@test
func fuzz_mime_bad_input(env : &mut TestEnv) {
    if(mime::is_text("application/") == true) { /* harmless classification */ }
    var t = mime::get_type("")
    if(t.size() > 64u) { env.error("mime type unexpectedly huge") }
    var e = mime::extension("")
    if(e.size() > 256u) { env.error("mime extension unexpectedly huge") }
}
