// ===== gzip (RFC 1952) — precompressed assets =====
//
// A browser caches compressed bytes, not decoded ones, so shipping a JS bundle
// gzipped is a transfer-size win on every request that is not already behind a
// compressing CDN. This is the repo's own encoder and decoder, so a build never
// depends on a native zlib being present on the machine doing the build.
//
// The deflate stream is **fixed-Huffman with greedy LZ77** (32 KiB window, hash
// chains over 3-byte prefixes). That is the simplest correct encoder that still
// gets most of the win on generated JavaScript: dynamic Huffman would be a few
// percent smaller and much more code, so it is deliberately not here. Compression
// is a build-time cost (`StaticSite.build()`), never per request.
//
// Bit order follows the spec exactly and is the part that is easy to get wrong:
// deflate packs bits into bytes low-bit first, while a Huffman code is written
// most-significant bit first (`gz_put_bits` vs `gz_put_code`). The decoder mirrors
// that, which is why `gunzip` can verify `gzip`'s output in-process (the library
// tests round-trip, and `scripts/static-site-test.sh` cross-checks with node's
// zlib so an outside implementation agrees too).
//
// Determinism: the gzip header carries no timestamp (`MTIME = 0`) and the encoder
// has no randomness, so the same input always produces the same bytes and a
// content hash still names the output it describes.

