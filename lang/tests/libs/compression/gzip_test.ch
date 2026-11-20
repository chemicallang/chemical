// Tests for `compression::gzip` / `gunzip` — the build-time asset precompressor.
//
// The encoder and decoder are independent implementations of the same spec
// (fixed-Huffman deflate), so a round-trip catches a bit-order or table error in
// either one. `scripts/static-site-test.sh` adds the outside check: node's zlib
// gunzips the bytes a real build writes.

using std::Result;
using std::string;
using std::string_view;
using std::vector;

func gz_roundtrip(text : string_view) : bool {
    var packed = compression::gzip(text.as_bytes())
    if(packed is Result.Err) { return false }
    var Ok(gz) = packed else unsafe { unreachable }
    var back = compression::gunzip(&gz)
    if(back is Result.Err) { return false }
    var Ok(plain) = back else unsafe { unreachable }
    if(plain.size() != text.size()) { return false }
    var i : size_t = 0
    while(i < plain.size()) {
        if(plain.at(i) != unsafe(*text.as_bytes().at(i))) { return false }
        i = i + 1
    }
    return true
}

func gz_size(text : string_view) : size_t {
    var packed = compression::gzip(text.as_bytes())
    if(packed is Result.Err) { return 0 }
    var Ok(gz) = packed else unsafe { unreachable }
    return gz.size()
}

func gz_repeat(unit : string_view, times : int) : string {
    var out = string()
    var i : int = 0
    while(i < times) {
        out.append_view(&unit)
        i = i + 1
    }
    return out
}

@test
public func test_gzip_roundtrips_empty_input(env : &mut TestEnv) {
    const empty = string_view("")
    if(!gz_roundtrip(empty)) { env.error("empty input did not round-trip") }
}

@test
public func test_gzip_roundtrips_short_text(env : &mut TestEnv) {
    if(!gz_roundtrip(string_view("a"))) { env.error("one byte did not round-trip") }
    if(!gz_roundtrip(string_view("hello hello hello"))) { env.error("short text did not round-trip") }
}

@test
public func test_gzip_roundtrips_generated_javascript(env : &mut TestEnv) {
    var js = gz_repeat(string_view("function scoped_component_1(props) { return props.title; }\n"), 400)
    var view = js.to_view()
    if(!gz_roundtrip(view)) { env.error("generated JS did not round-trip") }
}

@test
public func test_gzip_roundtrips_every_byte_value(env : &mut TestEnv) {
    // A text-shaped encoder must survive arbitrary bytes: the fixed-Huffman
    // literal table covers 0..255 and this walks all of them, twice, so a
    // distance-1 match crosses a literal of every value.
    var bytes = vector<u8>()
    var i : size_t = 0
    while(i < 512) {
        if(!bytes.try_push((i % 256) as u8)) { env.error("allocation failed"); return }
        i = i + 1
    }
    var packed = compression::gzip(&bytes)
    if(packed is Result.Err) { env.error("compress failed"); return }
    var Ok(gz) = packed else unsafe { unreachable }
    var back = compression::gunzip(&gz)
    if(back is Result.Err) { env.error("decompress failed"); return }
    var Ok(plain) = back else unsafe { unreachable }
    if(plain.size() != bytes.size()) { env.error("size changed"); return }
    var k : size_t = 0
    while(k < plain.size()) {
        if(plain.at(k) != bytes.at(k)) { env.error("byte mismatch after round-trip"); return }
        k = k + 1
    }
}

@test
public func test_gzip_shrinks_repetitive_text(env : &mut TestEnv) {
    var text = gz_repeat(string_view("    el.className = 'card';\n"), 200)
    var view = text.to_view()
    var packed = gz_size(view)
    if(packed == 0 || packed >= view.size() / 4) {
        env.error("repetitive text did not shrink to under a quarter of its size")
    }
}

@test
public func test_gzip_is_deterministic(env : &mut TestEnv) {
    // The header has no timestamp, so two builds produce identical bytes and a
    // content hash still names the output it describes.
    const text = string_view("deterministic bytes, deterministic hash")
    var first = compression::gzip(text.as_bytes())
    var second = compression::gzip(text.as_bytes())
    if(first is Result.Err || second is Result.Err) { env.error("compress failed"); return }
    var Ok(a) = first else unsafe { unreachable }
    var Ok(b) = second else unsafe { unreachable }
    if(a.size() != b.size()) { env.error("sizes differ between runs"); return }
    var i : size_t = 0
    while(i < a.size()) {
        if(a.at(i) != b.at(i)) { env.error("bytes differ between runs"); return }
        i = i + 1
    }
}

@test
public func test_gzip_writes_a_valid_header(env : &mut TestEnv) {
    const text = string_view("header check")
    var packed = compression::gzip(text.as_bytes())
    if(packed is Result.Err) { env.error("compress failed"); return }
    var Ok(gz) = packed else unsafe { unreachable }
    if(gz.size() < 18) { env.error("stream is shorter than a gzip header plus trailer"); return }
    if(gz.at(0) != 0x1f || gz.at(1) != 0x8b) { env.error("missing gzip magic bytes") }
    if(gz.at(2) != 8) { env.error("compression method is not deflate") }
}

@test
public func test_gunzip_rejects_a_truncated_stream(env : &mut TestEnv) {
    const text = string_view("truncation must be an error, not a short result")
    var packed = compression::gzip(text.as_bytes())
    if(packed is Result.Err) { env.error("compress failed"); return }
    var Ok(gz) = packed else unsafe { unreachable }
    var cut = vector<u8>()
    var i : size_t = 0
    while(i + 4 < gz.size()) {
        if(!cut.try_push(gz.at(i))) { env.error("allocation failed"); return }
        i = i + 1
    }
    var back = compression::gunzip(&cut)
    if(back is Result.Ok) { env.error("a truncated stream decoded successfully") }
}

@test
public func test_gunzip_rejects_a_corrupted_body(env : &mut TestEnv) {
    const text = string_view("a corrupted body must fail the CRC check")
    var packed = compression::gzip(text.as_bytes())
    if(packed is Result.Err) { env.error("compress failed"); return }
    var Ok(gz) = packed else unsafe { unreachable }
    // Flip a bit in the first deflate byte (offset 10 is the block header).
    gz.set(10, gz.at(10) ^ (0x08 as u8))
    var back = compression::gunzip(&gz)
    if(back is Result.Ok) { env.error("a corrupted body decoded successfully") }
}

@test
public func test_gunzip_rejects_bad_magic(env : &mut TestEnv) {
    var bytes = vector<u8>()
    var i : size_t = 0
    while(i < 24) {
        if(!bytes.try_push(0)) { env.error("allocation failed"); return }
        i = i + 1
    }
    var back = compression::gunzip(&bytes)
    if(back is Result.Ok) { env.error("a stream without gzip magic decoded successfully") }
}
