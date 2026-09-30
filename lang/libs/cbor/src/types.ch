// types.ch — the CBOR data model (RFC 8949).
//
// A CBOR document is a self-describing binary encoding built from eight "major
// types". Unlike JSON, the keys of a map may be ANY major type, not just text —
// which is exactly why COSE keys (the credential format WebAuthn hands us) are
// maps keyed by small integers, and why a JSON-shaped value type cannot
// represent them. `Map` therefore holds a flat key/value vector rather than an
// associative container.

public namespace cbor {

using std::string;
using std::vector;
using std::Result;

// A decoded CBOR document.
//
// Two shapes here are forced by the compiler rather than chosen:
//
//   * `Map` holds a FLAT key/value vector, not a vector of key/value structs.
//     The struct version is the obvious shape and it does not compile: a
//     `CborPair` holding a `CborValue` that (through `Map`) contains a
//     `CborPair` is "recursion in composition". A flat vector also matches the
//     wire format exactly and preserves duplicate keys, which a map keyed by
//     value would silently collapse. `map_len` is the pair count.
//   * `Tag` holds its item in a ONE-ELEMENT vector rather than inline. A
//     variant cannot contain itself directly — that is unbounded size — while
//     `vector<Self>` is fine, which is how `Array` gets away with the same
//     shape.
public variant CborValue {
    // Major type 0: 0 .. 2^64-1.
    Unsigned(value : u64)
    // Major type 1: the encoded value is -1 - n, so this holds the real number.
    Negative(value : i64)
    // Major type 2.
    Bytes(value : vector<u8>)
    // Major type 3. Always valid UTF-8; the decoder rejects anything else.
    Text(value : string)
    // Major type 4.
    Array(values : vector<CborValue>)
    // Major type 5: key/value pairs, flattened (k0, v0, k1, v1, …).
    Map(entries : vector<CborValue>)
    // Major type 6: a tagged value. The tag number is data and its meaning is
    // application-defined (RFC 8949 §3.4). `inner` holds exactly one item.
    Tag(number : u64, inner : vector<CborValue>)
    // Major type 7, simple values 0..23 and 32..255. 20/21/22/23 decode to
    // Bool/Null/Undefined below rather than landing here.
    Simple(value : u8)
    // Major type 7, half/single/double precision. Finite values only: the
    // infinities and NaNs are rejected — see CborError::Unsupported.
    Float(value : double)
    Bool(value : bool)
    Null()
    Undefined()
}

// A decode failure. Every case carries the byte offset it was detected at,
// because "invalid CBOR" without a position is close to useless when you are
// staring at a hex dump wondering which of 400 bytes the browser sent wrong.
public variant CborError {
    // Ran off the end of the input.
    UnexpectedEnd(offset : size_t)
    // Additional info 28, 29 or 30 — not assigned by RFC 8949.
    ReservedAdditionalInfo(ai : u8, offset : size_t)
    // Indefinite-length encoding used with a major type that has no such form
    // (only 2, 3, 4 and 5 may be indefinite), or a chunk of the wrong type
    // inside an indefinite string.
    IndefiniteNotAllowed(major : u8, offset : size_t)
    // The "break" stop code appeared where no indefinite-length item was open.
    UnexpectedBreak(offset : size_t)
    // A definite-length string claimed more bytes than the input holds.
    LengthOverrun(offset : size_t, wanted : u64, available : u64)
    // Major type 3 whose bytes are not well-formed UTF-8.
    NotUtf8(offset : size_t)
    // Nesting deeper than the decoder's limit. Untrusted input can otherwise
    // drive the parser into unbounded recursion.
    DepthExceeded(limit : u32)
    // Well-formed but not supported here. A CODE, not a message: this variant is
    // otherwise all-Copy, and a Copy type can be read out of the decoder's
    // `err` field without a move. Moving a field out of a struct that is
    // still in scope is rejected ("cannot move this value without
    // re-initializing memory"), and making the whole error type Copy is a much
    // smaller price than an unsafe bitwise-move helper.
    //
    //   1 = infinity or NaN, half precision
    //   2 = infinity or NaN, single precision
    //   3 = infinity or NaN, double precision
    //   4 = a negative integer below the i64 range
    //   5 = a single-precision float (not implemented; see decode.ch)
    //   6 = a double-precision float (not implemented; see decode.ch)
    Unsupported(reason : u8)
    // The input decoded cleanly but had bytes left over. Strict on purpose: a
    // caller that passes a buffer should be told about trailing garbage rather
    // than silently ignoring it.
    TrailingData(offset : size_t, total : u64)

    // Note on style: `string::append` takes a single `char` in Chemical, so
    // messages fold the literal into the initial `string(...)` and append only
    // numbers (or `append_string` for a dynamic string).
    func message(&self) : string {
        switch(self) {
            UnexpectedEnd(offset) => {
                var s = string("CborError: unexpected end of input at ")
                s.append_integer(offset as int)
                return s
            }
            ReservedAdditionalInfo(ai, offset) => {
                var s = string("CborError: reserved additional info ")
                s.append_integer(ai as int)
                s.append_string(string(" at "))
                s.append_integer(offset as int)
                return s
            }
            IndefiniteNotAllowed(major, offset) => {
                var s = string("CborError: indefinite length not allowed for major type ")
                s.append_integer(major as int)
                s.append_string(string(" at "))
                s.append_integer(offset as int)
                return s
            }
            UnexpectedBreak(offset) => {
                var s = string("CborError: break outside an indefinite-length item at ")
                s.append_integer(offset as int)
                return s
            }
            LengthOverrun(offset, wanted, available) => {
                var s = string("CborError: length ")
                s.append_integer(wanted as int)
                s.append_string(string(" overruns input at "))
                s.append_integer(offset as int)
                s.append_string(string(" ("))
                s.append_integer(available as int)
                s.append_string(string(" available)"))
                return s
            }
            NotUtf8(offset) => {
                var s = string("CborError: text string is not valid UTF-8 at ")
                s.append_integer(offset as int)
                return s
            }
            DepthExceeded(limit) => {
                var s = string("CborError: nesting deeper than ")
                s.append_integer(limit as int)
                return s
            }
            Unsupported(reason) => {
                var s = string("CborError: unsupported (")
                if(reason == 1u8) { s.append_string(string("infinity or NaN, half float")) }
                else if(reason == 2u8) { s.append_string(string("infinity or NaN, single float")) }
                else if(reason == 3u8) { s.append_string(string("infinity or NaN, double float")) }
                else if(reason == 5u8) { s.append_string(string("single-precision float, not implemented")) }
                else if(reason == 6u8) { s.append_string(string("double-precision float, not implemented")) }
                else { s.append_string(string("negative integer below the i64 range")) }
                s.append_string(string(")"))
                return s
            }
            TrailingData(offset, total) => {
                var s = string("CborError: ")
                s.append_integer((total - offset) as int)
                s.append_string(string(" trailing byte(s) after the top-level item, at "))
                s.append_integer(offset as int)
                return s
            }
        }
    }
}

// The nesting limit. WebAuthn's deepest document is an attestation object
// containing a COSE key containing a certificate chain — under 10. 64 leaves
// generous headroom while still bounding recursion on hostile input.
public const MAX_DEPTH : u32 = 64u

} // end namespace cbor


