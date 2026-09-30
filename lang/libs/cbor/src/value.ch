// value.ch — accessors and UTF-8 validation.
//
// The decoder is deliberately generic; this is the layer that makes a decoded
// document pleasant to consume. COSE keys — what WebAuthn hands us — are maps
// keyed by small integers, several negative, so the `map_*_by_int` lookups are
// the workhorse.
//
// ── Why the accessors are shaped the way they are ─────────────────────────
//
// Chemical will not let a variant's payload be moved out of a borrow: `var
// Bytes(b) = v` where `v` is a `&CborValue` binds by reference, and moving `b`
// onward is rejected ("cannot move this value without re-initializing memory").
// The stdlib's own JSON decoder works around exactly this: it extracts Copy
// payloads directly (bools, integers) and only ever *borrows* containers,
// wrapping them in a `span` or an iterator.
//
// So nothing here hands a `CborValue` payload back to a caller. Every accessor
// returns an owned copy of a Copy type, or a container rebuilt element by
// element — one copy of the bytes the caller actually asked for, and no moves
// at all. That is why the API is a set of narrow lookups
// (`map_bytes_by_int`) rather than a general `as_bytes() -> payload`.
//
// Container elements are reached with `get_ptr` and `&mut *`, not `get`:
// `Vector::get` has a `T : Copy` constraint and `CborPair` is not Copy.

