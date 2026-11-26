# Chemical UI — component catalog, API index, and test harness

**Status:** normative, v1 · **Date:** 2026-11-27
**Companions:** [`API.md`](API.md) §11/§12, [`CONTRACTS.md`](CONTRACTS.md) §11,
[`DESIGN.md`](DESIGN.md), [`EXECUTION.md`](EXECUTION.md) P4.1/P5.8/P6.1.

Agents must be able to discover the library without grepping source. P4.1 ships
the catalog schema, `ui_api_index.json`, the state matrix generator, and the
minimal lint. P5.8/P6.1 expand docs/tests; they are not the first delivery.

---

## 1. ComponentMeta schema (JSON)

Every `ui_kit` component exports a descriptor; `chemical ui catalog --json`
generates a stable array. Required fields:

```json
{
  "name": "Button",
  "module": "ui_kit",
  "summary": "Primary/secondary/ghost/destructive action.",
  "props_schema": { "$schema": "https://json-schema.org/draft/2020-12/schema", "type": "object", "properties": {}, "required": [], "additionalProperties": false },
  "slots": [{"name":"leading","required":false},{"name":"label","required":true}],
  "variants": {"variant":["primary","secondary","ghost","danger"],"size":["sm","md","lg"]},
  "defaults": {"variant":"secondary","size":"md","disabled":false,"loading":false},
  "required_a11y": ["name"],
  "keyboard": ["Space/Enter activates","Tab focuses","Escape cancels loading"],
  "states": ["default","hover","focus","pressed","disabled","loading","error","dark","compact","rtl","text200"],
  "examples": [{"title":"Primary","code":"..."},{"title":"Danger","code":"..."}],
  "a11y_contract": {"role":"Button","states":["disabled","busy"],"relations":[]},
  "tags": ["action","form"]
}
```

Rules:

- `props_schema` is generated from the `Props` struct; a component packet fails
  if it cannot generate. Token enums appear as string enums; defaults are
  explicit; `@optional`/defaulted fields are not `required`.
- `slots` are the only supported content injection; a component with free-form
  children says `"children": true` instead.
- `states` is the mandatory test matrix. P4.1 generates the state/golden
  checklist; the parent checks every cell.
- `a11y_contract` names the role, states, relations, and announcements. It is
  asserted by `ui_test`.
- Two examples minimum, compile-checked in `lang/tests/ui/catalog_snippets/`.

`ui_api_index.json` (generated, committed) contains every public symbol,
signature, default, enum member, `Props` field, slot, and protocol method. CI
fails on a removed/renamed symbol or changed signature/default without an
amendment. This is the API-freeze gate.

---

## 2. Discoverability

- `chemical ui catalog [--json] [--search Q] [--tag T]`; aliases
  (`dropdown`→Select, `modal`→Dialog) are part of the catalog.
- `ui_icons::find(name) : Option<IconId>` and `ui_icons::list()`; no magic ids.
- `chemical ui tokens [--json]`; every token has a swatch/preview.
- `.agents/skills/ui/SKILL.md`, `lang/libs/ui/AGENTS.md`, and `llms.txt` are
  generated from the catalog in P4.1 and drift-checked in CI. P5.8 expands
  examples/evals; it does not create discoverability from scratch.
- Each component doc has: purpose, props table, slots, variants, 2 examples,
  a11y contract, keyboard, when to use/not, and a screenshot.

---

## 3. Data views (freeze before P4.8)

All data views use `ItemSource`/`Key`/virtualization (CONTRACTS §2) and must not
allocate per offscreen row.

```chemical
@direct_init
public struct ListProps {
    var item_count : size_t
    var estimated_extent : float
    var overscan : u16
    var axis : Axis
    var selection : SelectionMode       // None | Single | Multiple
    var test_id : string_view
}

@direct_init
public struct TableProps {
    var columns : &[TableColumn]        // id, title, width, align, sortable
    var row_count : size_t
    var row_extent : float
    var selection : SelectionMode
    var sort : Option<SortSpec>
    var filter : Option<string_view>
    var test_id : string_view
}

public interface ItemSource {
    func count(&self) : size_t
    func key(&self, index : size_t) : Key
    func item(&self, b : &mut ViewBuilder, index : size_t) : NodeId
    func extent(&self, index : size_t) : float
}
```

`Table`/`Tree`/`VirtualList` require a `Key` per row; a column resize invalidates
only the text break cache, not the whole row tree. `Table` selection/sort/filter
events are typed payloads. A non-virtualised list >200 items is `UI-LIST-001`.