public namespace compression {

using std::Result;
using std::string_view;
using std::vector;

// ── Bit output ───────────────────────────────────────────────────────────────

// `<<` and `>>` widen their result to `long` even when both operands are `uint`,
// and a `long` then does not assign back to a `uint`. These keep the bit
// arithmetic in `uint` with one cast each, instead of one at every shift.
func gz_shl(value : uint, n : uint) : uint {
    return (value << n) as uint
}

func gz_shr(value : uint, n : uint) : uint {
    return (value >> n) as uint
}

// Accumulates bits into bytes, low-bit first. `failed` is set when the output
// buffer cannot grow: the caller turns that into `CompressionError.OutOfMemory()`
// instead of a panic.
struct GzOut {
    var bytes : vector<u8>
    // A 64-bit accumulator, not a byte-at-a-time one: a Huffman code is up to 9
    // bits, so `acc`/`count` hold a whole code plus the previous partial byte and
    // a call emits its bytes with a shift instead of a loop over bits. The bit
    // stream is unchanged; this is the same encoder, written so a 3 KB page does
    // not cost 20,000 function calls.
    var acc : u64
    var count : u64
    var failed : bool
}

// Built in place by the caller, never returned from a helper: a struct holding a
// `vector` must not be copied out of a function (the copy shares the buffer with
// the temporary that owns it, which then frees it).
func gz_out_init(out : &mut GzOut) {
    if(!out.bytes.try_reserve(4096)) { out.failed = true }
}

func gz_push_byte(out : &mut GzOut, value : uint) {
    if(out.failed) { return }
    if(!out.bytes.try_push(value as u8)) { out.failed = true }
}

func gz_put_bit(out : &mut GzOut, bit : uint) {
    gz_put_bits(out, bit & 1, 1)
}

// `n` bits of `value`, least-significant bit first: headers, lengths, distances.
func gz_put_bits(out : &mut GzOut, value : uint, n : uint) {
    if(out.failed || n == 0) { return }
    var v : u64 = value as u64
    if(n < 32) { v = v & (((1 as u64) << (n as u64)) - 1) }
    out.acc = out.acc | (v << out.count)
    out.count = out.count + (n as u64)
    while(out.count >= 8) {
        gz_push_byte(out, (out.acc & 0xFF) as uint)
        out.acc = out.acc >> 8
        out.count = out.count - 8
    }
}

// A Huffman code, most-significant bit first (deflate packs codes that way).
// Reversing first and then writing least-significant bit first is the same bit
// sequence, and it is one bulk write instead of a call per bit.
func gz_put_code(out : &mut GzOut, code : uint, n : uint) {
    var reversed : uint = 0
    var i : uint = 0
    while(i < n) {
        reversed = gz_shl(reversed, 1) | (gz_shr(code, i) & 1)
        i = i + 1
    }
    gz_put_bits(out, reversed, n)
}

// Pad the last byte with zero bits so the next field starts byte-aligned.
func gz_align(out : &mut GzOut) {
    if(out.failed || out.count == 0) { return }
    gz_push_byte(out, (out.acc & 0xFF) as uint)
    out.acc = 0
    out.count = 0
}

func gz_put_u32_le(out : &mut GzOut, value : uint) {
    gz_put_bits(out, value & 0xFF, 8)
    gz_put_bits(out, gz_shr(value, 8) & 0xFF, 8)
    gz_put_bits(out, gz_shr(value, 16) & 0xFF, 8)
    gz_put_bits(out, gz_shr(value, 24) & 0xFF, 8)
}

// ── The fixed Huffman literal/length code (RFC 1951 §3.2.6) ──────────────────

func gz_put_literal(out : &mut GzOut, sym : uint) {
    if(sym <= 143) {
        gz_put_code(out, 0x30 + sym, 8)
    } else if(sym <= 255) {
        gz_put_code(out, 0x190 + (sym - 144), 9)
    } else if(sym <= 279) {
        gz_put_code(out, sym - 256, 7)
    } else {
        gz_put_code(out, 0xC0 + (sym - 280), 8)
    }
}

// The length code (257..285) for a match of `len` bytes (3..258).
func gz_length_code(len : uint) : uint {
    if(len >= 258) { return 285 }
    if(len <= 10) { return 257 + (len - 3) }
    if(len <= 18) { return 265 + gz_shr(len - 11, 1) }
    if(len <= 34) { return 269 + gz_shr(len - 19, 2) }
    if(len <= 66) { return 273 + gz_shr(len - 35, 3) }
    if(len <= 130) { return 277 + gz_shr(len - 67, 4) }
    return 281 + gz_shr(len - 131, 5)
}

func gz_length_extra(code : uint) : uint {
    if(code <= 264) { return 0 }
    if(code <= 268) { return 1 }
    if(code <= 272) { return 2 }
    if(code <= 276) { return 3 }
    if(code <= 280) { return 4 }
    if(code <= 284) { return 5 }
    return 0
}

// First length a code covers; the extra bits select within its range.
func gz_length_base(code : uint) : uint {
    if(code <= 264) { return 3 + (code - 257) }
    if(code <= 268) { return 11 + gz_shl(code - 265, 1) }
    if(code <= 272) { return 19 + gz_shl(code - 269, 2) }
    if(code <= 276) { return 35 + gz_shl(code - 273, 3) }
    if(code <= 280) { return 67 + gz_shl(code - 277, 4) }
    if(code <= 284) { return 131 + gz_shl(code - 281, 5) }
    return 258
}

// The distance code (0..29) for a match `dist` bytes back (1..32768).
func gz_dist_code(dist : uint) : uint {
    if(dist >= 24577) { return 29 }
    if(dist >= 16385) { return 28 }
    if(dist >= 12289) { return 27 }
    if(dist >= 8193) { return 26 }
    if(dist >= 6145) { return 25 }
    if(dist >= 4097) { return 24 }
    if(dist >= 3073) { return 23 }
    if(dist >= 2049) { return 22 }
    if(dist >= 1537) { return 21 }
    if(dist >= 1025) { return 20 }
    if(dist >= 769) { return 19 }
    if(dist >= 513) { return 18 }
    if(dist >= 385) { return 17 }
    if(dist >= 257) { return 16 }
    if(dist >= 193) { return 15 }
    if(dist >= 129) { return 14 }
    if(dist >= 97) { return 13 }
    if(dist >= 65) { return 12 }
    if(dist >= 49) { return 11 }
    if(dist >= 33) { return 10 }
    if(dist >= 25) { return 9 }
    if(dist >= 17) { return 8 }
    if(dist >= 13) { return 7 }
    if(dist >= 9) { return 6 }
    if(dist >= 7) { return 5 }
    if(dist >= 5) { return 4 }
    if(dist >= 4) { return 3 }
    if(dist >= 3) { return 2 }
    if(dist >= 2) { return 1 }
    return 0
}

func gz_dist_extra(code : uint) : uint {
    if(code <= 3) { return 0 }
    return gz_shr(code, 1) - 1
}

// First distance a code covers. Codes 0..3 are single distances; from 4 on each
// code covers `2^extra` consecutive distances, so summing the previous ranges is
// both exact and shorter than a 30-entry table that has to stay in step with
// `gz_dist_extra`.
func gz_dist_base(code : uint) : uint {
    var base : uint = 1
    var c : uint = 0
    while(c < code) {
        base = base + gz_shl(1, gz_dist_extra(c))
        c = c + 1
    }
    return base
}

// ── LZ77 ─────────────────────────────────────────────────────────────────────

// The hash slot for `p` in a table of `hsize` entries. A short input gets a short
// table (see `gz_deflate_fixed`), so the slot is reduced into range here — one
// division per position, versus zeroing 256 KB of table per call.
func gz_hash_slot(src : &[u8], p : size_t, hsize : size_t) : size_t {
    var h = gz_hash_at_raw(src, p)
    if(h >= hsize) { h = h % hsize }
    return h
}

func gz_hash_at_raw(src : &[u8], p : size_t) : size_t {
    var v = gz_shl(unsafe(*src.at(p)) as uint, 16)
    v = v | gz_shl(unsafe(*src.at(p + 1)) as uint, 8)
    v = v | (unsafe(*src.at(p + 2)) as uint)
    v = v * (2654435761 as uint)
    return gz_shr(v, 17) & (32767 as size_t)
}

// Record position `p` as the newest occurrence of its 3-byte prefix.
// `head` stores `position + 1` (0 = empty), so a zeroed vector is already valid.
func gz_insert(src : &[u8], head : &mut vector<size_t>, prev : &mut vector<size_t>, p : size_t) {
    if(p + 2 >= src.size()) { return }
    if(p >= prev.size()) { return }
    if(head.size() == 0) { return }
    var h = gz_hash_slot(src, p, head.size())
    if(h >= head.size()) { return }
    unsafe {
        *prev.get_ptr(p) = *head.get_ptr(h)
        *head.get_ptr(h) = p + 1
    }
}

// One fixed-Huffman block holding all of `src`, plus the end-of-block symbol.
func gz_deflate_fixed(src : &[u8], out : &mut GzOut) {
    const WSIZE : size_t = 32768
    const HSIZE : size_t = 32768
    const MIN_MATCH : size_t = 3
    const MAX_MATCH : size_t = 258
    const MAX_CHAIN : uint = 48

    gz_put_bits(out, 1, 1)   // BFINAL
    gz_put_bits(out, 1, 2)   // BTYPE = 01, fixed Huffman

    var n = src.size()
    // The hash table is sized for the input, not always for a full window: a 2.5 KB
    // page zeroed a 32,768-entry table (256 KB of `memset`) per compression, which
    // cost more than the encoding. `gz_insert` skips a hash that is past the end of
    // a small table, so a short input loses some chain depth and stays correct.
    var hsize = HSIZE
    if(n < hsize) { hsize = n }
    if(hsize == 0) { hsize = 1 }
    var head = vector<size_t>()
    var prev = vector<size_t>()
    if(!head.try_reserve(hsize) || !prev.try_reserve(n)) {
        out.failed = true
        return
    }
    var z : size_t = 0
    while(z < hsize) {
        if(!head.try_push(0)) { out.failed = true; return }
        z = z + 1
    }
    z = 0
    while(z < n) {
        if(!prev.try_push(0)) { out.failed = true; return }
        z = z + 1
    }

    var pos : size_t = 0
    while(pos < n) {
        var best_len : size_t = 0
        var best_dist : size_t = 0
        if(pos + MIN_MATCH <= n) {
            var cand = head.at(gz_hash_slot(src, pos, head.size()))
            var chain : uint = 0
            // Newest occurrence first; a chain of older ones. Once a candidate is
            // farther back than the window, every later one is too (the chain is
            // ordered by decreasing position), so stop.
            while(cand != 0 && chain < MAX_CHAIN) {
                var cand_pos = cand - 1
                var dist = pos - cand_pos
                if(dist > WSIZE) { break }
                var max_len = n - pos
                if(max_len > MAX_MATCH) { max_len = MAX_MATCH }
                var l : size_t = 0
                while(l < max_len && unsafe(*src.at(cand_pos + l)) == unsafe(*src.at(pos + l))) {
                    l = l + 1
                }
                if(l > best_len) {
                    best_len = l
                    best_dist = dist
                    if(l >= MAX_MATCH) { break }
                }
                cand = prev.at(cand_pos)
                chain = chain + 1
            }
        }

        if(best_len >= MIN_MATCH) {
            var code = gz_length_code(best_len as uint)
            gz_put_literal(out, code)
            var extra = gz_length_extra(code)
            if(extra > 0) {
                gz_put_bits(out, (best_len as uint) - gz_length_base(code), extra)
            }
            var dcode = gz_dist_code(best_dist as uint)
            gz_put_code(out, dcode, 5)
            var dextra = gz_dist_extra(dcode)
            if(dextra > 0) {
                gz_put_bits(out, (best_dist as uint) - gz_dist_base(dcode), dextra)
            }
            var k : size_t = 0
            while(k < best_len) {
                gz_insert(src, &mut head, &mut prev, pos + k)
                k = k + 1
            }
            pos = pos + best_len
        } else {
            gz_put_literal(out, unsafe(*src.at(pos)) as uint)
            gz_insert(src, &mut head, &mut prev, pos)
            pos = pos + 1
        }
        if(out.failed) { return }
    }
    gz_put_literal(out, 256)   // end of block
}

// ── CRC-32 (the gzip trailer's integrity check) ──────────────────────────────

func gz_crc32_ptr(data : *u8, n : size_t) : uint {
    var crc : uint = 0xFFFFFFFF
    var i : size_t = 0
    unsafe {
        while(i < n) {
            crc = crc ^ (data[i] as uint)
            var k : uint = 0
            while(k < 8) {
                if((crc & 1) != 0) {
                    crc = gz_shr(crc, 1) ^ (0xEDB88320 as uint)
                } else {
                    crc = gz_shr(crc, 1)
                }
                k = k + 1
            }
            i = i + 1
        }
    }
    return crc ^ (0xFFFFFFFF as uint)
}

// ── Public: compress ────────────────────────────────────────────────────────

/// gzip-compress `data`. The result is a complete RFC 1952 stream (header,
/// fixed-Huffman deflate body, CRC-32 and input size) that `gunzip` accepts.
///
/// Never falls back to storing the input uncompressed: an asset that is already
/// compressed (a PNG) grows by a few bytes, so a caller that cares compares the
/// two sizes (`StaticSite` only keeps a compressed variant when it is smaller).
public func gzip(data : &[u8]) : Result<vector<u8>, CompressionError> {
    var out = GzOut {
        bytes : vector<u8>(),
        acc : 0,
        count : 0,
        failed : false
    }
    gz_out_init(&mut out)
    if(out.failed) { return Result.Err(CompressionError.OutOfMemory()) }

    gz_put_bits(&mut out, 0x1f, 8)   // magic
    gz_put_bits(&mut out, 0x8b, 8)
    gz_put_bits(&mut out, 8, 8)      // CM = deflate
    gz_put_bits(&mut out, 0, 8)      // FLG = 0 (no name/comment/extra/crc)
    gz_put_bits(&mut out, 0, 32)     // MTIME = 0: deterministic bytes
    gz_put_bits(&mut out, 0, 8)      // XFL
    gz_put_bits(&mut out, 3, 8)      // OS = Unix

    gz_deflate_fixed(data, &mut out)
    if(out.failed) { return Result.Err(CompressionError.OutOfMemory()) }
    gz_align(&mut out)

    gz_put_u32_le(&mut out, gz_crc32_ptr(data.data(), data.size()))
    gz_put_u32_le(&mut out, data.size() as uint)
    if(out.failed) { return Result.Err(CompressionError.OutOfMemory()) }

    // Copied into a fresh local rather than moved out of `out.bytes`: moving a
    // `vector` out of a struct field is a shape the compiler does not model
    // correctly today (the field's buffer ends up shared with a temporary that
    // frees it). One copy of a build-time buffer is the cheap side of that.
    var packed = vector<u8>()
    if(!packed.try_reserve(out.bytes.size())) {
        return Result.Err(CompressionError.OutOfMemory())
    }
    var i : size_t = 0
    while(i < out.bytes.size()) {
        if(!packed.try_push(out.bytes.at(i))) {
            return Result.Err(CompressionError.OutOfMemory())
        }
        i = i + 1
    }
    return Result.Ok(packed)
}

// ── Bit input (the decoder) ─────────────────────────────────────────────────

struct GzIn {
    var pos : size_t
    var acc : uint
    var nbits : uint
    var failed : bool
}

func gz_read_bit(rd : &mut GzIn, src : &[u8]) : uint {
    if(rd.failed) { return 0 }
    if(rd.nbits == 0) {
        if(rd.pos >= src.size()) {
            rd.failed = true
            return 0
        }
        rd.acc = unsafe(*src.at(rd.pos)) as uint
        rd.pos = rd.pos + 1
        rd.nbits = 8
    }
    const bit = rd.acc & 1
    rd.acc = gz_shr(rd.acc, 1)
    rd.nbits = rd.nbits - 1
    return bit
}

func gz_read_bits(rd : &mut GzIn, src : &[u8], n : uint) : uint {
    var value : uint = 0
    var i : uint = 0
    while(i < n) {
        value = value | gz_shl(gz_read_bit(rd, src), i)
        if(rd.failed) { return 0 }
        i = i + 1
    }
    return value
}

// Huffman codes arrive most-significant bit first.
func gz_read_code(rd : &mut GzIn, src : &[u8], n : uint) : uint {
    var value : uint = 0
    var i : uint = 0
    while(i < n) {
        value = gz_shl(value, 1) | gz_read_bit(rd, src)
        if(rd.failed) { return 0 }
        i = i + 1
    }
    return value
}

// Decode one fixed-Huffman symbol: 7, 8 or 9 bits depending on the prefix.
func gz_read_fixed(rd : &mut GzIn, src : &[u8]) : uint {
    var c = gz_read_code(rd, src, 7)
    if(rd.failed) { return 0xFFFF }
    if(c <= 0x17) { return 256 + c }
    c = gz_shl(c, 1) | gz_read_bit(rd, src)
    if(rd.failed) { return 0xFFFF }
    if(c >= 0x30 && c <= 0xBF) { return c - 0x30 }
    if(c >= 0xC0 && c <= 0xC7) { return 280 + (c - 0xC0) }
    c = gz_shl(c, 1) | gz_read_bit(rd, src)
    if(rd.failed) { return 0xFFFF }
    if(c >= 0x190 && c <= 0x1FF) { return 144 + (c - 0x190) }
    rd.failed = true
    return 0xFFFF
}

// A malformed stream can describe an unbounded output (a "zip bomb", or simply a
// wrong symbol read as a long length), so decoding stops at this many bytes and
// reports invalid data. It is far above any asset this encoder is used for.
func gz_inflate_fixed(rd : &mut GzIn, src : &[u8], out : &mut vector<u8>) {
    const MAX_OUTPUT : size_t = 268435456 as size_t
    while(!rd.failed) {
        if(out.size() > MAX_OUTPUT) { rd.failed = true; return }
        var sym = gz_read_fixed(rd, src)
        if(rd.failed) { return }
        if(sym < 256) {
            if(!out.try_push(sym as u8)) { rd.failed = true; return }
            continue
        }
        if(sym == 256) { return }
        // `sym` is the length symbol itself (257..285); the base/extra helpers
        // are indexed by it, not by `sym - 257`.
        if(sym > 285) { rd.failed = true; return }
        var len = gz_length_base(sym) + gz_read_bits(rd, src, gz_length_extra(sym))
        if(rd.failed) { return }
        var dcode = gz_read_code(rd, src, 5)
        if(rd.failed) { return }
        var dist = gz_dist_base(dcode) + gz_read_bits(rd, src, gz_dist_extra(dcode))
        if(rd.failed) { return }
        if(dist == 0 || (dist as size_t) > out.size()) { rd.failed = true; return }
        var k : uint = 0
        while(k < len) {
            var byte_val = out.at(out.size() - (dist as size_t))
            if(!out.try_push(byte_val)) { rd.failed = true; return }
            k = k + 1
        }
    }
}

func gz_read_u32_le(data : &[u8], at : size_t) : uint {
    var value = unsafe(*data.at(at)) as uint
    value = value | gz_shl(unsafe(*data.at(at + 1)) as uint, 8)
    value = value | gz_shl(unsafe(*data.at(at + 2)) as uint, 16)
    value = value | gz_shl(unsafe(*data.at(at + 3)) as uint, 24)
    return value
}

// ── Public: decompress ──────────────────────────────────────────────────────

/// Decode a gzip stream produced by `gzip` (or by any encoder that emits stored
/// or fixed-Huffman blocks). Every malformed stream is rejected rather than
/// producing partial bytes: a truncated body, a bad magic byte, an unknown block
/// type, a distance before the start of the output, a length mismatch, or a
/// CRC-32 that does not match the payload.
///
/// Dynamic-Huffman blocks (what `gzip -9` emits) are reported as
/// `UnsupportedAlgorithm()` — this decoder exists to verify the encoder, not to
/// replace a general-purpose inflater.
public func gunzip(data : &[u8]) : Result<vector<u8>, CompressionError> {
    if(data.size() < 18) { return Result.Err(CompressionError.InvalidData()) }
    if(unsafe(*data.at(0)) != 0x1f || unsafe(*data.at(1)) != 0x8b) {
        return Result.Err(CompressionError.InvalidData())
    }
    if(unsafe(*data.at(2)) != 8) {
        return Result.Err(CompressionError.UnsupportedAlgorithm())
    }
    var flags = unsafe(*data.at(3))
    // FEXTRA / FHCRC / reserved bits are never emitted here, and skipping them
    // would only add paths this decoder cannot test.
    if((flags & 0xE6) != 0) {
        return Result.Err(CompressionError.UnsupportedAlgorithm())
    }

    var pos : size_t = 10
    if((flags & 0x08) != 0) {
        while(pos < data.size() && unsafe(*data.at(pos)) != 0) { pos = pos + 1 }
        pos = pos + 1
    }
    if((flags & 0x10) != 0) {
        while(pos < data.size() && unsafe(*data.at(pos)) != 0) { pos = pos + 1 }
        pos = pos + 1
    }
    if(pos > data.size()) { return Result.Err(CompressionError.InvalidData()) }

    var rd = GzIn {
        pos : pos,
        acc : 0,
        nbits : 0,
        failed : false
    }
    var out = vector<u8>()
    var final : uint = 0
    while(final == 0) {
        final = gz_read_bit(&mut rd, data)
        var btype = gz_read_bits(&mut rd, data, 2)
        if(rd.failed) { return Result.Err(CompressionError.InvalidData()) }
        if(btype == 1) {
            gz_inflate_fixed(&mut rd, data, &mut out)
            if(rd.failed) { return Result.Err(CompressionError.InvalidData()) }
        } else if(btype == 0) {
            // Stored: the input is byte-aligned after the block header.
            rd.acc = 0
            rd.nbits = 0
            if(rd.pos + 4 > data.size()) { return Result.Err(CompressionError.InvalidData()) }
            var len = gz_read_u32_le(data, rd.pos) & 0xFFFF
            var nlen = gz_shr(gz_read_u32_le(data, rd.pos), 16) & 0xFFFF
            if((len ^ 0xFFFF) != nlen) { return Result.Err(CompressionError.InvalidData()) }
            rd.pos = rd.pos + 4
            if(rd.pos + (len as size_t) > data.size()) {
                return Result.Err(CompressionError.InvalidData())
            }
            var k : uint = 0
            while(k < len) {
                if(!out.try_push(unsafe(*data.at(rd.pos)))) {
                    return Result.Err(CompressionError.OutOfMemory())
                }
                rd.pos = rd.pos + 1
                k = k + 1
            }
        } else {
            return Result.Err(CompressionError.UnsupportedAlgorithm())
        }
    }

    // The trailer starts at the next unread byte: the bit reader consumed the
    // byte holding the end-of-block marker, and the rest of that byte is padding.
    if(rd.pos + 8 > data.size()) { return Result.Err(CompressionError.InvalidData()) }
    var crc_stored = gz_read_u32_le(data, rd.pos)
    var size_stored = gz_read_u32_le(data, rd.pos + 4)
    if(size_stored != (out.size() as uint)) {
        return Result.Err(CompressionError.InvalidData())
    }
    if(crc_stored != gz_crc32_ptr(out.data(), out.size())) {
        return Result.Err(CompressionError.InvalidData())
    }
    return Result.Ok(out)
}

} // end namespace compression
