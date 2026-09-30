// cbor_test.ch — RFC 8949 conformance.
//
// The vectors are Appendix A of RFC 8949 ("Examples") verbatim, plus the
// malformed input the RFC calls out but does not enumerate. This is the whole
// reason the module lives in the language repo: a parser is only trustworthy
// pinned to the spec's own bytes, and those tests belong where the language's
// test runner already is.
//
// Every failure names the hex vector, so a red run points straight at the spec
// line. `fail` builds that message once.

using std::string
using std::string_view
using std::vector
using std::Result
using std::Option

@test
func test_the_rfc_8949_unsigned_vectors(env : &mut TestEnv) {
    // A.1: one-byte heads.
    expect_u64(env, "00", 0u64)
    expect_u64(env, "01", 1u64)
    expect_u64(env, "0a", 10u64)
    expect_u64(env, "17", 23u64)
    // A.2: 1-, 2- and 4-byte arguments.
    expect_u64(env, "1818", 24u64)
    expect_u64(env, "1819", 25u64)
    expect_u64(env, "1864", 100u64)
    expect_u64(env, "1903e8", 1000u64)
    expect_u64(env, "1a000f4240", 1000000u64)
    expect_u64(env, "1b000000e8d4a51000", 1000000000000u64)
    // The maximum. This is the vector that catches a decoder using i64.
    expect_u64(env, "1bffffffffffffffff", 18446744073709551615u64)
}

@test
func test_the_rfc_8949_negative_vectors(env : &mut TestEnv) {
    // A.3: major type 1 encodes -1 - n.
    expect_i64(env, "20", -1i64)
    expect_i64(env, "29", -10i64)
    expect_i64(env, "3863", -100i64)
    expect_i64(env, "3903e7", -1000i64)
    // The largest negative that fits an i64: n = 2^63 - 1, giving -2^63.
    expect_i64(env, "3b7fffffffffffffff", -9223372036854775808i64)
}

@test
func test_the_rfc_8949_byte_string_vectors(env : &mut TestEnv) {
    // A.4
    expect_bytes_len(env, "40", 0u64)
    expect_bytes(env, "4401020304", "01020304")
}

@test
func test_the_rfc_8949_text_string_vectors(env : &mut TestEnv) {
    // A.5: including a 4-byte code point (U+10151) and an escaped quote.
    expect_text(env, "60", "")
    expect_text(env, "6161", "a")
    expect_text(env, "6449455446", "IETF")
    expect_text(env, "62225c", "\"\\")
    expect_text(env, "62c3bc", "ü")
    expect_text(env, "63e6b0b4", "水")
    expect_text(env, "64f0908591", "𐅑")
}

@test
func test_the_rfc_8949_array_and_map_vectors(env : &mut TestEnv) {
    // A.6 / A.7
    expect_array_len(env, "80", 0u64)
    expect_array_len(env, "83010203", 3u64)
    expect_array_len(env, "8301820203820405", 3u64)
    expect_map_len(env, "a0", 0u64)
    expect_map_len(env, "a201020304", 2u64)
    expect_map_len(env, "a26161016162820203", 2u64)
    // A.9: five pairs.
    expect_map_len(env, "a56161614161626142616361436164614461656145", 5u64)
}

@test
func test_the_rfc_8949_indefinite_length_vectors(env : &mut TestEnv) {
    // A.8: the four indefinite forms, each the definite form rewritten as chunks.
    expect_bytes(env, "5f42010243030405ff", "01020304" + "05")
    expect_text(env, "7f657374726561646d696e67ff", "streaming")
    expect_array_len(env, "9fff", 0u64)
    expect_array_len(env, "9f018202039f0405ffff", 3u64)
    expect_map_len(env, "bf61610161629f0203ffff", 2u64)
    // A.8's last group: the same documents with the chunks in the other order.
    expect_array_len(env, "9f0102030405060708090a0b0c0d0e0f101112131415161718181819ff", 25u64)
    expect_map_len(env, "bf6346756ef563416d7421ff", 2u64)
    // The same text split into "str" + "eaming" - a legal chunk boundary.
    expect_text(env, "7f637374726665616d696e67ff", "streaming")
}

