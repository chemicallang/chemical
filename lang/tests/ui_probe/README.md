# P0.1 UI feasibility probes

These are **probes, not library code**. They exist to replace assumptions in
`lang/docs/ui/RESEARCH.md` §7 with measured facts before P0.3a/P0.2.

Run everything:

```bash
lang/tests/ui_probe/run-probes.sh
# logs: lang/tests/ui_probe/build/logs/ (gitignored)
```

Run one probe:

```bash
cmake-build-debug/TCCCompiler lang/tests/ui_probe/foundation/chemical.mod \
    -o /tmp/foundation.exe --mode debug_quick && /tmp/foundation.exe
cmake-build-debug/Compiler    lang/tests/ui_probe/foundation/chemical.mod \
    -o /tmp/foundation_llvm.exe --mode debug_quick && /tmp/foundation_llvm.exe
```

## What each directory proves

| Path | Proves |
|------|--------|
| `foundation/` | interface with `&mut self`; generic `run` bounded on it; `Result`-returning interface; `std::function` value/non-capturing captures; `[no_panic]` chain; allocation-growth proxy; source location through a 3-level generic chain |
| `embed_lib/` + `embed/` | `intrinsics::embed_file()` and `get_raw_location()` from an imported library |
| `linkc/` | `link c "file.c"` compiles and links a C TU (extern calls are `unsafe`) |
| `image_bench/` | `image.encode_png` speed and encoded size at 1080p/4K |
| `dce/` | unused `ui_kit`-like public functions are eliminated (byte-identical stripped binaries) |
| `baseline/` | minimal headless window-like app: TCC vs LLVM compile time/size/runtime |
| `window_probe/` | real GTK3 window, two windows with independent user-data/callback routing, timer/run/close |
| `font_probe/` | system font parse/glyph/outline behavior + `fs::atomic_write` |
| `wasm_float/` | wasm32 float/double math through both backends |
| `raster/` | 1080p clear/fill, 1k/10k analytic AA quads, 5k/10k glyph blits, damage blit, PSS |
| `gpu/` | Vulkan loader + RADV/lavapipe device init + pipeline cache |
| `alloc/` | LD_PRELOAD allocator counter and forced-OOM behavior |
| `negative/no_panic/` | `[no_panic]` rejects an unannotated panicking std call (`SAFE-PANIC-001`) |
| `bugs/named_fn/` | reduced compiler bug: named function assigned to `std::function` (expected to fail) |

## Host-specific notes

- `font_probe` uses the reference host's system font paths (DejaVu, Noto,
  Noto CJK TTC, Noto Color Emoji, URW CFF). They are recorded in the evidence
  report; replace them on another host.
- `window_probe` needs a display (X11 or Wayland). Without one it prints
  `BLOCKED` and exits 0 instead of failing.
- `alloc/` is Linux/glibc only.
- The `.m` + framework probe is **BLOCKED** on this non-macOS host; the exact
  missing compiler features are in `lang/docs/ui/evidence/P0.1/BLOCKER.md`.

## Raw evidence

`lang/docs/ui/evidence/P0.1/REPORT.md` has the result table, commands, raster
numbers, GPU go/no-go and the blocker list. Raw logs are in
`lang/docs/ui/evidence/P0.1/logs/`.
