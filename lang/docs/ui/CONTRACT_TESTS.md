# P0.2 contract test inventory

`API.md` is the frozen source. This file lists what is **compile-gated** now and
what is a negative/runtime gate owned by a later packet.

## 1. Compile gate (green now)

```bash
./scripts/ui-test.sh --tcc  --filter contract
./scripts/ui-test.sh --llvm --filter contract
```

`scripts/ui-test.sh --filter contract` runs the compiler front end with
`--check` on `lang/tests/ui/contracts/chemical.mod`. The module defines every
public declaration in `API.md`; functions/methods are `@extern` or trivial
default bodies, so no UI runtime is linked or called. `--check` performs parse,
symbol resolution, generic/where-clause checks and type verification on both
TCC and LLVM.

The module defines:

- every struct/variant/enum/interface in §1–§9 and §11, including fields,
  private fields, defaults and methods;
- `Application` plus the sample `App` implementation (empty bodies);
- every free function/namespace function (`ui::`, `ui_host::`, `ui_images::`,
  `ui_icons::`, `ui_fonts::`);
- `Presenter`/`Surface<'a>`/`SurfaceDesc`/`SurfaceFormat`/`HostCaps` from
  `HOST.md`;
- `ItemSource`/`TableSource` and the data-view types `CATALOG.md` requires;
- `UiHarness<'a, A>` and its full method set.

## 2. Type/shape gates

| Area | Gate |
|------|------|
| `UiError`/`ErrorCode`/`ErrorDetail`/`RetryHint` | private fields + `@make` only; no public literal construction; `make` cannot set `Fatal` (P1.13 negative test) |
| `Key`/`NodeId`/`CanvasId`/`WindowId`/`IconId`/`FontHandle`/`ImageHandle`/`TimerId` | private/registry fields; no public literal construction |
| `FrameInfo`/`Viewport`/`Breakpoint` | exact fields used by `Application` and `ViewBuilder` |
| `UiEvent<'a>`/`Payload<'a>` | borrow-tied event; every payload struct is declared once |
| `TextEditState`/`ImeComposition`/`Range` | canonical once in §1.5; `text_state` returns `Option<&TextEditState>` |
| `TextStyle`/`TextFeatures`/`TextConstraints`/`TextLayoutMetrics`/`TextContext` | exact signatures and field names |
| `FontStack`/`FontFallback`/`FontSpec`/`FontCacheEntry`/`FontError` | per-script fallback and cache metadata |
| `CanvasId`/`CanvasPaint`/`Painter`/`Path`/`Gradient` | Canvas API compiles with no app-visible GPU type |
| `ImageState`/`ImageSource` | variant payloads, no `enum` payload placeholder |
| `ComponentMeta`/`ComponentSpec`/`PageSpec` | catalog-owned opaque structs; component props remain P4 |
| `TableSource`/`TableColumn`/`SortSpec`/`SelectionMode`/`ItemSource` | data-view contracts |
| `HostCaps`/`Surface`/`SurfaceDesc`/`SurfaceFormat`/`Presenter` | host/presenter freeze from `HOST.md` |
| `UiHarness<'a, A>` | lifetime + `A : Application` bound; every method signature |
| `Options`/`Limits` | defaults are represented in the contract module; `Options.make`/`Limits.make` constructors exist |

## 3. Negative gates (not expressible until the library exists)

These are listed so the parent can see they are not silently skipped:

1. **Forged `NodeId`/`WindowId`/handles** — private fields; a negative test in
   P1.2/P1.13 must try literal construction and fail.
2. **App-constructed `UiError.Fatal`** — `UiError.make` never sets `Fatal`;
   P1.13 seeds a test that the constructor path reports `Expected`/`Internal`
   and that `Fatal` is reachable only through the whitelist.
3. **Wrong `Application`/callback signature** — language-level negative tests
   already cover interface implementation mismatch; P0.1a added the
   `std::function` wrong-signature test. P1.1/P1.11 add the UI-specific
   duplicate-key and stale-focus cases.
4. **Stale `NodeId`** — P1.2/P1.4 tests replay a destroyed node and require
   `Option.None`/`UiError.StaleNode`, never a crash.
5. **Exhaustive event payloads** — P1.11 tests every `EventKind` against the
   `CONTRACTS.md` §3.1 mapping; the contract module only gates the types.

## 4. API index freeze

`lang/docs/ui/ui_api_index.json` is generated from `API.md` by
`scripts/ui-api-index.sh`. It includes declaration kind, owner, signature,
members (fields/defaults/enum members), fence/line and a stable key.

```bash
./scripts/ui-api-index.sh --check   # fails on a removed/renamed/changed symbol
```

Sample-only fences (`chemical sample`) are excluded. The identical
`UiCtx.set_text_scale` duplicate is removed; any new duplicate key is fatal.

## 5. Protocol/CLI freeze

`lang/docs/ui/contracts/protocol.json` freezes the `ui.debug.v1` methods,
notifications, CLI command names/flags/exit codes, and MCP tool names from
`PROTOCOL.md`. The contract module does not implement the server; P5.1 adds the
full JSON schema and P5.4 implements the CLI.

## 6. Verification

```bash
./scripts/ui-test.sh --tcc  --filter contract
./scripts/ui-test.sh --llvm --filter contract
./scripts/ui-api-index.sh --check
./scripts/ui-verify.sh --packet P0.2
```
