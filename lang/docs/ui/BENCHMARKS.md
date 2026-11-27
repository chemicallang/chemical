# Chemical UI — performance and validation budgets

**Status:** gate specification · **Date:** 2026-11-26
**Rule:** a packet that touches the frame path, layout, paint, text, or a
component may not be committed if it regresses a gate. The parent re-runs the
gate and inspects the JSON before committing.

---

## 1. Reference machine and build

**North star:** on every scenario we measure, be class-leading or explain the
trade-off in writing. If egui/Qt/GTK/Flutter beats us on startup, memory,
binary size, 100k-row scrolling, determinism, or agent inspectability, that is
either a regression to fix or a documented gap — never an unmentioned loss. We
do not claim a universal win; we publish the table.

All headline budgets are measured on the **reference profile**:

| Item | Value |
|------|-------|
| CPU | Reference class: Intel i5-1035G1 or equivalent 4C/8T, 16 GB. The actual CI/dev host may differ (record CPU/GPU/OS/governor in `ui-probe-env.sh`); if it differs, publish the ratio and do not compare absolute numbers across machines. |
| RAM | 16 GB |
| Display | 1920×1080, 60 Hz, scale 1.0; 120 Hz panel or synthetic 120 Hz clock for 120 fps tests; headless where available |
| OS | Linux (X11 or headless), release build |
| Renderer | CPU rasterizer for reference/goldens; GPU backend measured separately |
| Build | `--mode release_fast`, assertions off, LTO where supported |
| Fonts | goldens use a pinned test font (`lang/tests/ui/assets`); interactive runs use the system UI stack by default |

Secondary profiles: Apple M1 (macOS), Windows 11 (WARP/CPU present), and
`wasm32-unknown-unknown` in Chromium on M1. Budgets for those are listed
separately where they differ. Shared CI runners are allowed to be slower; only
deterministic counters gate there (see §5).

**Backend policy:** headline frame/startup numbers are for the **LLVM**
compiler. The **TCCCompiler** path must be fully functional and pass the same
semantic/geometry/golden suite; its gates are cold start ≤250 ms p50, binary
≤4 MB, and **no frame-p99 gate** (P0.1 measured 5.5–9.2× slower than LLVM on
tight pixel loops, not 1.5×). TCC is the fast-iteration backend; performance
claims name LLVM.

---

## 2. Hard budgets

### 2.1 Startup and footprint

| Metric | Budget | Gate |
|--------|--------|------|
| Cold start → first presented frame (GTK3 v1) | **≤220 ms p50 / ≤350 ms p99** (P0.1 measured GTK two-window init 107–210 ms; breakdown: exec/link ≤30, GTK/host ≤130, font ≤15, tree+layout ≤15, raster+present ≤15, slack ≤15) | release gate |
| Core cold start excluding GTK host (stretch target) | **≤120 ms p50**; requires a lighter X11/Wayland host (P3.2), not GTK | stretch, not a v1 gate |
| Warm start → first frame | **≤50 ms p50** | release gate |
| Empty-window PSS (Linux, 960×640 default) | **≤12 MB** (two 960×640 surfaces = 4.9 MB) | release gate |
| Empty-window PSS (1080p) | **≤24 MB** (two 8.3 MB surfaces) | release gate |
| Hello-world binary (stripped, LLVM, CPU only, no font) | **≤2.5 MB target; ≤3.5 MB hard ceiling** | release gate |
| Hello-world binary (TCC path, stripped) | **≤4.0 MB** | release gate |
| 100k-row virtualised table PSS | **≤100 MB** | release gate |
| Steady-state memory growth | **<0.5 MB/hour**; 10k open/close windows plateau | nightly |
| WASM hello bundle (brotli) | **≤350 KB** | release gate |
| WASM first frame (M1, Chromium) | **≤300 ms p50**, **≤800 ms mid-tier** | release gate |

### 2.2 Frame time (1920×1080, 1× DPI, CPU raster)

