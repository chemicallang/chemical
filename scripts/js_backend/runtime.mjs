// Chemical JS backend runtime (ABI: lang/docs/js-backend-abi.md, Route A).
//
// Emulated linear memory: a Uint8Array heap addressed by byte offsets. Every MIR
// load/store becomes a typed DataView access here; every aggregate is bytes, not
// a JS object, so layout is identical to the native backend and sizeof/offsetof
// agree. The emitter emits calls into this module, never raw DataView code, so
// the access rules (and the arithmetic semantics) live in one place.
//
// Memory regions:
//   [HEAP_START, HEAP_END)     heap   — malloc/realloc, module constants, globals
//   [FRAME_START, FRAME_END)   frames — per-call storage (alloca place slots)
// Frames are released at function exit (the emitter brackets every function with
// frame_mark/frame_release), so a function called in a loop does not leak; heap
// memory lives until there is a real free list.

const HEAP_BYTES = 16 << 20;
export const HEAP = new Uint8Array(HEAP_BYTES);
const DV = new DataView(HEAP.buffer);

const HEAP_START = 1024; // 0 stays the null pointer
const HEAP_END = HEAP_BYTES >>> 1;
const FRAME_START = (HEAP_BYTES >>> 1) + 64;
const FRAME_END = HEAP_BYTES;

let heapBrk = HEAP_START;
let frameBrk = FRAME_START;

function bump(brk, end, size, align, what) {
  const a = align > 0 ? align : 1;
  const addr = (brk + a - 1) & ~(a - 1);
  if (addr + size > end) throw new Error(`js runtime: ${what} exhausted (${size} bytes)`);
  return addr;
}

// ── heap allocation ─────────────────────────────────────────────────────────
// std calls malloc/realloc/free; see the note above about `free`.
export function alloc(size, align) {
  const addr = bump(heapBrk, HEAP_END, size, align, "heap");
  heapBrk = addr + size;
  return addr;
}
export function malloc(n) { return alloc(n, 8); }
export function realloc_(p, n) {
  const q = alloc(n, 8);
  if (p) HEAP.copyWithin(q, p, p + n);
  return q;
}
export function free_(_p) {}

// ── per-call frames ─────────────────────────────────────────────────────────
/** Begin a frame; returns the mark `frame_release` restores. */
export function frame_mark() { return frameBrk; }
export function frame_release(mark) { frameBrk = mark; }
/** Allocate zeroed storage for one `alloca` place. */
export function frame_alloc(size, align) {
  const addr = bump(frameBrk, FRAME_END, size, align, "frame");
  frameBrk = addr + size;
  HEAP.fill(0, addr, addr + size);
  return addr;
}

// ── typed memory access ─────────────────────────────────────────────────────
export function ld(p, size, signed) {
  if (size === 0) return 0;
  if (size === 1) return signed ? DV.getInt8(p) : DV.getUint8(p);
  if (size === 2) return signed ? DV.getInt16(p, true) : DV.getUint16(p, true);
  if (size === 4) return signed ? DV.getInt32(p, true) : DV.getUint32(p, true);
  if (size === 8) {
    return signed ? Number(DV.getBigInt64(p, true)) : Number(DV.getBigUint64(p, true));
  }
  throw new Error("js runtime: load of size " + size);
}
export function st(p, size, signed, v) {
  if (size === 0) return;
  if (size === 1) { signed ? DV.setInt8(p, v) : DV.setUint8(p, v); return; }
  if (size === 2) { signed ? DV.setInt16(p, v, true) : DV.setUint16(p, v, true); return; }
  if (size === 4) { signed ? DV.setInt32(p, v, true) : DV.setUint32(p, v, true); return; }
  if (size === 8) {
    v = Math.trunc(v);
    signed ? DV.setBigInt64(p, BigInt(v), true) : DV.setBigUint64(p, BigInt(v), true);
    return;
  }
  throw new Error("js runtime: store of size " + size);
}
export function ldF(p, size) {
  return size === 4 ? DV.getFloat32(p, true) : DV.getFloat64(p, true);
}
export function stF(p, size, v) {
  size === 4 ? DV.setFloat32(p, v, true) : DV.setFloat64(p, v, true);
}
export function memcpy(dst, src, n) {
  if (n > 0) HEAP.copyWithin(dst, src, src + n);
  return dst;
}
export function memmove(dst, src, n) {
  if (n > 0) HEAP.copyWithin(dst, src, src + n);
  return dst;
}
export function memset(dst, byte, n) {
  if (n > 0) HEAP.fill(byte, dst, dst + n);
  return dst;
}
export function fill(dst, byte, n) {
  if (n > 0) HEAP.fill(byte, dst, dst + n);
  return dst;
}

