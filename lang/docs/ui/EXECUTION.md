# Chemical UI — execution plan

**Status:** executable plan · **Date:** 2026-11-26
**Read first:** [`API.md`](API.md) (contracts), [`BENCHMARKS.md`](BENCHMARKS.md)
(gates), [`../proposals/0016-native-ui.md`](../proposals/0016-native-ui.md)
(design).

This document is written for a **parent agent** that launches one **subagent per
packet**, reviews the result, commits it, and only then starts the next packet.
The loop repeats until P8.2b. The parent is the quality gate, not a passive
messenger.

> **Start condition:** the maintainer must approve
> [`../proposals/0016-native-ui.md`](../proposals/0016-native-ui.md) first.
> Until then, only P0.1 (feasibility probes; no library API changes) may run.
> P0.3a creates the branch/scripts; P0.2 freezes the contract; P0.3b builds
> the test core. No implementation packet may start before P0.3b is green.

---

## 0. Operating model

### 0.1 Branch and commits

- P0.1 probes run on `main` (read-only/spike). Long-lived branch:
  `feature/ui-v1` (created from `main` at P0.3a); P0.2 onward uses it.
- One packet = at least one commit. Large packets may have several logical
  commits, but the last commit must leave the tree green.
- Commit subject: `ui(P1.3): view builder and arena`.
- Tag every completed packet: `ui-P1.3`.
- Update [`STATUS.md`](STATUS.md) in the same commit (packet, state, commit,
  evidence path, benchmark delta).
- Merge `feature/ui-v1` → `main` only at **stage gates** (after P0.4, P1.14, P2.11,
  P3.9, P4.12, P5.9, P6.7, P7.9, P8.2b), with the full relevant suites green.

### 0.2 The loop

For each packet, the parent does exactly this:

1. **Read** the packet and its dependencies; confirm the previous packet is
   tagged and green.
2. **Launch one subagent** with the prompt template in §0.8, including the
   packet ID, required reading, and the explicit exit criteria.
3. **Wait** for the completion report. Do not start the next packet early.
4. **Audit** the work: run `./scripts/ui-verify.sh --packet <ID>`, re-run the
   packet's key commands, read the diff, open the visual artifacts, check for
   stubs/skips, compare benchmark JSON.
5. **Reject or accept.** Rejection goes back to the same subagent session with a
   precise failure list. The parent never silently fixes a failed packet unless
   the failure is in the harness itself and is documented.
6. **Commit and tag** only when every exit criterion is evidenced.
7. **Update STATUS.md** and stage the next packet.

### 0.3 Global definition of done

A packet is done only if **all** of the following are true:

- [ ] Code builds with `TCCCompiler` and `Compiler` for the packet's targets.
- [ ] CPU path is always green; GPU path, when the packet touches it, is green on
      at least one real adapter and one software adapter (lavapipe/SwiftShader/
      WARP), with CPU fallback tested.
- [ ] `./scripts/ui-test.sh --tcc --filter <packet>` passes; no disabled,
      skipped, or weakened tests.
- [ ] Relevant golden/geometry/semantic tests pass; new PNG artifacts exist and
      were inspected.
- [ ] `./scripts/ui-bench.sh --compare` stays within the gates for scenarios the
      packet touches; counters are attached.
- [ ] LLVM ASAN/UBSAN pass on the headless suite; C-emitted output is also
      compiled with `cc -fsanitize=address,undefined`. TCC is never claimed as
      instrumented. TSAN runs for protocol/worker changes.
- [ ] No `panic`, `unwrap`/`expect`, `unreachable`, unchecked indexing, or
      `unsafe` in the safe surface; `[no_panic]` annotations added where the
      current std allows; the panic whitelist is unchanged or explicitly approved.
- [ ] Expected failures (font/image/layout/IO/OOM/stale/protocol) return typed
      errors or safe placeholders; none abort.
- [ ] The debug protocol can inspect the new feature (if it is user-visible).
- [ ] Public API matches `API.md`; any intentional difference is an approved
      amendment.
- [ ] Docs/catalog entries/examples updated.
- [ ] Evidence file `lang/docs/ui/evidence/<packet>/REPORT.md` contains: files
      changed, commands + raw output summary, benchmark JSON, screenshot list,
      known limitations, and a self-review.
- [ ] No `TODO`, `FIXME`, `stub`, `not implemented` in a shipped path unless it
      is a typed `NotImplemented` explicitly listed in the packet scope.
- [ ] No new third-party runtime dependency without parent approval.

### 0.4 Anti-skip rules (non-negotiable)

The subagent must not:

- delete or weaken a test, reduce assertions, widen tolerances, or mark tests
  ignored to get green;
- edit a golden image without `--update-goldens` and an explicit parent approval
  in the report;
- change a budget in `BENCHMARKS.md` to make a packet pass;
- change a frozen API in `API.md` without an amendment approved before coding;
- replace a real implementation with a stub, a no-op, or a `NotImplemented`
  outside the packet's declared scope;
- claim visual correctness without producing and inspecting the PNGs;
- claim performance without the JSON output;
- skip a platform because it is inconvenient; if a platform is blocked, record
  the blocker and stop (see §0.6), do not silently narrow scope.

Any of these is an automatic rejection.

Adding a new `panic`/`unreachable`/`unwrap` in `lang/libs/ui*` or an example app
is also an automatic rejection unless the packet is P7.6/P8.1 and the parent has
approved a documented `PANIC:` whitelist entry with a test.

### 0.5 Evidence layout

```
lang/docs/ui/evidence/<PACKET>/
  REPORT.md            # human-readable, required
  test.log             # raw test output (tail allowed, failures must be complete)
  bench.json           # raw benchmark output
  goldens/             # new/updated PNGs (1x, 2x, states)
  overlays/            # layout/text/focus overlay PNGs
  diff/                # before/after/diff for changed goldens
  protocol.json        # debug protocol requests/responses used
```

The parent opens at least three images for every `V` packet and records what it
saw in the review checklist.

### 0.6 Blocker protocol

If a packet is blocked by the compiler, std, a platform SDK, or a missing
capability:

1. Stop; do not invent a workaround that changes the public contract.
2. Write `lang/docs/ui/evidence/<PACKET>/BLOCKER.md` with: the exact failing
   command, the minimal repro, the root cause if known, options, and the
   smallest compiler/std change that would unblock it.
3. Return the blocker to the parent. The parent decides: authorize a compiler
   packet (with tests), reorder packets, or defer the feature with a documented
   `NotImplemented`.
4. A compiler change made under a UI packet must have its own tests and must not
   regress the compiler suites; it is committed separately as
   `compiler(ui): …`.

### 0.7 Review checklist (parent)

Run and record:

```bash
git status --short
git diff --stat main...HEAD
./scripts/ui-verify.sh --packet P1.3
./scripts/ui-test.sh --tcc --filter view_builder
./scripts/ui-test.sh --llvm --sanitize=address,undefined --filter view_builder
./scripts/ui-bench.sh --scenario static-1k --compare
./scripts/ui-shot.sh --headless --theme light --scale 1 --out /tmp/p13.png
```

Then check:

- [ ] diff contains only packet scope (no drive-by refactors);
- [ ] public signatures match `API.md`;
- [ ] tests assert behavior, not just execute code (spot-check 3);
- [ ] goldens are meaningful (open at least 3; check alignment, text, focus);
- [ ] counters/bench values are attached and within gates;
- [ ] evidence REPORT is complete and honest;
- [ ] no skips/stubs/TODO in shipped paths;
- [ ] docs/catalog updated;
- [ ] commit + tag + STATUS update.

### 0.8 Subagent prompt template

```
You are implementing packet <ID> — <NAME> — for the Chemical native UI library.

Repository: /home/wakaztahir/work/Chemical/chemical
Branch: feature/ui-v1 (do not switch branches; do not commit; the parent commits)

Read before coding, in this order:
1. lang/docs/ui/API.md (frozen contracts)
2. lang/docs/ui/CONTRACTS.md (normative freeze amendments)
3. lang/docs/ui/TEXT.md / HOST.md / PROTOCOL.md / CATALOG.md / DESIGN.md
   (only those relevant to this packet)
4. lang/docs/ui/BENCHMARKS.md (gates for this packet)
5. lang/docs/ui/EXECUTION.md §<packet> and §0.3–0.6
6. lang/docs/ui/RESEARCH.md (only the sections relevant to this packet)

Scope:
<packet goal and deliverables>

Non-negotiables:
- Follow API.md exactly. Do not invent public API. If a contract is impossible,
  stop and write BLOCKER.md.
- No stubs, no skipped tests, no weakened assertions, no golden updates without
  approval.
- Produce evidence under lang/docs/ui/evidence/<ID>/ (REPORT.md, test.log,
  bench.json, goldens, protocol.json as applicable).
- Run the exact commands listed in the packet's exit criteria and paste the
  result summary into REPORT.md.
- Build both TCC and LLVM paths for the targets in scope.
- Inspect every screenshot you produce with the image viewer and describe what
  you see in REPORT.md.

Return a completion report containing:
- files changed,
- commands run + pass/fail,
- benchmark JSON summary,
- screenshot list + what you verified visually,
- public API deviations (should be none),
- open issues/blockers.
```

---

## 1. Packet map

