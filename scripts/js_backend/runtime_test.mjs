// Tests for the JS backend runtime's data model and arithmetic (no emitter).
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
  rt.st(p, size, signed, value);
  check(`store/load ${size}B signed=${signed}`, rt.ld(p, size, signed), value);
}

// ── 8-byte truncation-free but Number-bounded (the documented 2^53 bound) ──
{
  const p = rt.alloc(8, 8);
  rt.st(p, 8, false, 2 ** 53);
  check("u64 keeps 2^53", rt.ld(p, 8, false), 2 ** 53);
}

// ── floats ─────────────────────────────────────────────────────────────────
{
  const p4 = rt.alloc(4, 4);
  rt.stF(p4, 4, 1.5);
  check("f32 round-trip", rt.ldF(p4, 4), 1.5);
  const p8 = rt.alloc(8, 8);
  rt.stF(p8, 8, 3.141592653589793);
  check("f64 round-trip", rt.ldF(p8, 8), 3.141592653589793);
}

// ── aggregation: a struct is bytes at computed offsets (Point{x,y}) ────────
{
  const p = rt.alloc(8, 4);
  rt.st(p + 0, 4, true, 7);
  rt.st(p + 4, 4, true, 9);
  check("struct field 0", rt.ld(p + 0, 4, true), 7);
  check("struct field 1", rt.ld(p + 4, 4, true), 9);
}

// ── memcpy / memset ────────────────────────────────────────────────────────
{
  const a = rt.alloc(8, 8);
  const b = rt.alloc(8, 8);
  rt.st(a, 8, false, 0x1122334455);
  rt.memcpy(b, a, 8);
  check("memcpy", rt.ld(b, 8, false), 0x1122334455);
  rt.memset(b, 0, 8);
  check("memset", rt.ld(b, 8, false), 0);
}

// ── frames: allocation is zeroed and reclaimed by the mark ─────────────────
{
  const mark = rt.frame_mark();
  const a = rt.frame_alloc(4, 4);
  rt.st(a, 4, true, 1234);
  rt.frame_release(mark);
  const b = rt.frame_alloc(4, 4);
  check("frame_release reclaims storage", b, a);
  check("frame_alloc zeroes", rt.ld(b, 4, true), 0);
  rt.frame_release(mark);
}

// ── C integer semantics ────────────────────────────────────────────────────
// binop(op, a, ab, as, b, bb, bs, rb, rs); op: 0=Add 1=Sub 2=Mul 3=Div 4=Rem
// 5=Shl 6=Shr 7=And 8=Or 9=Xor
const B = rt.binop;
check("i8 overflow wraps", B(0, 127, 8, true, 1, 8, true, 8, true), -128);
check("u8 overflow wraps", B(0, 255, 8, false, 1, 8, false, 8, false), 0);
check("i32 overflow wraps", B(0, 2147483647, 32, true, 1, 32, true, 32, true), -2147483648);
check("u32 overflow wraps", B(0, 4294967295, 32, false, 1, 32, false, 32, false), 0);
check("i32 div truncates toward zero", B(3, -7, 32, true, 2, 32, true, 32, true), -3);
check("i32 rem keeps the dividend sign", B(4, -7, 32, true, 2, 32, true, 32, true), -1);
check("div by zero is 0", B(3, 7, 32, true, 0, 32, true, 32, true), 0);
check("i32 shl 1<<31", B(5, 1, 32, true, 31, 32, true, 32, true), -2147483648);
check("u32 shl 1<<31", B(5, 1, 32, false, 31, 32, false, 32, false), 2147483648);
check("u32 shr is logical", B(6, 4294967295, 32, false, 1, 32, false, 32, false), 2147483647);
check("i32 shr is arithmetic", B(6, -8, 32, true, 1, 32, true, 32, true), -4);
check("mixed i32+u32 promotes unsigned", B(0, -1, 32, true, 1, 32, false, 32, false), 0);
check("u32 & u32", B(7, 0xffff0000, 32, false, 0x00ffff00, 32, false, 32, false), 0x00ff0000);
check("u32 | u32", B(8, 0xf0f0f0f0, 32, false, 0x0f0f0f0f, 32, false, 32, false), 0xffffffff);
check("u32 ^ u32", B(9, 0xffffffff, 32, false, 0xffffffff, 32, false, 32, false), 0);
check("u64 xor keeps 64 bits", B(9, 2 ** 40, 64, false, 1, 64, false, 64, false), 2 ** 40 + 1);
check("u64 shl 2^40<<1", B(5, 2 ** 40, 64, false, 1, 64, false, 64, false), 2 ** 41);

