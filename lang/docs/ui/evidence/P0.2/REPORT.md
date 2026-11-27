# P0.2 — freeze contracts v1: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**Status:** contract module builds, links and runs on both TCC and LLVM; API
index is green; awaiting parent review/commit. No library behavior or components
were implemented. `BENCHMARKS.md` was not modified.

---

## 1. API.md consolidation

`lang/docs/ui/API.md` is now the single frozen source. Changes:

| Finding | Correction |
|---|---|
| Identical `UiCtx.set_text_scale` repeated twice (old lines ~961/999) | removed the duplicate; one declaration in `UiCtx` remains |
| Namespace ownership ambiguous (`ui::` vs top-level fences) | added normative §0 note: §1–§9/§11 declarations are in `ui` unless a fence opens `ui_host`/`ui_images`/`ui_icons`/`ui_fonts`/`ui_test`; fences may omit the wrapper |
| App template `main` / sample `App` mistaken for public API | fenced as `chemical sample` (not indexed, not part of the freeze) |
| `button(...)` component helper mistaken for public API | fenced as `chemical sample`; `ui_kit` components remain catalog-owned (P4) |
| `ViewBuilder.component` referenced an undefined `Component` | added opaque `public struct Component` |
| `ViewBuilder.virtual_list` referenced an undefined `ItemSource` | added the core interface from `CATALOG.md` §3: `count/key/item/extent` |
| `HOST.md` types absent from API | added §1.6 `SurfaceDesc`, `SurfaceFormat`, `Surface<'a>`, `Presenter`, `HostCaps` |
| `ImageState` was an `enum` with payloads | corrected to `variant` with named payload fields (`Ready(value:)`, `Failed(err:)`) |
| `ImePayload` used `replace_start/replace_end`; CONTRACTS says `Range` | changed to `replace : Range` |
| `ValuePayload` used float `range_start/range_end`; CONTRACTS/TEXT say a text range | changed to `range : Range` |
| `UiHarness.wait_until` used non-Chemical `fn(&A) => bool` | corrected to `(x : &A) => bool` |
| `Key` private field `tag`/`id` collided with `tag()`/`id()` methods | renamed private fields `tag_value`/`id_value` (private names only) |
| `UiError` private `code`/`class` fields collided with `code()`/`class()` | renamed private fields `code_value`/`class_value` (private names only) |
| `Options.limits = Limits.make()` / `font_stack` default called a runtime function in a field default | field defaults removed; `Options.make()` owns those defaults (documented in the field comments) |
| `TEXT.md` text-change events absent from `EventKind` | added `SelectionChanged`/`TextChanged` plus `SelectionPayload`/`TextChangePayload` and `Payload` variants |
| `TEXT.md` shows owned `Option<TextEditState>`; CONTRACTS §3.2/API final pass require a borrow-tied `&` | API keeps `Option<&TextEditState>`; TEXT's snippet is stale and is superseded by API/CONTRACTS. Recorded here as a residual doc drift, not a code change. |
| `CONTRACTS.FrameInfo` lacked `text_scale`; final review requires it | API keeps `text_scale`; CONTRACTS is older prose. |
| `TreeSource`, `ListProps`, `TableProps` | left catalog-owned (P4.8); `TableSource`/`ItemSource` are the core interfaces and are in API |

`API.md` now declares every type referenced by a frozen declaration exactly once.
`ComponentMeta`/`ComponentSpec`/`PageSpec`/`AuditReport` remain opaque structs
because their fields are catalog/protocol-owned (CATALOG.md/PROTOCOL.md); they
are not undefined types.

---

## 2. Contract module

`lang/tests/ui/contracts/`

- `chemical.mod` — `application ui_contracts`, imports `std`, `cstd`, `core`.
- `src/main.ch` — every frozen declaration with exact signatures. Methods are
  `@extern` (or trivial constructor/default bodies); the sample `App` implements
  `ui::Application` with empty bodies. It does not call any UI runtime code.
- Namespaces: `ui`, `ui_host`, `ui_images`, `ui_icons`, `ui_fonts`.
- Includes `Presenter`/`Surface`/`HostCaps`, `ItemSource`/`TableSource`,
  `UiHarness<'a, A>`, `UiError` private fields, `Key`/handles, all event payloads,
  text/font faces, style/theme/semantics, and the §10 protocol/CLI are frozen in
  `lang/docs/ui/contracts/protocol.json`.

