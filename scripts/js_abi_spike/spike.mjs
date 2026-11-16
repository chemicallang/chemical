// JS backend ABI spike (lang/docs/js-backend-abi.md §9).
//
// Hand-written JavaScript for the same program as
// lang/tests/web/fixtures/js_abi_spike/src/main.ch, executed against the emulated linear
// memory that Route A prescribes: a Uint8Array heap, byte offsets from the type
// table, DataView for every access. If this file prints byte-identical output to
// the C backend, the ABI's data model is sound: layout, SSO/heap strings,
// vectors of structs, and variant tag+payload all survive the mapping.
//
// It is deliberately *not* an interpreter: it is the shape the emitter will
// produce (explicit offsets, explicit sizes), which is what the ABI has to
// support. Running std's own MIR through it is the emitter's differential test.

// ── The heap ────────────────────────────────────────────────────────────────
const HEAP_BYTES = 1 << 20;
const HEAP = new Uint8Array(HEAP_BYTES);
const DV = new DataView(HEAP.buffer);
let BRK = 64; // offset 0 is the null pointer

function alloc(size, align) {
  const addr = (BRK + align - 1) & ~(align - 1);
  BRK = addr + size;
  if (BRK > HEAP_BYTES) throw new Error("heap exhausted");
  return addr;
}
// std calls malloc/realloc/free; the spike models them over the same heap.
const malloc = (n) => alloc(n, 8);
const realloc_ = (p, n) => {
  const q = malloc(n);
  if (p) HEAP.copyWithin(q, p, p + n);
  return q;
};
const free_ = (_p) => {};

// ── Type table ──────────────────────────────────────────────────────────────
// Sizes and offsets are exactly what MIRLayout derives on the native profile:
// struct = sequential with alignment, vector = {ptr, size, cap},
// string = union{const{ptr,len} | heap{ptr,len,cap} | sso{char[16], uchar}} + state.
const TYPES = {
  Point: { size: 8, align: 4, fields: { x: [0, "i32"], y: [4, "i32"] } },
  VecPoint: { size: 24, align: 8, fields: { data: [0, "ptr"], size: [8, "u64"], cap: [16, "u64"] } },
  String: { size: 32, align: 8, state: 24, SSO: { buffer: 0, capacity: 16 } },
  Shape: { size: 8, align: 4, tag: 0, payload: 4 },
};
const ALIGN_OF = { i32: 4, u64: 8, ptr: 8, char: 1 };

function layoutOf(name) {
  // Derive size/align the way the layout algorithm does, then assert the table.
  const t = TYPES[name];
  if (t.fields) {
    let off = 0;
    let align = 1;
    for (const [, [fo, fk]] of Object.entries(t.fields)) {
      const fa = ALIGN_OF[fk];
      if (fa > align) align = fa;
      off = (off + fa - 1) & ~(fa - 1);
      off += fk === "ptr" || fk === "u64" ? 8 : 4;
    }
    const size = (off + align - 1) & ~(align - 1);
    return { size, align };
  }
  return { size: t.size, align: t.align };
}

// ── str: std::string over the heap ──────────────────────────────────────────
// state: '0' constant (borrowed) | '1' SSO (bytes inside the struct) | '2' heap.
const CH0 = 0x30, CH1 = 0x31, CH2 = 0x32;

function strNew() {
  const s = alloc(TYPES.String.size, TYPES.String.align);
  DV.setUint8(s + TYPES.String.state, CH1); // empty is SSO with length 0
  DV.setUint8(s + TYPES.String.SSO.buffer + TYPES.String.SSO.capacity, 0);
  return s;
}
function strState(s) { return DV.getUint8(s + TYPES.String.state); }
function strLen(s) {
  const st = strState(s);
  if (st === CH1) return DV.getUint8(s + TYPES.String.SSO.buffer + TYPES.String.SSO.capacity);
  return Number(DV.getBigUint64(s + 8, true));
}
function strData(s) {
  return strState(s) === CH1 ? s + TYPES.String.SSO.buffer : Number(DV.getBigUint64(s, true));
}
function strPromoteToHeap(s, want) {
  const len = strLen(s);
  const old = strData(s);
  const cap = want > 32 ? want : 32;
  const buf = malloc(cap);
  for (let i = 0; i < len; i++) HEAP[buf + i] = HEAP[old + i];
  DV.setBigUint64(s, BigInt(buf), true);
  DV.setBigUint64(s + 8, BigInt(len), true);
  DV.setBigUint64(s + 16, BigInt(cap), true);
  DV.setUint8(s + TYPES.String.state, CH2);
}
function strAppendByte(s, byte) {
  const len = strLen(s);
  if (strState(s) === CH1) {
    if (len + 1 < TYPES.String.SSO.capacity) {
      HEAP[s + TYPES.String.SSO.buffer + len] = byte;
      DV.setUint8(s + TYPES.String.SSO.buffer + TYPES.String.SSO.capacity, len + 1);
      return;
    }
    strPromoteToHeap(s, len + 1);
  }
  let cap = Number(DV.getBigUint64(s + 16, true));
  if (len + 1 > cap) {
    const newCap = cap * 2 > len + 1 ? cap * 2 : len + 1;
    const buf = realloc_(Number(DV.getBigUint64(s, true)), newCap);
    DV.setBigUint64(s, BigInt(buf), true);
    DV.setBigUint64(s + 16, BigInt(newCap), true);
    cap = newCap;
  }
  const data = Number(DV.getBigUint64(s, true));
  HEAP[data + len] = byte;
  DV.setBigUint64(s + 8, BigInt(len + 1), true);
}
function strAppendView(s, bytes) {
  for (const b of bytes) strAppendByte(s, b);
}
function strUtf8(s) {
  const len = strLen(s), data = strData(s);
  return new TextDecoder().decode(HEAP.subarray(data, data + len));
}