// Back-compat aliases used by the runtime's own tests and older fixtures.
export const load = ld;
export const store = st;
export const loadF = ldF;
export const storeF = stF;

// ── integers, the way C means them ──────────────────────────────────────────
// A value is a JS number (exact for |v| < 2^53). Every operation is followed by
// `norm`, which wraps it into its result type; that is what makes i8/u8/i32/u32
// arithmetic and comparisons agree with the native backend bit-for-bit. 64-bit
// values are numbers up to 2^53 and use BigInt only where a 64-bit bitwise or
// shift would otherwise be truncated to 32 bits.
const MAX_SAFE = Number.MAX_SAFE_INTEGER;

export function norm(v, bits, signed) {
  if (!Number.isFinite(v)) return 0; // C would be UB; stay deterministic
  v = Math.trunc(v);
  if (bits <= 0) return v;
  if (bits >= 64) {
    // A 64-bit value lives in a JS number, which is exact only to 2^53. Signed
    // values pass through. Unsigned values *saturate*: a guard such as
    // `x >= (size_t)-1` must stay false, and wrapping to -1 would make it fire.
    // The 2^53 bound is documented in lang/docs/js-backend-abi.md §7.
    if (signed) return v;
    if (v < 0) return MAX_SAFE;
    return v > MAX_SAFE ? MAX_SAFE : v;
  }
  const m = Math.pow(2, bits);
  v = v % m;
  if (v < 0) v += m;
  if (signed && v >= m / 2) v -= m;
  return v;
}

// C integer promotion: anything narrower than int promotes to signed int.
function promote(bits, signed) {
  return bits < 32 ? [32, true] : [bits, signed];
}

// C usual arithmetic conversions on the promoted operand types.
function usual(ab, as, bb, bs) {
  const [a1, as1] = promote(ab, as);
  const [b1, bs1] = promote(bb, bs);
  if (as1 === bs1) return a1 >= b1 ? [a1, as1] : [b1, bs1];
  const u = as1 ? [b1, bs1] : [a1, as1];
  const s = as1 ? [a1, as1] : [b1, bs1];
  return u[0] >= s[0] ? u : s;
}

function shl(x, y, bits) {
  const s = y & (bits - 1);
  if (bits <= 32) return x * Math.pow(2, s);
  return Number(BigInt.asIntN(64, BigInt(x) << BigInt(s)));
}
function shr(x, y, bits, signed) {
  const s = y & (bits - 1);
  if (bits === 32 && signed) return x >> s;
  if (bits <= 32) return Math.floor(x / Math.pow(2, s));
  const b = BigInt(x);
  return Number(BigInt.asIntN(64, signed ? b >> BigInt(s) : BigInt.asUintN(64, b) >> BigInt(s)));
}

