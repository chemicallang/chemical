// decode.ch — the CBOR decoder (RFC 8949).
//
// Design notes that are not obvious from the code:
//
//   * The internal API returns `bool` and the decoder CARRIES its own error
//     (`self.err`). Two language constraints force this shape:
//       - a pattern match binds by reference, so a value cannot be moved onward
//         out of a `Result` ("cannot move this value without re-initializing
//         memory"), and
//       - `Result<Unit, E>` does not convert to `Result<T, E>`, so a helper
//         returning a value could not propagate its own error.
//     A decoder-held error sidesteps both: a helper returns false, sets
//     `self.err`, and the caller just propagates the false. Only the public
//     `decode` builds a `Result`.
//   * Every value is written through an out-parameter, so each nested item is
//     an owned local that can be moved freely into its parent.
//   * Nothing is reserved from a length the *input* claims. A CBOR head can
//     assert 2^64-1 items; growing on demand keeps a hostile header from
//     becoming a huge allocation. Each iteration still fails fast on the bytes
//     that are not there.
//   * Strict about trailing data. `decode` requires the whole buffer to be one
//     item, because for WebAuthn the buffer is attacker-influenced (it arrives
//     base64url-encoded in a POST body) and silently ignoring extra bytes is how
//     a parser gets confused about what it was given.
//   * Bounded recursion: one `depth` per nested item, capped at MAX_DEPTH.
//   * Floats are built arithmetically rather than by reinterpreting bits:
//     scaling by 2^n in a loop is exact in IEEE-754 (it only ever multiplies or
//     divides by two), so this needs no bit-cast primitive and loses no
//     precision.

public namespace cbor {

using std::string;
using std::vector;
using std::Result;
using std::string_view;

public struct CborDecoder {
    var data : vector<u8>
    var pos : size_t
    var err : CborError

    func remaining(&self) : u64 {
        if(self.pos >= self.data.size()) { return 0u64 }
        return (self.data.size() - self.pos) as u64
    }

    func fail(&mut self, e : CborError) : bool {
        self.err = e
        return false
    }

    // Big-endian unsigned integer of `n` bytes (RFC 8949 §3).
    func read_uint(&mut self, n : u64, out : &mut u64) : bool {
        if(self.remaining() < n) {
            return fail(CborError.LengthOverrun(self.pos, n, self.remaining()))
        }
        var v = 0u64
        for(var i = 0u64; i < n; i++) {
            // Read the cursor alone, then advance it. The earlier version of this
            // loop was `self.data.get(self.pos + (i as size_t))` and it decoded
            // "IETF" as "IT" and 0x3903e7 as -769 instead of -1000.
            //
            // That was OUR bug, not a compiler one, and it is worth writing down
            // because it looks exactly like a reordering bug: `pos` advances AND
            // `i` advances, so `pos + i` denotes indices 0, 2, 4, 6 - a stride
            // of 2. The compiler read precisely the position the source named.
            // `lang/tests/common/src/values/cursor_stride.ch` now pins that
            // semantic in the common tests, so the "the compiler reorders a
            // field read against a mutation in the same iteration" reading
            // cannot be mistaken for this again.
            //
            // The out-of-range `get` past the end returning 0 is what turned a
            // stride bug into "IT\0\0" instead of an obvious failure.
            var at = self.pos
            var byte_val = self.data.get(at)
            self.pos += 1u
            v = (v << 8) | (byte_val as u64)
        }
        *out = v
        return true
    }

    // Is the next byte the "break" stop code (0xFF)? Used inside an
    // indefinite-length item to know when to stop.
    func at_break(&mut self) : bool {
        if(self.pos >= self.data.size()) { return false }
        return self.data.get(self.pos) == 0xFFu8
    }

    func skip_break(&mut self) {
        self.pos += 1u
    }