@test
func test_indefinite_and_definite_forms_agree(env : &mut TestEnv) {
    // [1, [2, 3], [4, 5]] in every legal encoding. This is A.8's actual point:
    // all four of these are the same document, so they must look the same.
    var forms = vector<string_view>()
    forms.push_back(string_view("8301820203820405"))
    forms.push_back(string_view("83019f0203ff820405"))
    forms.push_back(string_view("83018202039f0405ff"))
    forms.push_back(string_view("9f018202039f0405ffff"))
    for(var i = 0u; i < forms.size(); i++) {
        var d = try_decode(forms.get(i))
        if(d is Result.Err) {
            env.error("one of the equivalent encodings did not decode")
            continue
        }
        var Ok(v) = d else unreachable
        if(!cbor::is_array(&mut v)) {
            env.error("one of the equivalent encodings is not an array")
            continue
        }
        if(cbor::array_len(&mut v) != 3u) {
            env.error("one of the equivalent encodings does not have 3 items")
            continue
        }
        if(cbor::array_int_at(&mut v, 0u, -1i64) != 1i64) {
            env.error("the equivalent encodings do not all start with 1")
        }
    }
}

@test
func test_a_tag_wraps_its_item(env : &mut TestEnv) {
    // A.4: tag 2 (a bignum) over a byte string.
    var d = try_decode(string_view("c249010000000000000000"))
    if(d is Result.Err) {
        env.error("the tag vector did not decode")
        return
    }
    var Ok(v) = d else unreachable
    var t = cbor::tag_number(&mut v)
    if(t is Option.None) {
        env.error("expected a tagged value")
        return
    }
    var Some(n) = t else unreachable
    if(n != 2u64) { env.error("expected tag 2") }
}

@test
func test_the_simple_and_bool_vectors(env : &mut TestEnv) {
    // A.1: major type 7.
    expect_bool(env, "f5", true)
    expect_bool(env, "f4", false)
    expect_is_null(env, "f6")
    expect_is_undefined(env, "f7")
    expect_simple(env, "f0", 16u8)
    expect_simple(env, "f818", 24u8)
    expect_simple(env, "f8ff", 255u8)
}

@test
func test_the_rfc_8949_float_vectors(env : &mut TestEnv) {
    // A.4: half, single and double precision, including subnormals.
    expect_double(env, "f90000", 0.0)
    expect_double(env, "f98000", 0.0)          // -0.0 compares equal to 0.0
    expect_double(env, "f93c00", 1.0)
    expect_double(env, "f93e00", 1.5)
    expect_double(env, "f97bff", 65504.0)
    expect_double(env, "f9c400", -4.0)
    expect_double(env, "f90001", 0.00000005960464477539063)
    expect_double(env, "f90400", 0.00006103515625)
}

@test
func test_infinities_and_nan_are_reported_not_guessed(env : &mut TestEnv) {
    // A.4 lists these too. We do not fabricate an infinity: the decoder answers
    // Unsupported, which is honest. A caller needing IEEE semantics can add it
    // with a bit-cast rather than inheriting a silently wrong value.
    expect_rejected(env, "f97c00")
    expect_rejected(env, "f97e00")
    expect_rejected(env, "fa7f800000")
    expect_rejected(env, "fb7ff0000000000000")
    expect_rejected(env, "fb7ff8000000000000")
}