`V` = visual artifact required. `P` = performance gate. `D` = debug-protocol
check. `S` = safety/sanitizer check. `X` = cross-platform target.

| ID | Packet | V | P | D | S | X |
|----|--------|---|---|---|---|---|
| P0.1 | Feasibility and compiler probe | | | | | |
| P0.3a | Scaffolding, scripts, manifests | | | | | |
| P0.2 | Freeze contracts v1 | | | | | |
| P0.3b | Test core, bench baselines, lint | | | | | |
| P0.4 | Design spec + exact tokens | V | | | | |
| P1.1 | Math, geometry, color, errors | | | | | |
| P1.2 | Arena, generational IDs, keys | | | | | |
| P1.3 | View builder and view arena | P | | | | |
| P1.4 | Reconciler and retained tree | P | | | | |
| P1.5 | Style, tokens, themes | V | | | | |
| P1.5b | Font metrics fixture before layout | | | | | |
| P1.6 | Layout v1 (flex/stack) | P | P | | | |
| P1.7 | Layout v2 (wrap/grid/scroll) | V | P | | | |
| P1.8 | Display list and painter | | P | | | |
| P1.9 | CPU rasterizer v1 | V | P | | S | |
| P1.10 | Headless host, app loop, PNG | V | P | D | S | |
| P1.11 | Input, focus, hit testing | V | | D | S | |
| P1.12 | Semantics tree and audit core | V | | D | S | |
| P1.13 | Typed-error boundary, fault injection, panic audit | V | | D | S | |
| P1.14 | Linux walking skeleton (real window) | V | P | D | S | X |
| P2.1 | Font provider (system-first) | | | | | |
| P2.1b | `ui_fonts` cache/download/pin | | | | | |
| P2.1c | Font determinism/fallback | | | | | |
| P2.2 | Glyph rasterizer and atlas | V | P | | S | |
| P2.3 | Shaping v1 and line breaking | V | P | | S | |
| P2.4 | Text editing core and IME hooks | V | P | D | S | |
| P2.5 | System shaper backends | V | | | S | X |
| P2.6 | Bidi and complex-script conformance | V | | | S | X |
| P2.7 | Icons and vector paths | V | | | S | |
| P2.8 | Images: decode, scale, 9-slice | V | | | S | |
| P2.9 | Gradients, shadows, opacity, transforms | V | P | | S | |
| P2.10 | Animation system and reduced motion | V | P | D | S | |
| P2.11 | Design system polish and themes | V | | D | | |
| P3.1 | Platform interface and present contract | | | D | S | X |
| P3.2 | Linux host (X11/Wayland via GTK3) | V | P | D | S | X |
| P3.3 | Windows host (Win32) | V | P | D | S | X |
| P3.4 | macOS host (Cocoa) | V | P | D | S | X |
| P3.5 | IME and text input integration | V | P | D | S | X |
| P3.6 | Multi-window, monitors, DPI, menus/dialogs | V | | D | S | X |
| P3.7 | WASM canvas host and DOM semantics | V | P | D | S | X |
| P3.8 | Native accessibility bridges | V | | D | S | X |
| P3.9 | Frame pacing, damage tiles, present optimization | | P | D | S | X |
| P3.10 | GPU backend interface/selection/resources | | P | D | S | X |
| P3.11 | Vulkan backend (Linux/Windows) | V | P | D | S | X |
| P3.12 | macOS GPU backend | V | P | D | S | X |
| P3.13 | WASM WebGPU/WebGL2 backend | V | P | D | S | X |
| P3.14 | GPU parity/device loss/effects | V | P | D | S | X |
| P3.15 | GPU benchmarks/CI/claims | V | P | | S | X |
| P4.1 | Component framework, catalog, docs generator | V | | D | | |
| P4.2 | Text/icon/surface primitives | V | | D | | |
| P4.3 | Buttons and selection controls | V | | D | | |
| P4.4 | Text input family | V | P | D | S | |
| P4.4b | Date/time/color pickers | V | | D | | |
| P4.5 | Menus, popovers, tooltips, command palette | V | | D | | |
| P4.6 | Dialogs, sheets, toasts, overlays | V | | D | | |
| P4.7 | Layout and navigation components | V | | D | | |
| P4.8 | Virtual list, table, tree (100k) | V | P | D | S | |
| P4.9 | Forms, validation, labels, errors | V | | D | | |
| P4.10 | Feedback and status components | V | | D | | |
| P4.11 | Page patterns, guidelines, templates | V | | D | | |
| P4.12 | Charts (v1) | V | P | D | S | |
| P5.1 | Debug server: discovery, tree, inspect | | | D | S | |
| P5.2 | Input, action, frame, clock, animation control | | | D | S | |
| P5.3 | Screenshot, overlay, capture, replay | V | | D | S | |
| P5.4 | `chemical ui` CLI and MCP adapter | | | D | | |
| P5.5 | In-app inspector overlay | V | | D | | |
| P5.6 | Audit, perf stats, structured logs | V | P | D | | |
| P5.7 | State snapshot and fast watch/restart | | P | D | | |
| P5.8 | Agent docs, skill, catalog, eval harness | | | | | |
| P5.9 | Compile-time `ui_lint` rules | | | | | |
| P6.1 | `ui_test` harness | V | | | S | |
| P6.2 | Golden corpus and review workflow | V | | | | |
| P6.3 | Fuzzers and sanitizer campaign | | | | S | |
| P6.4 | Bench harness, baselines, CI gates | | P | | | |
| P6.5 | Cross-platform CI matrix | V | P | D | S | X |
| P6.6 | Accessibility automation and manual checklist | V | | D | | X |
| P6.7 | Comparison benchmarks and published report | V | P | | | X |
| P7.1 | Dogfood: settings app | V | P | D | | X |
| P7.2 | Dogfood: analytics dashboard | V | P | D | | X |
| P7.3 | Dogfood: chat app | V | P | D | | X |
| P7.4 | Dogfood-driven API gap fixes | V | P | D | S | X |
| P7.5 | Performance hardening | V | P | D | | X |
| P7.6 | Safety hardening and long-run validation | | | | S | X |
| P7.7 | Docs, book, README, published claims | V | | | | |
| P7.8a | Fourth example app: Media Library | V | P | D | S | X |
| P7.8b | Example benchmarks/screenshots packaging | V | P | | | X |
| P7.9 | Publish `chemicallang/ui-examples` | V | P | | | X |
| P8.1 | Release gate audit | V | P | D | S | X |
| P8.2a | Bundles, assets, templates | V | P | D | S | X |
| P8.2b | Updates, crash reporting, final validation | V | P | D | S | X |

---

## 2. Packets

Each packet lists the **minimum** scope. A subagent may add internal helpers,
but may not expand the public API. If a packet is too large for one session,
split it into `a`/`b` sub-packets with the parent's approval; both keep the same
exit criteria split across them.

**Packet size budget:** a packet should be implementable and verified in one
session. At most **5 components** per kit packet; at most **2 platforms** per
host packet; at most **1 protocol surface** (CLI, MCP, relay) per packet. If a
packet touches >15 files or >2,000 added lines, split it before launch.

**Mandatory splits already identified:** P1.13 (error boundary / fault inject /
panic audit), P2.5a/b/c (shaper interface, Windows/macOS, Linux/fallback),
P3.2a/b (X11/GTK, Wayland), P4.2–P4.5/P4.7/P4.10 (3–5 components each),
P5.4a/b (CLI, MCP/relay), P7.8a/b (Media app, benchmark packaging). The map
above uses the unsuffixed IDs; split them at launch and keep the gate.

### Stage 0 — Decisions and guardrails

**Execution order in this stage is P0.1 → P0.3a → P0.2 → P0.3b → P0.4.**
The numeric suffixes reflect content, not order: the scaffold/`ui-verify` harness
(P0.3a) must exist before P0.2 can compile the frozen declarations, and P0.2 must
freeze the API before P0.3b builds the test core against it.

#### P0.1 — Feasibility and compiler probe

**Goal:** replace assumptions with verified facts before writing the library.

Deliver:

- `lang/docs/ui/evidence/P0.1/REPORT.md` with a table of verified facts and
  exact commands.
- Verify: `Application` interface with `&mut self` and a generic `run` bounded on `Application`;
  `std::function` capture rules; `[no_panic]` behavior on a library-like call
  chain; `intrinsics::get_raw_location()` and `embed_file()` from a library;
  `link c` with `.c` and `.m` on macOS; `window` macOS gap; wasm host ABI
  coverage; dead-code elimination for unused `ui_kit` components; `float` math on
  wasm; `image` PNG encode speed; TCC vs LLVM baseline of a minimal window.
- GPU spike: Vulkan loader/ICD (Linux/Windows), Metal (macOS), WebGPU/WebGL2
  (Chromium), lavapipe/SwiftShader/WARP; device init, pipeline cache, device
  loss. Compare native Vulkan+Metal+WebGPU vs SDL3 GPU vs OpenGL3/GLES3/WebGL2
  on binary size, dependency, startup, CI. Record a go/no-go for P3.10.
- **Raster prototype (P0 go/no-go):** implement a throwaway 1080p blend + 10k
  defined AA quads + glyph blit in Chemical/C; measure on the reference laptop.
  If the budgets in `BENCHMARKS.md` are impossible, adjust the algorithm/budget
  **now**, not at P1.9.
