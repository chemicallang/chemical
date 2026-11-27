# Golden churn rule (P0.4)

**Status:** normative · **Date:** 2026-11-27
**Token source:** [`tokens.json`](tokens.json) · **Themes:** `lang/libs/ui/src/theme.ch`

`tokens.json` is the single source of truth. `theme.ch` and the three static
reference screens are generated from it. There is no second hand-maintained
token table.

## The rule

After P1.5, **any token change lists every updated golden in the packet report**.
P2.11 may polish shadows/easing only; it may not invent a second token set.

A token edit is incomplete unless all of these are done together:

1. Edit `lang/docs/ui/tokens.json` only (colors/modes, spacing, radii, typography,
   elevation, motion, breakpoints, focus, density, disabled opacity).
2. Run `./scripts/ui-gen-theme.sh` and `./scripts/ui-gen-design.sh`.
3. `./scripts/ui-gen-theme.sh --check` and `./scripts/ui-gen-design.sh --check`
   must pass.
4. Re-render `reference-settings.png`, `reference-dashboard.png`,
   `reference-chat.png`, and `tokens.png` with
   `./scripts/ui-render-design.sh`.
5. Re-run contrast gates (`./scripts/ui-test.sh --tcc --filter design` and
   `--llvm --filter design`).
6. In the packet/PR report, list every golden touched by the change: the three
   reference PNGs, the token-sheet PNG, and any renderer/component golden whose
   pixels move because of the token.

## What counts as a golden

- The reference PNGs in `lang/docs/ui/design/` (visual composition specs).
- The token-sheet PNG (`tokens.png`).
- Every future CPU-renderer golden under `lang/tests/ui/goldens/` (P1.9/P1.10).
- Browser/universal component goldens whose resolved token values change.

The P0.4 reference PNGs are **static specs, not renderer goldens**; they still
follow this rule so design documentation cannot silently drift before P1.5.

## Forbidden

- Editing `theme.ch` by hand (it is generated).
- Editing a reference HTML color literal by hand instead of a token.
- Adding a token in `theme.ch` only, or in HTML only.
- Declaring "polish" that changes a token value without regenerating and listing
  goldens.

## Approval

A token change after P1.5 is a parent-approved design change. Include the
commands, the contrast table, the screenshot links, and the complete golden list
in the packet report.