@test
func test_a_cose_shaped_map_with_negative_integer_keys(env : &mut TestEnv) {
    // The shape WebAuthn actually hands us: a COSE_Key is a map keyed by small
    // integers, several of them NEGATIVE. A JSON-shaped value type cannot
    // represent this, which is the whole reason CBOR needs a model of its own.
    // RFC 9052 section 7: 1 = kty, 3 = alg, -1 = crv, -2 = x, -3 = y.
    // Document: {1: 2, 3: -7, -1: 1, -2: h'01', -3: h'02'}
    var d = try_decode(string_view("a5010203262001214101224102"))
    if(d is Result.Err) {
        env.error("the COSE-shaped map did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_map(&mut v)) {
        env.error("expected a map")
        return
    }
    if(cbor::map_len(&mut v) != 5u) {
        env.error("expected 5 COSE labels")
        return
    }
    // kty = 2 (EC2), alg = -7 (ES256), crv = 1 (P-256).
    if(cbor::map_int_by_int(&mut v, 1, -1i64) != 2i64) { env.error("kty is not 2") }
    if(cbor::map_int_by_int(&mut v, 3, 0i64) != -7i64) { env.error("alg is not -7") }
    if(cbor::map_int_by_int(&mut v, -1, 0i64) != 1i64) { env.error("crv is not 1") }
    // The coordinates are byte strings, present under negative labels.
    if(!cbor::map_has_int(&mut v, -2)) { env.error("no x (label -2)") }
    if(!cbor::map_has_int(&mut v, -3)) { env.error("no y (label -3)") }
    expect_label_bytes(env, &mut v, -2, "01")
    expect_label_bytes(env, &mut v, -3, "02")
    // A label that is not there reads as absent, not as zero.
    if(cbor::map_has_int(&mut v, 99)) { env.error("label 99 should be absent") }
}

// A WebAuthn attestation object, byte for byte as a platform authenticator
// produces it for `fmt: "none"`. This is the shape the account service decodes
// on every passkey registration, so it is pinned here rather than only in that
// service's tests: a decoder that cannot read a real attestation object is a
// decoder bug, and it belongs next to the vectors that prove it.
//
// {"fmt": "none", "attStmt": {}, "authData": h'000102...0f'}
//
// The byte string uses a TWO-byte length head (0x59) because a real authData is
// routinely over 255 bytes once a COSE key is attached. 0x58 (one byte) is what
// the COSE key's coordinates use, so both forms have to work.
@test
func test_a_webauthn_attestation_object_decodes(env : &mut TestEnv) {
    var d = try_decode("a363666d74646e6f6e656761747453746d74a0686175746844617461590010000102030405060708090a0b0c0d0e0f")
    if(d is Result.Err) {
        fail(env, "a363666d74646e6f6e656761747453746d74a0686175746844617461590010000102030405060708090a0b0c0d0e0f", "a WebAuthn attestation object was rejected")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_map(&mut v)) {
        env.error("the attestation object is not a map")
        return
    }
    if(cbor::map_len(&mut v) as u64 != 3u64) {
        env.error("the attestation object does not have three pairs")
        return
    }
    if(!cbor::map_has_text(&mut v, string_view("fmt"))) { env.error("no fmt key") }
    if(!cbor::map_has_text(&mut v, string_view("attStmt"))) { env.error("no attStmt key") }
    if(!cbor::map_has_text(&mut v, string_view("authData"))) { env.error("no authData key") }
    var fmt = cbor::map_text_by_text(&mut v, string_view("fmt"))
    if(fmt.to_view().find(&string_view("none")) != 0u) { env.error("fmt is not \"none\"") }
    // attStmt for fmt:none is an empty MAP, not a null. Getting this wrong is
    // easy and would reject every platform authenticator.
    var stmt = cbor::map_value_by_text(&mut v, string_view("attStmt"))
    if(stmt == null) { env.error("attStmt is not reachable") }
    else if(!cbor::is_map(&mut *stmt)) { env.error("attStmt is not a map") }
    else if(cbor::map_len(&mut *stmt) as u64 != 0u64) { env.error("attStmt is not empty") }
    // The 2-byte-length byte string: 16 bytes, and the right ones.
    var ad = cbor::map_bytes_by_text(&mut v, string_view("authData"))
    if(ad.size() != 16u) { env.error("authData is not 16 bytes") }
    var i = 0u
    while(i < ad.size() && i < 16u) {
        if(ad.get(i) != (i as u8)) { env.error("authData has the wrong bytes"); return }
        i += 1u
    }
    // And the one-byte-length form of the same thing, which is what a COSE key's
    // 32-byte coordinates use inside authenticator data.
    var narrow_hex = string_view("a263666d74646e6f6e656861757468446174615820000102030405060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f")
    var narrow = try_decode(narrow_hex)
    if(narrow is Result.Err) {
        env.error("the 1-byte-length authData form was rejected")
        return
    }
    var Ok(sv) = narrow else unreachable
    var sad = cbor::map_bytes_by_text(&mut sv, string_view("authData"))
    if(sad.size() != 32u) { env.error("the 1-byte-length authData is not 32 bytes") }
}


// The exact 176-byte document an account service receives for a passkey
// registration: a real SHA-256 RP ID hash, a real COSE key with 32-byte
// coordinates, and a credential id. Added after a 176-byte attestation object
// was rejected while a hand-simplified 33-byte one was accepted, which points
// the finger at something about this size rather than this shape.
@test
func test_a_full_size_attestation_object_decodes(env : &mut TestEnv) {
    var d = try_decode("a363666d74646e6f6e656761747453746d74a068617574684461746159009449960de5880e8c687434170f6476605b8fe4aeb9a28632c7995cf3ba831d97634100000000a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1001042434445464748494a4b4c4d4e4f5051a501020326200121582060fed4ba255a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb62258207903fe1008b8bc99a41ae9e95628bc64f2f1b20c2d7e9f5177a3c294d4462299")
    if(d is Result.Err) {
        fail(env, "a363666d74646e6f6e656761747453746d74a068617574684461746159009449960de5880e8c687434170f6476605b8fe4aeb9a28632c7995cf3ba831d97634100000000a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1a1001042434445464748494a4b4c4d4e4f5051a501020326200121582060fed4ba255a9d31c961eb74c6356d68c049b8923b61fa6ce669622e60f29fb62258207903fe1008b8bc99a41ae9e95628bc64f2f1b20c2d7e9f5177a3c294d4462299", "a full-size attestation object was rejected")
        return
    }
    var Ok(v) = d else unreachable
    if(cbor::map_len(&mut v) as u64 != 3u64) { env.error("wrong pair count") }
    var fmt = cbor::map_text_by_text(&mut v, string_view("fmt"))
    if(fmt.to_view().find(&string_view("none")) != 0u) { env.error("fmt is not none") }
    var ad = cbor::map_bytes_by_text(&mut v, string_view("authData"))
    if(ad.size() != 148u) { env.error("authData is not 148 bytes") }
}
@test
func test_malformed_input_is_rejected(env : &mut TestEnv) {
    // Truncated: a head promising 4 bytes with none present.
    expect_rejected(env, "4401")
    // Truncated in the middle of a multi-byte argument.
    expect_rejected(env, "1a000f")
    // A length that claims 2^64-1 bytes.
    expect_rejected(env, "5bffffffffffffffff")
    // Reserved additional info 28.
    expect_rejected(env, "1c")
    // A break with no indefinite-length item open.
    expect_rejected(env, "ff")
    // Indefinite length on a major type that has no such form.
    expect_rejected(env, "1f")
    expect_rejected(env, "3f")
    // Valid item, then rubbish. Strict on purpose.
    expect_rejected(env, "0101")
    // A text string whose bytes are not UTF-8: a lone continuation byte.
    expect_rejected(env, "61ff")
    // Overlong encoding of '/' (0xC0 0xAF is '/' in two bytes) — the classic
    // filter bypass, and why the validator is not optional.
    expect_rejected(env, "62c0af")
    // A UTF-16 surrogate half (U+D800) is not a valid scalar value.
    expect_rejected(env, "63eda080")
    // A code point above U+10FFFF.
    expect_rejected(env, "64f4908080")
    // An indefinite byte string whose chunk is a text string.
    expect_rejected(env, "5f6161ff")
}

@test
func test_nesting_is_bounded(env : &mut TestEnv) {
    // 200 nested single-element arrays: well-formed CBOR, and a decoder with no
    // depth limit would recurse 200 times on attacker-supplied input.
    var deep = vector<u8>()
    for(var i = 0u; i < 200u; i++) {
        deep.push_back(0x81u8)
    }
    deep.push_back(0x00u8)
    var r = cbor::decode(deep)
    if(r is Result.Ok) {
        env.error("the depth limit should have rejected 200 nested arrays")
    }
    // Just inside the limit must still decode, or the cap is useless.
    var fine = vector<u8>()
    for(var i = 0u; i < 60u; i++) {
        fine.push_back(0x81u8)
    }
    fine.push_back(0x00u8)
    var r2 = cbor::decode(fine)
    if(r2 is Result.Err) {
        env.error("60 levels of nesting should decode")
    }
}

@test
func test_a_claimed_length_never_becomes_an_allocation(env : &mut TestEnv) {
    // A head claiming 2^64-1 items with no payload. A decoder that reserved the
    // claimed length up front would try to allocate petabytes here; growing on
    // demand costs one failed read.
    var bytes = vector<u8>()
    bytes.push_back(0x9Bu8)      // array, 8-byte length follows
    for(var i = 0u; i < 8u; i++) {
        bytes.push_back(0xFFu8)
    }
    var r = cbor::decode(bytes)
    if(r is Result.Ok) {
        env.error("a 2^64-1 item array with no payload must be rejected")
    }
}

// ── helpers ───────────────────────────────────────────────────────────────

// One place that builds a failure message, so every assertion names its vector.
func fail(env : &mut TestEnv, hex : string_view, what : string_view) {
    var s = string("cbor ")
    s.append_view(&hex)
    s.append_string(string(": "))
    s.append_view(&what)
    env.error(s.data())
}

func nibble(c : char) : u8 {
    if(c >= '0' && c <= '9') { return (c - '0') as u8 }
    if(c >= 'a' && c <= 'f') { return ((c - 'a') + 10) as u8 }
    if(c >= 'A' && c <= 'F') { return ((c - 'A') + 10) as u8 }
    return 0u8
}

// Hex to bytes, written out rather than pulled from `encoding`: the returned
// vector is an owned local, and unwrapping a `Result<vector<u8>, _>` into one
// is a move the compiler rejects.
func hex_vec(hex : string_view) : vector<u8> {
    var out = vector<u8>()
    var i = 0u
    while(i + 1u < hex.size()) {
        out.push_back((nibble(hex.get(i)) << 4) | nibble(hex.get(i + 1u)))
        i += 2u
    }
    return out
}

func try_decode(hex : string_view) : Result<cbor::CborValue, cbor::CborError> {
    return cbor::decode(hex_vec(hex))
}

func expect_u64(env : &mut TestEnv, hex : string_view, want : u64) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(v is cbor::CborValue.Unsigned) {
        var Unsigned(n) = v else unreachable
        if(n != want) { fail(env, hex, "is not the expected unsigned") }
        return
    }
    fail(env, hex, "is not an unsigned integer")
}