    // The argument following a head byte, per its additional-info field. `ai`
    // must not be 31; the caller handles that (indefinite) form itself.
    func read_argument(&mut self, ai : u8, out : &mut u64) : bool {
        if(ai < 24u8) {
            *out = ai as u64
            return true
        }
        if(ai == 24u8) { return self.read_uint(1u64, out) }
        if(ai == 25u8) { return self.read_uint(2u64, out) }
        if(ai == 26u8) { return self.read_uint(4u64, out) }
        if(ai == 27u8) { return self.read_uint(8u64, out) }
        // 28, 29 and 30 are reserved; 31 never reaches here.
        return fail(CborError.ReservedAdditionalInfo(ai, self.pos - 1u))
    }

    // Append `len` bytes to `out` as the payload of a definite-length string.
    // The length is checked against the input BEFORE anything is reserved, so a
    // head claiming 2^64-1 bytes costs one comparison, not one allocation.
    func append_bytes(&mut self, len : u64, out : &mut vector<u8>) : bool {
        if(self.remaining() < len) {
            return fail(CborError.LengthOverrun(self.pos, len, self.remaining()))
        }
        for(var i = 0u64; i < len; i++) {
            // Read the cursor alone, then advance it - see read_uint for why
            var at = self.pos
            var byte_val = self.data.get(at)
            self.pos += 1u
            out.push_back(byte_val)
        }
        return true
    }

    // An indefinite-length byte or text string: a run of definite-length chunks
    // of the SAME major type, terminated by a break (RFC 8949 3.2.3).
    func read_indefinite_bytes(&mut self, major : u8, out : &mut vector<u8>) : bool {
        while(true) {
            if(self.at_break()) {
                self.skip_break()
                return true
            }
            if(self.pos >= self.data.size()) {
                return fail(CborError.UnexpectedEnd(self.pos))
            }
            var at = self.pos
            var initial = self.data.get(self.pos)
            self.pos += 1u
            var chunk_major = (initial >> 5) as u8
            var ai = initial & 0x1Fu8
            if(chunk_major != major) {
                // A chunk of a different type: not a valid indefinite string.
                return fail(CborError.IndefiniteNotAllowed(chunk_major, at))
            }
            if(ai == 31u8) {
                // RFC 8949 forbids a nested indefinite chunk here.
                return fail(CborError.IndefiniteNotAllowed(major, at))
            }
            var len = 0u64
            if(!self.read_argument(ai, &mut len)) { return false }
            if(!self.append_bytes(len, out)) { return false }
        }
        return false
    }

    // 2^e as a double, by repeated scaling. Exact: every step multiplies or
    // divides by two, neither of which rounds.
    func scale_pow2(mant : double, e : i64) : double {
        var v = mant
        var k = 0i64
        while(k < e) {
            v = v * 2.0
            k += 1i64
        }
        k = 0i64
        while(k > e) {
            v = v / 2.0
            k -= 1i64
        }
        return v
    }

    // A float of the width implied by `ai` (25 = half, 26 = single, 27 = double).
    func read_float(&mut self, ai : u8, out : &mut CborValue) : bool {
        if(ai == 25u8) {
            var h = 0u64
            if(!self.read_uint(2u64, &mut h)) { return false }
            var neg = (h & 0x8000u64) != 0u64
            var exp = ((h >> 10) & 0x1Fu64) as i64
            var mant = h & 0x3FFu64
            if(exp == 31) {
                return fail(CborError.Unsupported(1u8))
            }
            // exp == 0 is subnormal (or zero): no implicit leading bit.
            var v = scale_pow2(mant as double, exp - 24i64)
            if(exp != 0) {
                v = scale_pow2((mant + 1024u64) as double, exp - 25i64)
            }
            if(neg) { v = 0.0 - v }
            *out = CborValue.Float(v)
            return true
        }
        // Single and DOUBLE precision are REJECTED rather than decoded.
        //
        // A deliberate incompleteness, not an oversight. The half-float path
        // above is verified against RFC 8949 Appendix A. The single- and
        // double-precision paths were written and then failed their own RFC
        // vectors (0xfa47c35000, whose RFC value is 100000.0, came out wrong),
        // and a parser that returns a confidently WRONG number is worse than one
        // that refuses. A value a security-relevant consumer might have trusted
        // has to be either right or absent.
        //
        // Nothing in WebAuthn uses CBOR floats - an attestation object is maps,
        // byte strings, integers and text - so this costs the passkey work
        // nothing. Re-enabling is self-contained: the arithmetic is
        // `2^(exp-bias) + mant * 2^(exp-bias-52)`, both terms exact, and
        // `scale_pow2` is already correct for the half case.
        if(ai == 26u8) {
            return fail(CborError.Unsupported(5u8))
        }
        return fail(CborError.Unsupported(6u8))
    }

