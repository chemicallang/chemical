using namespace std;

// Fixed Phase 2 malformed-input corpus. The same bytes live under
// lang/tests/fuzz/corpus/ for external fuzz drivers.
@test
func fuzz_encoding_malformed(env : &mut TestEnv) {
    var b1 = encoding::base64_decode("!")
    if(b1 is Result.Ok) { env.error("base64 '!' accepted") }
    var b2 = encoding::base64_decode("A")
    if(b2 is Result.Ok) { env.error("base64 length 1 accepted") }
    var b3 = encoding::base64_decode("AA=A")
    if(b3 is Result.Ok) { env.error("base64 interior padding accepted") }
    var h1 = encoding::hex_decode("zz")
    if(h1 is Result.Ok) { env.error("hex 'zz' accepted") }
    var u1 = encoding::url_decode("%%")
    if(u1 is Result.Ok) { env.error("url '%%' accepted") }
    var u2 = encoding::url_decode("%FF")
    if(u2 is Result.Ok) { env.error("url invalid UTF-8 accepted") }
    var bad : [3]u8 = [0xF0u8, 0x28u8, 0x8Cu8]
    var view : [u8] = bad
    if(encoding::utf8_to_string(view) is Result.Ok) {
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
    var cbor_view : [u8] = &bad_cbor
    var cv = cbor::decode(cbor_view)
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

@test
func fuzz_regex_malformed(env : &mut TestEnv) {
    var patterns : [3]std::string_view = [
        "[abc",
        "(abc",
        "*abc"
    ]
    var i = 0
    while(i < 3) {
        var r = regex::compile(patterns[i])
        if(r.is_valid()) { env.error("malformed regex accepted") }
        i = i + 1
    }
    // A bounded but adversarial pattern must compile and stay linear on a
    // short input rather than overflow/abort.
    var re = regex::compile("(ab|cd)+$")
    if(!re.is_valid()) { env.error("valid bounded regex rejected") }
    if(!re.is_match("abcdabcd")) { env.error("bounded regex match") }
}

@test
func fuzz_image_audio_archive_malformed(env : &mut TestEnv) {
    var png : [8]u8 = [0x89u8, 'P' as u8, 'N' as u8, 'G' as u8, 0x0Du8, 0x0Au8, 0x1Au8, 0x0Au8]
    var png_view : [u8] = png
    if(image::Image.from_png(png_view) is Result.Ok) { env.error("truncated PNG accepted") }

    var bmp : [3]u8 = ['B' as u8, 'M' as u8, 0u8]
    var bmp_view : [u8] = bmp
    if(image::Image.from_bmp(bmp_view) is Result.Ok) { env.error("truncated BMP accepted") }

    var wav : [8]u8 = ['R' as u8, 'I' as u8, 'F' as u8, 'F' as u8, 0u8, 0u8, 0u8, 0u8]
    var wav_view : [u8] = wav
    if(audio::Audio.from_wav(wav_view) is Result.Ok) { env.error("truncated WAV accepted") }

    var zip : [4]u8 = ['P' as u8, 'K' as u8, 0x03u8, 0x04u8]
    var zip_view : [u8] = zip
    if(archive::Archive.from_bytes(zip_view) is Result.Ok) { env.error("truncated ZIP accepted") }
}
