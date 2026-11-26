# Chemical UI — native, safe, AI-first UI toolkit

**Status:** draft plan, awaiting maintainer approval · **Date:** 2026-11-26
**Proposal:** [`../proposals/0016-native-ui.md`](../proposals/0016-native-ui.md)
**Execution:** [`EXECUTION.md`](EXECUTION.md) · **API freeze:** [`API.md`](API.md) · **Perf gates:** [`BENCHMARKS.md`](BENCHMARKS.md) · **Evidence:** [`RESEARCH.md`](RESEARCH.md)

This directory is the canonical plan for the `ui` family of libraries that ship
with the Chemical compiler (`lang/libs/ui*`). It is written to be executed by a
chain of coding agents, one **packet** at a time, with a parent agent reviewing,
validating, and committing each packet before the next one starts.

## Documents

| File | Purpose |
|------|---------|
| [`PLAN.md`](PLAN.md) | One-page plan and the decisions that define the toolkit. |
| [`../proposals/0016-native-ui.md`](../proposals/0016-native-ui.md) | The design: goals, architecture, decisions, scope, risks, acceptance. |
| [`API.md`](API.md) | Frozen v1 public contracts (app model, keys, style, components, protocol). Read before writing code. |
| [`BENCHMARKS.md`](BENCHMARKS.md) | Hard performance budgets, scenarios, measurement method, CI gates, comparison policy. |
| [`EXECUTION.md`](EXECUTION.md) | The packet list, per-packet acceptance criteria, parent/subagent loop, anti-skip rules. |
| [`RESEARCH.md`](RESEARCH.md) | Condensed web/codebase research and the decisions it forced. |
| [`STATUS.md`](STATUS.md) | Live progress table; the parent updates it after every packet. |

## Non-negotiable summary

1. **Correctness is structural.** The runtime owns mutable UI state; app code
   owns the model and mutates it only in `update`. Cross-frame references are
   generational IDs, never pointers. Expected failures are typed `Result`s;
   panic is a reviewed last resort only when continuing would be unsafe, never a
   substitute for `Option`/`Result`. No `unwrap`-style API, no `unsafe` in the
   safe surface.
2. **Performance is a gate.** No per-frame tree allocation, no relayout of clean
   subtrees, no idle wakeups, deterministic counters in CI, and hard budgets in
   `BENCHMARKS.md`. Optimizations are kept only if a measurement proves them.
3. **Visual truth is testable.** A deterministic CPU renderer is the reference;
   an optional GPU backend consumes the same display list. Every component ships
   semantic, geometry, and golden-image tests across states, themes, DPI, and
   text scale. Goldens are bit-exact when a font is pinned; system-font runs use
   per-platform tolerance. Every visual packet produces a PNG a human/parent
   agent inspects.
4. **Agents can see and inspect everything.** A first-class semantic tree and a
   local JSON-RPC debug protocol (`snapshot`, `inspect`, `act`, `clock`,
   `screenshot`, `capture`, `audit`, `perf`) ship with the library, plus an MCP
   adapter and a `chemical ui` CLI. Screenshots are a fallback, not the only
   channel.
5. **Beauty is enforced, not hoped for.** A single high-quality design system,
   typed tokens, complete components, page patterns, and an `audit` command that
   fails on objective violations (contrast, spacing drift, missing accessible
   names, overflow, inconsistent radii, tap-target size, raw values in chrome).
6. **One source, every target.** Core is pure Chemical over a thin platform
   interface. v1 targets Linux (X11/Wayland through one host), Windows, macOS,
   and `wasm32-unknown-unknown` (canvas + DOM semantics); headless is a
   first-class target used by CI. GPU (Vulkan/Metal/WebGPU) ships as an optional
   v1.1 track, never a correctness dependency.
7. **No default font payload.** The theme uses the platform UI font stack. Apps
   can pin a file, download an OFL font into a cache, or embed a ~42 KB Latin
   subset; goldens pin a test font. The library never links a font into every
   app.
8. **APIs freeze before v1.** The contracts in `API.md` change only through an
   explicit, parent-approved amendment recorded in that file. No repeated
   redesign after implementation starts.
9. **Real apps prove it.** Four AI-written apps (settings, dashboard, chat,
   media) are built at the end, benchmarked on both TCC and LLVM (LLVM for
   numbers), visually reviewed, and pushed to `chemicallang/ui-examples` with
   `gh`.
