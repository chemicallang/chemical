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

// ── async: promises behind integer tokens ───────────────────────────────────
// An `async func` is emitted as an `async` arrow, and its result is a JS
// Promise. MIR values are words and aggregates are memory, so a promise cannot
// travel through a `FutureHandle<T>` slot directly: it is stored in this table
// and the slot keeps its index. `await` reads that index back.
const PROMISES = [null];
/** Store `promise` behind a token and write the token into a handle slot. */
export function promise_put(handle, promise) {
  PROMISES.push(promise);
  st(handle, 8, 0, PROMISES.length - 1);
}
/** The promise a handle token names; throws on a handle this runtime did not make. */
export function promise_get(token) {
  if (typeof token !== "number" || (token | 0) !== token || token <= 0 ||
      token >= PROMISES.length) {
    throw new Error("js runtime: awaited value is not a promise-backed future (token " +
                    token + "; a ready future built by the async library needs the "
                    + "coroutine lowering, which a JS build does not use)");
  }
  return PROMISES[token];
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

// ── ctype.h ─────────────────────────────────────────────────────────────────
// Over the heap's bytes; each returns 1/0 (nonzero means "yes" in C).
export function isspace(c) { return c === 32 || (c >= 9 && c <= 13) ? 1 : 0; }
export function isdigit(c) { return c >= 48 && c <= 57 ? 1 : 0; }
export function isalpha(c) { return ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) ? 1 : 0; }
export function isalnum(c) { return (isalpha(c) || isdigit(c)) ? 1 : 0; }
export function isxdigit(c) {
  return ((c >= 48 && c <= 57) || (c >= 65 && c <= 70) || (c >= 97 && c <= 102)) ? 1 : 0;
}
export function isupper(c) { return (c >= 65 && c <= 90) ? 1 : 0; }
export function islower(c) { return (c >= 97 && c <= 122) ? 1 : 0; }
export function isprint(c) { return (c >= 32 && c < 127) ? 1 : 0; }
export function isgraph(c) { return (c > 32 && c < 127) ? 1 : 0; }
export function iscntrl(c) { return (c < 32 || c === 127) ? 1 : 0; }
export function ispunct(c) { return (isgraph(c) && !isalnum(c)) ? 1 : 0; }
export function toupper(c) { return (c >= 97 && c <= 122) ? c - 32 : c; }
export function tolower(c) { return (c >= 65 && c <= 90) ? c + 32 : c; }

export function abs(x) { return x < 0 ? -x : x; }
export function labs(x) { return x < 0 ? -x : x; }
export function llabs(x) { return x < 0 ? -x : x; }

// ── number parsing (`<stdlib.h>`) ───────────────────────────────────────────
function digit_value(c) {
  if (c >= 48 && c <= 57) return c - 48;
  if (c >= 97 && c <= 122) return c - 97 + 10;
  if (c >= 65 && c <= 90) return c - 65 + 10;
  return 99;
}

function parse_int_at(p, base) {
  let i = p;
  while (isspace(HEAP[i])) i++;
  let sign = 1;
  if (HEAP[i] === 45) { sign = -1; i++; } else if (HEAP[i] === 43) i++;
  let b = base;
  if (b === 0) {
    b = 10;
    if (HEAP[i] === 48) {
      const n1 = HEAP[i + 1] | 32;
      if (n1 === 120) { b = 16; i += 2; } else { b = 8; i += 1; }
    }
  } else if (b === 16 && HEAP[i] === 48 && (HEAP[i + 1] | 32) === 120) {
    i += 2;
  }
  let value = 0;
  let any = false;
  for (;;) {
    const d = digit_value(HEAP[i]);
    if (d >= b) break;
    value = value * b + d;
    any = true;
    i++;
  }
  if (!any) i = p;
  return { value: sign < 0 ? -value : value, end: i, any };
}

export function strtol(p, endptr, base) {
  const r = parse_int_at(p, base);
  if (endptr) st(endptr, 8, false, r.any ? r.end : p);
  return norm(r.value, 64, true);
}
export function strtoll(p, endptr, base) { return strtol(p, endptr, base); }
export function strtoul(p, endptr, base) {
  const r = parse_int_at(p, base);
  if (endptr) st(endptr, 8, false, r.any ? r.end : p);
  return norm(r.value, 64, false);
}
export function strtoull(p, endptr, base) { return strtoul(p, endptr, base); }
export function atoi(p) { return strtol(p, 0, 10) | 0; }

