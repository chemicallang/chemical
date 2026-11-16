// Tests for the JS backend runtime's data model (no emitter needed).
// Run: node scripts/js_backend/runtime_test.mjs
import { rt } from "./runtime.mjs";

let failures = 0;
function check(name, actual, expected) {
  const ok = Object.is(actual, expected);
  if (!ok) {
    failures++;
    console.error(`FAIL ${name}: expected ${JSON.stringify(expected)}, got ${JSON.stringify(actual)}`);
  } else {
    console.log(`ok   ${name}`);
  }
}

// ── typed load/store round-trips, one per width and signedness ─────────────
for (const [size, signed, value] of [
  [1, true, -128], [1, false, 255],
  [2, true, -32768], [2, false, 65535],
  [4, true, -2147483648], [4, false, 4294967295],
  [8, true, -1], [8, false, 2 ** 40],
]) {
  const p = rt.alloc(size, size);
  rt.store(p, size, signed, value);
  check(`store/load ${size}B signed=${signed}`, rt.load(p, size, signed), value);
}

// ── 8-byte truncation-free but Number-bounded (the documented 2^53 bound) ──
{
  const p = rt.alloc(8, 8);
  rt.store(p, 8, false, 2 ** 53);
  check("u64 keeps 2^53", rt.load(p, 8, false), 2 ** 53);
}

// ── floats ─────────────────────────────────────────────────────────────────
{
  const p4 = rt.alloc(4, 4);
  rt.storeF(p4, 4, 1.5);
  check("f32 round-trip", rt.loadF(p4, 4), 1.5);
  const p8 = rt.alloc(8, 8);
  rt.storeF(p8, 8, 3.141592653589793);
  check("f64 round-trip", rt.loadF(p8, 8), 3.141592653589793);
}

// ── aggregation: a struct is bytes at computed offsets (Point{x,y}) ────────
{
  const p = rt.alloc(8, 4);
  rt.store(p + 0, 4, true, 7);
  rt.store(p + 4, 4, true, 9);
  check("struct field 0", rt.load(p + 0, 4, true), 7);
  check("struct field 1", rt.load(p + 4, 4, true), 9);
}

// ── memcpy / memset ────────────────────────────────────────────────────────
{
  const a = rt.alloc(8, 8);
  const b = rt.alloc(8, 8);
  rt.store(a, 8, false, 0x1122334455);
  rt.memcpy(b, a, 8);
  check("memcpy", rt.load(b, 8, false), 0x1122334455);
  rt.memset(b, 0, 8);
  check("memset", rt.load(b, 8, false), 0);
}

// ── null-terminated strings live in the same heap ──────────────────────────
{
  const p = rt.cstr(new TextEncoder().encode("hello"));
  check("cstr", rt.readCStr(p), "hello");
  check("cstr is not empty-terminated early", rt.readCStr(rt.cstr(new TextEncoder().encode(""))), "");
}

// ── printf: the formats the fixtures use ───────────────────────────────────
{
  const fmt = rt.cstr(new TextEncoder().encode("sum=%d str=%s z=%zu f=%f c=%c pct=%%\n"));
  let captured = "";
  const write = process.stdout.write.bind(process.stdout);
  process.stdout.write = (s) => { captured += s; return true; };
  rt.printf(fmt, 15, rt.cstr(new TextEncoder().encode("ok")), 42, 1.5, 65);
  process.stdout.write = write;
  check("printf", captured, "sum=15 str=ok z=42 f=1.5 c=A pct=%\n");
}

if (failures) {
  console.error(`\n${failures} failure(s)`);
  process.exit(1);
}
console.log("\nruntime: all checks passed");