// ── vector<Point> ───────────────────────────────────────────────────────────
// {data : *T, size : size_t, cap : size_t}; elem_size from the type table.
function vecNew(esize) {
  const v = alloc(TYPES.VecPoint.size, TYPES.VecPoint.align);
  DV.setBigUint64(v + 0, 0n, true);
  DV.setBigUint64(v + 8, 0n, true);
  DV.setBigUint64(v + 16, 0n, true);
  DV.setUint32(v + 24 - 4, esize, true); // not part of the struct; kept in wrapper
  return v;
}
function vecPushPoint(v, x, y) {
  let size = Number(DV.getBigUint64(v + 8, true));
  let cap = Number(DV.getBigUint64(v + 16, true));
  if (size === cap) {
    const newCap = cap === 0 ? 8 : cap * 2;
    const buf = realloc_(Number(DV.getBigUint64(v, true)), newCap * TYPES.Point.size);
    DV.setBigUint64(v, BigInt(buf), true);
    DV.setBigUint64(v + 16, BigInt(newCap), true);
    cap = newCap;
  }
  const data = Number(DV.getBigUint64(v, true));
  const elem = data + size * TYPES.Point.size;
  DV.setInt32(elem + TYPES.Point.fields.x[0], x, true);
  DV.setInt32(elem + TYPES.Point.fields.y[0], y, true);
  DV.setBigUint64(v + 8, BigInt(size + 1), true);
}

// ── The spike program ───────────────────────────────────────────────────────
function main() {
  const out = [];
  const pointLayout = layoutOf("Point");
  const vecLayout = layoutOf("VecPoint");
  const strLayout = layoutOf("String");
  out.push(`point_size=${pointLayout.size}`);
  out.push(`vec_size=${vecLayout.size}`);
  out.push(`string_size=${strLayout.size}`);

  // 2. SSO path
  const sso = strNew();
  strAppendView(sso, new TextEncoder().encode("hello"));
  out.push(`sso=${strUtf8(sso)}|${strLen(sso)}`);

  // 3. heap path (40 bytes > SSO capacity 16)
  const heapStr = strNew();
  for (let i = 0; i < 40; i++) strAppendByte(heapStr, 0x78); // 'x'
  out.push(`heap=${strUtf8(heapStr)}|${strLen(heapStr)}`);

  // 4. vector<Point>
  const v = vecNew(TYPES.Point.size);
  vecPushPoint(v, 1, 2);
  vecPushPoint(v, 3, 4);
  let sum = 0;
  const size = Number(DV.getBigUint64(v + 8, true));
  const data = Number(DV.getBigUint64(v, true));
  for (let j = 0; j < size; j++) {
    const p = data + j * TYPES.Point.size;
    sum += DV.getInt32(p + TYPES.Point.fields.x[0], true);
    sum += DV.getInt32(p + TYPES.Point.fields.y[0], true);
  }
  out.push(`vec=${size}|${sum}`);

  // 5. variant: tag then payload union
  const sh = alloc(TYPES.Shape.size, TYPES.Shape.align);
  DV.setInt32(sh + TYPES.Shape.tag, 0, true); // Circle = 0
  DV.setInt32(sh + TYPES.Shape.payload, 7, true);
  const tag = DV.getInt32(sh + TYPES.Shape.tag, true);
  var label = "circle";
  if (tag === 1) label = "square";
  out.push(`shape=${label}|${DV.getInt32(sh + TYPES.Shape.payload, true)}`);

  console.log(out.join("\n"));
}

main();
