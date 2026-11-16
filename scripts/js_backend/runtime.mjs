// Chemical JS backend runtime (ABI: lang/docs/js-backend-abi.md, Route A).
//
// Emulated linear memory: a Uint8Array heap addressed by byte offsets. Every MIR
// load/store becomes a typed DataView access here; every aggregate is bytes, not
// a JS object, so layout is identical to the native backend and sizeof/offsetof
// agree. The emitter emits calls into this module, never raw DataView code, so
// the access rules live in one place.

const PAGE = 1 << 20;
export const HEAP = new Uint8Array(PAGE);
const DV = new DataView(HEAP.buffer);

// ── Allocator ───────────────────────────────────────────────────────────────
// Bump allocation for now: std calls malloc/realloc/free, and `free` is a no-op.
// A real free list is a later optimisation; correctness first, and the heap is
// process-lifetime for a page bundle anyway.
let brk = 1024; // 0 stays the null pointer
export function alloc(size, align) {
  const a = align > 0 ? align : 1;
  const addr = (brk + a - 1) & ~(a - 1);
  brk = addr + size;
  if (brk > HEAP.length) throw new Error("js runtime: heap exhausted");
  return addr;
}
export function malloc(n) { return alloc(n, 8); }
export function realloc_(p, n) {
  const q = alloc(n, 8);
  if (p) HEAP.copyWithin(q, p, p + n);
  return q;
}
export function free_(_p) {}

// ── Typed memory access ─────────────────────────────────────────────────────
export function load(p, size, signed) {
  if (size === 1) return signed ? DV.getInt8(p) : DV.getUint8(p);
  if (size === 2) return signed ? DV.getInt16(p, true) : DV.getUint16(p, true);
  if (size === 4) return signed ? DV.getInt32(p, true) : DV.getUint32(p, true);
  if (size === 8) {
    return signed ? Number(DV.getBigInt64(p, true)) : Number(DV.getBigUint64(p, true));
  }
  throw new Error("js runtime: load of size " + size);
}
export function store(p, size, signed, v) {
  if (size === 1) { signed ? DV.setInt8(p, v) : DV.setUint8(p, v); return; }
  if (size === 2) { signed ? DV.setInt16(p, v, true) : DV.setUint16(p, v, true); return; }
  if (size === 4) { signed ? DV.setInt32(p, v, true) : DV.setUint32(p, v, true); return; }
  if (size === 8) {
    signed ? DV.setBigInt64(p, BigInt(v), true) : DV.setBigUint64(p, BigInt(v), true);
    return;
  }
  throw new Error("js runtime: store of size " + size);
}
export function loadF(p, size) {
  return size === 4 ? DV.getFloat32(p, true) : DV.getFloat64(p, true);
}
export function storeF(p, size, v) {
  size === 4 ? DV.setFloat32(p, v, true) : DV.setFloat64(p, v, true);
}
export function memcpy(dst, src, n) { HEAP.copyWithin(dst, src, src + n); }
export function memset(dst, byte, n) { HEAP.fill(byte, dst, dst + n); }

// ── Strings (null-terminated views into the heap) ────────────────────────────
export function cstr(bytes) {
  const p = alloc(bytes.length + 1, 1);
  HEAP.set(bytes, p);
  HEAP[p + bytes.length] = 0;
  return p;
}
export function readCStr(p) {
  let end = p;
  while (HEAP[end] !== 0) end++;
  return new TextDecoder().decode(HEAP.subarray(p, end));
}

// ── printf ──────────────────────────────────────────────────────────────────
// Enough of C's printf for the programs the backend is tested on. Formats are
// read from the heap (they are `const.string` values), so this is the same code
// path a native printf would take over the same bytes.
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
    // flags, width and precision
    while (spec && "0123456789.-+ #".includes(spec)) spec = fmt[i++];
    // length modifiers: %zu, %lu, %lld, %hhu ...
    while (spec === "z" || spec === "l" || spec === "h") spec = fmt[i++];
    const a = args[ai++];
    switch (spec) {
      case "d": case "i": out += String(Math.trunc(a)); break;
      case "u": out += String(Math.trunc(a)); break;
      case "x": out += Math.trunc(a).toString(16); break;
      case "X": out += Math.trunc(a).toString(16).toUpperCase(); break;
      case "o": out += Math.trunc(a).toString(8); break;
      case "c": out += String.fromCharCode(a); break;
      case "s": out += a ? readCStr(a) : "(null)"; break;
      case "f": case "g": case "e": out += String(a); break;
      case "p": out += "0x" + Math.trunc(a).toString(16); break;
      default: out += "%" + spec; break;
    }
  }
  process.stdout.write(out);
}

// One namespace object so emitted code can import a single binding.
export const rt = {
  alloc, malloc, realloc_, free_, load, store, loadF, storeF,
  memcpy, memset, cstr, readCStr, printf, HEAP,
};
export default rt;