- **Environment:** commit `lang/tests/ui_probe/ENV.md` +
  `scripts/ui-probe-env.sh` listing required tools/packages (GTK3 dev, Xvfb,
  Mesa/llvmpipe/lavapipe, Vulkan loader, clang, Python/fonttools, Node/Playwright,
  `gh`). Record the actual host CPU/GPU/OS; the reference i5-1035G1 may not be
  present, so absolute claims wait for an equivalent runner. Missing tool → `BLOCKED`.
- **Committed probes:** move the probe to `lang/tests/ui_probe/` (not gitignored),
  with per-OS pass/fail/blocked logs. Probe: per-window GTK signal/Win32
  WndProc/Cocoa delegate user data, IME preedit/candidate, WASM present/rAF/
  pointer/IME, source locations through a 3-level generic component, allocation
  counting/OOM, `.m` + framework linking, `gh auth status` + org repo
  permission.
- Font probe: system fonts on each OS, fallback coverage (Latin/CJK/emoji),
  TTF sizes, TTC/CFF/cmap-12/composite support in `font`, runtime download/cache/
  atomic write, WASM fetch + `FontFace`/Canvas2D metrics, zero-font container.
  Confirm no font is linked by default.
- A minimal spike under `lang/compiled/ui_probe/` (not shipped) proving each
  risky capability on Linux and, where available, Windows/macOS/WASM.
- `BLOCKER.md` for anything that needs a compiler/std change, with a minimal
  repro and proposed patch size.

Acceptance: every claim in `RESEARCH.md` §7 is either confirmed or corrected;
the environment probe records the real host and tool availability; the raster
prototype has a go/no-go with numbers; `gh auth status` + org repo permission are
recorded; macOS/IME/WASM probes are pass/fail/blocked with logs; the parent signs
off on the blocker list and any deferred platform runners.

#### P0.2 — Freeze contracts v1 (executes after P0.3a)

**Goal:** make `API.md` the reviewed source of truth.

Deliver:

- Fold `CONTRACTS.md`, `TEXT.md`, `HOST.md`, `PROTOCOL.md`, `CATALOG.md`, and
  `DESIGN.md` into `API.md`; resolve every P0/P1 review finding.
- **Every public declaration compiles.** Commit snippets under
  `lang/tests/ui/contracts/` (not `lang/compiled/`); no "where possible".
- Generate `ui_api_index.json`; add a CI diff gate (fail on removed/renamed
  public symbol, changed signature/default, changed Props/slot).
- Freeze `UiError` codes/classes, key/event lifetimes, `Application`, `Options`,
  `Limits`, `TextEditState`, `ComponentMeta`, protocol schema, CLI commands.
- A `CONTRACT_TEST` list: every signature/semantic is compile- or test-gated.

Acceptance: parent approves the contract; `ui-verify --packet P0.2` compiles all
snippets and passes the API-diff gate; no code packets start before this.

#### P0.3a — Scaffolding, scripts, manifests (runs before P0.2)

**Goal:** the branch, packages, verification harness, and machine-readable gate
artifacts that P0.2 needs.

Deliver:

- Branch `feature/ui-v1`.
- `lang/libs/ui/chemical.mod`, `src/` skeleton with `API_VERSION`.
- Empty `ui_kit`, `ui_host`, `ui_native`, `ui_web`, `ui_debug`, `ui_test`,
  `ui_icons`, `ui_cli`, `ui_fonts`, `ui_gpu`, **`ui_lint`** packages with valid
  `chemical.mod` and typed `NotImplemented` entry points. GPU backend packages
  are scaffolded in P3.10–P3.13.
- `lang/tests/ui/` standalone suite + `lang/tests/ui_bench/` shell.
- `scripts/ui-test.sh`, `ui-bench.sh`, `ui-shot.sh`, `ui-verify.sh`,
  `ui-api-index.sh` with documented flags, JSON output, and exit codes.
  `ui-test.sh` grammar: `--filter <packet-tag|glob>`, `--sanitize=address,undefined`
  (LLVM only), `--emit-c --cc-sanitize` (C output), `--update-goldens` via
  `CHEMICAL_UI_UPDATE_GOLDENS=1`. `ui-bench.sh --compare` fails on >2% counter
  regression; `--scenario <name>`.
- `lang/docs/ui/packets/schema.json` + parent-authored manifests for P0.1–P1.14;
  `evidence/schema.json`; `STATUS.schema.json`; `STUBS.md` table schema.
  `ui-verify` validates evidence hashes/commit/artifacts and runs
  `--self-test` against `lang/tests/ui/fixtures/bad_packet.json` (must fail).
- `STATUS.md` + `STATUS.json` + `STUBS.md` (each scaffold `NotImplemented` maps
  to the packet that removes it).
- CI workflow `ui.yml`: Linux + macOS + Windows compile/smoke, contract-snippet
  job. If a runner is unavailable, record `BLOCKED` in P0.1; never silently skip.
- `lang/tests/ui/contracts/` harness that P0.2 fills.

Acceptance: branch/packages/scripts exist; `ui-verify --self-test` goes red on
the bad packet and green on the scaffold packet; scripts are executable and
documented. Commit/tag `ui-P0.3a`.

#### P0.3b — Test core, bench baselines, minimal lint (runs after P0.2)

- **`ui_test_core`:** headless frame/settle, semantics/geometry snapshots,
  `assert_golden` + diff artifacts, `find_all`, app access. P6.1 hardens it.
- **Bench:** `scenes/empty.json`, `scenes/static-1k.json`, `scenes/list-100k.json`
  + schema + `baseline/reference-linux-x86_64.json`; `ui-bench.sh --compare`
  works and refuses without a baseline.
- **Minimal lint:** `UI-KEY-001`, `UI-PANIC-001`, `UI-RAW-001`; P5.9 adds rules.
- A deterministic pixel fixture written through `image` (not a renderer golden;
  the first real golden is P1.9/P1.10).
- `STATUS.json` generated from STATUS.md.

Acceptance: `./scripts/ui-test.sh --llvm --filter scaffold` passes; the 3 bench
scenes run and compare; the self-test catches a missing golden; commit/tag
`ui-P0.3b`.

---

#### P0.4 — Design spec and exact tokens

**Goal:** remove design ambiguity before layout/components.

Deliver:

- `lang/docs/ui/DESIGN.md` values implemented as `theme.ch`: colors light/dark/
  high-contrast, spacing, radii, type ramp, elevation, motion, breakpoints,
  density, focus, contrast.
- 3 reference screens (settings, dashboard, chat) as static HTML/SVG
  composition specs generated from `tokens.json` (no renderer dependency);
  parent aesthetic sign-off. The first PNG goldens come with P1.9/P1.10.
- Token sheet as `tokens.json` + an HTML/SVG preview (not a renderer golden).
- A `GOLDEN-CHURN.md` rule: any token change after P1.5 lists every updated
  golden; P2.11 is polish only.

Acceptance: parent signs the design spec; P1.5/P1.6 use these exact values.

---

### Stage 1 — Core (pure, headless)

#### P1.1 — Math, geometry, color, errors

- `Point`, `Size`, `Rect`, `Insets`, `Transform2D`, `Color` (sRGB u8),
  premultiply/unpremultiply, lerp, contrast ratio, finite/clamp helpers.
- `UiError` taxonomy and `Result` helpers; no panics on NaN/Inf/negative.
- Unit/property tests for every operation; `Rect` intersection/union/contains,
  transform composition, color alpha compositing.

Acceptance: property tests with 10k random cases; ASAN; no unchecked indexing.

#### P1.2 — Arena, generational IDs, keys

- Slot arena with `NodeId { index, generation }`, free-list, generation bump,
  stale-handle `None`, reserve/clear.
- `Key` hash (FNV-1a 64), `key`, `key_of`, `key_pair`; collision handling by
  label compare; keyed map for state slots.
- Tests: 1M insert/remove random; stale/forged/generation-wrap handles never
  alias; reorder stability; injected same-hash collision; swapped `key_pair`;
  duplicate-key scope; intern cap; 10M key uniqueness.

Acceptance: fuzz 1M operations; no leaks; O(1) lookup.

#### P1.3 — View builder and view arena

- `ViewBuilder` with all §3 methods; per-frame arena; node kinds; child append;
  cycle rejection; text/span/image/icon leaves; `component` marker.
- No heap allocation per node after the arena warms; measure `view_stats`.
- Compile-only API snippet tests; structural tests (parent/child order).

Acceptance: build 100k nodes ≤5 ms (view only) on reference; zero per-node
`malloc`; V: builder debug dump (`view.tree.txt`) + a simple 3-node frame.

#### P1.4 — Reconciler and retained tree

- Retained `Node` table, state slots (hover/focus/scroll/text/animation/custom),
  keyed diff: create/update/move/remove, `Key` stability, structural path.
- Preserve state across reorder; destroy removed nodes exactly once; stale
  references rejected.
- Tests: randomized tree diffs compared to a reference model; 10k reorder;
  no leaks; duplicate key diagnostic.

Acceptance: differential test 10k random operations; state preservation proof;
P: reconcile 10k nodes ≤1 ms.

#### P1.5 — Style, tokens, themes

- `Style` value builder, `Theme.light/dark/high_contrast`, token tables, type
  scale, spacing, radii, elevation, focus ring, reduced-motion tokens.
