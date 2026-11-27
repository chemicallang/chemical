# P0.1 blockers

Each item records the exact failing command, the minimal repro committed in
this packet, the root cause (verified or best hypothesis), and the smallest
change that unblocks it. Items 1 and 3 need a **compiler/std** change; the rest
are library/platform work and are listed here so the parent can order them.

---

## 1. Compiler/std: named function assigned to `std::function`

> **RESOLVED in P0.1a.** Both backends now wrap the named function in a
> non-capturing callable instance (thunk + `make2`); the repro passes. See
> [`../P0.1a/REPORT.md`](../P0.1a/REPORT.md). The text below is the original
> P0.1 finding, kept for history.

**Repro (committed):** `lang/tests/ui_probe/bugs/named_fn/`

```bash
cmake-build-debug/TCCCompiler lang/tests/ui_probe/bugs/named_fn/chemical.mod \
    -o /tmp/named_fn.exe --mode debug_quick
# error: cannot convert 'static void (int, int)' to
#        'struct std_stddefault_function_instance' in .../object_tcc.o

cmake-build-debug/Compiler lang/tests/ui_probe/bugs/named_fn/chemical.mod \
    -o /tmp/named_fn.exe --mode debug_quick && /tmp/named_fn.exe
# g=16777217
# named_fn FAIL   (exit 1)
```

Minimal source:

```chemical
var g : int = 0
func inc(a : int, b : int) { unsafe { g = g + a + b } }
public func main() : int {
    var f : std::function<(a : int, b : int) => void> = inc
    f(1, 2)
    return 0
}
```

**Impact:** `w.on_resize(my_named_handler)` / `w.on_event(my_named_handler)`
crashes at runtime on TCC and passes garbage on LLVM. Explicit lambda syntax
works on both:

```chemical
w.on_resize(||(width : int, height : int) => { unsafe { ... } })
```

**Root cause (hypothesis, reduced):** the front end accepts a bare function
reference where a `%capture` value is expected but does not synthesize the
non-capturing `std::default_function_instance` (`fn_pointer`, null capture,
null destructor, size 0). LLVM emits a call with the wrong value shape; TCC's C
translation rejects the conversion. The non-capturing **lambda** path already
builds the correct struct via `std::default_function_instance.make`, so the fix
is to route a named function through the same construction.

**Smallest unblock:** a callable/`std::function` conversion in the type checker
or `FunctionDeclaration` call path that wraps `fn_ptr` as
`make2(ptr, null, null, 0, 0)` (or a `comptime` overload of `make`). Estimated
**100–250 LOC + tests** (front-end conversion + both backends + a negative test
that rejects a wrong signature). No MIR/ABI change expected.

**Until fixed:** document "event handlers must be written as lambdas" in the
frozen API or (better) fix before P0.2 freezes handler signatures.

---

## 2. `font` library: cmap format 4 layout is wrong (and formats are missing)

**Repro (committed):** `lang/tests/ui_probe/font_probe/`

```bash
cmake-build-debug/TCCCompiler lang/tests/ui_probe/font_probe/chemical.mod \
    -o /tmp/font_probe.exe --mode release_fast && /tmp/font_probe.exe
# glyph A U+0041 id=60189
# glyph eacute U+00E9 id=60357
# glyph CJK U+4E2D id=14601   (DejaVu has no CJK glyph)
```

Independent Python cmap oracle for the same file: `A`→**36**, `é`→**171**,
`中`→**0**.

**Root cause (verified by reading the parser):**
`lang/libs/font/src/ttf.ch:366-404` reads the format-4 arrays with a stride of
4 from `base+14`:

```
end_offset        = base + 14 + i*4                 # should be base + 14 + 2*i
start_offset      = base + 16 + i*4                 # should be base + 16 + 2*segCount + 2*i
id_delta_offset   = base + 14 + segCount*4 + i*2    # should be base + 16 + 4*segCount + 2*i
id_range_offset   = base + 14 + segCount*6 + i*2    # should be base + 16 + 6*segCount + 2*i
```

The format-4 array layout is: `endCode[segCount]`, `reservedPad`,
`startCode[segCount]`, `idDelta[segCount]`, `idRangeOffset[segCount]`. The
`glyph_offset` computation also needs the `idRangeOffset` slot address, not
`base+14+segCount*6+i*2`.

**Impact:** every system-font glyph lookup returns a wrong glyph or fails.
P1.5b cannot produce correct metrics and P2.1/P2.2 cannot render text.

**Smallest unblock (library, no compiler change):** rewrite the four offsets
and the `idRangeOffset` address in `cmap_format4_lookup` (≈20–30 LOC) and add
cmap oracle tests using DejaVu + Noto + a CJK font. Then, in the same P1.5b
work, add: cmap 12 (supplementary/emoji), TTC header, CFF/OTTO outline
(`CFF ` table) or a documented "system fallback only" decision, composite
glyphs, and contour ends in `GlyphOutline` (points/flags cannot express
contours today). That is P1.5b/P2.1 scope, not P0.1.

**Not a compiler/std blocker.**