| Scenario | p50 | p99 | p99.9 / missed |
|----------|-----|-----|----------------|
| Idle (no animation) | **0 ms CPU**, 0 wakeups/s | — | no repaint |
| 1k-node dashboard, full repaint | **≤3 ms** | **≤6 ms** | **≤16.7 ms**, <0.1% missed @60 |
| 1k-node dashboard, damage repaint (hover/typing) | **≤0.8 ms** | **≤1.5 ms** | no full repaint |
| 10k-row virtual list, scrolling | **≤4 ms** | **≤8 ms** | <0.5% missed @60; p99 ≤6 ms @120 |
| 1k animated cards (transform/opacity) | **≤5 ms** | **≤8 ms** | <0.5% missed @60 |
| Text-heavy page (5k glyphs, cached) | **≤4 ms** | **≤7 ms** | — |
| Resize 1080p (interactive) | **≥60 fps** | — | no >50 ms stall |
| Resize 4K (CPU) | **≥30 fps** | — | document GPU gap |
| Theme switch (full repaint) | **≤2 frames** | — | no flash |

### 2.3 Layout, text, paint

| Metric | Budget |
|--------|--------|
| Layout, 10k simple nodes (fixed sizes, no text), cold | **≤2.5 ms target / ≤4 ms hard** (rebaseline after P0.1 probe; Taffy anchor 2.69 ms/10k on M1) |
| Layout, 10k mixed (20% text/auto/wrap), cold | **≤6 ms target / ≤9 ms hard** |
| Layout, 100k wide/shallow nodes, cold | **≤40 ms target / ≤60 ms hard** |
| Single fixed-width label change in 10k (no wrap/auto) | **≤50 µs target / ≤100 µs hard** |
| Relayouts per hover/click | **≤2 nodes + ancestors** |
| View build, 100k text+child nodes (LLVM release) | **≤8 ms p50 / ≤12 ms p95** (5 ms stretch; P1.3c 6.9 ms); TCC functional only |
| Layout v2, 10k wrap mixed (LLVM release) | **≤8 ms p50 / ≤12 ms p95** (P1.7b 7.7/10.2) |
| Layout v2, 10k grid (LLVM release) | **≤7 ms p50 / ≤11 ms p95** (P1.7b 6.1/9.3) |
| Layout v2, 100k scroll shell (LLVM release) | **≤80 ms p50 / ≤100 ms p95** (P1.7b 72.7/74.7); virtualized 100k table is P4.8 |
| Paint, 10k solid/opaque quads (≤16×8 px) | **≤1.2 ms target / ≤2.0 ms hard** (P0.1 scalar 0.96–1.9 ms) |
| Paint, 10k AA quads (≤16×8 px, ~1.5 Mpx) | **≤2.5 ms target (AVX2/span) / ≤6 ms hard (SSE2)**; scalar reference ≤15 ms (P0.1: 10.6 ms scalar, 5.1 ms SSE2) |
| Paint, 10k glyphs (8×10 px bbox, cached) | **≤2 ms**; 16×16 stamps are reference-only ≤13 ms (P0.1: 6.7 ms text-order) |
| Text shaping, 1k glyphs | **≤0.4 ms** cached, **≤4 ms** cold; 2k-char paragraph re-shape ≤2 ms |
| Text measure cache hit | **≥99%** in scroll/typing scenarios |
| Allocation in steady-state frame | **0** process-allocator calls after warm-up; cold/text/atlas high-water reported separately |
| Damage ratio on scroll | **≤1.5× newly exposed area**; report `scroll_blit_bytes`/`scroll_blit_us` separately (1080p ≤1.5 ms, 4K ≤6 ms) |

### 2.4 Input and interaction

| Metric | Budget |
|--------|--------|
| Event → update applied | **≤1 ms p99** |
| Event → presented frame (full repaint) | **≤10 ms p99** (4 ms event + 6 ms paint) @60 |
| Keystroke → caret/text on screen (damage-only) | **≤4 ms p99**; full repaint ≤10 ms |
| Scroll wheel → offset applied | **≤1 frame** |
| Focus traversal (Tab) | **≤0.5 ms** per step, 10k nodes |
| IME preedit update | **≤1 frame** |

### 2.5 Build and iteration

| Metric | Budget |
|--------|--------|
| TCC rebuild, 1k-LOC UI app | **≤300 ms** save → runnable |
| LLVM rebuild, same app | **≤2 s** |
| State restore after restart | **≤50 ms** |
| Inspector open / snapshot 10k nodes | **≤100 ms** |
| `chemical ui shot` end-to-end (headless) | **≤1.5 s** including build reuse |

### 2.6 GPU budgets (optional accelerator)

GPU measurements are separate from CPU goldens and are per backend/driver. The
CPU path remains the correctness reference.