- Resolve style → paint properties; contrast computation.
- Token audit: raw color/size detection in kit (mechanism for later lint).
- V: token sheet PNG (color swatches, type ramp, spacing, radii, elevation) with
  light/dark side by side; golden.

Acceptance: contrast ≥4.5:1 for body text in both themes; 8-pt scale; token
coverage test; V reviewed.

#### P1.5b — Font metrics fixture before layout

- Implement the `font` parser extensions needed for metrics: `OS/2`, `hhea`
  line gap, `name`, cmap 12, composite depth, `units_per_em` rejection.
- Commit `lang/tests/ui/assets/fonts.json` + a small OFL TTF + SHA-256 + license.
  Commit `metrics.json` generated by `fonttools ttLib` (exact command) for
  ascent/descent/line-gap/cap/x-height/advances; this is the oracle.
- `ui_fonts/fonts.json` and `ui_icons/icons.json` carry URL/version/SHA-256/license.
- Expose `TextContext.measure`/`TextLayoutMetrics` with ascent/descent/line gap/
  cap/x-height; no rasterization yet.
- Tests: expected metrics for the fixture, zero-font container, missing glyph,
  composite `é`, CJK/emoji coverage report.
- P1.6 baseline/auto sizing uses this; it may not stub text metrics.

Acceptance: metrics match the committed oracle; no allocation per measure.

#### P1.6 — Layout v1 (flex/stack)

- Row/column/stack; sizing (`Auto/Px/Percent/Fill`), grow/shrink/basis,
  min/max, aspect; padding/gap; align/justify; baseline; integer rounding;
  invalid input clamping.
- Geometry snapshot API; property tests against a reference model; WPT/Taffy
  fixture subset for the supported subset.
- P: 10k simple ≤2.5/4 ms; 10k mixed ≤6/9 ms; 100k ≤40/60 ms; single fixed-width label ≤50/100 µs.
- V: layout gallery with 30 cases + overlay boxes; goldens.

Acceptance: all fixtures within 0.5 px; no NaN; P gates; V reviewed.

#### P1.7 — Layout v2 (wrap/grid/scroll/layers)

- Wrap; explicit grid tracks (`Px/Fr/Auto`) and placement; scroll (measure,
  clip, offset, scrollbar geometry); layers/z-order; absolute positioning;
  `Overflow` semantics.
- Scroll state keyed by node; scroll anchoring; RTL direction flag (layout only;
  bidi text is P2.6).
- WPT grid/flex fixture subset as geometry oracle.
- V: complex layout gallery (dashboard card, settings form, data table shell);
  overlay + goldens.

Acceptance: fixtures; scroll offset preservation; P gates; V reviewed.

#### P1.8 — Display list and painter

- Primitive list (rect, rounded, border, text run, image, path, clip, layer,
  transform); deterministic ordering; batching/culling; `Painter` API; canvas
  content; display-list serialization for tests.
- A **minimal** `RenderBackend` contract + CPU adapter: display-list consumption,
  damage list, present, device-lost notification. GPU details (swapchain,
  persistent scene texture, pipeline cache, texture formats, atlas R8) are
  specified in P3.10 before any GPU code; P1.8 does not freeze an unused GPU ABI.
- Damage computation hooks (node bounds → tiles) but full repaint allowed.
- Tests: ordering, clip nesting, transform composition, culling; fuzz list
  generation.

Acceptance: serialized display list matches expected for 20 scenes; no command
outside clip; P: 10k quads command build ≤0.3 ms.

#### P1.9 — CPU rasterizer v1

- Premultiplied RGBA8; **sRGB-encoded 8-bit UNORM** blending formula from
  `CONTRACTS.md` §6 (integer `div255`, no float); solid/rounded rects, borders,
  AA (4×4 coverage), clip rect/rounded; pixel-centre/half-open convention.
- SIMD-friendly loops but scalar reference; no FMA-dependent rounding; tile is
  a scissor, coverage in absolute device coordinates.
- Fuzz: random rects/clips/radii; invariants (no OOB, alpha bounds, clip, no
  seams at tile edges).
- V: raster torture scene (overlaps, alpha, radii, thin borders) at 1x/2x;
  bit-exact goldens on x86-64 and macOS ARM (P0.3a CI job).
- P: 10k defined quads ≤1.2 ms; 1080p clear+fill ≤1 ms.

Acceptance: identical PNG hash across Linux x86-64 and macOS ARM for the
torture scene (pinned font if text); ASAN clean; V reviewed.

#### P1.10 — Headless host, app loop, PNG

- `run_headless`, `Options`, frame scheduling, virtual clock, dirty flags,
  `UiCtx`, `Application` wiring; `--ui-shot`, `--ui-size`, `--ui-scale`,
  `--ui-theme`, `--ui-fixed-time`, `--ui-frames`.
- Event coalescing/order, `EventSender`/`UserEvent`, `set_timeout`,
  `request_frame`, `EventKind.Frame`, frame loop per `CONTRACTS.md` §1.4/§9.
- Tests: 100 events → at most one view/reconcile; `on_start` before first view;
  timers never fire after close; worker post ordering; `UserEvent` text ownership.
- PNG output via `image`; screenshot hash; structured `FrameStats`.
- Example app `hello_ui`; golden test.
- D: a minimal `perf.stats`-like snapshot (protocol server comes later) exposed
  through `UiCtx.stats`.

Acceptance: `ui-shot` deterministic across runs; V: hello + a 100-node scene;
P: empty frame ≤0.05 ms CPU (excluding present).

#### P1.11 — Input, focus, hit testing, widget states

- Pointer/key/text normalization, hit testing (clip/transform/z), pointer
  capture, click/double-click synthesis, scroll wheel/touchpad, focus order,
  tab traversal, roving focus, keyboard activation, hover/active/disabled.
- Runtime widget state machine for a minimal button/checkbox/scroll container.
- Tests: synthetic event sequences; focus after reorder/removal; stale focus
  fallback; click on clipped/offscreen nodes.
- D: event log + focus chain in `FrameStats`/debug dump.
- V: focus/hover/pressed state sheet.

Acceptance: 100 event-sequence tests; no focus escape; V reviewed.

#### P1.12 — Semantics tree and audit core

- Semantics roles/states/actions/relations on nodes; derived from widget props;
  stable IDs; incremental update struct; audit for missing name/role, contrast,
  target size, focus order, relations, modal/live, secure redaction.
- `UiCtx.announce`, heading mapping, reading order/RTL, Canvas semantic overlay.
- Tests: every built-in widget has a contract; audit negative cases.
- D: semantic tree dump (JSON) and `tree.find`-equivalent internal API.
- V: semantics overlay PNG (labels/bounds/roles).

Acceptance: 100% of interactive nodes have name/role; audit catches injected
violations; V reviewed.

#### P1.13 — Typed-error boundary, fault injection, panic audit

- Per-subtree **typed-error** containment + standard error card; global error
  report; `UiError` propagation. This does not catch panics (no unwinding).
- Fault injection: OOM, bad font, corrupt image, NaN style, stale node, huge
  list, invalid key, protocol garbage.
- App-error path: `view` returns `Err`, `b.error` marks a subtree, and
  `ctx.report_error` renders a keyed card; test retry/dismiss/state preservation.
- Static no-panic audit script (grep + AST-ish checks), a
  `lang/docs/ui/PANIC_WHITELIST.md` file, `UI-PANIC-003` transitive-trap scan,
  and a negative test. Every whitelist
  site names the invariant, why continuing is unsafe, and the test that keeps it
  unreachable.
- ASAN/UBSAN full headless suite.
- V: error-card states; screenshot.

Acceptance: every injected expected fault yields an error card or degraded UI,
never a crash; the audit catches a seeded `panic` and a seeded `unwrap`; the
panic whitelist is justified.

#### P1.14 — Linux walking skeleton (real window)

- GTK3 host + real surface/present/input per `HOST.md`; open a window, render a
  rect/button/flex layout (no text/glyph path yet), click, screenshot, close.
  Text field/list/IME arrive in P2.4/P2.5/P4.8; this packet proves host/present/
  input/damage before Stage 2.
- Measure startup breakdown, PSS at 960×640, idle wakeups (remove/replace the
  1 ms pump here), input latency.
- This is the first real-window vertical slice; Stage 2 does not start until it
  is green. P3.2 hardens/extends it.
- V: real-window screenshot + headless compare; startup/RSS table.

Acceptance: app runs on X11 and Wayland; stage gate: merge to `main`.

---

### Stage 2 — Text and visual quality

#### P2.1 — Font provider and metrics (system-first)

- Font discovery: system UI stack on Windows/macOS/Linux, explicit files, and a
  test font fixture. No font is embedded or linked by default.
- `FontFamily`, `FontSpec`, `FontHandle`, `FontProvider`; metrics (units, ascent/
  descent, cap height, x-height), fallback chain, missing-glyph handling.
- System font enumeration: Windows Fonts dir + registry, macOS system/library
  dirs, Linux standard dirs + fontconfig-free scan; WASM: `document.fonts` +
  Canvas2D metrics.
- Tests: metrics for known fonts; missing/corrupt font → fallback + warning;
  assert a hello app links **0 bytes** of font by default.

Acceptance: runs on all target OSes with only system fonts; no panic; binary-size
check confirms zero embedded font.

