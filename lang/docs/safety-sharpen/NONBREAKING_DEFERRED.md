# Safety Sharpen — deferred to the breaking branch

Items on the Phase 3 worklist that genuinely **cannot** be implemented without
rejecting code that compiles today, removing/renaming a public API, or otherwise
breaking the stable safe-API release. Each entry states the reason and the
intended breaking-branch task.

Additive/unimplemented work that does **not** break anything lives in
[`PHASE3_REMAINING.md`](PHASE3_REMAINING.md) (async effect propagation, module
defaults on plugin-generated declarations, generic `try_*` annotations,
`[noreturn]` loop divergence, pattern-bound payload moves, HRTB widening, the
regex/TLS codegen fixes, `thread::scope`, slice coercion, Option/Result
combinators, and the 4 MB size goal).

| Item | Why breaking | Breaking-branch task |
|---|---|---|
| **P3.3 direct-reference return loan** | Tracking a loan for every safe callee that returns a *direct* reference rejects valid code today: an escaped loan in one `return` branch is not path-sensitive (a later `&mut` conflicts). The fix is a borrowing-model change, not an additive bug fix. | Make escaped loans path-scoped, then classify direct-reference returns and reject an escaping direct reference with `SAFE-BORROW-001`; add the false-positive case from the html tokenizer to the positive suite. |
| **P3.7 raw-surface audit offenders** | `scripts/safety-raw-surface-audit.py` reports four pre-existing non-breaking offenders: `universal_cbi` `body_check`/`hydration_check_body`, `web::App::handle`, and `webserve::serve`. They accept caller raw pointers while declared safe. Making them `unsafe func` (or taking a view/wrapper) is a public-breaking change. They are not in the reviewed `::raw` allowlist (they are not raw-named; they are raw-typed safe entry points). | Convert each to `unsafe func` or a checked owning/view parameter and update every caller; add the regression to the raw-surface gate. |

No other Phase 3 item is breaking: see
[`PHASE3_REMAINING.md`](PHASE3_REMAINING.md) for the additive/internal set.