| Metric | Budget |
|--------|--------|
| GPU device init → first frame | **≤80 ms** (cached shaders/pipelines) |
| 4K full repaint, 1k nodes | **≤4 ms p99** @60; **≤6 ms p99** @120 |
| 4K damage repaint (hover/typing) | **≤1.5 ms p99** including the full-surface blit; ≤0.2 ms if only the damage rect is blitted |
| 10k quads | **≤0.3 ms GPU** (CPU AA hard gate is ≤6 ms) |
| 100k glyphs, cached atlas | **≤1.5 ms GPU** |
| 1k shadows/blurred layers | **≤1 ms GPU** (same scene as CPU ≤2 ms) |
| GPU memory (textures/buffers) | **≤64 MB at 1080p**, **≤160 MB at 4K** (scene texture + 2 swapchain images + atlas); layer/image allocations count inside the total |
| GPU upload per steady frame | **0 bytes** unless the atlas generation changes (then upload the atlas delta, not the full texture) |
| Draw calls, dashboard | **≤64**; large table **≤256** |
| Device loss recovery | first frame after loss **≤250 ms**; CPU fallback keeps app alive |
| Partial present | GPU renders damage to a persistent scene texture then blits to the swapchain; budget the blit (4K ≤1 ms) |
| GPU idle | 0% CPU/GPU, no fences spinning |

GPU goldens use per-OS/GPU baselines with `maxDiffPixels ≤0.1%` and YIQ ≤0.2.
They never gate cross-machine equality. CI uses lavapipe (Vulkan), SwiftShader
(WebGPU/WebGL), and WARP (D3D12) for functional coverage, not for perf claims.

### 2.7 Font size budgets

No font is embedded by default. When an app opts into a pinned font:

| Asset | Uncompressed | Notes |
|-------|--------------|-------|
| Inter UI subset (ASCII + Latin-1 + punctuation, static 400) | **≈37 KB TTF** | smaller ASCII-first variant; ~17 KB gzip |
| Inter Latin subset (U+0000–00FF + punctuation, static 400) | **≈42 KB TTF** | **chosen preset**; ~20 KB gzip |
| Inter static, one weight, full charset | **≈224–342 KB** | measured instanced 400 = 342 KB |
| Inter variable, full | **≈880 KB** | measured |
| Inter variable UI subset | **≈115 KB TTF** | all weights; ~63 KB gzip |
| Liberation Sans Regular | **≈411 KB** | system-installed; license-dependent |
| Noto Sans Regular | **≈513 KB** | system-installed |
| Lato Regular | **≈657 KB** | system-installed |
| DejaVu Sans | **≈760 KB** | system-installed |
| Noto Color Emoji | **≈11 MB** | never bundle by default |
| CJK fallback (Droid Sans Fallback) | **≈4 MB** | rely on system |

Policy:

- Release binaries embed **0 bytes** of font by default.
- A pinned Latin font (42 KB) is **≈1.7%** of a 2.5 MB hello binary; allow
  it when the app wants cross-platform visual identity, but do not make it the
  default.
- Full multilingual/emoji coverage is always delegated to system fonts; do not
  bundle it.
- The compiler ships no font asset by default. `ui_fonts` downloads a
  hash-verified OFL subset into the user cache; an app may embed its own copy.
  Importing `ui` links 0 font bytes.

---

## 3. Deterministic counter gates

Wall-clock numbers are noisy on shared CI. The following counters are exact and
must not regress by more than **2%** without a parent-approved, documented
trade-off:

| Counter | `FrameStats` field |
|---------|----------------------|
| View nodes created | `nodes` |
| Retained nodes visited during reconcile | `visited` |
| Nodes relaid out | `relaid_out`, `layout_nodes` |
| Virtual-list rebuilds | `virtual_rebuilds` |
| Text runs measured / cache hits | `text_measured`, `text_cache_hits` |
| Paint commands emitted | `commands` |
| Tiles painted / full repaints | `tiles_painted`, `full_repaints` |
| Bytes touched by raster/scroll blit | `paint_bytes`, `scroll_blit_bytes` |
| Heap allocations (steady/cold) | `allocs`, `allocs_cold`, `alloc_bytes_high_water` |
| Event/update/submit/present latency | `event_to_update_us`, `update_to_submit_us`, `submit_to_present_us` |
| Frame count to settle | `settle_frames` |

`ui::FrameStats` exposes these in-process; `chemical ui bench --json` and
`perf.stats` report them.

---

## 4. Benchmark scenarios

