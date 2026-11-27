# P0.1 — Feasibility and compiler probe: report

**Date:** 2026-11-27 · **Host:** Ubuntu 26.04.1, AMD Ryzen 5 7430U (12T),
14 GB, Radeon Barcelo + llvmpipe, governor `powersave`, live `DISPLAY=:0`
(see `ENV.md` and `logs/env.log`).

**Verdict:** the core language/platform capabilities needed by the UI library
are available, but **three measured gaps must be decided before P0.3a/P1.9**:

1. `std::function` from a **named function** is miscompiled (TCC rejects the C,
   LLVM passes garbage). Explicit lambda syntax works. → `BLOCKER.md` §1.
2. The `font` parser's **cmap format 4 lookup is wrong** (returns wrong glyph
   IDs); TTC/CFF-outline/cmap-12/composite/contour ends are also missing.
   → `BLOCKER.md` §2. This blocks P1.5b/P2.1, not the core runtime.
3. The **raster scalar budgets** in `BENCHMARKS.md` cannot be met for 10k
   AA quads (7.3–14.8 ms measured vs 1.2 ms budget; 4.3–5.1 ms with an SSE2 interior
   span). Proposed correction below; `BENCHMARKS.md` was **not** edited.

GPU is a **GO** for the optional Vulkan track on this host: loader + RADV +
lavapipe all create a device and pipeline cache in 24–46 ms (budget ≤80 ms).
macOS/Windows/WASM GPU remain BLOCKED on this host.

---

## 1. Claim table (RESEARCH.md §7)

| RESEARCH §7 claim | Result | Evidence |
|---|---|---|
| `window` has Windows and Linux/GTK3 backends, no macOS backend | **CONFIRMED** | `find lang/libs/window -iname '*cocoa*' -o -iname '*mac*'` → empty; `window/chemical.mod` sources only `win` + `posix` |
| `window` has no drawable/present/damage; `window_run` blocks in a global loop | **CONFIRMED** | `posix/linux.ch:1177 window_run()` → global `gtk_main()`; no present/damage API in `src/types.ch` |
| `window::Event` exposes platform key codes, integer wheel, no IME; Linux no `EVENT_CHAR`; Win32 truncates UTF-16 | **CONFIRMED** | `posix/linux.ch` maps `event.keyval`; no `GtkIMContext`/preedit; `win/win.ch` has `WM_CHAR`/`WM_KEY*`, no `WM_IME_*` |
| `window` installs a 1 ms async timer for the process lifetime | **CONFIRMED for `window_run_async`** (not plain `window_run`) | `posix/linux.ch:1370 g_timeout_add(1u, linux_async_pump_tick, ...)` in `window_async_platform_install_pump`; plain `window_run` only calls `gtk_main()` |
| `font` parses only `glyf`, cmap 0/4/6; no composite/CFF/TTC/cmap-12/OS2/name/GSUB/GPOS; no contour ends | **CONFIRMED and WORSE**: cmap format 4 layout is wrong | `font_probe` output; independent Python cmap oracle; `ttf.ch:366` |
| `std::vector::push` / `string::append` / `std::function` / `Image.new` panic on OOM; `UiError` strings allocate | **CONFIRMED** | forced OOM: `panic: function capture allocation failed`; exit 255 through `__chc_panic`, no unwind |
| TCC has no sanitizer support | **CONFIRMED (source)** | `LabBuildCompiler.cpp:937` refuses `--sanitize` on TCC |
| `image` decodes/encodes PNG/BMP/PPM and blits | **CONFIRMED, with a caveat**: PNG encoder uses **stored deflate** (no compression) and is slow on TCC | 1080p PNG = 8,296,178 bytes; TCC 1330 ms vs LLVM 56 ms |
| `link c "file.c"` works; macOS `.m` unverified | **CONFIRMED** for `.c`; **BLOCKED** for `.m` (non-macOS, no framework-link directive) | `linkc` probe; compiler has no `link framework` |
| `[no_panic]` rejects calls to unannotated std; module defaults pending | **CONFIRMED** (rejects even `vector::make`, so it is not usable for ordinary std code) | `negative/no_panic`: `SAFE-PANIC-001` |
| Interfaces support `&mut self` and generic impls; `Application` + generic `run` feasible | **CONFIRMED** | `foundation` PASS both backends |
| `std` has vector/string/Option/Result/Arc/mutex/atomics/function | **CONFIRMED** | probes compile/run |
| `intrinsics::get_raw_location()` and `embed_file()` exist | **CONFIRMED**; `embed_file` resolves relative to the calling library file | `embed` probe PASS both backends |
| Compiler <5 MB; `chemical ui` must be a tiny launcher | **NOT RE-MEASURED** (no `ui` CLI yet) | n/a |
| WASM target is freestanding scalar host ABI | **CONFIRMED** for math/DOM; host ABI lacks present/rAF/pointer/IME | `wasm_float` PASS; `wasm_ui` imports only 7 DOM/log functions |