public namespace cbor {

using std::string;
using std::string_view;
using std::vector;
using std::Result;
using std::Option;

// ── UTF-8 validation ──────────────────────────────────────────────────────
//
// Rejects what a strict decoder must: truncated sequences, bad continuation
// bytes, overlong encodings (the classic smuggle — 0xC0 0x80 is NUL in two
// bytes), UTF-16 surrogates (U+D800..U+DFFF are not scalar values), and
// anything above U+10FFFF.

func is_valid_utf8(bytes : &vector<u8>) : bool {
    var i = 0u
    var n = bytes.size()
    while(i < n) {
        var b0 = bytes.get(i) as u32
        if(b0 < 0x80u32) {
            i += 1u
            continue
        }
        var extra = 0u32
        var cp = 0u32
        var min_cp = 0u32
        if(b0 >= 0xC2u32 && b0 <= 0xDFu32) {
            extra = 1u32
            cp = b0 & 0x1Fu32
            min_cp = 0x80u32
        } else if(b0 >= 0xE0u32 && b0 <= 0xEFu32) {
            extra = 2u32
            cp = b0 & 0x0Fu32
            min_cp = 0x800u32
        } else if(b0 >= 0xF0u32 && b0 <= 0xF4u32) {
            extra = 3u32
            cp = b0 & 0x07u32
            min_cp = 0x10000u32
        } else {
            // 0x80..0xC1 (a continuation byte, or an overlong lead) and
            // 0xF5..0xFF (beyond U+10FFFF).
            return false
        }
        if(i + (extra as size_t) + 1u > n) { return false }
        for(var k = 1u32; k <= extra; k++) {
            var bk = bytes.get(i + (k as size_t)) as u32
            if(bk < 0x80u32 || bk > 0xBFu32) { return false }
            cp = (cp << 6) | (bk & 0x3Fu32)
        }
        if(cp < min_cp) { return false }
        if(cp >= 0xD800u32 && cp <= 0xDFFFu32) { return false }
        if(cp > 0x10FFFFu32) { return false }
        i += (extra as size_t) + 1u
    }
    return true
}

// ── Scalar accessors ──────────────────────────────────────────────────────
//
// Integers, bools and simple values are Copy, so their payloads come straight
// out of the borrow.

public func as_u64(v : &CborValue) : Option<u64> {
    if(v is CborValue.Unsigned) {
        var Unsigned(n) = v else unreachable
        return Option.Some<u64>(n)
    }
    return Option.None<u64>()
}

// An integer as i64, whichever major type it was encoded with. `None` if it is
// not an integer, or does not fit.
public func as_i64(v : &CborValue) : Option<i64> {
    if(v is CborValue.Unsigned) {
        var Unsigned(n) = v else unreachable
        if(n > 9223372036854775807u64) { return Option.None<i64>() }
        return Option.Some<i64>(n as i64)
    }
    if(v is CborValue.Negative) {
        var Negative(n) = v else unreachable
        return Option.Some<i64>(n)
    }
    return Option.None<i64>()
}

public func as_bool(v : &CborValue) : Option<bool> {
    if(v is CborValue.Bool) {
        var Bool(b) = v else unreachable
        return Option.Some<bool>(b)
    }
    return Option.None<bool>()
}

public func is_null(v : &CborValue) : bool {
    return v is CborValue.Null
}

public func is_undefined(v : &CborValue) : bool {
    return v is CborValue.Undefined
}

public func is_map(v : &CborValue) : bool {
    return v is CborValue.Map
}

public func is_array(v : &CborValue) : bool {
    return v is CborValue.Array
}

public func is_bytes(v : &CborValue) : bool {
    return v is CborValue.Bytes
}

public func is_text(v : &CborValue) : bool {
    return v is CborValue.Text
}

public func is_number(v : &CborValue) : bool {
    return v is CborValue.Unsigned || v is CborValue.Negative
}

// The tag number of a tagged value, if it is tagged.
public func tag_number(v : &CborValue) : Option<u64> {
    if(v is CborValue.Tag) {
        var Tag(n, boxed) = v else unreachable
        return Option.Some<u64>(n)
    }
    return Option.None<u64>()
}

// ── Copying out ───────────────────────────────────────────────────────────

// A copy of a byte string's payload, rebuilt element by element (u8 is Copy, so
// this is a read, not a move). Empty when `v` is not a byte string, so a caller
// that must distinguish "absent" from "empty" checks `is_bytes` first — which
// matters for WebAuthn, where a missing `x5c` and an empty one differ.
public func bytes_copy(v : &CborValue) : vector<u8> {
    if(v is CborValue.Bytes) {
        var Bytes(b) = v else unreachable
        var out = vector<u8>()
        for(var i = 0u; i < b.size(); i++) {
            out.push_back(b.get(i))
        }
        return out
    }
    return vector<u8>()
}

// A copy of a text string's payload. `string` has a `copy()`, so this is a
// real copy rather than a move.
public func text_copy(v : &CborValue) : string {
    if(v is CborValue.Text) {
        var Text(t) = v else unreachable
        return t.copy()
    }
    return string("")
}

// ── Map lookups ───────────────────────────────────────────────────────────
//
// Each takes the map itself rather than its entry vector, so a caller never has
// to extract the container. Entries are a flat key/value vector, so a lookup
// steps two at a time. COSE keys use integer labels: 1 = kty, 3 = alg,
// -1 = crv, -2 = x, -3 = y (RFC 9052 §7).

// The number of PAIRS in a map (the entry vector holds two values per pair).
public func map_len(v : &CborValue) : size_t {
    if(v is CborValue.Map) {
        var Map(m) = v else unreachable
        return m.size() / 2u
    }
    return 0u
}

public func array_len(v : &CborValue) : size_t {
    if(v is CborValue.Array) {
        var Array(a) = v else unreachable
        return a.size()
    }
    return 0u
}

// The value at pair `index`, or null when out of range. A pointer, so a caller
// can read it without moving the payload out of the variant.
func map_value_at(v : &CborValue, index : size_t) : *mut CborValue {
    if(!(v is CborValue.Map)) { return null }
    var Map(m) = v else unreachable
    if((index + 1u) * 2u > m.size()) { return null }
    return m.get_ptr(index * 2u + 1u)
}

// The index of the pair whose key is the integer `key`, or -1.
func map_find_int(v : &CborValue, key : i64) : int {
    if(!(v is CborValue.Map)) { return -1 }
    var Map(m) = v else unreachable
    var pairs = m.size() / 2u
    for(var i = 0u; i < pairs; i++) {
        var got = as_i64(&mut *m.get_ptr(i * 2u))
        if(got is Option.None) { continue }
        var Some(kv) = got else unreachable
        if(kv == key) { return i as int }
    }
    return -1
}

// `string_view` has no `==`, so a text key is matched on length then position.
func text_equals(t : &CborValue, key : string_view) : bool {
    if(!(t is CborValue.Text)) { return false }
    var Text(s) = t else unreachable
    var tv = s.to_view()
    return tv.size() == key.size() && tv.find(&key) == 0u
}

// The index of the pair whose key is the text `key`, or -1.
func map_find_text(v : &CborValue, key : string_view) : int {
    if(!(v is CborValue.Map)) { return -1 }
    var Map(m) = v else unreachable
    var pairs = m.size() / 2u
    for(var i = 0u; i < pairs; i++) {
        if(text_equals(&mut *m.get_ptr(i * 2u), key)) { return i as int }
    }
    return -1
}

// The byte string stored under an integer key. Empty when the key is absent or
// the value is not a byte string.
public func map_bytes_by_int(v : &CborValue, key : i64) : vector<u8> {
    var i = map_find_int(v, key)
    if(i < 0) { return vector<u8>() }
    return bytes_copy(&mut *map_value_at(v, i as size_t))
}

// The text stored under an integer key. Empty when absent or not text.
public func map_text_by_int(v : &CborValue, key : i64) : string {
    var i = map_find_int(v, key)
    if(i < 0) { return string("") }
    return text_copy(&mut *map_value_at(v, i as size_t))
}

// The integer stored under an integer key, or `fallback` when the key is
// absent or the value is not an integer. COSE labels are small and signed, so
// a fallback of 0 means "no such label".
public func map_int_by_int(v : &CborValue, key : i64, fallback : i64) : i64 {
    var i = map_find_int(v, key)
    if(i < 0) { return fallback }
    var got = as_i64(&mut *map_value_at(v, i as size_t))
    if(got is Option.None) { return fallback }
    var Some(n) = got else unreachable
    return n
}

// Whether an integer key is present, so a caller can tell a stored zero from
// an absent label.
public func map_has_int(v : &CborValue, key : i64) : bool {
    return map_find_int(v, key) >= 0
}

// Whether a text key is present, for the same reason.
public func map_has_text(v : &CborValue, key : string_view) : bool {
    return map_find_text(v, key) >= 0
}

// The text stored under a text key. Empty when absent.
public func map_text_by_text(v : &CborValue, key : string_view) : string {
    var i = map_find_text(v, key)
    if(i < 0) { return string("") }
    return text_copy(&mut *map_value_at(v, i as size_t))
}

// The byte string stored under a text key. Empty when absent.
//
// Needed for the shapes WebAuthn hands over, where a text-keyed map holds byte
// strings: a CBOR attestation object is {"fmt": "none", "attStmt": {},
// "authData": h'...'}, and the credential public key inside its authData is
// reached the same way.
public func map_bytes_by_text(v : &CborValue, key : string_view) : vector<u8> {
    var i = map_find_text(v, key)
    if(i < 0) { return vector<u8>() }
    return bytes_copy(&mut *map_value_at(v, i as size_t))
}

// A pointer to the value stored under a text key, or null when absent.
//
// A POINTER rather than a copy, because a nested value is often a whole
// sub-document and copying one out is a deep copy the caller almost never
// wants. The caller reads through it with the same accessors, one level down:
//
//   var stmt = cbor::map_value_by_text(&attestation, string_view("attStmt"))
//   var sig  = cbor::map_bytes_by_text(stmt, string_view("sig"))
//
// That is the shape a WebAuthn attestation statement has: a map inside a map.
// Without this, the accessors above can only ever see the top level, and a
// caller would be pushed into re-decoding raw bytes it never had.
public func map_value_by_text(v : &CborValue, key : string_view) : *mut CborValue {
    var i = map_find_text(v, key)
    if(i < 0) { return null }
    return map_value_at(v, i as size_t)
}

// The same, for an integer key.
public func map_value_by_int(v : &CborValue, key : i64) : *mut CborValue {
    var i = map_find_int(v, key)
    if(i < 0) { return null }
    return map_value_at(v, i as size_t)
}

// The value at `index` of an array, as a pointer, so an array of sub-documents
// can be walked. Null when out of range.
public func array_value_at(v : &CborValue, index : size_t) : *mut CborValue {
    if(!(v is CborValue.Array)) { return null }
    var Array(a) = v else unreachable
    if(index >= a.size()) { return null }
    return a.get_ptr(index)
}

// The byte string at `index` of an array. An attestation certificate chain is
// an array of byte strings, so this is how `x5c` is walked. Empty when out of
// range or not a byte string.
public func array_bytes_at(v : &CborValue, index : size_t) : vector<u8> {
    if(!(v is CborValue.Array)) { return vector<u8>() }
    var Array(a) = v else unreachable
    if(index >= a.size()) { return vector<u8>() }
    return bytes_copy(&mut *a.get_ptr(index))
}

// The integer at `index` of an array, or `fallback` when out of range or not
// an integer.
public func array_int_at(v : &CborValue, index : size_t, fallback : i64) : i64 {
    if(!(v is CborValue.Array)) { return fallback }
    var Array(a) = v else unreachable
    if(index >= a.size()) { return fallback }
    var got = as_i64(&mut *a.get_ptr(index))
    if(got is Option.None) { return fallback }
    var Some(n) = got else unreachable
    return n
}

// A short human-readable rendering, for diagnostics and test output. NOT
// canonical CBOR and not meant to be parsed again - it exists so a failing
// assertion or a log line shows what the decoder actually produced.
//
// This is also the only practical way to see a decoded value: the error side of
// a `Result` cannot be read out (see decode.ch), and a `CborValue`'s payloads
// cannot be moved out of a borrow, so there is no other way to print one.
public func describe(v : &CborValue) : string {
    if(v is CborValue.Unsigned) {
        var Unsigned(n) = v else unreachable
        var s = string("u:")
        s.append_integer(n as int)
        return s
    }
    if(v is CborValue.Negative) {
        var Negative(n) = v else unreachable
        var s = string("i:")
        s.append_integer(n as int)
        return s
    }
    if(v is CborValue.Bytes) {
        var s = string("h:")
        var Bytes(b) = v else unreachable
        for(var i = 0u; i < b.size(); i++) {
            var one = b.get(i)
            var pair = string("")
            pair.append_integer(((one >> 4) as int) + 48)
            pair.append_integer((one & 0x0Fu8) as int + 48)
            s.append_string(&pair)
        }
        return s
    }
    if(v is CborValue.Text) {
        var s = string("\"")
        var Text(t) = v else unreachable
        s.append_string(&t)
        s.append_string(string("\""))
        return s
    }
    if(v is CborValue.Bool) {
        var Bool(b) = v else unreachable
        if(b) { return string("true") }
        return string("false")
    }
    if(v is CborValue.Null) { return string("null") }
    if(v is CborValue.Undefined) { return string("undefined") }
    if(v is CborValue.Simple) {
        var Simple(n) = v else unreachable
        var s = string("simple:")
        s.append_integer(n as int)
        return s
    }
    if(v is CborValue.Float) {
        var Float(f) = v else unreachable
        var s = string("f:")
        s.append_double(f, 8)
        return s
    }
    if(v is CborValue.Array) {
        var s = string("[")
        var Array(a) = v else unreachable
        for(var i = 0u; i < a.size(); i++) {
            if(i > 0u) { s.append_string(string(",")) }
            s.append_string(describe(&mut *a.get_ptr(i)))
        }
        s.append_string(string("]"))
        return s
    }
    if(v is CborValue.Map) {
        var s = string("{")
        var Map(m) = v else unreachable
        var pairs = m.size() / 2u
        for(var i = 0u; i < pairs; i++) {
            if(i > 0u) { s.append_string(string(",")) }
            s.append_string(describe(&mut *m.get_ptr(i * 2u)))
            s.append_string(string(":"))
            s.append_string(describe(&mut *m.get_ptr(i * 2u + 1u)))
        }
        s.append_string(string("}"))
        return s
    }
    if(v is CborValue.Tag) {
        var Tag(n, boxed) = v else unreachable
        var s = string("tag:")
        s.append_integer(n as int)
        s.append_string(string("("))
        if(boxed.size() > 0u) {
            s.append_string(describe(&mut *boxed.get_ptr(0u)))
        }
        s.append_string(string(")"))
        return s
    }
    return string("?")
}
} // end namespace cbor