`lang/tests/ui_bench/` is a standalone app with one scenario per subcommand.
Scene JSON schema: `{name, viewport, scale, renderer, seed, warmup_frames,
frames, node_kind, node_count, node_size, overlap, text, font_px, max_width,
row_height, overscan, velocity, digit_set, animation:{property,duration,easing,count,card_size},
gpu:{backend,scene_texture}}`. Every run echoes `scene_hash`, resolution, renderer,
backend/device/driver, CPU/OS/governor, and font hash. P0.3b commits `empty`,
`static-1k`, `list-100k`; the rest land with their packets.

1. `empty` — create/show/quit.
2. `static-1k` — 1,000 nodes, no input, first frame + 60 settled frames.
3. `static-10k` — layout + full repaint.
4. `list-100k` — virtualised table, 10 s of synthetic scrolling, fixed velocity.
5. `text-5k` — 5,000 glyphs, mixed styles, one paragraph edit per second.
6. `animate-1k` — 1,000 independent opacity/transform animations.
7. `resize` — 320×240 → 1920×1080 in 16 steps, then back.
8. `theme` — light/dark switch 20 times.
9. `input-latency` — click → state change → paint, 1,000 iterations.
10. `idle` — 60 s with no input; CPU, wakeups, RSS.
11. `startup` — cold/warm process start to first frame (hyperfine or harness).
12. `build` — TCC/LLVM compile of a fixed 1k-LOC app.
13. `wasm` — same as `static-1k` in Chromium, plus bundle size.
14. `examples-settings` — startup, RSS, form interaction latency, dialog open,
    theme switch.
15. `examples-dashboard` — 100k-row table scroll, chart repaint, filter latency,
    4K resize.
16. `examples-chat` — 10k-message virtual list, composer keystroke→paint, command
    palette open, split-pane resize.
17. `examples-media` — 10k-track playlist scroll, album grid, animated
    visualiser at 60/120 fps, image cache churn, CPU path (GPU optional).
18. `examples-dashboard-combined` — 100k scroll + chart repaint in one frame;
    p99 ≤8 ms @60 (6 ms @120), filter ≤16 ms debounced.
19. `examples-media-combined` — 10k scroll + visualiser in one frame;
    p99 ≤8 ms @60 (6 ms @120), or the visualiser pauses during scroll.

The four public example apps are benchmarked from their own `bench.sh` and
`bench/results.json`; the main repo mirrors the summary. This proves the library
on real screens, not just synthetic scenes:

| App | Proves |
|-----|--------|
| Settings | forms, validation, dialogs, search, keyboard, dark/light, 200% text, a11y |
| Dashboard | 100k virtual table, charts, resizing, filters, empty/loading/error |
| Chat | virtual list, IME composer, command palette, tabs, split panes, notifications |
| Media | images, grid, virtual playlist, context menus, drag-reorder, Canvas animation, CPU visualiser (GPU optional) |

Each scenario prints one JSON object:

```json
{
  "scenario": "list-100k",
  "profile": "reference-linux-x86_64",
  "commit": "…",
  "build": "release_fast",
  "frames": 2000,
  "runs": 5,
  "machine": {"cpu":"...", "gpu":"...", "os":"...", "governor":"performance"},
  "renderer": "cpu",
  "scene_hash": "...",
  "font_hash": "...",
  "frame_us": {"p50": 2100, "p95": 3400, "p99": 5100, "p999": 9000, "max": 12000},
  "missed_pct": 0.2,
  "counters": {"nodes": 100000, "tiles": 42, "full": 0, "commands": 830, "allocs": 0},
  "startup_ms": null,
  "rss_kb": 61200,
  "notes": ""
}
```

Baselines live in `lang/tests/ui_bench/baseline/<profile>.json` and are updated
only in a dedicated commit with the raw run attached.

---

## 5. CI gates

### PR gate (≤10 min, shared runner)

- Build TCC + LLVM.
- Unit, integration, semantic, geometry, and CPU golden tests.
- Deterministic counter comparison against baseline (fail >2%).
- `ui audit` on the component gallery and dogfood apps.
- ASAN+UBSAN on the Linux headless suite.
- No-panic audit: no `panic`/`unwrap`/`expect`/`unreachable`/`unsafe` in the
  safe surface (static search + compiler checks where available); every
  approved `PANIC:` whitelist site has a test proving the state unreachable.
- Sanitizer matrix: LLVM ASAN+UBSAN is the PR gate; C-emitted output is compiled
  with `cc -fsanitize=address,undefined` on PR; TCC itself has no sanitizer and
  must not be claimed as instrumented. TSAN runs on PR for protocol/worker
  changes.