---

## 3. Compiler: `.m` + framework linking on macOS

**Status: BLOCKED on this host (not macOS).** Source inspection shows:

- `link c "file.m"` would pass the file to clang, which infers Objective-C from
  the extension, but there is **no `link framework` directive**; `link "Cocoa"`
  becomes `-lCocoa`, not `-framework Cocoa`.
- No `-fobjc-arc`/Objective-C mode is set explicitly, and the linker flags path
  has no framework handling (`compiler/lab/LabBuildCompiler.cpp`,
  `compiler/Codegen.cpp:894 compile_c_file_to_object`).
- The `window` library has no Cocoa backend (`find lang/libs/window -iname
  '*cocoa*'` → empty), so P3.4 must add it (compiler + ObjC shim).

**Exact commands that would test it once a macOS runner exists:**

```bash
clang -x objective-c -c test.m -o test.o
clang test.o -framework Cocoa -o test
cc -c test.m -o test.o   # the compiler's current .m path
```

**Smallest unblock (compiler + platform library):** a `link framework "Cocoa"`
directive in `chemical.mod` (parser + `ModToLabConverter` + `LabBuildCompiler`
link args; ≈100–200 LOC) and a Cocoa shim compiled as ObjC. P3.4 is the packet
that owns it. No P0.1 code depends on this.

---

## 4. Platform library: no IME in `window` and no Cocoa backend

- Linux `window`: only `GDK_KEY_PRESS/RELEASE` → `event.keyval`; no
  `GtkIMContext`, no preedit/commit/candidate events. `Event` has only
  `key`/`key_char`.
- Win32 `window`: `WM_KEY*`/`WM_CHAR`; no `WM_IME_STARTCOMPOSITION`,
  `WM_IME_COMPOSITION`, `WM_IME_SETCONTEXT`, candidate positioning.
- macOS: no backend at all.
- `window_run()` is a global `gtk_main()` with a 1 ms async timer; no
  per-window present/frame trigger.

**Impact:** P3.5 (IME) and P3.4 (Cocoa) cannot start from the current library;
P1.14 must add a per-window presenter before Stage 2. This is expected plan
work, recorded as BLOCKED for P0.1 (not a compiler/std change).

---

## 5. `image` PNG encoder: stored deflate + TCC cost

**Repro:** `lang/tests/ui_probe/image_bench/`

```bash
cmake-build-debug/TCCCompiler lang/tests/ui_probe/image_bench/chemical.mod \
    -o /tmp/img_tcc.exe --mode release_fast && /tmp/img_tcc.exe
# 1920x1080 best_ms=1330 bytes=8296178
cmake-build-debug/Compiler ... 
# 1920x1080 best_ms=56  bytes=8296178
```

**Root cause:** `lang/libs/image/src/png.ch:740 png_store_deflate` writes
`BTYPE=0` stored blocks (no compression), and the encoder copies every row
byte-by-byte. 1080p output is 8.3 MB; 4K is 33.2 MB.

**Impact:** P1.10 screenshot artifacts are large, and `chemical ui shot`'s
≤1.5 s budget is consumed almost entirely by PNG encoding on the TCC path
(1.33 s). LLVM is 56 ms and is acceptable for correctness.

**Options:** (a) implement a real fixed-Huffman deflate in Chemical (no new
dependency; P1.10/P2.8 scope), (b) link a small zlib-compatible C file through
`link c`, or (c) keep stored deflate and raise the screenshot budget/size
expectation. The parent should choose before P1.10; do not silently change the
budget.

---

## 6. Raster budgets (design decision, not a code bug)

See `REPORT.md` §4 for the measured table. The scalar prototype cannot meet
"10k AA quads ≤1.2 ms" (7.3–14.8 ms), and the TCC relaxation "≤1.5× LLVM" does not
hold for pixel loops (6–9×). Proposed: keep 1.2 ms for **solid/opaque** quads,
set **AA** quads to 15 ms scalar / 6 ms SSE2 (target 2.5 ms with AVX2+span),
and specify the glyph bbox for the 10k-glyph budget. `BENCHMARKS.md` was not
edited.

---

## 7. GTK3 startup and WASM host ABI (platform, expected)

- Two GTK windows create in **107–210 ms** here, vs the `GTK/host init ≤30 ms`
  breakdown. P1.14/P3.2 must measure a single-window cold start and either
  correct the budget or move to a lighter host path.
- `lang/runtime/wasm/host.mjs` exposes only DOM create/text/attr/append/event +
  log/time. P3.7 must add canvas present, rAF, pointer/wheel, IME, clipboard and
  font metrics; P3.13 adds WebGPU/WebGL2. No compiler change is implied.

---

## Not blockers (verified)

`Application` with `&mut self` + generic `run`; `std::function` value/non-capturing
capture; `embed_file()` from a library; `link c "*.c"`; DCE of unused public
functions; wasm32 float/double math; allocation counting; GTK per-window user
data; `fs::raw::atomic_write`; Vulkan loader/RADV/lavapipe device init and
pipeline cache.