/** The longest `[+-]?digits[.digits][e[+-]digits]` prefix, as a number. */
export function strtod(p, endptr) {
  let i = p;
  while (isspace(HEAP[i])) i++;
  let start = i;
  if (HEAP[i] === 45 || HEAP[i] === 43) i++;
  while (isdigit(HEAP[i])) i++;
  if (HEAP[i] === 46) {
    i++;
    while (isdigit(HEAP[i])) i++;
  }
  if (HEAP[i] === 101 || HEAP[i] === 69) {
    const save = i;
    i++;
    if (HEAP[i] === 45 || HEAP[i] === 43) i++;
    if (isdigit(HEAP[i])) {
      while (isdigit(HEAP[i])) i++;
    } else {
      i = save;
    }
  }
  if (i === start) {
    if (endptr) st(endptr, 8, false, p);
    return 0;
  }
  let text = "";
  for (let k = start; k < i; k++) text += String.fromCharCode(HEAP[k]);
  if (endptr) st(endptr, 8, false, i);
  const v = Number(text);
  return Number.isFinite(v) ? v : 0;
}
export function atof(p) { return strtod(p, 0); }

// ── string.h` beyond strlen/strcmp ──────────────────────────────────────────
export function strncmp(a, b, n) {
  for (let i = 0; i < n; i++) {
    const x = HEAP[a + i];
    const y = HEAP[b + i];
    if (x !== y) return x < y ? -1 : 1;
    if (x === 0) return 0;
  }
  return 0;
}
export function strcpy(dst, src) {
  let i = 0;
  for (;;) {
    const c = HEAP[src + i];
    HEAP[dst + i] = c;
    if (c === 0) break;
    i++;
  }
  return dst;
}
export function strncpy(dst, src, n) {
  let i = 0;
  for (; i < n; i++) {
    const c = HEAP[src + i];
    HEAP[dst + i] = c;
    if (c === 0) break;
  }
  for (; i < n; i++) HEAP[dst + i] = 0;
  return dst;
}
export function strcat(dst, src) {
  let i = strlen(dst);
  let k = 0;
  for (;;) {
    const c = HEAP[src + k];
    HEAP[dst + i + k] = c;
    if (c === 0) break;
    k++;
  }
  return dst;
}
export function strncat(dst, src, n) {
  let i = strlen(dst);
  for (let k = 0; k < n; k++) {
    const c = HEAP[src + k];
    HEAP[dst + i + k] = c;
    if (c === 0) return dst;
  }
  HEAP[dst + i + Math.min(n, strlen(src))] = 0;
  return dst;
}
export function strchr(p, ch) {
  const c = ch & 0xff;
  let i = 0;
  for (;;) {
    const x = HEAP[p + i];
    if (x === c) return p + i;
    if (x === 0) return 0;
    i++;
  }
}
export function strrchr(p, ch) {
  const c = ch & 0xff;
  let found = 0;
  let i = 0;
  for (;;) {
    const x = HEAP[p + i];
    if (x === c) found = p + i;
    if (x === 0) return found;
    i++;
  }
}
export function strstr(hay, needle) {
  const nlen = strlen(needle);
  if (nlen === 0) return hay;
  let i = 0;
  for (;;) {
    if (HEAP[hay + i] === 0) return 0;
    if (strncmp(hay + i, needle, nlen) === 0) return hay + i;
    i++;
  }
}
export function strdup(p) {
  const n = strlen(p);
  const q = alloc(n + 1, 1);
  for (let i = 0; i <= n; i++) HEAP[q + i] = HEAP[p + i];
  return q;
}
export function memcmp(a, b, n) {
  for (let i = 0; i < n; i++) {
    const x = HEAP[a + i];
    const y = HEAP[b + i];
    if (x !== y) return x < y ? -1 : 1;
  }
  return 0;
}
export function memchr(p, ch, n) {
  const c = ch & 0xff;
  for (let i = 0; i < n; i++) if (HEAP[p + i] === c) return p + i;
  return 0;
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
      if (spec === "*") {
        // `%.*s` / `%.*s`-style: the precision is the next argument.
        precision = Math.trunc(args[ai++]);
        spec = fmt[i++];
      } else {
        precision = 0;
        while (spec >= "0" && spec <= "9") { precision = precision * 10 + (spec.charCodeAt(0) - 48); spec = fmt[i++]; }
      }
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
      case "s": {
        let text = a ? readCStr(a) : "(null)";
        if (a && precision >= 0) text = text.slice(0, precision);
        out += text;
        break;
      }
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
// ── the DOM host (`dom`, proposal 0008) ─────────────────────────────────────
// A browser object cannot live in the emulated heap, so a DOM reference is a
// **handle**: an index into `hosts`, a table of the real JS objects. `0` is null,
// and an index is never reused, so a stale handle names the object it was made
// from (the browser keeps it alive for as long as the page does) instead of
// aliasing a different element.
//
// Everything else here is marshalling: strings travel as heap `string_view`s
// (`{ptr,len}`, 16 bytes) or as C strings returned by the host, and a Chemical
// callback travels as its function address (a symbol id resolved through `$fns`,
// which the emitted module registers with `rt.register_fns`).
let fnTable = [];
let fnTableRegistered = false;

/** Give the runtime the emitted module's `$fns` table, so host callbacks can call
 * back into Chemical code. A bundle is one ES module per Chemical module and each
 * registers its own table, so this *merges*: symbol ids are unique across the
 * whole program, and overwriting would drop every other module's functions. */
export function register_fns(table) {
  fnTableRegistered = true;
  for (let i = 0; i < table.length; i++) {
    if (table[i] !== undefined) fnTable[i] = table[i];
  }
}

/** The merged table. An indirect call site must resolve through this, not through
 * its own module's table: the address it calls may have been taken in another
 * module of the bundle (a closure created there, called here). */
export const fns = fnTable;

const hosts = [null];

function retain(value) {
  if (value === null || value === undefined) return 0;
  hosts.push(value);
  return hosts.length - 1;
}

function resolve_handle(handle) {
  const value = hosts[handle];
  if (value === undefined) throw new Error("js runtime: invalid dom handle " + handle);
  return value;
}

const domDecoder = new TextDecoder();
const domEncoder = new TextEncoder();

/** A heap `string_view` as a JS string. */
function text_at(viewAddr) {
  if (!viewAddr) return "";
  const ptr = ld(viewAddr, 8, false);
  const len = ld(viewAddr + 8, 8, false);
  if (!ptr || !len) return "";
  return domDecoder.decode(HEAP.subarray(ptr, ptr + len));
}

/** A JS string as a heap C string (the shape a `*char` return needs). */
function cstr_of(text) {
  return cstr(domEncoder.encode(text));
}

/** A JS string as a heap `string_view`, for a Chemical callback parameter. */
function view_of(text) {
  const bytes = domEncoder.encode(text);
  const addr = alloc(16, 8);
  st(addr, 8, false, cstr(bytes));
  st(addr + 8, 8, false, bytes.length);
  return addr;
}

function call_chemical(fn, ...args) {
  if (!fnTableRegistered) throw new Error("js runtime: a dom callback fired before rt.register_fns");
  return indirect(fnTable, fn)(...args);
}

function document_object() {
  if (typeof globalThis.document === "undefined") {
    throw new Error("js runtime: no document available (dom code must run in a browser)");
  }
  return globalThis.document;
}

function element(handle) {
  const value = resolve_handle(handle);
  if (!value) throw new Error("js runtime: a dom operation used a null element");
  return value;
}

function storage() {
  if (typeof globalThis.localStorage === "undefined") {
    throw new Error("js runtime: no localStorage available");
  }
  return globalThis.localStorage;
}

export const dom = {
  document: () => retain(document_object()),
  body: (doc) => retain(element(doc).body),

  get_element_by_id: (doc, id) => retain(element(doc).getElementById(text_at(id))),
  query_selector: (root, selector) => retain(element(root).querySelector(text_at(selector))),
  create_element: (doc, tag) => retain(element(doc).createElement(text_at(tag))),

  set_text: (handle, text) => { element(handle).textContent = text_at(text); },
  text: (handle) => cstr_of(element(handle).textContent ?? ""),
  set_html: (handle, html) => { element(handle).innerHTML = text_at(html); },

  set_attribute: (handle, name, value) => { element(handle).setAttribute(text_at(name), text_at(value)); },
  get_attribute: (handle, name) => {
    const value = element(handle).getAttribute(text_at(name));
    return value === null ? 0 : cstr_of(value);
  },
  has_attribute: (handle, name) => (element(handle).hasAttribute(text_at(name)) ? 1 : 0),
  remove_attribute: (handle, name) => { element(handle).removeAttribute(text_at(name)); },

  set_value: (handle, value) => { element(handle).value = text_at(value); },
  value: (handle) => cstr_of(element(handle).value ?? ""),

  append_child: (parent, child) => { element(parent).appendChild(element(child)); },
  remove: (handle) => {
    const node = element(handle);
    if (node.parentNode) node.parentNode.removeChild(node);
  },
  focus: (handle) => { element(handle).focus(); },

  // A listener receives a pointer to a one-field `Event { handle }` slot: MIR passes
  // an aggregate by address, so the Chemical callback reads its handle from there.
  on: (handle, event, fn) => {
    element(handle).addEventListener(text_at(event), (browserEvent) => {
      const slot = alloc(8, 8);
      st(slot, 8, false, retain(browserEvent));
      call_chemical(fn, slot);
    });
  },

  prevent_default: (event) => { const e = resolve_handle(event); if (e && e.preventDefault) e.preventDefault(); },
  stop_propagation: (event) => { const e = resolve_handle(event); if (e && e.stopPropagation) e.stopPropagation(); },
  target: (event) => retain(resolve_handle(event).target),
  key: (event) => cstr_of(resolve_handle(event).key ?? ""),

  // `fetch(url, callback)`; the callback is `(status : int, body : string_view)`,
  // with status 0 for a network failure (the body is then the error message).
  fetch_text: (url, fn) => {
    globalThis.fetch(text_at(url))
      .then((response) => response.text().then((body) => call_chemical(fn, response.status, view_of(body))))
      .catch((error) => call_chemical(fn, 0, view_of(String(error && error.message ? error.message : error))));
  },

  // The awaitable form: the *handle* is an sret destination the emitted call
  // passes first, and the future is a real host promise stored behind a token.
  // `promise_put` is what makes `await` on the Chemical side suspend on it.
  fetch_status: (handle, url) => {
    promise_put(handle, globalThis.fetch(text_at(url))
      .then((response) => response.status)
      .catch(() => 0));
  },

  storage_get: (key) => {
    const value = storage().getItem(text_at(key));
    return value === null ? 0 : cstr_of(value);
  },
  storage_set: (key, value) => { storage().setItem(text_at(key), text_at(value)); },
  storage_remove: (key) => { storage().removeItem(text_at(key)); },

  url_origin: (url) => cstr_of(new URL(text_at(url)).origin),
  url_pathname: (url) => cstr_of(new URL(text_at(url)).pathname),
  url_search_param: (url, name) => {
    const value = new URL(text_at(url)).searchParams.get(text_at(name));
    return value === null ? 0 : cstr_of(value);
  },
};

export const rt = {
  alloc, malloc, realloc_, free_, frame_mark, frame_release, frame_alloc,
  ld, st, ldF, stF, load, store, loadF, storeF,
  memcpy, memmove, memset, fill,
  norm, binop, cmp, fbinop, fcmp, unop, cast, padd, psub, pdiff,
  cstr, readCStr, strlen, strcmp, strncmp, strcpy, strncpy, strcat, strncat,
  strchr, strrchr, strstr, strdup, memcmp, memchr,
  isspace, isdigit, isalpha, isalnum, isxdigit, isupper, islower, isprint,
  isgraph, iscntrl, ispunct, toupper, tolower, abs, labs, llabs,
  strtol, strtoll, strtoul, strtoull, atoi, strtod, atof,
  printf, putchar, abort, exit, trap, indirect, fns, HEAP,
  register_fns, dom, promise_put, promise_get,
};
/** Resolve a MIR function address (a symbol id) to the emitted function. */
export function indirect(table, id) {
  const fn = table[id];
  if (typeof fn !== "function") {
    throw new Error("js runtime: indirect call through an unresolved function address (" + id + ")");
  }
  return fn;
}

export default rt;