#### P2.1b — `ui_fonts` package: cache, download, pin

- `ui_fonts` optional package: `add_file`, `add_bytes`, `fetch(url, sha256)`,
  atomic cache write, license metadata, SHA-256 verification, size limits.
- `use_inter_latin()` preset (≈42 KB TTF UI subset, OFL); download to cache.
  No font asset ships in the compiler; an app may embed its own copy.
- CLI: `chemical ui fonts add/list/remove`, `ui fonts pin`.
- WASM: `fetch` + `FontFace`/IndexedDB cache; no silent network by default.
- Tests: hash mismatch, offline cache hit, corrupt file, license gate, no
  partial writes; binary-size report with/without the preset.

Acceptance: opt-in only; no font bytes in the default app; cache safe.

#### P2.1c — Font determinism and fallback policy

- Pin semantics: `--ui-font`, `Options.font_file`, test fixture; resolved font
  hash reported by the protocol; goldens bit-exact only with a pinned font.
- System-stack mode: per-platform tolerance; layout uses measured metrics, no
  assumptions.
- Fallback chain for Latin/CJK/emoji; per-run glyph coverage report.
- Tests: same text on all platforms with/without pin; debug `fonts.list`.

Acceptance: determinism contract tested both ways; no silent fallback.

#### P2.2 — Glyph rasterizer and atlas

- TTF outline → scanline AA bitmap (fixed-point, deterministic), glyph cache
  keyed by (font, size, glyph, subpixel), atlas packing + eviction, alpha blit.
- V: glyph grid for ASCII/Latin-1/Greek/Cyrillic at 11/13/16/24/32 px; zoomed
  crop; goldens.
- P: 1k glyphs ≤0.4 ms cached; atlas 5k glyphs ≤8 MB at 1x, ≤32 MB at 2x (or fewer bins); 4 MB pages; eviction/repack only.

Acceptance: bit-exact goldens; no seams; V reviewed.

#### P2.3 — Shaping v1 and line breaking

- Latin/Greek/Cyrillic shaping: cmap, hmtx, kern/GPOS pair subset, standard
  ligatures, grapheme clusters, UAX #14 subset, ellipsis, tabs, letter spacing.
- Cache shaped runs by (bytes, style, max width); re-shape only dirty
  paragraphs.
- V: typography sheet (headings/body/caption/code, wrapping, ellipsis, tabs);
  goldens.

Acceptance: reference strings match expected advances within 1/64 px; line
breaks match the conformance corpus; P gate.

#### P2.4 — Text editing core and IME hooks

- Caret, selection, word/line/document navigation, home/end, shift-selection,
  double/triple click, undo/redo, clipboard, password masking, read-only,
  validation hook, horizontal scroll, IME preedit/commit hooks.
- Tests: 200 editing sequences; grapheme-safe deletion; selection after
  reorder; IME mock.
- V: input states (focus, selection, error, disabled, password, 200% text).

Acceptance: model-based editing test; no panic on arbitrary text; V reviewed.

#### P2.5a — TextShaper interface + platform bridge contract
- `ShapedGlyph { face, glyph_id, cluster, advance_q, offset_x_q, offset_y_q }`;
  platform must expose font file+face or a bitmap fallback.
- Cache/incremental shaping contract; no per-run allocations.
#### P2.5b — DirectWrite + CoreText backends
- Windows/macOS shape + metrics + fallback; C/ObjC shims; tests.
#### P2.5c — HarfBuzz (Linux) + portable fallback
- Dependency decision in P0.1; link only if approved; portable Latin fallback.
- If a backend is absent, tofu + warning, never a crash.
- Glyph atlas rasterizes the returned IDs from the same font file; metrics
  consistent with layout.
- Tests: Arabic, Devanagari, Thai, Hebrew (RTL), emoji fallback; platform
  goldens with tolerance.
- D: `fonts.list`, `text.runs` inspection.

Acceptance: complex scripts render connected/correct on each desktop; fallback
chain tested; no panic on missing fonts.

#### P2.6 — Bidi and complex-script conformance

- Bidi paragraph/run resolution, RTL layout mirroring, caret movement in bidi,
  selection, line breaking for CJK/Thai, cluster hit testing.
