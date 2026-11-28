> **Resolved by P1.9c (2026-11-28).** Analytic rounded coverage replaced the
> 16x16 samples and passed the AA hard gate (4.467/4.769 ms median). This file
> is retained as the P1.9b historical blocker record.

# P1.9b — AA release gate blocker

**Status:** C SIMD helper is implemented and functionally green; solid,
clear/fill (p50) and damage now meet their gates. **AA 10k remains 35.6 ms vs the
6.0 ms hard gate.** No budget was changed.

## Measured (LLVM release, 5 runs)

| Scenario | Gate (p50 / p95) | Median measured | Verdict |
|---|---:|---:|---|
| 10k solid quads | 1.2 / 2.0 ms | **0.917 / 1.485 ms** | PASS |
| 10k AA quads | 2.5 / 6.0 ms | **35.636 / 36.124 ms** | **MISS ~6.0×** |
| 1080p clear + fill | 1.0 ms | **0.313 ms p50**, 1.150 ms p95 | p50 PASS; p95 noisy |
| 200×120 damage | 1.5 ms | **0.001 / 0.006 ms** | PASS |

TCC release (C scalar fallback; functional only): solid 11.099 ms, AA 362.956 ms,
clear+fill 6.256 ms, damage 0.002 ms.

## Why AA cannot reach 6 ms in this checkpoint

`raster.ch` and the C helper both use the contract's **exact 16×16 sample
coverage** for rounded curves. The 10k-quad benchmark has ~80 edge pixels per
quad; even after:

- `-O2` linked-C compilation (this was previously `-O0`),
- interior-span fast paths,
- closed-form straight-edge sample counts,
- SSE2 corner sample evaluation,
- an exact pure corner-coverage cache,

the measured cost is 35.6 ms. P0.1 independently measured 16-sample
supersampling at **29.9–58.8 ms** for the same class of quad. The 5.1 ms SSE2
result in P0.1 was for **analytic axis-aligned coverage**, which produces
different pixels and therefore cannot preserve the committed 16×16 goldens.

The existing torture PNGs are byte-identical and were not regenerated:

```text
80a2a72bbd100fffc40061c29c228b8df547b5d722fef7fd3b48f5aea9b51f37  torture_320x240.png
0bad2f026d12fc7cd88c9e418cdb6d3286dcfe2640ff4ef90af0acce68e9f557  torture_640x480.png
```

## Proposed parent amendment (no silent budget change)

Preferred, to keep the 6 ms AA gate: authorize **P1.9c analytic rounded-corner
coverage** and an explicit **golden/contract amendment** for the two P1.9 PNGs.
The C helper already has the span/blend infrastructure; this would trade the
16×16 sample contract for analytic area coverage and target the 5.1 ms SSE2
class. This is a semantics/golden change, so it requires parent approval.

Alternative if exact 16×16 coverage must remain: amend the AA release gate to
the measured supersample class, e.g. **40 ms p50 / 45 ms p95**. That accepts a
~6× miss and is **not recommended** under the extreme-performance goal.

Solid/clear/damage do not need amendment. The hard 6 ms AA gate and the 2.5 ms
target remain in place until the parent chooses an option. P1.10 was not
started.