---

## 2. Probe matrix and exact commands

All probes were run via:

```bash
lang/tests/ui_probe/run-probes.sh
```

which builds each probe with **both** `cmake-build-debug/TCCCompiler` and
`cmake-build-debug/Compiler` and writes raw logs to
`lang/docs/ui/evidence/P0.1/logs/`. It finishes `PASS=27 FAIL=0`.

| Probe | TCC | LLVM | Notes |
|---|---|---|---|
| `Application` interface + `&mut self` + `func <A> run(...) where A : App` | PASS | PASS | `foundation` |
| Same with `Result<T, E>` returns across the interface + generic `run` | PASS | PASS | `foundation` (`application_result_interface`) |
| `std::function`: by-value capture, non-capturing lambda | PASS | PASS | `|base|(...)` and `||(...)` |
| `std::function`: explicit `|&x|` borrow capture | PASS only inside `unsafe` | same | `SAFE-UNSAFE-002`; expected until typed capture tracks the reference lifetime |
| `std::function`: **named function** assigned to the type | **FAIL (C translation)** | **FAIL at runtime (garbage)** | reduced repro `bugs/named_fn/`; see `BLOCKER.md` §1 |
| `[no_panic]` transitive chain (`checked_add` → `layout_add` → `component_add`) | PASS | PASS | `foundation` |
| `[no_panic]` + panicking std call | expected diagnostic | expected diagnostic | `SAFE-PANIC-001` on `vector::make` |
| `intrinsics::embed_file()` from a library | PASS | PASS | 47-byte asset embedded from `embed_lib/src/asset.txt` |
| `intrinsics::get_raw_location()` from a library | PASS | PASS | nonzero; line = the intrinsic call inside the library function |
| source location through 3-level generic chain | PASS (nonzero) | PASS (nonzero) | captures the intrinsic's own line (202), not the outer call site (208); a helper function therefore loses the component instantiation site unless the caller threads a location token or calls the intrinsic at the macro site |
| `link c "file.c"` + `@extern` | PASS (extern call in `unsafe`) | PASS | `linkc` |
| dead-code elimination of unused `ui_kit`-like functions | PASS | PASS | stripped binaries byte-identical with/without 3 unused public components (`sizes.log`) |
| minimal window-like baseline build/run | PASS | PASS | `baseline`; see §5 |
| GTK3 window create/run/two-window user data | PASS | PASS | 2 windows, resize callbacks routed independently (`resize1=5`, `resize2=14` latest; 5/9 earlier) |
| `image.encode_png` 1080p/4K | PASS | PASS | see §4 |
| `fs::raw::atomic_write` (temp+rename) | PASS | PASS | read-back exact, no partial file |
| wasm32 float/double math | PASS | PASS | Node, `ci=312 di=900` |
| forced OOM | aborts via `__chc_panic` (exit 255) | not run (TCC shim path) | expected; first allocating operation panics |
| `gh auth status` + org permission | PASS | — | §7 |