- Conformance corpus (selected UAX #9/#14 cases); visual goldens.
- V: RTL settings page, mixed Arabic/Latin paragraph, CJK wrapping.

Acceptance: conformance corpus pass; V reviewed.

#### P2.7 — Icons and vector paths

- `Path` command list, stroker/filler, SVG-ish path parser for the curated icon
  set only; `ui_icons` catalog (Lucide-compatible, ISC); size/stroke tokens.
- Tests: every icon parses and renders; no NaN; outline vs fill.
- V: icon sheet at 16/20/24 px, light/dark; goldens.

Acceptance: 100% of shipped icons render; V reviewed.

#### P2.8 — Images: decode, scale, 9-slice

- `ImageSource` (bytes/path/handle), PNG core + JPEG v1 (`ui_images`), **dimension/pixel/
  encoded/decoded caps** (`Limits`), checked arithmetic, `try_resize`, bilinear/
  nearest scale, 9-slice, fit modes, async load + cancellation, cache LRU.
- Decode never occurs in `view`; `ImageHandle`/`ImageReady`/`ImageFailed` per
  `CONTRACTS.md` §10. Differential tests vs libpng; decompression-bomb tests.
- Tests: corrupt/oversized images; EXIF? (no); cache eviction; no leaks.
- V: image gallery (contain/cover/fill/9-slice, 1x/2x); goldens.

Acceptance: no panic on fuzz corpus; P: decode 1MP PNG ≤10 ms; V reviewed.

#### P2.9 — Gradients, shadows, opacity, transforms

- Linear/radial gradients, box/gaussian shadows (cached 9-slice sprites),
  opacity layers, 2D transforms (translate/scale/rotate), rounded clipping,
  backdrop scrim.
- Deterministic AA; layer caching; damage bounds.
- V: elevation/gradient/transform gallery; goldens; P: 1k shadows ≤2 ms.

Acceptance: no seams/rings; P gate; V reviewed.

#### P2.10 — Animation system and reduced motion

- Virtual clock, easing tokens, spring, implicit animations keyed by node+
  property, interruption/reversal, enter/exit transitions, reduced motion.
- D: animation list/seek/rate in the internal debug API (protocol in P5).
- V: frame sequences for 3 animations (t=0/16/33/100/250/500 ms) + overlay of
  current values.

Acceptance: numeric curve tests; deterministic frame hashes; V reviewed.

#### P2.11 — Design system polish and themes

- Final type ramp, spacing, radii, elevation, focus ring, disabled/hover
  states, density (comfortable/compact), high-contrast, reduced-motion behavior.
- Compare against a written visual spec (spacing/contrast/alignment).
- V: full token sheet + 3 reference screens in light/dark/compact/high-contrast;
  parent aesthetic review.

Acceptance: parent signs the visual spec; stage gate: merge to `main`.

---

### Stage 3 — Platforms

#### P3.1 — Platform interface and present contract

- Implement `HOST.md` exactly: `SurfaceDesc/Format`, `Presenter.acquire/present/
  resize/request_frame`, double buffering, damage, logical↔physical boundary,
  `HostCaps`, close/shutdown, `!Send` UI state, worker/protocol queues, pointer/
  touch/drag/file drops.
- Headless implements all interfaces; contract tests run on headless.
- D: host info (`backend`, scale, monitors, vsync).

Acceptance: headless host passes the full suite; no OS type leaks into core.

#### P3.2a — Linux X11/GTK3 host hardening

- GtkDrawingArea + cairo image surface present, GDK input, clipboard, cursor,
  timers, DPI, focus, close; XShm path if measured better; Xvfb tests.
- No GTK widgets in the render path; per-window session, no global `window_run`.
- V: real-window screenshot + headless compare; startup/RSS.

#### P3.2b — Wayland-native host

- `wl_shm` buffer/damage/frame callback, xdg-shell, libinput/IME bridge; Weston
  headless tests; fractional scaling. If not ready, document a `HostCaps`
  fallback to X11/GTK3, not a silent fake.

Acceptance: both hosts pass the same scene; P gates; V reviewed.

#### P3.3 — Windows host (Win32)

- Win32 window, DIB present, WM_INPUT/pointer, wheel, DPI awareness, clipboard,
  timers, focus; Wine smoke if available; native CI when available.
- V: real-window screenshot + headless compare.

Acceptance: builds/runs on Windows; P gates; V reviewed.

#### P3.4 — macOS host (Cocoa)

- `link c` Objective-C shim: NSWindow/NSView, layer contents/CGImage present,
  NSEvent input, trackpad, DPI, clipboard, timers, focus; menu bar basics.
- Add a `window` macOS backend if that is the cleaner path; test with a
  standalone probe first.
- V: real-window screenshot + headless compare; startup/RSS.

Acceptance: builds/runs on macOS ARM + x86-64; P gates; V reviewed.

#### P3.5 — IME and text input integration

- Preedit/commit/cancel, candidate window positioning, composition underline,
  dead keys, keyboard layouts, key repeat, text input vs key events.
- Mock IME tests + manual check on each OS (ibus/fcitx, Windows IME, macOS
  Japanese/Chinese).
- V: composition states screenshot + caret/candidate rect overlay.

Acceptance: no lost/duplicated characters in a 1k-event fuzz; V reviewed.

#### P3.6 — Multi-window, monitors, DPI, menus/dialogs

- Multiple windows (shared atlases/theme), per-window tree/damage/clock,
  monitor enumeration, per-monitor DPI changes, window state (min/max/full),
  native menus/commands, file dialogs, message boxes, notifications. Tray is
  v1.1 and must not block the v1 gate.
- V: two-window scene, moved across simulated DPI scales; menu screenshot.

Acceptance: no cross-window focus leaks; DPI change re-lays out correctly.

#### P3.7 — WASM canvas host and DOM semantics

- Implement `HOST.md` §7 imports: canvas create/resize/present damaged tiles,
  rAF, pointer/key/wheel/text/IME, clipboard, DPI, visibility, semantics DOM,
  font fetch. Present only damaged tiles; full-frame copy is a fallback.
- Choose the WASM text backend (`TEXT.md` §5.1): BrowserCanvas default
  (tolerance-only) or PinnedFont for deterministic demos; wait for font-ready
  before first layout; never silently reflow.
- Debug transport: `globalThis.__chemicalUi.call(method, params)` + optional
  WebSocket relay to `chemical ui relay` (no listening socket in the browser).
- Playwright E2E: hello, form, scroll, IME mock, screenshot compare; bundle size.
- V: Chromium screenshot + headless golden compare (tolerance); P: ≤350 KB
  brotli hello, first frame ≤300 ms.

Acceptance: runs in Chromium/Firefox/WebKit where supported; a11y tree visible
to Playwright's ARIA snapshot; V reviewed.

#### P3.8 — Native accessibility bridges

- Linux AT-SPI (D-Bus) via `libatspi` shim; Windows UIA via COM shim; macOS
  NSAccessibility via Objective-C shim. Incremental tree updates; actions.
- Smoke tests: Accerciser/pyatspi, Inspect.exe/UIA, Accessibility Inspector.
- V: a11y tree dump + screen-reader manual checklist.

Acceptance: the component gallery is navigable and announced by Orca/NVDA/
VoiceOver; no focus/action mismatch; V reviewed.

#### P3.9 — Frame pacing, damage tiles, present optimization

- Tile damage (64×64), dirty propagation, scroll blit + expose strips, layer
  caching, vsync alignment, coalescing, occlusion, idle suspension.
- P: damage ≤1.5× exposed area; idle 0 wakeups; p99 gates; no full repaint on
  hover/typing.
- V: damage heatmap screenshots.

Acceptance: P gates on all desktop targets; stage gate: merge to `main`.

---

### Stage 3G — GPU acceleration (optional track, v1.1 by default)

The GPU track is **parallel to the kit**, not a dependency for v1 correctness.
The CPU backend always remains the reference and the fallback. If the maintainer
wants GPU in v1, move P3.10–P3.15 before Stage 4 and make P3.15 a stage gate.

#### P3.10 — GPU backend interface, selection, resources

- `RenderBackend` GPU implementation: device/queue creation, swapchain/present,
  buffers, textures, pipelines, samplers, fences, descriptor/pipeline layout,
  device-loss notification.
- Resource caches: glyph atlas upload, image textures, shadow/layer textures,
  pipeline cache; no re-upload of clean resources.
- Selection policy (`auto/cpu/gpu`, backend/device preference), init failure,
  device loss ×2 → CPU fallback, visible error event.
- D: `gpu.info`, `gpu.set`, `perf.stats` GPU fields.
- Tests: headless software adapter (lavapipe/SwiftShader/WARP), forced CPU,
  forced GPU, simulated device loss.

Acceptance: app renders identically enough to CPU (tolerance) on a software
adapter; fallback never crashes; startup ≤80 ms.

#### P3.11 — Vulkan backend (Linux + Windows)

- Vulkan 1.1/1.2 (conservative feature set), SPIR-V shaders committed to the
  repo, pipeline cache, validation layers in debug, lavapipe CI.
- Instanced quads/rounded rects, glyph atlas, image textures, clips/scissor,
  layers/opacity, shadows (SDF or cached 9-slice textures).
- V: GPU vs CPU golden diff (tolerance) for the component gallery; 4K capture.

Acceptance: runs on real NVIDIA/AMD/Intel + lavapipe; P3.15 budgets; no
device-loss crash.

#### P3.12 — macOS GPU backend (Metal native or MoltenVK)

- Choose in P0.1: native Metal via Objective-C shim (preferred) or MoltenVK if
  the dependency/size is acceptable.
- Present via CAMetalLayer; resize/DPI; device loss.
- V: macOS GPU vs CPU diff; startup/memory.

Acceptance: builds/runs on Apple Silicon + Intel; P3.15 budgets; no extra
runtime dependency if Metal native.

#### P3.13 — WASM GPU backend (WebGPU, WebGL2 fallback)

- WebGPU device/queue/swapchain via host imports; WGSL shaders; fallback to
  WebGL2/Canvas2D if unavailable.
- Text: glyph atlas generated by the browser (OffscreenCanvas/FontFace) or
  Canvas2D text fallback; document the difference.
- V: Chromium/Firefox/Safari capture; bundle size budget.
- P: first GPU frame ≤500 ms; 4K WebGPU frame budgets.

Acceptance: works on a WebGPU-capable browser; graceful CPU/Canvas fallback;
no security regressions.

#### P3.14 — GPU parity, device loss, and effects

- Full primitive parity: rounded clips, gradients, shadows, opacity layers,
  transforms, images, paths, text, damage/scissor.
- Device loss recovery (recreate all resources), adapter removal, out-of-memory
  texture, shader/pipeline failure → CPU fallback.
- V: side-by-side CPU/GPU gallery with diff overlays; effects gallery.

Acceptance: no GPU-only visual feature; every primitive has a CPU and GPU path;
parity within tolerance.

#### P3.15 — GPU benchmarks, CI, and claims

- GPU scenarios (4K 60/120, 10k quads, 100k glyphs, 1k shadows, 100k-row table),
  per-backend/per-driver baselines, lavapipe/SwiftShader/WARP functional CI,
  real-GPU nightly.
- Publish CPU vs GPU table; do not gate cross-machine GPU equality.
- V: benchmark charts + captures.

Acceptance: budgets met on the reference GPU; CPU fallback still green; honest
report.

---

### Stage 4 — Widget kit

Every component packet must deliver: `Props` + constructor + `ComponentMeta`,
two examples, semantic/geometry/golden tests for all states (default, hover,
focus, pressed, disabled, loading, error, dark, compact, 200% text, RTL where
applicable), keyboard contract, and a catalog entry.

#### P4.1 — Component framework, catalog, docs generator

- `ComponentMeta`, prop schema JSON, slots, examples, a11y contract, docs
  generator (`ui catalog --json`), `ui_api_index.json`, `llms.txt`, and
  `.agents/skills/ui/SKILL.md` generated from the catalog (drift-checked).
- Ship the minimal lint from `CATALOG.md` §6 here: `UI-KEY-001`, `UI-LIST-001`,
  `UI-RAW-001`, `UI-A11Y-001/002`, `UI-PANIC-001/002`, `UI-ALLOC-001`.
- V: generated catalog page for 3 sample components + state matrix; goldens.

#### P4.2 — Text/icon/surface primitives

- Text, Heading, Icon, Image, Divider, Spacer, Badge, Avatar, Card, Surface,
  Skeleton (static), Label.
- V: primitive gallery (all states/themes); goldens.

#### P4.3 — Buttons and selection controls

- Button (primary/secondary/ghost/danger, sizes, icon, loading, disabled),
  IconButton, Toggle, Checkbox, Radio, RadioGroup, Switch, Slider, Segmented
  Control, ToggleGroup.
- Keyboard/focus/disabled contracts; V: state matrix.

#### P4.4 — Text input family

- TextInput, TextArea, SearchField, PasswordField, NumberField, with labels,
  hints, errors, prefixes/suffixes, clear button, character count.
- Uses P2.4 editing core; IME; V: form matrix.

#### P4.4b — Date/time and color pickers

- Calendar, DatePicker, DateRangePicker, TimePicker, ColorPicker (tokens +
  custom brand color), keyboard/a11y, validation, open/selected/invalid states.
  v1 time zones are UTC/fixed offsets (the in-tree `datetime` has no tzdb/DST);
  a Settings/Dashboard "time zone" control is v1.1 unless tzdb lands.
- V: Dashboard date range + Settings timezone/color states; goldens.

Acceptance: Dashboard can express its date range without hand-rolling a
calendar; audit/keyboard tests pass.

#### P4.5 — Menus, popovers, tooltips, command palette

- Popover positioning (flip/shift/collision), DropdownMenu, ContextMenu,
  MenuBar, Tooltip, CommandPalette with filter/keyboard, focus trap/restore.
- V: open/closed/overflow positioning; goldens.

#### P4.6 — Dialogs, sheets, toasts, overlays

- Dialog, AlertDialog, Sheet (side/bottom), Toast queue, Scrim, overlay stack,
  escape/click-outside, focus trap, inert background, reduced motion.
- V: modal/non-modal/stacked; goldens.

#### P4.7 — Layout and navigation components

- Grid, Wrap, SplitPane (drag/keyboard), Sidebar (collapsible), Page/Shell,
  Tabs, Navbar, Breadcrumb, Pagination, Toolbar, StatusBar.
- V: responsive layouts at 640/1024/1440; goldens.

#### P4.8 — Virtual list, table, tree (100k)

- `VirtualList` fixed/variable heights, overscan, scroll anchoring, keyboard
  nav, selection; `Table`/`DataGrid` sort/filter/select/resize/column pin;
  `Tree` expand/collapse/virtualized.
- P: 100k rows scroll p99 ≤8 ms; memory ≤100 MB; no per-row view allocation
  for offscreen rows.
- V: 100k-row table + scroll capture; goldens.

#### P4.9 — Forms, validation, labels, errors

- Form/Field composition, required/optional, validation states, help text,
  error summary, submit/cancel, disabled/read-only, autofocus, a11y wiring.
- V: complete settings form; goldens.

#### P4.10 — Feedback and status components

- Alert, Callout, ProgressBar (determinate/indeterminate), Spinner, Skeleton
  (animated), EmptyState, ErrorState, Stat, Kbd, Tooltip (if not in P4.5).
- V: state gallery; goldens.

#### P4.11 — Page patterns, guidelines, templates

- Patterns: dashboard shell, settings, auth/login, chat, master-detail, data
  table page, empty/loading/error states.
- `lang/docs/ui/GUIDELINES.md`: spacing, type, color, hierarchy, density,
  responsive rules, motion, a11y, do/don't with screenshots.
- V: every pattern in light/dark/compact/200%; parent aesthetic review.

Acceptance: patterns compile and pass interaction tests.

#### P4.12 — Charts (v1, required by Dashboard)

- `ui_charts`: line, bar, area, scatter, pie/donut, heatmap on Canvas; axes,
  ticks, legend, tooltip, selection, empty/loading/error states; a11y table
  fallback; theme tokens; keyboard focus.
- V: chart gallery + dashboard integration; P: 10k-point chart p99 ≤4 ms alone; combined scroll+chart ≤8 ms @60.
  audit clean.

Acceptance: Dashboard can use charts without hand-drawing; stage gate.

---

### Stage 5 — Agent and debug tooling

#### P5.1 — Debug server: discovery, tree, inspect

- TCP NDJSON JSON-RPC, token auth, discovery file, log ring, `hello`,
  `app.info`, `tree.snapshot`, `tree.find`, `node.inspect`, `node.highlight`,
  source locations, component stack.
- Transport abstraction shared by native TCP, WASM DOM bridge, and WebSocket
  relay; one JSON schema (`protocol.schema.json`).
- D: self-test client; protocol conformance tests.
- P: snapshot 10k nodes ≤100 ms.

#### P5.2 — Input, action, frame, clock, animation control

- `input.*`, `action.invoke`, `clock.*`, `frame.step`, `animation.*`,
  `wait.idle`; deterministic stepping; no re-entrancy.
- D: tests that inject events and assert state/semantics.

#### P5.3 — Screenshot, overlay, capture, replay

- `screenshot` (PNG base64 + hash), `overlay.set`, `capture.start/stop`
  (frames + input), replay with frame-hash equality.
- V: overlay screenshots (boxes/layout/text/focus/damage/fps); capture dir.

#### P5.4 — `chemical ui` CLI and MCP adapter

- P5.4a: self-built `ui_cli` (TCC, cached) + tiny compiler launcher; commands/
  flags/exit codes from `PROTOCOL.md` §3; schema-generated client.
- P5.4b: MCP stdio adapter + WASM relay/DOM bridge; high-level tools; discovery/
  pid/window selection; diff.
- D: end-to-end: launch, snapshot, click, type, IME, screenshot, audit,
  `wait.idle`, `state.snapshot/restore`.

#### P5.5 — In-app inspector overlay

- F12/inspector: tree, props, layout boxes, tokens, focus, event log, fps graph;
  pick node by pointer; highlight; copy key/test id; toggle via protocol.
- V: inspector screenshots.

#### P5.6 — Audit, perf stats, structured logs

- `audit.run` with stable rule IDs and fixes; `perf.stats` ring buffer; `log.tail`;
  `events.subscribe`; JSON schema.
- V: audit report on a deliberately bad screen (contrast, tap target, missing
  name, overflow, raw token).

#### P5.7 — State snapshot and fast watch/restart

- Tree state snapshot/restore (focus/scroll/text/theme/window); opt-in app state
  hooks; `chemical ui run --watch` rebuild/restart with restore; measure.
- P: restore ≤50 ms; watch rebuild ≤300 ms TCC.

#### P5.8 — Agent docs, skill, catalog, eval harness

- `.agents/skills/ui/SKILL.md`, `lang/libs/ui/AGENTS.md`, `llms.txt` generated
  from the catalog, per-component docs (props, examples, a11y, do/don't).
- Eval harness: 12 prompts, headless validation (compile, semantic tests,
  audit, screenshots), rubric; baseline scores.
- V: eval report with screenshots.

#### P5.9 — `ui_lint` hardening (rules beyond P4.1)

- P4.1 ships the minimal rules. This packet adds CBI diagnostics: missing keyed
  list items, icon-only without label, raw token/color in kit props, blocking IO
  in `view` (if effects allow), unsafe in UI code, huge static list suggestion,
  duplicate keys in a literal list.
- Extend `UI-PANIC-001` (`unwrap`/`expect`/`panic`/`unreachable`) and
  `UI-PANIC-002` (`PANIC:` justification) to the full AST checks; tests/examples
  are handled separately.
- `UI-PANIC-003` scans `ui*` **and its transitive UI dependencies** (`font`,
  `image`, `window`) for `unreachable`/trap sites; each must be replaced or
  whitelisted with a proof. `[no_panic]` does not reject traps today.
- Each rule: positive/negative tests, error message with a fix, docs.
- D: lint runs in the normal build; `--ui-lint=off` escape for migration.

Acceptance: lint catches the seeded bad app; no false positive on the gallery;
stage gate.

---

### Stage 6 — Validation infrastructure

#### P6.1 — `ui_test` hardening
- P0.3b ships the core; this packet adds `wait_until`, state/async helpers, IME/
  drag/touch, `assert_semantics`, `assert_no_overflow`, scoped queries.
- Tests: harness self-tests (settle bounds, stale query, input routing).

#### P6.2 — Golden corpus expansion/review workflow
- P0.3b ships the golden core; this packet expands to every component state/
  theme/DPI/scale, independent ComponentSpec/PageSpec assertions, diff review,
  CI artifact upload, and the reference-spec gallery.
- V: corpus index page with thumbnails.

#### P6.3 — Fuzzers and sanitizer campaign

- Layout/style/event/font/image/protocol **and** window/timer lifecycle, key
  generation/collision, text editing/IME, atlas eviction, state.restore, path
  confinement fuzzers; invariant checks; corpora; 24h run; fix all; regression
  tests; child-process OOM/fatal probes.

#### P6.4 — Bench harness, baselines, CI gates

- `ui_bench` scenarios; JSON schema; baseline per profile; counter gate;
  wall-clock nightly; GPU scenarios (P3.15); binary/WASM/font-size report;
  report generator.

#### P6.5 — Cross-platform CI matrix

- Linux Xvfb + llvmpipe, Wayland headless, Windows, macOS, WASM Playwright;
  GPU functional CI on lavapipe (Vulkan), SwiftShader (WebGPU/WebGL), WARP
  (D3D12); artifacts; dashboard; flake quarantine policy (no silent skips).

#### P6.6 — Accessibility automation and manual checklist

- Rule engine + platform smoke tests; keyboard-only suite; screen-reader
  checklist; 200% text; reduced motion; contrast; target size.

#### P6.7 — Comparison benchmarks and published report

- `bench/ui-compare/` for egui/iced/Qt/GTK/Flutter when installed; same
  scenarios; raw data; honest report; README claims policy.
- V: charts + report.

---

### Stage 7 — Examples, dogfood, and hardening

The four example apps are written **at the end**, by AI agents, using only the
mature public library — not before the core/kit exists. They are the usability
gate and the public proof, not a demo scaffold.
#### P7.1 — Settings app

- Real app in `lang/compiled/ui_settings/` with navigation, forms, toggles,
  sliders, dialogs, search, keyboard shortcuts, light/dark, 200% text; built with
  the public first-party packages (`ui`, `ui_kit`, `ui_host`, `ui_fonts`,
  `ui_images`, `fs`, `datetime`). No `ui::raw` chrome.
- Full semantic/interaction/golden tests; audit clean; perf budget.
- P: startup/RSS/frame p50/p99/binary size; commit `bench.json`; LLVM benchmark.
- V: screen tour (10 screenshots); this app is published in P7.9.

#### P7.2 — Analytics dashboard

- Real app in `lang/compiled/ui_dashboard/`: virtualized table, charts on
  Canvas, filters, date range, resizing, empty/loading/error states, export;
  100k rows.
- P: startup/RSS/100k scroll/chart frame p99/binary; commit `bench.json`.
- V: dashboard tour; P gates; this app is published in P7.9.

#### P7.3 — Chat app

- Real app in `lang/compiled/ui_chat/`: virtualized message/file list, composer
  with IME, command palette, keyboard shortcuts, tabs, split panes, animations,
  notifications. **No code editor** (v1.1); sample data avoids color emoji (v1
  tofu + warning) or uses PNG emoji assets.
- P: startup/RSS/10k scroll/keystroke latency/frame p99/binary; commit `bench.json`.
- V: interaction capture; P gates; this app is published in P7.9.

#### P7.4 — Dogfood-driven API gap fixes

- Only gaps proven by the four apps; each fix updates `API.md` (additive if
  possible), tests, and docs. No speculative additions.

#### P7.5 — Performance hardening

- Profile the four apps on CPU and GPU; optimize hot paths; meet every P gate;
  publish before/after; no budget changes.

#### P7.6 — Safety hardening and long-run validation

- 24h fuzz, ASAN/UBSAN/TSAN, leak/long-run, fault injection, no-panic audit;
  review `PANIC_WHITELIST.md` line by line; fix all; release checklist.

#### P7.7 — Docs framework, book, getting started

- Getting started, architecture, component gallery index, cookbook, debugging
  guide, agent guide, migration/versioning framework. The final README includes
  the fourth app after P7.8; P7.9 updates the docs.
- V: docs site screenshots; README numbers verified at P7.9.

#### P7.8a — Fourth example app: Media Library

- Real app in `lang/compiled/ui_media/`: a visually rich fourth app with
  album/artist grid, virtualised playlist (10k
  tracks), search/filter, context menu, drag-to-reorder, now-playing panel with
  an animated Canvas visualiser (CPU), keyboard shortcuts, light/dark, 200% text,
  empty/loading/error states, image loading/caching.
- Exercises images, grids, virtualisation, menus, animation, Canvas, CPU visualiser (GPU optional), focus/keyboard, and performance under continuous animation.
- No actual audio playback is required (it is a UI/benchmark app); a simulated
  transport is fine.
- Tests: semantic, interaction, audit, goldens, 10k-track scroll, 60/120 fps
  visualiser; no `unwrap`/`panic`/`unsafe`.
- P: 10k tracks p99 ≤8 ms; animation p99 ≤8 ms; startup/RSS budgets; LLVM
  benchmark.
- V: light/dark/compact/200% screenshots + animation capture.

Acceptance: app is beautiful, audit-clean, both backends build, benchmarks
recorded; parent reviews the screen tour.

#### P7.8b — Benchmark and screenshot packaging for all four apps

- Collect `bench/results.json`, machine specs, screenshots (light/dark/compact/
  200%), and README text for each app; verify CPU path (GPU optional).
- Ensure no app depends on a deferred feature; charts use P4.12, media
  visualiser uses Canvas on CPU, GPU is optional.
- Evidence: per-app benchmarks + screenshot gallery.

#### P7.9 — Publish `chemicallang/ui-examples` (private first, public at P8.1)

- Create a public repo at **`github.com/chemicallang/ui-examples`** containing
  the four example apps (**settings, dashboard, chat, media**), each in its own
  directory with source, assets, screenshots, tests/bench scripts, and a
  `README.md`.
- Layout:
  ```
  ui-examples/
    README.md            # what each app demonstrates, gallery, quick start
    BENCHMARKS.md        # summary table + machine specs + raw JSON links
    LICENSE              # MIT
    settings/  dashboard/  chat/  media/
      chemical.mod  src/  README.md  screenshots/  bench.sh  bench/results.json
  ```
- Root `README.md`: what each app demonstrates, screenshot gallery, quick start,
  prerequisites, build/run for **both** TCC and LLVM (recommend LLVM for
  benchmarking), benchmark summary table, link to the main Chemical repo, MIT
  license, and a note that the apps were generated/reviewed by AI agents.
- Per-app `README.md`: features, screenshots, architecture notes, build/run
  commands for both backends, `--ui-renderer=cpu|gpu`, benchmark table with
  machine specs and raw `bench/results.json`, and the library features it
  exercises. At minimum:
  ```bash
  # TinyCC (fast build; functional check)
  cmake-build-debug/TCCCompiler settings/chemical.mod \
      -o settings/settings-tcc --mode release_fast
  ./settings/settings-tcc

  # LLVM (recommended for benchmarks)
  cmake-build-debug/Compiler settings/chemical.mod \
      -o settings/settings --mode release_fast
  ./settings/settings --ui-renderer=cpu
  ./settings/settings --ui-renderer=gpu   # if a GPU backend is available
  ```
  GPU builds import the platform backend package (`ui_gpu_vulkan`,
  `ui_gpu_metal`, or `ui_gpu_webgpu`); CPU-only builds import none and link
  zero GPU bytes.
- `BENCHMARKS.md` + `bench/` raw JSON: startup, RSS, binary size, frame
  p50/p99, 100k-row/10k-track scroll, animation; CPU and GPU where available;
  LLVM build named as the benchmark backend; TCC results included for
  completeness. Include a "vs budget" column and, only when measured on the same
  machine, a reference comparison (egui/GTK/Qt/Flutter); never copy numbers.
  Each app exposes `./bench.sh --backend llvm --renderer cpu --runs 30` (and
  `gpu` where available) writing `bench/results.json`.
- `LICENSE` (MIT) at the root; no copied proprietary fonts/assets; screenshots
  and generated code are original. Examples may use system fonts or include the
  OFL 42 KB Inter subset with its license; never Segoe UI/SF Pro files.
- Optional GitHub Actions: build both backends on Linux (using a released
  compiler artifact or a documented setup), run tests/audit, upload screenshots;
  do not fail the repo if the compiler artifact is unavailable — document it.
- Push with the GitHub CLI. The implementing agent must run:
  ```bash
  gh auth status
  git init -b main
  git add .
  git commit -m "Initial Chemical UI examples"
  gh repo create chemicallang/ui-examples --private --source=. --remote=origin --push
  # P8.1 makes the repo public after the release audit:
  # gh repo edit chemicallang/ui-examples --visibility public --accept-visibility-change-consequences
  # if the repo already exists: git remote add origin … && git push -u origin main
  gh repo view chemicallang/ui-examples
  git ls-remote origin
  ```
  If `gh` is missing/unauthenticated or the push fails, stop with a `BLOCKER.md`;
  do not fake the URL or claim a push.
- Reproducibility: add `scripts/setup-examples.sh` that downloads/points to a
  compiler artifact containing `lang/libs/ui*` (or pins remote imports), and a
  `THIRD_PARTY.md` (OFL/ISC/CC0). READMEs use the installed `chemical` binary,
  not `cmake-build-debug/*`. Fresh-clone builds **all four** apps with TCC and
  LLVM, runs audit/screenshot/bench, and checks no secrets/large junk.
- Evidence: public URL, commit hash, CI run (if any), benchmark JSON,
  screenshots, `gh` output, fresh-clone verification.

Acceptance: the four apps are public after P8.1, build on both backends, look beautiful,
pass audit/tests, and their documented benchmarks reproduce and meet the
`BENCHMARKS.md` budgets with margin (or a documented blocker). The parent
reviews the repo, the screenshots, and at least one fresh clone.

---

### Stage 8 — Release

#### P8.1 — Release gate audit

- Run the full matrix; verify every **v1/CPU** budget; GPU budgets only when
  P3.15 is in scope; binary sizes; **TCCCompiler ≤5 MB steady-state**; API index diff/freeze audit; deprecations; license/attribution
  audit (fonts/icons/examples); security review of the debug server; panic
  whitelist; accessibility manual pass; comparison report; fresh-clone
  `ui-examples` build **all four** apps on both backends + benchmark
  reproduction; then make the examples repo public:
  ```bash
  gh repo edit chemicallang/ui-examples --visibility public --accept-visibility-change-consequences
  gh repo view chemicallang/ui-examples
  ```
  Verify an unauthenticated `git ls-remote`.

#### P8.2a — Bundles, assets, templates

- `chemical bundle` for Linux AppImage/deb, Windows MSI/MSIX, macOS
  `.app`/notarization, WASM static; asset manifest + `resolve_asset`; app icons;
  installed-path resolution (no CWD); `chemical new --ui` template; docs linked.
- Acceptance: bundle each example on its host, launch from an arbitrary CWD,
  verify assets/icons, record sizes.

#### P8.2b — Updates, crash reporting, final validation

- Update feed contract (signed manifest, version check, optional download) and
  crash reporter (`crashsave`/UI error report) with a diagnostics bundle.
- Verify `ui-examples` public, fresh clone builds all four apps (TCC+LLVM),
  benchmark reproduction, final parent review.
- If updates/crash are not ready, mark `deferred` in `STATUS.md` and remove the
  v1 claim; do not silently omit.

---

## 3. Status table

Update after every packet. `—` means not started.

| Packet | State | Commit | Evidence | Bench delta |
|--------|-------|--------|----------|-------------|
| P0.1 | — | — | — | — |
| P0.2 | — | — | — | — |
| P0.3a | — | — | — | — |
| P0.3b | — | — | — | — |
| P1.1 | — | — | — | — |
| … | — | — | — | — |

The canonical machine-readable copy is `lang/docs/ui/STATUS.json` (created in
P0.3a) so a script can render the table.

---

## 4. Stage gates

A stage is not merged to `main` until:

- every packet in the stage is tagged and its evidence exists;
- the full suite for that stage is green on TCC + LLVM + the stage's targets;
- the parent has reviewed the visual gallery and benchmark report;
- `STATUS.md` is current;
- no open `BLOCKER.md` without an approved deferral;
- the stage's public API matches `API.md` and any amendments are recorded.