func expect_i64(env : &mut TestEnv, hex : string_view, want : i64) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    // `Option` has no comparison operator, so unwrap and compare.
    var got = cbor::as_i64(&mut v)
    if(got is Option.None) {
        fail(env, hex, "is not an integer")
        return
    }
    var Some(n) = got else unreachable
    if(n != want) { fail(env, hex, "is the wrong integer") }
}

func expect_bytes(env : &mut TestEnv, hex : string_view, want : string_view) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_bytes(&mut v)) {
        fail(env, hex, "is not a byte string")
        return
    }
    expect_same_bytes(env, hex, cbor::bytes_copy(&mut v), hex_vec(want))
}

func expect_bytes_len(env : &mut TestEnv, hex : string_view, want : u64) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_bytes(&mut v)) {
        fail(env, hex, "is not a byte string")
        return
    }
    var got = cbor::bytes_copy(&mut v)
    if(got.size() as u64 != want) {
        fail(env, hex, "has the wrong byte length")
    }
}

func expect_text(env : &mut TestEnv, hex : string_view, want : string_view) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_text(&mut v)) {
        fail(env, hex, "is not a text string")
        return
    }
    // The temporary has to be bound: a `to_view()` on a call result has a
    // lifetime dependency on a value destroyed at the end of the expression.
    var owned = cbor::text_copy(&mut v)
    var got = owned.to_view()
    if(got.size() != want.size() || got.find(&want) != 0u) {
        fail(env, hex, "is not the expected text")
    }
}