    // A major-type-3 payload. RFC 8949 §3.1 requires well-formed UTF-8 and it
    // is worth enforcing: a Chemical `string` holding invalid UTF-8 is a latent
    // bug in every consumer, and WebAuthn's text fields are exactly where a
    // hostile authenticator would probe.
    func make_text(&mut self, bytes : &vector<u8>, out : &mut CborValue) : bool {
        if(!is_valid_utf8(bytes)) {
            return fail(CborError.NotUtf8(self.pos))
        }
        if(bytes.size() == 0) {
            *out = CborValue.Text(string(""))
            return true
        }
        *out = CborValue.Text(string.constructor(bytes.data() as *char, bytes.size()))
        return true
    }

    func read_array(&mut self, len : u64, depth : u32, out : &mut CborValue) : bool {
        var items = vector<CborValue>()
        for(var i = 0u64; i < len; i++) {
            var item = CborValue.Null()
            if(!self.read_item(depth + 1u, &mut item)) { return false }
            items.push_back(item)
        }
        *out = CborValue.Array(items)
        return true
    }

    func read_indefinite_array(&mut self, depth : u32, out : &mut CborValue) : bool {
        var items = vector<CborValue>()
        while(true) {
            if(self.at_break()) {
                self.skip_break()
                *out = CborValue.Array(items)
                return true
            }
            var item = CborValue.Null()
            if(!self.read_item(depth + 1u, &mut item)) { return false }
            items.push_back(item)
        }
        return false
    }

    func read_map(&mut self, len : u64, depth : u32, out : &mut CborValue) : bool {
        var entries = vector<CborValue>()
        for(var i = 0u64; i < len; i++) {
            var key = CborValue.Null()
            if(!self.read_item(depth + 1u, &mut key)) { return false }
            var val = CborValue.Null()
            if(!self.read_item(depth + 1u, &mut val)) { return false }
            entries.push_back(key)
            entries.push_back(val)
        }
        *out = CborValue.Map(entries)
        return true
    }

    func read_indefinite_map(&mut self, depth : u32, out : &mut CborValue) : bool {
        var entries = vector<CborValue>()
        while(true) {
            if(self.at_break()) {
                self.skip_break()
                *out = CborValue.Map(entries)
                return true
            }
            var key = CborValue.Null()
            if(!self.read_item(depth + 1u, &mut key)) { return false }
            var val = CborValue.Null()
            if(!self.read_item(depth + 1u, &mut val)) { return false }
            entries.push_back(key)
            entries.push_back(val)
        }
        return false
    }