/** Binary integer op. `op` is the MIRBinaryOp value; bits/signed describe types. */
export function binop(op, a, ab, as, b, bb, bs, rb, rs) {
  const [cb, cs] = usual(ab, as, bb, bs);
  const x = norm(a, cb, cs);
  const y = norm(b, cb, cs);
  let r = 0;
  switch (op) {
    case 0: r = x + y; break;
    case 1: r = x - y; break;
    case 2: r = x * y; break;
    case 3: r = y === 0 ? 0 : Math.trunc(x / y); break;
    case 4: r = y === 0 ? 0 : x % y; break;
    case 5: r = shl(x, y, cb); break;
    case 6: r = shr(x, y, cb, cs); break;
    // JS bitwise operators are 32-bit, so a wider operand takes BigInt.
    case 7: r = norm(cb > 32 ? Number(BigInt.asIntN(64, BigInt(x) & BigInt(y))) : (x & y), cb, cs); break;
    case 8: r = norm(cb > 32 ? Number(BigInt.asIntN(64, BigInt(x) | BigInt(y))) : (x | y), cb, cs); break;
    case 9: r = norm(cb > 32 ? Number(BigInt.asIntN(64, BigInt(x) ^ BigInt(y))) : (x ^ y), cb, cs); break;
    // comparisons are emitted through `cmp`, never here
    default: r = 0; break;
  }
  return norm(r, rb, rs);
}

/** Comparison (`Lt`..`Ne`, op 10..15); returns 1 or 0 like a MIR bool value. */
export function cmp(op, a, ab, as, b, bb, bs) {
  const [cb, cs] = usual(ab, as, bb, bs);
  const x = norm(a, cb, cs);
  const y = norm(b, cb, cs);
  switch (op) {
    case 10: return x < y ? 1 : 0;
    case 11: return x <= y ? 1 : 0;
    case 12: return x > y ? 1 : 0;
    case 13: return x >= y ? 1 : 0;
    case 14: return x === y ? 1 : 0;
    case 15: return x !== y ? 1 : 0;
    default: return 0;
  }
}

export function fbinop(op, a, b, dbl) {
  if (!dbl) { a = Math.fround(a); b = Math.fround(b); }
  let r;
  switch (op) {
    case 0: r = a + b; break;
    case 1: r = a - b; break;
    case 2: r = a * b; break;
    case 3: r = a / b; break;
    default: r = 0; break;
  }
  return dbl ? r : Math.fround(r);
}

export function fcmp(op, a, b, dbl) {
  if (!dbl) { a = Math.fround(a); b = Math.fround(b); }
  switch (op) {
    case 10: return a < b ? 1 : 0;
    case 11: return a <= b ? 1 : 0;
    case 12: return a > b ? 1 : 0;
    case 13: return a >= b ? 1 : 0;
    case 14: return a === b ? 1 : 0;
    case 15: return a !== b ? 1 : 0;
    default: return 0;
  }
}

export function unop(op, a, bits, signed, flt, dbl) {
  if (flt) {
    if (op === 0) return Math.fround(-a);   // Neg
    if (op === 2) return a === 0 ? 1 : 0;   // Not
    return Math.fround(a);                  // Plus
  }
  if (dbl) {
    if (op === 0) return -a;
    if (op === 2) return a === 0 ? 1 : 0;
    return a;
  }
  switch (op) {
    case 0: return norm(-a, bits, signed);
    case 2: return a === 0 ? 1 : 0;
    case 3: return norm(~a, bits, signed);
    default: return norm(a, bits, signed);
  }
}

/** `cast(v, fromBits, fromSigned, fromFloat, toBits, toSigned, toFloat)`. */
export function cast(v, fb, fs, ff, tb, ts, tf) {
  if (tf) return tb === 32 ? Math.fround(v) : v;
  if (ff) v = Math.trunc(fb === 32 ? Math.fround(v) : v);
  return norm(v, tb, ts);
}

// ── pointer arithmetic (C scales by the pointee size) ───────────────────────
export function padd(p, i, scale) { return p + i * scale; }
export function psub(p, i, scale) { return p - i * scale; }
export function pdiff(a, b, scale) { return Math.trunc((a - b) / scale); }