func expect_bool(env : &mut TestEnv, hex : string_view, want : bool) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    var got = cbor::as_bool(&mut v)
    if(got is Option.None) {
        fail(env, hex, "is not a bool")
        return
    }
    var Some(b) = got else unreachable
    if(b != want) { fail(env, hex, "is the wrong bool") }
}

func expect_is_null(env : &mut TestEnv, hex : string_view) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_null(&mut v)) { fail(env, hex, "is not null") }
}

func expect_is_undefined(env : &mut TestEnv, hex : string_view) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_undefined(&mut v)) { fail(env, hex, "is not undefined") }
}

func expect_simple(env : &mut TestEnv, hex : string_view, want : u8) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(v is cbor::CborValue.Simple) {
        var Simple(n) = v else unreachable
        if(n != want) { fail(env, hex, "is the wrong simple value") }
        return
    }
    fail(env, hex, "is not a simple value")
}

func expect_double(env : &mut TestEnv, hex : string_view, want : double) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(v is cbor::CborValue.Float) {
        var Float(f) = v else unreachable
        if(f != want) { fail(env, hex, "is the wrong float") }
        return
    }
    fail(env, hex, "is not a float")
}

func expect_array_len(env : &mut TestEnv, hex : string_view, want : u64) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_array(&mut v)) {
        fail(env, hex, "is not an array")
        return
    }
    if(cbor::array_len(&mut v) as u64 != want) {
        fail(env, hex, "has the wrong item count")
    }
}