### Compile/build/run results (P0.2b)

```bash
cmake-build-debug/TCCCompiler lang/tests/ui/contracts/chemical.mod \
    -o /tmp/ui_contract_tcc.exe --mode debug_quick --no-cache   # exit 0
/tmp/ui_contract_tcc.exe                                        # ui contract PASS
cmake-build-debug/Compiler lang/tests/ui/contracts/chemical.mod \
    -o /tmp/ui_contract_llvm.exe --mode debug_quick --no-cache  # exit 0
/tmp/ui_contract_llvm.exe                                       # ui contract PASS

./scripts/ui-test.sh --tcc  --filter contract   # builds, links, runs -> PASS
./scripts/ui-test.sh --llvm --filter contract   # builds, links, runs -> PASS
./scripts/ui-test.sh --tcc  --filter contract --contract-check-only  # optional front-end check
```

Raw: `logs/contract_build_tcc.log`, `logs/contract_build_llvm.log`,
`logs/contract_tcc_check.log`, `logs/contract_llvm_check.log`,
`logs/test_tcc.log`, `logs/test_llvm.log`.

`--contract-check-only` keeps the fast front-end-only gate. The normal contract
filter now proves the full build/link/run path on both backends.

**Declarations still failing: none.**

---

## 3. API index / diff gate

`scripts/ui-api-index.sh`:

- skips `chemical sample` fences;
- extracts declaration kind/owner/signature/**members** (fields, defaults, enum
  members), fence/line, stable key;
- deduplicates exact repeats and fails on conflicting duplicate keys;
- `--check` regenerates to a temp file and compares the committed baseline.

```bash
./scripts/ui-api-index.sh --check
# ui-api-index: --check OK (index matches baseline)
```

Baseline: `lang/docs/ui/ui_api_index.json` (367 declarations, 0 duplicates).
Tamper proof: editing a baseline signature makes `--check` exit 1 with a diff
(`logs/api_index_check_tampered.log`); restoring it exits 0
(`logs/api_index_check.log`).

CI: `.github/workflows/ui.yml` contracts job runs `--self-test` and
`ui-api-index.sh --check`; the contract-snippets job builds TCC+LLVM and runs
`ui-test --filter contract` on both, plus `--check`.

---

## 4. CONTRACT_TEST list

`lang/docs/ui/CONTRACT_TESTS.md` lists:

- the compile gate and the contract module coverage;
- type/shape gates for every frozen family (errors, handles, FrameInfo/events,
  text/fonts, canvas/painter, catalog, table/list, host/presenter, harness,
  options/limits);
- negative gates that need the library before they can be written (forged
  `NodeId`/handles, forged `UiError.Fatal`, stale IDs, event exhaustiveness) and
  which packet owns each;
- the API index freeze and protocol/CLI freeze.

Existing negative language tests already cover wrong interface/impl signatures;
P0.1a added `neg_named_function_signature_mismatch`. The UI-specific negative
cases are explicitly scheduled for P1.2/P1.13/P1.11 rather than silently skipped.

---

## 5. Protocol / CLI freeze

`lang/docs/ui/contracts/protocol.json` freezes:

- `ui.debug.v1` request methods (40), notifications (5);
- CLI `chemical ui …` command names, flags, and exit codes 0/2/3/4/5/6;
- MCP tool names (`ui_snapshot` … `ui_catalog`).

`API.md` §10 links to it. P5.1 adds the full JSON schema; P5.4 implements the CLI.
No server or CLI behavior was added here.

---

## 6. Other acceptance

| Command | Result |
|---|---|
| `./scripts/ui-test.sh --tcc --filter scaffold` | PASS (`ui scaffold PASS`) |
| `./scripts/ui-verify.sh --self-test` | PASS (bad red, good green) |
| `./scripts/ui-verify.sh --packet P0.2` | PASS |
| `./scripts/ui-api-index.sh --check` | PASS |
| TCC/LLVM contract build+link+run | PASS |
| `ui-test --filter contract --contract-check-only` | PASS |

Raw logs are in `lang/docs/ui/evidence/P0.2/logs/`.

`STATUS.md` marks P0.2 `done` (parent commit/tag pending); `STATUS.json` is
regenerated from it.

---

## 7. P0.2b — method mangling fix (resolved)

P0.2a's residual tooling blocker is fixed. `@extern` on a method no longer sets
`no_mangle`; the owner prefix is retained. `@extern` on a top-level
function/global/struct stays bare, and explicit `@no_mangle` on a method still
produces the bare name.

- `compiler/frontend/AnnotationController.cpp`: `annot_handler_extern` only
  applies `set_no_mangle(true)` when the function is not a member of a
  struct/interface/union/variant/impl.
- Before: `extern struct ... text(...)` for `ViewBuilder`, `Style`, `Theme`,
  `UiCtx`, `Painter` → TCC C redefinition.
- After: `ui_contracts_uiViewBuildertext`, `ui_contracts_uiStyletext`,
  `ui_contracts_uiThemetext`, `ui_contracts_uiUiCtxtext`,
  `ui_contracts_uiPaintertext` (and similarly `wrap`), all distinct.
- Regression: `lang/tests/src/basic/extern_method_mangle.ch` + C definitions in
  `lang/tests/ext/file.c` prove two `@extern func text` methods reach different
  C functions, a top-level `@extern` stays bare, and `@no_mangle` on a method
  remains bare.
- `scripts/ui-test.sh --filter contract` now builds and runs the contract module
  (both backends); `--contract-check-only` keeps the front-end check available.

## 8. Residual limitations

- The contract module is still a compile-time declaration surface; its methods
  are `@extern` prototypes, so a *called* method would need a C definition. The
  module's own `main` only prints `ui contract PASS`; it does not call UI code.
- **TEXT.md drift:** its owned `Option<TextEditState>` snippet predates the
  borrow-tied API; API/CONTRACTS win. P2.4 should reconcile the prose.
- **CONTRACTS/FrameInfo drift:** API keeps `text_scale`; CONTRACTS is older prose.
- **Catalog-owned** `TreeSource`/`ListProps`/`TableProps` remain outside API until
  P4.8.
- UI-specific negative tests (forged handles/Fatal/stale IDs) need the library
  and are scheduled in P1.2/P1.13/P1.11.
- macOS/Windows CI remain BLOCKED as recorded in P0.1/P0.3a.
- **Catalog-owned data-view props** (`ListProps`, `TableProps`) and `TreeSource`
  are not in API.md; CATALOG.md owns them until P4.8. `ItemSource` and
  `TableSource` are frozen in API because `ViewBuilder`/table cells use them.
- **TEXT.md drift:** its owned `Option<TextEditState>` snippet predates the
  borrow-tied API; API/CONTRACTS win. P2.4 must reconcile the prose.
- **Negative UI tests** (forged handles/Fatal/stale IDs/event exhaustiveness) need
  the library implementation and are scheduled, not run here.
- **macOS/Windows** CI jobs remain BLOCKED as recorded in P0.1/P0.3a; no
  platform behavior was claimed.
- No commit was made; branch remains `feature/ui-v1`.

---

## Parent audit and P0.2b resolution (2026-11-27)

P0.2a's audit found the TCC object/link failure: `@extern` methods were emitted
with bare names (`text`) and collided. **P0.2b fixes this** in
`compiler/frontend/AnnotationController.cpp`: `@extern` on a method no longer
sets `no_mangle`, so the owner prefix is retained; top-level `@extern` and
explicit `@no_mangle` behavior is unchanged.

- Before: `text`, `wrap` (bare) across `ViewBuilder`, `Style`, `Theme`, `UiCtx`,
  `Painter`.
- After: `ui_contracts_uiViewBuildertext`, `ui_contracts_uiStyletext`,
  `ui_contracts_uiThemetext`, `ui_contracts_uiUiCtxtext`,
  `ui_contracts_uiPaintertext`, etc.
- `scripts/ui-test.sh --filter contract` now performs a real build+run on TCC and
  LLVM; `--contract-check-only` keeps the front-end check.
- Regression: `lang/tests/src/basic/extern_method_mangle.ch` + C definitions in
  `lang/tests/ext/file.c` prove two same-named `@extern` methods reach different
  C functions, top-level `@extern` stays bare, and explicit `@no_mangle` on a
  method stays bare.

No public declaration was skipped.
