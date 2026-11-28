# P1.12 — semantics tree and audit core: report

**Date:** 2026-11-28 · **Branch:** `feature/ui-v1` · **State:** green.
No commit; P1.13 not started.

## 1. Semantics tree

`lang/libs/ui/src/semantics.ch` adds:

- `SemanticsNode` + `SemanticsTree`, built from the same `ViewBuilder`/`LayoutState`
  and resolved `Style` used by paint. Each node stores role, label/description/
  value/alt/lang/placeholder/error, `SemState`/`SemAction`, hierarchy/relations,
  bounds/device bounds, test id, clip flag and scroll offset.
- Explicit `ViewBuilder.semantics` is authoritative; implicit roles are applied
  for text (`Text`), image/icon (`Image`) and scroll (`ScrollView`), and
  focusable nodes without a role become `Unknown` so the audit can fail closed.
- Traversal follows normal display order then overlay layer buckets; hidden
  (`SemState.hidden`) nodes and their subtrees are excluded. Focus/hover state
  is projected into `SemState.focused` from the keyed runtime state.
- Strings are copied into a tree-owned `FrameArena`, so snapshots survive the
  builder frame.
- Accessors: `semantics_find_role`, `semantics_find_name`,
  `semantics_find_test_id`, `semantics_bounds`, `semantics_focused`,
  `semantics_scroll_offset`, `semantics_node_at`, and JSON
  `semantics_snapshot`.
- `semantics_diff` emits Add/Update/Remove `SemanticsUpdate` values by `Key`.
- Secure values/labels are redacted in snapshots; the audit also reports a
  secure-value leak.

The 4-node fixture snapshot is committed at
`lang/tests/ui/fixtures/semantics/snapshot.json` (SHA-256
`65c127b0d6f4422a0dc930d06576e678064c761c609105caccc35146c8fef71e`).
The hidden fixture node is excluded; the tree contains Button, Link, Heading,
and Text in order; the Button is focused. Overlay-layer semantics are ordered
after base nodes.

## 2. Audit core

`semantics_audit` emits deterministic `AuditFinding` values with code/severity/
node/key/message/fix. Implemented rules:

| Code | Severity | Rule |
|---|---|---|
| `UI-A11Y-001` | Error | missing accessible name on an interactive role |
| `UI-A11Y-002` | Error | focusable node with `Role.Unknown` |
| `UI-A11Y-003` | Error | focusable node is disabled |
| `UI-A11Y-004` | Warning | interactive node is clipped/outside its clip |
| `UI-A11Y-005` | Error | interactive target below 24×24 logical px |
| `UI-A11Y-006` | Warning | interactive target below preferred 44×44 |
| `UI-A11Y-007` | Error | text contrast below 4.5:1 (3:1 large text) |
| `UI-A11Y-008` | Error | image/icon has no alt/name |
| `UI-A11Y-009` | Error | secure semantics value leaks |
| `UI-A11Y-010` | Warning | heading hierarchy skips a level |
| `UI-A11Y-011` | Error | duplicate key |
| `UI-A11Y-012` | Error | duplicate test id |

The bad fixture produces errors for missing name, small target, low contrast,
missing image alt, secure value, duplicate test id and duplicate key, plus a
heading warning; the clean fixture passes (`passed == true`). Findings are
emitted in node/traversal order and never silently green.

Contrast uses a deterministic 8-bit sRGB luminance approximation (no floating
power dependency); thresholds are configurable through `AuditOptions`.

## 3. Overlay PNG

`lang/tests/ui/fixtures/semantics/overlay.png` (320×200) SHA-256:

```text
6ebc0757ae6ae62ee0199e2dc3acc507a4c3de4975ca30c9b152248a3f9f8c70
```

Visual review: the light surface contains four semantic bounds. The blue
bounds are the Button and Link; the green bounds are Heading and Text. Each box
is labelled with its role and accessible label (`BUTTON SAVE`, `LINK OPEN`,
`HEADING WELCOME`, `TEXT BODY COPY`) using a small test-only 5×7 debug bitmap;
production text glyph rasterization remains P2.2. The overlay has no missing
nodes, overlapping-order mistakes, fringes, or stray marks.

## 4. Verification and timing

- `scripts/ui-test.sh --tcc --filter semantics`: PASS
- `scripts/ui-test.sh --llvm --filter semantics`: PASS
- `scripts/ui-test.sh --llvm --sanitize=address,undefined --filter semantics`: PASS
- `scripts/ui-test.sh --llvm --release --filter semantics`: PASS
- `scripts/ui-test.sh --tcc --release --filter semantics`: PASS
- Semantics tree 10k build, LLVM release: **p50 4.822 ms / p95 7.529 ms**;
  TCC functional **p50 23.866 ms / p95 26.494 ms**.
- P1.6/P1.7 layout, view, style, theme, display, raster, input, headless
  regressions: PASS TCC/LLVM.
- P1.8 display release **2137/6024 µs** (3/7 ms gate PASS).
- P1.9 raster release solid **1021/1604 µs**, AA **4990/5350 µs**, clear **351/548 µs**,
  damage **2/6 µs** (gates PASS).
- `ui-api-index --check`: 438 declarations, unchanged.
- `ui-verify`: self-test and P1.12 packet validation PASS.

## 5. Files

- `lang/libs/ui/src/semantics.ch` (new)
- `lang/tests/ui/src/semantics.ch`
- `lang/tests/ui/fixtures/semantics/{overlay.png,snapshot.json,SHA256SUMS}`
- `lang/tests/ui/src/main.ch`, `scripts/ui-test.sh`
- `lang/docs/ui/evidence/P1.12/REPORT.md` + `logs/`

## 6. Limitations

- No protocol server, native a11y bridge, full component catalog, or
  `UiHarness` production wrapper; those are P3.8/P5/P6/P4.
- Semantics tree is rebuilt each frame in tests; incremental updates are
  available but not yet wired to a protocol transport.
- Contrast uses an 8-bit sRGB approximation, not full ICC/color-space
  conversion.
- The overlay's role/label glyphs are a test-only debug bitmap; production
  text remains P2.2.
- Audit checks are structural; no false-green policy is claimed beyond the
  implemented rules.
- No panic/unwrap/unreachable/unchecked indexing was added; expected failures
  remain typed where applicable.