- Idle test runs only after the host 1 ms async pump is removed/replaced; 60 s
  with no timer/rAF wakeups. Allocation counting uses a linker-wrapped allocator
  in the bench build; cold/high-water allocations are reported separately.
- Expected-failure audit: missing font/image, invalid layout, stale ID, IO,
  OOM, and protocol garbage return `Result`/placeholder, never abort.
- WASM smoke: build + run one scenario in Chromium.

### Nightly (dedicated runner)

- Wall-clock scenarios with 30 interleaved samples; fail on p99 regression
  >10% or startup >15%.
- TSAN on async/image decode paths.
- Fuzz smoke: 10k iterations per target.
- Leak/long-run: 10k window create/destroy, 1h idle.
- Cross-platform CPU goldens (Windows, macOS).

### Release gate

- Full matrix: Linux X11, Linux Wayland, Windows, macOS, WASM.
- Real-hardware frame/input numbers on the reference profile. GPU budgets are
  release gates only when the GPU track (P3.15) is in scope.
- 24h fuzz run on layout/style/event/font/image.
- Manual screen-reader pass (Orca, NVDA, VoiceOver) for the component gallery.
- Panic-whitelist audit: every `PANIC:` site justified and covered; no
  `unwrap`/`expect`/`unreachable` in library or examples.
- Published benchmark report with raw data and methodology.
- Binary size report (per target, stripped, with and without kit, with and
  without the optional font).
- Toolchain size: the steady-state `TCCCompiler` must stay under the AGENTS
  hard ceiling (5 MB; first release 7–15 MB allowed). Report the UI-related
  delta; it must not push the compiler over the ceiling.
- Public `chemicallang/ui-examples` repo: four apps, fresh-clone build on TCC+LLVM,
  reproduced benchmark JSON, screenshots, audit clean.

---

## 6. Statistics and honesty

- Use **≥2000 post-warmup frames** (or 10 s) for frame gates; report p50/p95/p99/
  p99.9/max and missed frames. Do not report p99.9 from <2000 frames; report max
  and missed count instead. `samples` in JSON means frames sampled.
- Interleave baseline and candidate runs on the same machine; use bootstrap
  confidence intervals; treat wall-clock differences under 5% as noise.
- Never compare across machines, compiler versions, refresh rates, or debug vs
  release builds without saying so.
- Input-to-photon truth needs a photodiode rig; CI can only report
  event→submit and submit→present timestamps. State that limitation.
- CPU and GPU timings are separate; a CPU golden can pass while the GPU path is
  wrong.
- A claim in the README must link to the exact scenario, commit, machine, and
  raw JSON.

---

## 7. Competitive targets (what we intend to win, and where we may not)

These are the claims the release report should test. They are **scenario
specific**; the README will not say "beats every framework".

| Opponent | Scenario | Our target | Anchor |
|----------|----------|------------|--------|
| egui / Tauri / iced | cold start to first frame | ≤120 ms vs published 230–380 ms | Kalbertodt 2023 |
| egui / iced | hello binary size | ≤2.5 MB target / 3.5 MB hard vs published 17–18 MB | same |
| GTK3 / Qt / Dear ImGui | empty-window PSS | ≤12 MB vs published 5–17 MB | Szibele 2019 |
| every toolkit in the comparison suite | bit-exact headless frame hashes on x86-64 + ARM with a pinned font | pass; most toolkits cannot do this | our harness |
| Vulkan/Metal/D3D12/WebGPU toolkits | 4K 60/120 fps with the same scene | match/beat on the GPU track; CPU fallback always works | our harness |
| Qt/GTK/Flutter | system-font UI without bundling a font | 0-byte default font payload; optional 42 KB pinned subset | measured |
| egui / iced | per-node allocation and view rebuild cost | zero per-node heap, no full relayout | our counters |
| Flutter / Qt / GTK | cold start + memory for a small app | ≤120 ms, ≤12 MB | our harness |
| Slint | dynamic runtime composition without a DSL/codegen step | full Chemical API, no generated per-element code | our API |
| web stacks | native startup/memory/a11y without a browser | ≤120 ms, ≤12 MB, AT-SPI/UIA | our harness |
| every toolkit | 100k-row virtualised scroll at 60/120 fps with bounded memory | p99 ≤8 ms @60, ≤6 ms @120 | our harness |
| every toolkit | agent can inspect/act/seek/capture without screenshots | protocol + MCP + CLI | our harness |

Where we may lose, and will say so:

- GPU-accelerated 4K/120 with heavy blur (our v1 CPU path).
- Mature widget breadth vs Qt/GTK on day one (ours is staged).
- Text/script coverage vs Qt/Flutter before the shaping work matures.
- Platform integration depth (tray, native print) in v1; notifications are v1.

Honesty about these is part of the README.

---

## 8. Comparison policy

The comparison suite (`bench/ui-compare/`, not a dependency of the library)
runs the same scenarios in egui, iced, Qt Widgets/QML, GTK4, and Flutter when
the tools are installed. It records:

- startup to first frame, RSS, binary size;
- frame p50/p99 for the same node counts and animations;
- 100k-row scroll;
- input latency proxy;
- build time (where meaningful).

Rules:

- Compare only scenarios each framework can express fairly; publish the code.
- Do not claim a universal win. Claim specific, measured wins
  ("≤120 ms cold start", "≤12 MB empty PSS", "100k rows at 60 fps", "bit-exact
  headless goldens").
- If a framework wins a scenario, say so and link the data. Credibility is the
  point.
- The README brag line is allowed only after the release gate has the numbers:
  e.g. *"Chemical UI: native 60/120 fps, 12 MB idle at 960×640 (CPU), 120 ms cold
  start, bit-exact headless rendering with a pinned font, first-class agent
  inspection."*

---

## 9. Visual validation protocol

Every packet that changes pixels must produce artifacts under
`lang/docs/ui/evidence/<packet>/`:

1. **Semantics snapshot** (JSON) for the affected nodes.
2. **Geometry snapshot** (JSON: x/y/w/h, baselines, scroll extents).
3. **Golden PNGs** at 1× and 2× for each state: default, hover, focus, pressed,
   disabled, loading, error, dark, compact, 200% text, RTL (where applicable).
4. **Overlay PNG** with layout boxes, text bounds, focus path, and hit point.
5. **A written visual assertion** from the subagent: what it inspected, what it
   expected, what it saw, and any pixel-level concern.

The subagent must open the PNGs (the harness supports images) and describe
them; the parent opens at least three per visual packet. A packet with no
artifact is incomplete even if unit tests pass.

### Golden determinism rules

- Headless, CPU rasterizer, pinned test font, fixed DPI/scale/time/locale/theme. Scene JSON files are committed under `lang/tests/ui_bench/scenes/` and define node kinds/sizes/text/overlap/font size; baselines are committed at P0.3b.
- 1× and 2× only; no GPU in the primary gate.
- Integer blend math and fixed-point coverage so x86-64 and ARM produce the
  same bytes for the same scene and pinned font.
- With the system font stack (no pinned font), goldens are per-platform with
  tolerance; they are not a cross-platform correctness gate.
- Per-OS tolerance is allowed only for platform-present screenshots and GPU
  readback, never for the CPU headless reference. Threshold: 0 differing pixels
  for the headless pinned-font reference; for present/GPU paths use ≤0.1%
  differing pixels and a YIQ ≤0.2 check.
- Golden updates require `--update-goldens` plus parent review of old/new/diff.

### Animation validation

- Capture frames at t = 0, 16, 33, 100, 250, 500 ms (and custom keyframes).
- Assert the animation curve numerically (not just pixels).
- Test interruption, reversal, reduced motion, and window occlusion.
- `capture.start` records PNG frames + timestamped input; `replay` must
  reproduce the same frame hashes.

---

## 10. Safety validation

- ASAN+UBSAN on every PR for the headless suite; TSAN nightly for any threaded
  path.
- Fuzz targets: layout tree, style values, event stream, font parser, PNG
  decoder, text editing, protocol parser. Invariants: no panic, finite
  geometry, non-negative sizes, no NaN/Inf, paint inside clip, idempotent
  layout, no unbounded allocation.
- Fault injection: allocation failure, missing font, corrupt image, stale
  `NodeId`, invalid key, NaN style, huge list. Each must produce a typed error
  or degraded UI, never a crash.
- Example apps are part of the safety gate: no `unwrap`/`expect`/`panic`/
  `unsafe`; every error path renders a real UI state; 24h idle/fuzz clean.
- Leak check: 10k window open/close and 1h idle; RSS must plateau.
- No-panic audit: static scan of `lang/libs/ui*` for `panic`, `unreachable`,
  `unwrap`/`expect`, unchecked `[i]`, and `unsafe` outside `ui::raw`; a negative
  test suite proves the scan is meaningful. Panic is process-fatal and is a
  release blocker, not a handled error.