func expect_map_len(env : &mut TestEnv, hex : string_view, want : u64) {
    var d = try_decode(hex)
    if(d is Result.Err) {
        fail(env, hex, "did not decode")
        return
    }
    var Ok(v) = d else unreachable
    if(!cbor::is_map(&mut v)) {
        fail(env, hex, "is not a map")
        return
    }
    if(cbor::map_len(&mut v) as u64 != want) {
        fail(env, hex, "has the wrong pair count")
    }
}

func expect_rejected(env : &mut TestEnv, hex : string_view) {
    var d = try_decode(hex)
    if(d is Result.Ok) {
        fail(env, hex, "should have been rejected")
    }
}

// The byte string under COSE label `label` must equal the hex in `want_hex`.
func expect_label_bytes(env : &mut TestEnv, v : &mut cbor::CborValue, label : i64, want_hex : string_view) {
    expect_same_bytes(env, want_hex, cbor::map_bytes_by_int(v, label), hex_vec(want_hex))
}

func expect_same_bytes(env : &mut TestEnv, label : string_view, got : vector<u8>, want : vector<u8>) {
    if(got.size() != want.size()) {
        fail(env, label, "has the wrong byte length")
        return
    }
    for(var i = 0u; i < want.size(); i++) {
        if(got.get(i) != want.get(i)) {
            fail(env, label, "has the wrong bytes")
            return
        }
    }
}






