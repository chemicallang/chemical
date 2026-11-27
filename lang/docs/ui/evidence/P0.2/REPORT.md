# P0.2 — freeze contracts v1: report

**Branch:** `feature/ui-v1` (no commit made by this subagent)
**Status:** contract module and API index are green on both front ends; awaiting
parent review/commit. No library behavior or components were implemented.
`BENCHMARKS.md` was not modified.

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

### Compile results

```bash
cmake-build-debug/TCCCompiler lang/tests/ui/contracts/chemical.mod --check --mode debug_quick --no-cache
# exit 0
cmake-build-debug/Compiler lang/tests/ui/contracts/chemical.mod --check --mode debug_quick --no-cache
# exit 0
./scripts/ui-test.sh --tcc  --filter contract   # ui contract PASS
./scripts/ui-test.sh --llvm --filter contract   # ui contract PASS
```

Raw: `logs/contract_tcc_check.log`, `logs/contract_llvm_check.log`,
`logs/test_tcc.log`, `logs/test_llvm.log`.

`--check` is deliberate: the contract's methods are `@extern` prototypes, and
the existing C emitter does not owner-mangle `@extern` methods. Emitting/linking
the prototype-only module produces a pre-existing C collision (`text`, `wrap`,
`make`, …) unrelated to declarations; real library methods will have bodies and
normal mangling. The front-end compile check is the contract gate. This is a
compiler tooling limitation, not a declaration failure.

**Declarations still failing: none** under the contract check.

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
| TCC/LLVM contract check | PASS |

Raw logs are in `lang/docs/ui/evidence/P0.2/logs/`.

`STATUS.md` marks P0.2 `review`; `STATUS.json` is regenerated from it.

---

## 7. Residual limitations

- **Compiler tooling:** `@extern` methods are not owner-mangled in C emission,
  so a module of only prototypes cannot be object-linked. The contract gate uses
  `--check`; this does not affect real declarations with bodies. If the parent
  requires object/link output for the contract module, the compiler name
  mangler needs a targeted fix.
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

## Parent audit (2026-11-27)

The front-end contract check is green on TCC and LLVM, and LLVM builds and links
the contract module. A full **TCC object/link** of the contract module still fails
because the C emitter does not owner-mangle `@extern` methods (`text` is
redefined across `ViewBuilder`, `Style`, `Theme`, `UiCtx`, `Painter`). This is a
pre-existing compiler name-mangling bug, not a declaration/type error.

Checkpoint split:

- **P0.2a** (this commit): API consolidation, contract module, API-index diff
  gate, protocol freeze, front-end `--check` on both backends.
- **P0.2b** (next): fix `@extern` method owner-mangling, make the contract
  module build/link/run on both backends, switch the contract filter from
  `--check` to a real build+run, then mark P0.2 done.

No public declaration is being skipped; the remaining issue is codegen/link.