### Commands used (representative)

```bash
# foundation, both backends
cmake-build-debug/TCCCompiler lang/tests/ui_probe/foundation/chemical.mod \
    -o /tmp/foundation_tcc.exe --mode debug_quick && /tmp/foundation_tcc.exe
cmake-build-debug/Compiler    lang/tests/ui_probe/foundation/chemical.mod \
    -o /tmp/foundation_llvm.exe --mode debug_quick && /tmp/foundation_llvm.exe

# named-function bug
cmake-build-debug/TCCCompiler lang/tests/ui_probe/bugs/named_fn/chemical.mod \
    -o /tmp/named_fn.exe --mode debug_quick   # C translation error
cmake-build-debug/Compiler    lang/tests/ui_probe/bugs/named_fn/chemical.mod \
    -o /tmp/named_fn.exe --mode debug_quick && /tmp/named_fn.exe  # g=16777217

# negative no_panic
cmake-build-debug/TCCCompiler lang/tests/ui_probe/negative/no_panic/chemical.mod \
    -o /tmp/neg.exe --mode debug_quick  # exits 1, SAFE-PANIC-001

# wasm
scripts/wasm-c-build.sh lang/tests/ui_probe/wasm_float/chemical.mod /tmp/f.wasm --export main
node lang/tests/wasm/run.mjs /tmp/f.wasm --expect-rc 0 --expect-text "wasm_float PASS"
cmake-build-debug/Compiler lang/tests/ui_probe/wasm_float/chemical.mod \
    --target wasm32-unknown-unknown -o /tmp/f2.wasm --mode debug_quick
node lang/tests/wasm/run.mjs /tmp/f2.wasm --expect-rc 0 --expect-text "wasm_float PASS"
```

---

## 3. Built-in diagnostics / safety findings

- **`[no_panic]` is transitive but currently unusable for std**:
  `func bad_alloc() [no_panic] { var v = vector<int>(); v.push(1) }` fails with
  `SAFE-PANIC-001: function 'bad_alloc' calls 'std_...vector...make', whose
  effect set is not explicitly [no_panic]`. Even construction is rejected.
  A `[no_panic]` chain of pure arithmetic works.
- **Borrow capture** of function-local storage needs `unsafe`:
  `SAFE-UNSAFE-002` on `|&base|`. UI event closures should capture by value or
  use `unsafe` at the host boundary.
- **Extern C calls** need `unsafe`: `SAFE-FFI-001`. This is expected but means
  every host shim call is an explicit boundary.
- **Mutable globals** need `unsafe`/atomics: `SAFE-GLOBAL-001`. The window probe
  uses `unsafe` in the callback bodies.
- **OOM** aborts through `__chc_panic` (no unwinding). The foundation uses 17
  process-allocator calls total (`malloc=9 realloc=8 free=10`); forcing failure
  at allocation 5 aborted at the first allocating operation (earlier run:
  `panic: function capture allocation failed`; latest:
  `panic: couldn't realloc in std::string -> resize_heap`), exit 255, no
  recovery. This confirms `CONTRACTS.md` §7.3: the UI needs its own
  allocation-free counters, and expected OOM must become a typed error before
  the frame path.

---

## 4. Raster prototype (P0 go/no-go)

`lang/tests/ui_probe/raster/raster_probe.c`, scalar C `-O2`, 1920×1080
premultiplied RGBA8, integer `div255`, analytic axis-aligned coverage. Run
pinned with `taskset -c 2`, 11 iterations; numbers are **median** (min–max in
the log). The host is `powersave` and not the reference i5-1035G1; a second run
in a lower-clock state produced ~2× slower numbers, so treat the spread as
real. Raw: `logs/raster.log`.