// ── 64-bit values beyond 2^53: unsigned saturates, signed stays exact ──────
// `(size_t)-1` is not representable; saturating keeps `x >= (size_t)-1` false
// instead of comparing against -1 (which fired real std guards once).
check("u64 0-1 saturates", B(1, 0, 64, false, 1, 64, false, 64, false), Number.MAX_SAFE_INTEGER);
check("u64 -1 saturates", rt.norm(-1, 64, false), Number.MAX_SAFE_INTEGER);
check("i64 -1 stays exact", rt.norm(-1, 64, true), -1);

// comparisons (10=Lt 11=Le 12=Gt 13=Ge 14=Eq 15=Ne)
const C = rt.cmp;
check("u32 max > i32 1 (unsigned)", C(12, 4294967295, 32, false, 1, 32, true), 1);
check("i8 -1 < 0", C(10, -1, 8, true, 0, 8, true), 1);
check("u64 eq", C(14, 2 ** 40, 64, false, 2 ** 40, 64, false), 1);
check("u64 max > 32 is false", C(12, 32, 64, false, Number.MAX_SAFE_INTEGER, 64, false), 0);
check("u64 max >= 32 is true", C(13, Number.MAX_SAFE_INTEGER, 64, false, 32, 64, false), 1);

// casts: cast(v, fb, fs, ff, tb, ts, tf)
check("i32 -1 -> u8", rt.cast(-1, 32, true, false, 8, false, false), 255);
check("u8 200 -> i8", rt.cast(200, 8, false, false, 8, true, false), -56);
check("double 3.9 -> i32", rt.cast(3.9, 64, true, true, 32, true, false), 3);
check("double -3.9 -> i32", rt.cast(-3.9, 64, true, true, 32, true, false), -3);
check("i32 -1 -> f64", rt.cast(-1, 32, true, false, 64, true, true), -1);
check("f64 1/3 -> f32 rounds", rt.cast(1 / 3, 64, true, true, 32, true, true), Math.fround(1 / 3));

// unop(op, a, bits, signed, flt, dbl): 0=Neg 1=Plus 2=Not 3=BitNot
check("i8 neg wraps", rt.unop(0, -128, 8, true, false, false), -128);
check("not 0", rt.unop(2, 0, 32, false, false, false), 1);
check("bitnot u8", rt.unop(3, 0, 8, false, false, false), 255);

// pointer arithmetic scales by the pointee size
check("padd", rt.padd(1000, 3, 4), 1012);
check("psub", rt.psub(1000, 3, 4), 988);
check("pdiff", rt.pdiff(1012, 1000, 4), 3);

// ── null-terminated strings live in the same heap ──────────────────────────
{
  const p = rt.cstr(new TextEncoder().encode("hello"));
  check("cstr", rt.readCStr(p), "hello");
  check("strlen", rt.strlen(p), 5);
  check("cstr is not empty-terminated early", rt.readCStr(rt.cstr(new TextEncoder().encode(""))), "");
  const q = rt.cstr(new TextEncoder().encode("help"));
  check("strcmp orders", rt.strcmp(p, q) < 0 ? "lt" : "not-lt", "lt");
  check("strcmp equal", rt.strcmp(p, p), 0);
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
{
  const fmt = rt.cstr(new TextEncoder().encode("pi=%.2f neg=%d\n"));
  let captured = "";
  const write = process.stdout.write.bind(process.stdout);
  process.stdout.write = (s) => { captured += s; return true; };
  rt.printf(fmt, 3.14159, -7);
  process.stdout.write = write;
  check("printf precision", captured, "pi=3.14 neg=-7\n");
}

// ── traps name the check that fired ────────────────────────────────────────
{
  let message = "";
  try {
    rt.trap("__chc_div_zero_fail");
  } catch (error) {
    message = String(error.message);
  }
  check("trap names the helper", message, "js runtime: trap: __chc_div_zero_fail");
}

if (failures) {
  console.error(`\n${failures} failure(s)`);
  process.exit(1);
}
console.log("\nruntime: all checks passed");