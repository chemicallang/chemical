# P1.13 — typed-error boundary, fault injection and panic audit: report

**Date:** 2026-11-28 · **Branch:** `feature/ui-v1` · **State:** green.
No commit; P1.14 not started.

## 1. Error boundary

`headless_run_impl` now contains application callbacks instead of returning the
first callback error:

- `on_start`, `view`, `update` and Canvas `paint` failures are converted into a
  keyed error frame. `on_close` returns `CloseAction` (it has no error channel),
  so no failure can escape from it.
- A normal app view with `ViewBuilder.error(node, err)` is detected by
  `view_first_error`; the errored frame is replaced by the same card.
- `UiCtx.report_error(key, err)` records an error without panicking; the runtime
  consumes it after the callback and shows the card.
- The window/headless loop survives every contained failure. The last error is
  retained in `HeadlessState.last_error`; the failed view/layout/display state
  is rebuilt from a fresh keyed card, so no partially corrupt subtree is
  presented.
- Card content: `Role.Alert`, safe static text from `UiError.detail().message`,
  a focusable `Role.Button` labelled `Retry`, test ids `error-card`/`error-retry`,
  and a fixed error key. The retry event clears the error, restores the saved
  focus key and requests the app view on the next frame. Non-retry input is
  ignored while the card is active.
- Retry is not recursive: it is handled by the event loop and the next frame;
  the loop's `max_frames` bounds repeated failures. `Fatal` cannot be forged by
  an app: `UiError.make` classifies app codes as Expected/Internal, and
  `make_fatal` remains non-public and unused. No unwinding is used.

`paint` is now actually invoked for `ViewKind.Canvas` nodes through
`headless_paint_canvases`; a paint failure discards the partial display list and
renders the same error card.

## 2. Fault matrix

Test-only `headless_fault_set(site, code, count)` injects a typed `UiError` at
these sites:

| Site | Injection | Assertion |
|---|---|---|
| 1 | `on_start` failure | loop survives; card shown |
| 2 | `view` failure | loop survives; card shown |
| 3 | `update` failure | loop survives; card shown |
| 4 | layout failure/warning | card shown; no invalid frame |
| 5 | font startup failure | card shown; no abort |
| 6 | Canvas `paint` failure | partial display list discarded; card shown |

The matrix exercises `OutOfMemory`, `Image`, `Font`, `Layout`, `Protocol`,
`NotFound`, `StaleNode` and `StaleKey`; callback tests cover start/view/update
failures, `b.error`,
`ctx.report_error`, retry, forged-fatal rejection, and `b.error` with a partial
view. `lang/tests/ui/src/errors.ch` also checks that the paint fault was
actually consumed (`headless_fault_remaining() == 0`). Every run returns
`Result.Ok` or the error card; none aborts. The image/font cases are typed
decode/startup failures (no decoder crash); allocation failure is a typed
`OutOfMemory` simulation, not a hostile allocator.

## 3. Panic audit

`lang/docs/ui/PANIC_WHITELIST.md` records **no allowed abort sites** in
`lang/libs/ui*`. `scripts/ui-lint.sh` (UI-PANIC-001) scans
`lang/libs/ui*` and `lang/compiled/ui_*` for `panic`/`unwrap`/`expect`/
`unreachable` and unchecked access (`unchecked`/`take_unchecked`/
`get_unchecked`); it reports **0 findings**. The clean fixture passes and
`bad_panic`, `bad_unchecked`, key and raw-pointer fixtures fail as expected.
The only unchecked access in the UI library (`semantics.ch` string escaping)
was converted to a checked `switch` rather than whitelisted. P1.13 added no
panic/unwrap/unreachable/unchecked indexing. A private unused `make_fatal`
helper remains the only Fatal constructor; it is not reachable from app code.

## 4. Error-card PNG

`lang/tests/ui/fixtures/errors/error_card.png` (320×200) SHA-256:

```text
28b1d7e0c946204d47d29b22165ee241e69ed50e94ca9c72070ad25794b5cd41
```

The headless render contains a light surface, a white rounded error card with a
2 px danger border, a blue 120×44 Retry button and safe labels `UI ERROR`,
`VIEW FAILURE`, `RETRY`. Labels are drawn in the test with the P1.12 test-only
5×7 debug bitmap after the real headless frame; production text glyph
rasterization remains P2.2. The card contains no secret value, source path or
dynamic app data. Focus is assigned to `error-retry` while the card is active
and restored to the previous key after retry.

## 5. Verification

- `scripts/ui-test.sh --tcc --filter errors` — PASS
- `scripts/ui-test.sh --tcc --filter fault` — PASS
- `scripts/ui-test.sh --llvm --filter errors` — PASS
- `scripts/ui-test.sh --llvm --sanitize=address,undefined --filter errors` — PASS
- `scripts/ui-test.sh --llvm --release --filter errors` — PASS
- `scripts/ui-test.sh --tcc --release --filter errors` — PASS
- Headless/input/semantics/display/raster TCC regressions — PASS
- `scripts/ui-lint.sh` — 0 findings; `--fixtures` (including `bad_unchecked`) — PASS
- `ui-api-index --check` — 438 declarations, unchanged
- `ui-verify --self-test` — PASS

Logs are in `lang/docs/ui/evidence/P1.13/logs/`.

## 6. Files

- `lang/libs/ui/src/app.ch` — boundary, error card, fault hooks, Canvas paint hook
- `lang/libs/ui/src/view.ch` — `view_first_error`, Canvas accessor
- `lang/tests/ui/src/errors.ch` — fault matrix, boundary/retry/fatal/card tests
- `lang/tests/ui/src/headless.ch` — P1.10 error-path expectation updated
- `lang/tests/ui/src/main.ch`, `scripts/ui-test.sh` — `errors`/`fault` filters
- `lang/tests/ui/fixtures/errors/{error_card.png,SHA256SUMS}`
- `lang/tests/ui/fixtures/lint/bad_unchecked/view.ch`
- `scripts/ui-lint.sh`, `lang/libs/ui/src/semantics.ch`
- `lang/docs/ui/PANIC_WHITELIST.md`
- `lang/docs/ui/evidence/P1.13/REPORT.md` + `logs/`

## 7. Limitations and non-goals

- No protocol server, native a11y bridge, full component catalog or production
  `UiHarness`; these remain P3/P4/P5/P6.
- `on_close` has no `Result` return, and Chemical has no catchable exceptions, so
  a panic there is process-fatal by the language model; the lint/whitelist gate is
  the protection. `paint` is wrapped only where a Canvas node exists.
- Error-card message/retry text uses test-only debug glyphs in the PNG; the
  production text path is still P2.2.
- Faults are typed simulations of OOM/decode/resource failures; the tests do not
  require a failing OS allocator or corrupt font/image decoder.
- Retry is next-frame and loop-bounded; there is no user-visible retry counter or
  backoff policy in this packet.
- No compiler-wide `[no_panic]` enforcement; P7.6 owns the std panic whitelist.