| Scenario | Budget (`BENCHMARKS.md`) | Measured median | Verdict |
|---|---|---|---|
| 1080p clear | ≤1 ms | **0.13–0.16 ms** | PASS |
| 1080p opaque fill | ≤1 ms | **0.30–0.38 ms** | PASS |
| 1k AA quads (~16×8 px) | (part of 1k dashboard ≤3 ms) | **0.73–1.42 ms** across runs | PASS for the quad part |
| **10k AA quads (~16×8 px)** | **≤1.2 ms** | **7.3–14.8 ms** scalar (latest 10.65); **4.3–5.1 ms** SSE2 interior span; **4.7–5.3 ms** opaque 32-bit interior | **FAIL scalar by ~6–12×** |
| 10k medium AA quads (48×24, overlapping) | none | 46.6–90.5 ms scalar; 13.6–28.0 ms SSE2 | reference only |
| 10k AA quads, 4×4 supersample | none (rejected) | 29.9–58.8 ms | do not use 16-sample AA for 10k |
| 10k solid quads (~15×7 px) | ≤1.2 ms | **0.96–1.9 ms** across runs | PASS at high clock; 1.9 ms in the low-clock run |
| 5k glyph blits (16×16) | — | 3.4–8.0 ms (text order / random) | reference |
| **10k glyph blits (16×16)** | **≤2 ms** | **6.7–12.9 ms** text order | FAIL for 16×16 stamps; ≈2.1–4 ms extrapolated for 8×10 glyphs |
| 1080p damage blit | ≤1.5 ms | **0.88–1.16 ms** | PASS |
| 200×120 damage blit | — | 0.002 ms | PASS |
| PSS (two 8.3 MB surfaces) | 1080p empty ≤24 MB | VmHWM **18.2 MB** | PASS for surfaces only |

### Proposed budget correction (parent decision; file not edited)

The 1.2 ms row mixes "solid" and "defined AA" and does not state quad area.
Measured facts:

- 10k **AA** quads covering ~1.5 M pixels cost **7.3–14.8 ms** scalar; an SSE2
  interior span still costs **4.3–5.1 ms**. Hitting 1.2 ms would require
  ~0.8 ns/pixel, i.e. an AVX2, span-based, precomputed-coverage renderer with
  opaque interior stores — not proven here.
- 10k **solid** quads at the same size cost **0.96–1.9 ms** across runs, so that
  row is achievable scalar when the governor boosts.
- 10k **glyphs** at 16×16 (2.56 M px) cost **6.7–12.9 ms**; at a realistic 8×10 glyph
  box (0.8 M px) the same throughput extrapolates to ≈2.1–4 ms, i.e. the 2 ms
  budget is plausible once the glyph bbox is specified.

Recommended amendment (do not silently apply):

```
Paint, 10k solid/opaque quads (<=16x8 px):  <=1.2 ms (scalar, boosted CPU) / <=2.0 ms (powersave)
Paint, 10k AA quads (<=16x8 px):             <=15 ms scalar reference (7.3-14.8 measured) / <=6 ms SSE2; target 2.5 ms with AVX2+span
Paint, 10k glyphs (8x10 px, cached):         <=2 ms; 16x16 stamps: <=13 ms (6.7-12.9 measured)
```

If the parent wants the original 1.2 ms AA gate kept, P1.9 must ship a SIMD
span renderer and the gate must be re-measured at that point; the scalar
reference (which P1.9 promises) cannot meet it.

---

## 5. TCC vs LLVM baseline (minimal window-like app)

`lang/tests/ui_probe/baseline` = 960×640 RGBA surface, 120 frames of clear +
quad fill + full damage copy + checksum. Build `--mode release_fast`, stripped.