    // One complete data item, written into `out`.
    func read_item(&mut self, depth : u32, out : &mut CborValue) : bool {
        if(depth > MAX_DEPTH) {
            return fail(CborError.DepthExceeded(MAX_DEPTH))
        }
        if(self.pos >= self.data.size()) {
            return fail(CborError.UnexpectedEnd(self.pos))
        }
        var start = self.pos
        var initial = self.data.get(self.pos)
        self.pos += 1u
        var major = (initial >> 5) as u8
        var ai = initial & 0x1Fu8

        // Major 7: simple values and floats. Handled before the shared argument
        // path because ai 25/26/27 mean "a float", not "a number".
        if(major == 7u8) {
            if(ai == 31u8) {
                return fail(CborError.UnexpectedBreak(start))
            }
            if(ai < 20u8) {
                *out = CborValue.Simple(ai)
                return true
            }
            if(ai == 20u8) {
                *out = CborValue.Bool(false)
                return true
            }
            if(ai == 21u8) {
                *out = CborValue.Bool(true)
                return true
            }
            if(ai == 22u8) {
                *out = CborValue.Null()
                return true
            }
            if(ai == 23u8) {
                *out = CborValue.Undefined()
                return true
            }
            if(ai == 24u8) {
                var sv = 0u64
                if(!self.read_uint(1u64, &mut sv)) { return false }
                *out = CborValue.Simple((sv & 0xFFu64) as u8)
                return true
            }
            if(ai == 25u8 || ai == 26u8 || ai == 27u8) {
                return self.read_float(ai, out)
            }
            return fail(CborError.ReservedAdditionalInfo(ai, start))
        }

        // Indefinite length: only the two string types and the two container
        // types have that form.
        if(ai == 31u8) {
            if(major == 2u8) {
                var bytes = vector<u8>()
                if(!self.read_indefinite_bytes(2u8, &mut bytes)) { return false }
                *out = CborValue.Bytes(bytes)
                return true
            }
            if(major == 3u8) {
                var bytes = vector<u8>()
                if(!self.read_indefinite_bytes(3u8, &mut bytes)) { return false }
                return self.make_text(&bytes, out)
            }
            if(major == 4u8) { return self.read_indefinite_array(depth, out) }
            if(major == 5u8) { return self.read_indefinite_map(depth, out) }
            return fail(CborError.IndefiniteNotAllowed(major, start))
        }

        var arg = 0u64
        if(!self.read_argument(ai, &mut arg)) { return false }

        if(major == 0u8) {
            *out = CborValue.Unsigned(arg)
            return true
        }
        if(major == 1u8) {
            // The wire form is -1 - n. The largest n (2^64-1) is -2^64, which
            // does not fit an i64, so that one input is out of range here.
            if(arg > 9223372036854775807u64) {
                return fail(CborError.Unsupported(4u8))
            }
            *out = CborValue.Negative(-1i64 - (arg as i64))
            return true
        }
        if(major == 2u8) {
            var bytes = vector<u8>()
            if(!self.append_bytes(arg, &mut bytes)) { return false }
            *out = CborValue.Bytes(bytes)
            return true
        }
        if(major == 3u8) {
            var bytes = vector<u8>()
            if(!self.append_bytes(arg, &mut bytes)) { return false }
            return self.make_text(&bytes, out)
        }
        if(major == 4u8) { return self.read_array(arg, depth, out) }
        if(major == 5u8) { return self.read_map(arg, depth, out) }
        // Major 6: a tag. The tagged item is read at the SAME depth — a tag is
        // transparent, not a level of nesting.
        var inner = CborValue.Null()
        if(!self.read_item(depth, &mut inner)) { return false }
        var boxed = vector<CborValue>()
        boxed.push_back(inner)
        *out = CborValue.Tag(arg, boxed)
        return true
    }
}

// Decode exactly one CBOR item from `data`, which must be fully consumed.
public func decode(data : vector<u8>) : Result<CborValue, CborError> {
    var d = CborDecoder {
        data : data,
        pos : 0u,
        err : CborError.UnexpectedEnd(0u)
    }
    var v = CborValue.Null()
    if(!d.read_item(0u, &mut v)) {
        return Result.Err<CborValue, CborError>(d.err)
    }
    if(d.pos != d.data.size()) {
        return Result.Err<CborValue, CborError>(CborError.TrailingData(d.pos, d.data.size() as u64))
    }
    return Result.Ok<CborValue, CborError>(v)
}

// Why `data` did not decode, or an empty string if it did.
//
// This exists because of a real ergonomic hole: a caller holding a
// `Result<CborValue, CborError>` cannot read the error out of it � a pattern
// match binds by reference and the language rejects moving that binding � so
// `Err` is otherwise write-only. Re-decoding to produce a message is cheap
// next to staring at a hex dump wondering which byte the parser disliked.
public func explain(data : vector<u8>) : string {
    var d = CborDecoder {
        data : data,
        pos : 0u,
        err : CborError.UnexpectedEnd(0u)
    }
    var v = CborValue.Null()
    if(d.read_item(0u, &mut v)) {
        if(d.pos != d.data.size()) {
            return string("trailing data")
        }
        return string("")
    }
    return d.err.message()
}
} // end namespace cbor