// ── C strings (bytes in the same heap) ──────────────────────────────────────
export function cstr(bytes) {
  const p = alloc(bytes.length + 1, 1);
  HEAP.set(bytes, p);
  HEAP[p + bytes.length] = 0;
  return p;
}
export function readCStr(p) {
  if (!p) return "(null)";
  let end = p;
  while (HEAP[end] !== 0) end++;
  return new TextDecoder().decode(HEAP.subarray(p, end));
}
export function strlen(p) {
  let n = 0;
  while (HEAP[p + n] !== 0) n++;
  return n;
}
export function strcmp(a, b) {
  let i = 0;
  for (;;) {
    const x = HEAP[a + i];
    const y = HEAP[b + i];
    if (x !== y) return x < y ? -1 : 1;
    if (x === 0) return 0;
    i++;
  }
}

// ── printf ──────────────────────────────────────────────────────────────────
// Enough of C's printf for the programs the backend is tested on. The format is
// read from the heap (it is a `const.string`), so this takes the same code path
// a native printf would over the same bytes.
export function printf(fmtPtr, ...args) {
  let out = "";
  const fmt = readCStr(fmtPtr);
  let i = 0;
  let ai = 0;
  while (i < fmt.length) {
    const c = fmt[i++];
    if (c !== "%") { out += c; continue; }
    let spec = fmt[i++];
    if (spec === "%") { out += "%"; continue; }
    // flags
    while (spec && "-+ #0".includes(spec)) spec = fmt[i++];
    // width
    while (spec >= "0" && spec <= "9") spec = fmt[i++];
    // precision
    let precision = -1;
    if (spec === ".") {
      spec = fmt[i++];
      precision = 0;
      while (spec >= "0" && spec <= "9") { precision = precision * 10 + (spec.charCodeAt(0) - 48); spec = fmt[i++]; }
    }
    // length modifiers: %zu, %lu, %lld, %hhu ...
    while (spec === "z" || spec === "l" || spec === "h" || spec === "j" || spec === "t") spec = fmt[i++];
    const a = args[ai++];
    switch (spec) {
      case "d": case "i": out += String(Math.trunc(a)); break;
      case "u": out += String(Math.trunc(a)); break;
      case "x": out += Math.trunc(a).toString(16); break;
      case "X": out += Math.trunc(a).toString(16).toUpperCase(); break;
      case "o": out += Math.trunc(a).toString(8); break;
      case "c": out += String.fromCharCode(a); break;
      case "s": out += a ? readCStr(a) : "(null)"; break;
      case "f": out += precision >= 0 ? a.toFixed(precision) : String(a); break;
      case "g": case "e": out += String(a); break;
      case "p": out += "0x" + Math.trunc(a).toString(16); break;
      default: out += "%" + spec; break;
    }
  }
  process.stdout.write(out);
  return out.length;
}

export function putchar(c) {
  process.stdout.write(String.fromCharCode(c));
  return c;
}
export function abort() {
  throw new Error("js runtime: abort()");
}
/**
 * The C backend instruments checked operations (division by zero, bounds,
 * null dereference) with calls into `__chc_*` runtime helpers. On this backend
 * those are traps: JS cannot fault, so the check-out throws. The first argument
 * is the helper's name, so a trap names the check that fired.
 */
export function trap(name, ...args) {
  const which = typeof name === "string" ? name : "<checked operation>";
  // `__chc_panic(message)` passes a `*char`; surfacing it makes a trap readable.
  let detail = "";
  if (name === "__chc_panic" && typeof args[0] === "number" && args[0] > 0) {
    try {
      detail = ": " + readCStr(args[0]);
    } catch (_) {
      // the pointer was not a readable heap address; keep the generic message
    }
  }
  throw new Error("js runtime: trap: " + which + detail);
}
export function exit(code) {
  throw new Error("js runtime: exit(" + code + ")");
}

// One namespace object so emitted code can import a single binding.
export const rt = {
  alloc, malloc, realloc_, free_, frame_mark, frame_release, frame_alloc,
  ld, st, ldF, stF, load, store, loadF, storeF,
  memcpy, memmove, memset, fill,
  norm, binop, cmp, fbinop, fcmp, unop, cast, padd, psub, pdiff,
  cstr, readCStr, strlen, strcmp, printf, putchar, abort, exit, trap, HEAP,
};
export default rt;