| Metric | TCCCompiler | Compiler (LLVM) |
|---|---|---|
| Compile wall | **0.38 s** (`/usr/bin/time`) | **1.71 s** |
| Max RSS | 24.7 MB | 111.8 MB |
| Stripped binary | **244,432 B** | **121,368 B** |
| 120-frame runtime | **6.97 s** (latest run; 11.3 s earlier) | **1.23 s** |
| TCC/LLVM runtime ratio | **≈5.5–9.2×** | 1× |

Real GTK3 `window_probe` (`--mode release_fast`): TCC build 1.10 s / 323,936 B
stripped; LLVM 4.48 s / 166,896 B stripped. Two-window creation **107–210 ms**
across runs (GTK init; run-to-run variance, not a debug/release split) — already
above the `GTK/host init ≤30 ms` budget breakdown before any font scan or
raster. Raw: `logs/env.log`, `logs/window_release_sizes.log`,
`logs/sizes.log` (debug_quick sizes), `logs/window_probe_*.run.log`.

**Budget implication:** the documented TCC relaxation is "frame p99 ≤1.5×
LLVM". Tight pixel loops are **≈6–9× slower** under TCC on this host, not 1.5×.
The TCC path should stay functionally gated (as `BENCHMARKS.md` says) but the
1.5× frame claim needs an explicit correction or a TCC-specific scenario.

---

## 6. GPU spike and P3.10 go/no-go

Probe: `lang/tests/ui_probe/gpu/vulkan_probe.py` (ctypes; no Vulkan headers on
this host). Raw: `logs/vulkan_default.json`, `logs/vulkan_lavapipe.json`,
`logs/probe_breakdown.log`.

| Check | Result |
|---|---|
| Loader | `libvulkan.so.1` 1.4.341, load 0.25 ms |
| Default adapter | **AMD Radeon Graphics (RADV RENOIR)**, integrated, API 1.4.335, 5 queue families |
| Software adapter | **llvmpipe / lavapipe** (`lvp_icd.json`), CPU, API 1.4.335 |
| Instance + device create | PASS on both |
| Pipeline cache | PASS; create + `vkGetPipelineCacheData` (32 bytes lavapipe / 36 bytes RADV empty cache). Actual cache reuse across runs is **not tested** (no shaders/pipelines created) |
| Init → device ready | **24–46 ms** across runs (27.3 ms RADV, 24.0 ms lavapipe in the latest run; 45.9/43.9 earlier), including instance/device enumeration — budget ≤80 ms |
| Device loss | **NOT TESTED**: no way to force loss on lavapipe and no safe trigger on the shared desktop GPU; P3.10/P3.14 must test with a simulated loss hook |
| Metal | **BLOCKED**: not macOS, no Metal SDK |
| WebGPU/WebGL2 | **BLOCKED**: no browser harness for a GPU host ABI; `lang/runtime/wasm/host.mjs` has no GPU imports |
| SDL3 GPU | **BLOCKED**: SDL3 dev/runtime absent (`pkg-config sdl3` fails; only SDL2 runtime 1.96 MB present) |

**Design comparison (no new dependency installed):**

| Option | Binary/dependency | Startup | CI | Verdict |
|---|---|---|---|---|
| Native Vulkan | + `libvulkan.so.1` (0.59 MB loader); driver is system (lavapipe 14.3 MB, RADV/Mesa system) | 24–46 ms measured | lavapipe works headless; no `vulkaninfo` needed (ctypes probe) | **GO** for P3.10/P3.11 |
| SDL3 GPU | + SDL3 library (≈2 MB) not present; wraps Vulkan/Metal/D3D12 | unmeasured | needs SDL3 package in CI | **NO-GO** for v1; avoid a new runtime dep |
| GL3/GLES3/WebGL2 | libGL 0.54 MB + libEGL 0.07 MB present; WebGL2 only in browser | unmeasured | llvmpipe GL works, but GL3 is deprecated on macOS and lacks pipeline cache/device-loss semantics | fallback only (WASM), not the native primary |
| Metal native | framework only, no extra runtime | unmeasured | **BLOCKED** here | P3.12 deferred until a macOS runner |