---

## 4. Test harness (v1)

```chemical
public struct UiHarness<'a, A> {
    public func new(app : &'a mut A, options : Options) : Result<UiHarness<'a, A>, UiError>
    public func app(&self) : &A
    public func app_mut(&mut self) : &mut A
    public func frame(&mut self) : Result<FrameStats, UiError>
    public func advance(&mut self, ms : double) : Result<FrameStats, UiError>
    public func settle(&mut self) : Result<u32, UiError>   // frames; error if unstable
    public func wait_until(&mut self, pred : fn(&A) => bool, timeout_ms : u32) : Result<std::Unit, UiError>

    public func by_key(&self, key : Key) : Option<NodeId>
    public func find_role(&self, role : Role) : Option<NodeId>
    public func find_name(&self, name : string_view) : Option<NodeId>
    public func find_test_id(&self, id : string_view) : Option<NodeId>
    public func find_all(&self, q : Query) : &[NodeId]
    public func within(&self, parent : NodeId, q : Query) : &[NodeId]
    public func focused(&self) : Option<NodeId>
    public func scroll_offset(&self, key : Key) : Point
    public func bounds(&self, id : NodeId) : Option<Rect>
    public func semantics(&self, id : NodeId) : Option<Semantics>
    public func text_of(&self, id : NodeId) : Option<&str>

    public func click(&mut self, id : NodeId)
    public func double_click(&mut self, id : NodeId)
    public func hover(&mut self, id : NodeId)
    public func press(&mut self, id : NodeId) / release
    public func drag(&mut self, from : NodeId, to : NodeId)
    public func key(&mut self, code : KeyCode, mods : Modifiers)
    public func type_text(&mut self, text : string_view)
    public func ime(&mut self, ev : ImeTestEvent)
    public func set_scale(&mut self, scale : float)

    public func screenshot(&self, path : string_view) : Result<Screenshot, UiError>
    public func assert_golden(&self, path : string_view, opts : GoldenOptions) : Result<GoldenResult, UiError>
    public func assert_semantics(&self, expected : &ComponentSpec) : Result<std::Unit, UiError>
    public func assert_no_overflow(&self) : Result<std::Unit, UiError>
    public func audit(&self) : AuditReport
}
```

`GoldenOptions { max_diff_pixels : u32 = 0, yiq : float = 0.0, masks : &[Rect] }`;
the default is bit-exact. `screenshot` returns `{path, width, height, hash}`.
`assert_golden` writes `*.diff.png`/`*.new.png` on failure. `settle` has a
documented bound (e.g. 120 frames) and returns `UiError.Timeout` if unstable.

Every component packet checks: semantics, geometry, all state goldens,
keyboard/focus, a11y relations, overflow, 200% text, light/dark/compact.

---

## 5. Goldens and independent specs

- The first golden must be accompanied by a `ComponentSpec` (roles, names,
  required states, slot contents, token spacing) generated from
  `ComponentMeta`; a golden alone cannot certify completeness.
- Page-pattern goldens have a `PageSpec` (required elements, hierarchy,
  responsive states).
- Agents may not "fix" a golden to match a broken layout unless the semantic
  spec also changes and the parent approves.
- `--update-goldens` writes old/new/diff and requires parent review.
- Pinned font makes headless goldens bit-exact; system-font/GPU/WASM goldens are
  per-platform tolerance and non-gating for cross-machine equality.

---

## 6. Minimal lint at P4.1

Ship these before the kit grows; P5.9 only adds rules:

| Rule | Severity | Scope |
|------|----------|-------|
| `UI-KEY-001` unkeyed list item / duplicate key | error | lib + app |
| `UI-LIST-001` non-virtualised list >200 items | error (audit) | app |
| `UI-RAW-001` raw color/px in kit props | error | kit |
| `UI-A11Y-001` missing name/role | error | kit + app |
| `UI-A11Y-002` missing relation (label/error/controls) | error | kit |
| `UI-PANIC-001` `unwrap`/`expect`/`panic`/`unreachable` | error | lib + app |
| `UI-PANIC-002` `PANIC:` without justification | error | lib |
| `UI-ALLOC-001` infallible growth in hot path | warning | lib |
| `UI-THEME-001` theme branch/raw color in component | error | kit |

`--ui-lint=off` is allowed only for migration and is visible in the audit; CI
uses it nowhere. Rules may be disabled only with a documented parent approval.