**P3.10 recommendation:** **GO**, CPU-first with Vulkan as the optional native
accelerator on Linux/Windows; keep CPU as reference/fallback; do not add SDL3;
add a synthetic device-loss test in P3.10 before claiming recovery. macOS
(Metal) and WASM (WebGPU/WebGL2) remain BLOCKED runners.

---

## 7. Font findings

System discovery (`fc-list` on this host): **464 entries / 243 families**;
398 TTF, 36 OTF, 30 TTC entries. Families include DejaVu Sans, Noto Sans,
Liberation, Lato, Noto Sans CJK (TTC), Noto Color Emoji, URW base-35 CFF OTFs.
Script coverage counts from `fc-list :lang=`: en 207, zh 31, ja 31, ko 30,
ar 12, he 20, th 6, hi 4, emoji 1 (raw: `logs/font_system.log`). Exact sizes:

| File | Size |
|---|---|
| DejaVuSans.ttf | 759,720 B |
| NotoSans-Regular.ttf | 512,672 B |
| NotoSansCJK-Regular.ttc | 19,484,784 B |
| NotoColorEmoji.ttf | 11,020,136 B |
| NimbusSans-Regular.otf (CFF) | 82,264 B |
| Lato-Regular.ttf | 657,212 B |

`font_probe` results (identical on TCC and LLVM):

| Font | Load | `A` | `é` | `中` | `😀` | Outline |
|---|---|---|---|---|---|---|
| DejaVuSans.ttf | OK (6253 glyphs, em 2048) | **id=60189** | **id=60357** | **id=14601** (font has no CJK!) | FAIL | `A` points=16, `é` points=16, `中` unsupported |
| NotoSans-Regular.ttf | OK (3317 glyphs) | FAIL | FAIL | FAIL | FAIL | all FAIL |
| NotoSansCJK-Regular.ttc | **LOAD_FAIL** | — | — | — | — | — |
| NotoColorEmoji.ttf | OK (4124 glyphs) | FAIL | FAIL | FAIL | FAIL | all FAIL |
| NimbusSans-Regular.otf (CFF) | OK (855 glyphs) | FAIL | FAIL | FAIL | FAIL | all FAIL |

Independent oracle (Python parse of DejaVu's cmap) says `A`→**36**, `é`→**171**,
`中`→**0**. The parser returns 60189/60357/14601 → **cmap format 4 is read with
wrong array offsets** (`ttf.ch:366-404`: end/start/delta/range arrays are read
with a stride of 4 from `base+14` instead of the spec layout at
`base+14+2*segCount`). This is a library bug, not a compiler/std blocker; see
`BLOCKER.md` §2.

Other confirmed gaps: no TTC, no cmap 12 (supplementary planes/emoji), no CFF
outline (`glyf` absent), no composite glyphs (`UnsupportedFeature`), and
`GlyphOutline` has no contour-end field (points/flags only). So system-font
support in P1.5b/P2.1 needs a real cmap parser and outline work before text can
be rendered.Runtime cache: `fs::raw::atomic_write` (temp file + rename in the same
directory) **works** and read-back is exact (`P0.1 font_atomic_write PASS`).
A hash-verified font download/cache package (`ui_fonts`) does **not** exist yet;
`http` + `fs` are the building blocks. WASM `FontFace`/`document.fonts` metrics
are **BLOCKED**: no such host import in `lang/runtime/wasm/host.mjs` or
`lang/libs/dom` (only `Canvas2D.setFont`/`fillText`).

**No font is linked by default:** the tree contains only
`lang/tests/fuzz/corpus/ttf_bad.ttf` (a fuzz fixture); `ldd` on the foundation
probe shows no freetype/harfbuzz/fontconfig/pango/cairo. PASS.

---

## 8. WASM host ABI coverage

Toolchain works here: `scripts/wasm-test.sh --skip-playwright` → **4 passed,
0 failed**; `wasm_float` passes on both backends; Playwright is present only
under `lang/compiled/wasm_ui/node_modules`.

Imports/exports of the shipped demo `lang/compiled/wasm_ui/app.wasm`:

```
imports: env.chc_log, env.chc_wasm_dom_body, env.chc_wasm_dom_create_element,
         env.chc_wasm_dom_set_text, env.chc_wasm_dom_append_child,
         env.chc_wasm_dom_set_attribute, env.chc_wasm_dom_add_event_listener
exports: memory, chc_wasm_on_event, start
```

| Capability | WASM host | `lang/libs/dom` (JS backend) | Status for P3.7 |
|---|---|---|---|
| console/time | `chc_log`, `chc_now_millis` | — | OK |
| DOM create/text/attr/append/event | 6 imports | full DOM API | OK for DOM semantics |
| canvas create/resize/present | **absent** | `Canvas2D` API | **BLOCKED** |
| rAF | **absent** | `requestAnimationFrame` | **BLOCKED** |
| pointer/wheel | **absent** | pointer events + capture | **BLOCKED** |
| IME preedit/commit | **absent** | **absent** | **BLOCKED** |
| font/FontFace metrics | **absent** | only canvas `setFont` | **BLOCKED** |
| clipboard | **absent** | not found | **BLOCKED** |

Math (float/double) works on wasm32 through both backends; SIMD is out of
scope, as documented.

---

## 9. macOS / Windows / IME / user data

| Probe | Result | Reason |
|---|---|---|
| GTK per-window user data + callbacks | **PASS** | two windows, independent resize counters (5 vs 13); safe `Window` registers a stable `WindowState*`; GTK signals route by widget context |
| Win32 per-window user data | **STATIC PASS / RUNTIME BLOCKED** | `win/win.ch` uses `SetWindowLongPtrW(GWLP_USERDATA, w)` and `w.user_data`; not runnable on this host |
| Cocoa per-window user data | **BLOCKED** | no Cocoa files/backend in `lang/libs/window` |
| Linux IME preedit/candidate | **BLOCKED (capability absent)** | no `GtkIMContext`, no preedit/commit event; `Event` has keyval/`key_char` only |
| Win32 IME preedit/candidate | **BLOCKED (capability absent)** | no `WM_IME_*` handlers |
| macOS IME | **BLOCKED** | no backend |
| WASM present/rAF/pointer/IME | **BLOCKED** | not in the host import table (§8) |
| `.m` + framework linking | **BLOCKED** | not macOS; compiler has no `link framework` directive; `link "Cocoa"` would become `-lCocoa`, not `-framework Cocoa` |
| `gh auth status` | **PASS** | logged in as `wakaztahir`, scopes `gist/read:org/repo/workflow` |
| `gh` org permission | **PASS** | `chemicallang` org admin active; `repos/chemicallang/components-e2e` → `admin/maintain/push/pull`; can create `ui-examples` |

---

## 10. Blockers (see `BLOCKER.md` for repros and patch sizes)

| # | Blocker | Class | Affects |
|---|---|---|---|
| 1 | Named function → `std::function` miscompiled (TCC C error; LLVM garbage) | **compiler/std** | UI event handler ergonomics; workaround: explicit lambda |
| 2 | `font` cmap format 4 layout wrong; no TTC/CFF outline/cmap-12/composite/contour ends | library | P1.5b, P2.1, P2.2 |
| 3 | No `link framework` + `.m` host shim support | **compiler** | P3.4 macOS |
| 4 | No IME in `window` (GTK/Win32) and no Cocoa backend | platform library | P3.4, P3.5 |
| 5 | `image` PNG uses stored deflate (8.3 MB/1080p) and TCC encode 1.33 s | library perf | P1.10 screenshots, `ui shot` budget |
| 6 | GTK3 two-window creation 107–210 ms vs 30 ms host-init breakdown | platform | P1.14/P3.2 budget |
| 7 | WASM host lacks present/rAF/pointer/IME/font imports | platform | P3.7, P3.13 |

No compiler/std change is needed for: `Application`/generic `run`, `embed_file`,
`link c`, DCE, wasm float math, allocation counting, or the core window probe.

---

## 11. Self-review

- **No library code was added** and nothing under `lang/libs/ui*` exists.
- `BENCHMARKS.md`, `API.md`, `CONTRACTS.md`, `TEXT.md`, `HOST.md`,
  `PROTOCOL.md`, `CATALOG.md`, `DESIGN.md`, `EXECUTION.md` were **not edited**.
  The raster budget problem is recorded as a *proposed* amendment, not applied.
- Every PASS above was produced by a command in `run-probes.sh` or a command
  quoted in this report; every non-run platform is marked **BLOCKED** with the
  reason, not skipped.
- The window probe initially crashed when a **named function** was passed to
  `on_resize`; this was reduced to `bugs/named_fn/` and recorded as a blocker.
  The probe now uses explicit lambdas, and both backends pass.
- The font result is worse than RESEARCH §7 predicted: not only are formats
  missing, the format-4 lookup is wrong. The independent Python cmap oracle
  (`A`→36) is included in the log/analysis so this is not a probe bug.
- Raster numbers are noisy on this `powersave` host; both the high-clock and
  low-clock runs are described, and the budget conclusion uses the *faster*
  run so it cannot be accused of pessimism.
- The GPU probe uses ctypes because no Vulkan headers are installed; it is
  still a real loader/ICD/device/pipeline-cache exercise, not a mock. Device
  loss is explicitly **not** claimed.
- The OOM probe uses an LD_PRELOAD shim (no compiler change); the panic path and
  exit code are real. It measures the process allocator, not a future UI
  counter, and the report says so.

---

## 7. Parent sign-off (2026-11-27)

Decisions after auditing the probes:

- **Raster budgets amended** in `BENCHMARKS.md` §2.3: solid/opaque 10k quads
  ≤1.2/2.0 ms; AA 10k quads ≤2.5 ms target / ≤6 ms hard with SSE2; scalar
  reference ≤15 ms; 10k glyphs specified at an 8×10 bbox. `P1.9` uses
  analytic/16×16 coverage. This is a measured correction, not a relaxed green.
- **TCC policy amended**: TCC is the fast-iteration/functional backend; frame
  p99 is not gated on TCC (measured 5.5–9.2× LLVM on pixel loops). The 1.5×
  claim is removed.
- **Startup budget amended**: GTK3 v1 cold start is ≤220 ms p50 / ≤350 ms p99
  (measured two-window GTK init 107–210 ms). The ≤120 ms p50 target is retained
  as a stretch goal requiring a lighter X11/Wayland host (P3.2), not GTK.
- **Named-function `std::function` bug**: scheduled as compiler checkpoint
  `P0.1a` before P0.2 freezes handler signatures. Explicit lambdas remain the
  documented workaround until then.
- **Font cmap-4/TTC/CFF/cmap-12/composite/contour work**: scheduled in
  `P1.5b`/`P2.1`; cmap-4 is a parser bug, not a compiler blocker.
- **PNG stored deflate**: accepted for early headless screenshots; real
  compression is a `P1.10`/`P2.8` decision, and benchmark screenshots use LLVM.
- **GPU**: Vulkan-first optional v1.1 track is GO; SDL3 is NO-GO; Metal/WebGPU/
  WebGL2 remain BLOCKED until their runners exist. P3.10 must add a synthetic
  device-loss test.
- **Platform runners**: macOS/Windows/WASM browser/GPU are BLOCKED on this host;
  they are not silently omitted. P0.3a CI records them as BLOCKED until runners
  exist.
