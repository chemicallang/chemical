# Component Body Typing Plan (v1)

**Status:** approved implementation scope, 2026-11-25 (all phases 0–5, one branch)
**Delivery workflow:** see [`component-body-typing-workflow.md`](component-body-typing-workflow.md) for the test matrix, performance method, review protocol, CI wiring and the holes found while reviewing the workflow.
**Goal:** check `#universal` component bodies against real, indexed declarations (props and browser globals) without adding a second language, without shipping type information, and without putting plugin logic in C++.

---

## 1. Summary

A `#universal` body is JS text. `props` is typed at the call site, `document` is not, and nothing checks a body until a browser throws. A separate `.ext` declaration format and per-declaration annotations were rejected: they create a second source of truth.

This plan uses one generic module capability:

```text
# chemical.mod
index_with "universal"
```

The compiler parses the directive into module metadata. After that module's signatures are linked, it dispatches a generic **`IndexModule`** CBI hook to each registered indexer named in the directive, passing the module's public top-level declarations. The `universal` plugin builds a per-job index of ambient roots and their types. The body checker resolves names in two steps:

```text
document              -> one lookup in the indexed ambient table
document.something    -> + one member lookup on the indexed type
props.a               -> one lookup in the component's lexical scope
```

The DOM is the first provider. It stays a Chemical module, is renamed to browser spelling (`querySelector`, `activeElement`, …), gains `index_with "universal"`, and is imported by the components library **on the client side only**.

### Implementation scope (approved)

The branch implements **Phases 0–5 in order**, plus the workflow gates. The client-job slice is the first mergeable checkpoint, not the final state:

- **Phase 0**: `index_with` metadata, `Module.addIndexer`, `do_library_job` + JS library mode, `target_data.js` fix.
- **Phase 1**: `IndexModule`/`JobBegin` hooks, namespace-aware declaration collection, probe, enum sync.
- **Phase 2**: client-job index + checker plumbing; server job still skips.
- **Phase 3**: browser-spelled `dom`, root members, 001/002/006/008, counted 005, WebView allowed-surface test.
- **Phase 4**: server-side declarations-only indexing, `host` gating, consumer-side refusal in C and LLVM, object-cache bypass, `web-split-test` update.
- **Phase 5**: `Element`/`DOMRect`/`classList`/`style`/`dataset`, deep chains, `Option<T>` unwrapping, **strict library mode**.

**Strict library mode is the Phase 5 gate.** `WEB-BODY-005` becomes an error for `lang/libs/components` once the Element surface is complete. It cannot be enabled earlier without rejecting the corpus; the branch is one delivery, so the final state is strict.

**LSP stays deferred.** The editor does not surface these diagnostics in this branch; a parse smoke test is added and the limitation is documented.

### Non-goals

- No `.ext` files, no `@ambient`/`@process_with` annotations.
- No generic `lookup_type(name)` / `find_module(name)` API for the checker.
- No full JavaScript type system: no unions, overload resolution, optional chaining, generics, or inference beyond simple initializers.
- No migration of the 196 components' bodies. They keep JS; the library adds `import dom if client`.
- No source line numbers: JS AST nodes carry no location. Diagnostics carry the component's encoded location.
- No change to `#universal` syntax, props structs, call sites, `#html`, or the bundle format.
- No LSP diagnostics in this branch.

---

## 2. Verified facts

| # | fact | evidence |
|---|---|---|
| F1 | `chemical.mod` identifiers are dispatched by hash in the parser; a new directive is a new `case`, no lexer token | `parser/Parser.cpp:160-205`; `parser/statements/OptionStmt.cpp:15` |
| F2 | Parsed metadata lives in `ModuleFileData`; the normal dependency path is `build_module_from_mod_file`. The converter is the `translate-mod`/generated-build.lab path | `compiler/processor/ModuleFileData.h`; `compiler/lab/LabBuildCompiler.cpp:2639`; `compiler/lab/mod_conv/ModToLabConverter.cpp:89` |
| F3 | `LabModule` is the module record; `direct_files[i].result->unit.scope.body.nodes` holds parsed top-level nodes | `compiler/lab/LabModule.h`; `compiler/processor/ASTFileResult.h:22`; `ast/base/ASTUnit.h:21`; `ast/structures/FileScope.h` |
| F4 | CBI hooks are keyed by `(name, CBIFunctionType)` and found with `findHook` | `compiler/cbi/model/CompilerBinder.h:110` |
| F5 | `CBIFunctionType` exists in C++ and Chemical and must stay value-identical; append only | `compiler/cbi/model/CBIFunctionType.h`; `lang/libs/lab/src/lab.ch:91` |
| F6 | `ASTProcessor::sym_res_module` runs declare → link signatures → after-link-signature → link bodies; a hook after the signature pass sees linked types and runs before dependents' body checks | `compiler/ASTProcessor.cpp:568-780` |
| F7 | `dom` is **`public namespace dom { … }`**; its declarations are nested | `lang/libs/dom/src/main.ch:30-682` |
| F8 | Plugins already receive `VecRef<ASTNode>` (`PtrVec` over `std::vector<void*>`) | `lang/libs/compiler/src/PtrVec.ch`; `compiler/cbi/bindings/CBI.cpp:152-157` |
| F9 | `Namespace` exposes `nodes` and its own specifier; `Scope` exposes `nodes`; `FileScope.body` is the root | `ast/structures/Namespace.h:45-55`; `ast/structures/Scope.h:27`; `ast/structures/FileScope.h:13` |
| F10 | `BaseDefMember` extends `ASTNode`, so `getAccessSpecifier()` is already available | `lang/libs/compiler/src/ASTBuilder.ch:362-370`; `lang/libs/compiler/src/ast/base/ASTNode.ch:12` |
| F11 | Top-level `VarInitStatement` has no `getName`/`getType` CBI accessors | `lang/libs/compiler/src/ASTBuilder.ch:408` |
| F12 | `ComponentSignature.js_body`, `propsName` and `propsStruct` are available at symres; props are resolved after the hydration check | `lang/libs/html_comp/ast.ch:17-40`; `lang/libs/universal_cbi/src/sym_res/sym_res.ch:27-50` |
| F13 | The C emitter consults `MIRTargetSupport`; the LLVM emitter (`CodegenMir.cpp`) does not | `compiler/mir/MIREmitter.cpp:2308,2605`; `compiler/CodegenMir.cpp` |
| F14 | `do_js_job` requires `main()`, writes a bundle, and ignores `job->abs_path`, `job->objects` and `ship_files`; it is not a Library backend | `compiler/lab/LabBuildCompiler.cpp:5402-5425`; `compiler/lab/LabBuildContext.cpp:199-215` |
| F15 | `target_data.js` is true for Executable, Library and Interpretation | `compiler/lab/LabBuildContext.cpp:63-67` |
| F16 | `components/chemical.mod` does not currently import `dom`; `chemical.mod` supports conditional imports | `lang/libs/components/chemical.mod`; `lang/tests/web/fixtures/dom_types_only/chemical.mod` |
| F17 | The LSP parses `chemical.mod` but does not run `ASTProcessor::sym_res_module` or CBI SymRes hooks | `server/mod_file/Importer.cpp:131` |
| F18 | `ModuleFileData::clear()` enumerates every field; new fields must be added | `compiler/processor/ModuleFileData.h:169-190` |
| F19 | `StructDefinition::getFunctions()` includes generated constructors/destructors | `ast/structures/StructDefinition.h`; codegen's `generate_automatic_functions` |
| F20 | `hydration_safety.ch` already has a browser-global list (including `sessionStorage`) and region analysis | `lang/libs/universal_cbi/src/sym_res/hydration_safety.ch:18-40` |

---

## 3. User-visible syntax

### 3.1 Module directive

In a provider module's `chemical.mod`:

```text
module dom

source "src"
import cstd
import std
import core

# any plugin registered as "universal" may index this module's public surface
index_with "universal"
```

Rules:

- `index_with` is a single identifier. No lexer change.
- The value is one or more string literals, comma-separated; the statement is repeatable.
- Put metadata after the imports; `build_module_from_mod_file` and the LSP importer collect imports before metadata, so an `index_with` before the import block would stop import collection. Document this ordering.
- An unregistered indexer name is **ignored silently** (the directive is inert when that plugin is absent), with a note under `-v`.
- No `option host` in v1. The server-side declarations-only mode is Phase 4.

### 3.2 Component source

Unchanged:

```chemical
public struct CardProps {
    var title : std::string_view
    var count : int
}

public #universal Card(props : CardProps) {
    return <div class="card">{props.title} {props.count}</div>
}
```

Call sites (`<Card title="ok" />`), props structs, `@optional`, defaults, `#html` and `#css` do not change.

`lang/libs/components/chemical.mod` gains one conditional import:

```text
import dom if client
```

The 196 JS bodies do not change.

### 3.3 What becomes an error

For a component with typed props, checked in the client/JS job:

- `props.titel` → prop does not exist (WEB-BODY-001).
- `document.titel` → direct member of an ambient root does not exist (WEB-BODY-002).
- `document` with no provider → unknown host global (WEB-BODY-008).
- a hook inside `if`/handler → must run every render (WEB-BODY-006).
- anything else the checker cannot type → **counted** (WEB-BODY-005), never an error in v1.

---

## 4. Architecture and data flow (v1)

```text
chemical.mod (client side)
   import dom if client
   dom/chemical.mod: index_with "universal"
        │
        ▼
ModuleFileData.indexers
        │
        ▼
LabModule.indexers
        │
        ▼
[client/JS job]  ASTProcessor::sym_res_module
   declare → link signatures → after-link-signature
        │
        ├── for each module->indexers:
        │       binder.findHook(name, IndexModule)
        │       collect public top-level declarations (recurse public namespaces)
        │       call hook(module, decls)
        │
        ▼
universal index (per client job)
   ambient: root name -> Entry { kind, type, provider }
        │
        ▼
[client/JS job] component body check (symres)
   resolve name:  locals/params → props → known built-ins → ambient
   resolve member: only a direct member of an ambient root
```

The index is **per job**, **read-only after build**, and never crosses a job boundary. `JobBegin` clears it and records whether the job is the client/JS side; the body check early-returns on a server job.

---

## 5. Compiler work

### 5.1 Parse `index_with`

- Add `std::vector<chem::string_view> indexers` to `ModuleFileData`; add it to `ModuleFileData::clear()`.
- Add `case hash_fn("index_with")` to `Parser::parseModuleFile`, with `parseIndexWithStmt`:
  - one or more `String` tokens separated by `CommaSym`, appended to `data.indexers`;
  - diagnose a non-string value with its source location.
- LSP: `server/mod_file/Importer.cpp` uses the same parser, so the directive parses, but the LSP does not run the indexing hook. **v1 does not check component bodies in the editor.** State this; wiring the LSP is a follow-up (it needs an index in its own analysis model).

### 5.2 Propagate to `LabModule`

- Add `std::vector<chem::string> indexers` to `LabModule` (owned copies; `ModuleFileData` views do not outlive the module record).
- `build_module_from_mod_file`: copy `modFileData.indexers` into `module->indexers` before the allocators are cleared.
- `convertToBuildLab`: emit `mod.addIndexer("…")` per entry.
- Extend the `Module` compiler interface with only what the converter needs:
  ```chemical
  func addIndexer(&mut self, name : &std::string_view)
  ```
  Implement in `compiler/cbi/bindings/LabCBIAddons.{h,cpp}`; add the symbol to `CBI.cpp`. `addIndexer` **copies** the view into `LabModule` (the generated build.lab literal lives in a build-script arena that is destroyed after `build` returns).

### 5.3 CBI hooks

Append two values to both enums (`CBIFunctionType.h`, `lab.ch`):

```text
IndexModule
JobBegin
```

Typedefs in `compiler/cbi/model/Model.h` (add `<vector>`):

```cpp
class LabModule;
class LabJob;

// Called once per job before any module is processed. Stateful plugins clear
// per-job tables here and record the job's side (client/server, JS/native).
typedef void (*JobBeginFunc)(LabJob* job);

// Called after the marked module's signatures are linked, before the body pass.
// `declarations` is a transient std::vector<void*> of public ASTNode*; the plugin
// retains pointers (job-arena lifetime) and copies only owned metadata.
// The plugin receives the Module CBI interface, not the C++ LabModule*.
typedef void (*IndexModuleFunc)(LabModule* module, std::vector<void*>* declarations);
```

The plugin-side signature uses `*mut Module` (the CBI interface) and `*mut VecRef<ASTNode>`; these are ABI-compatible with the C++ pointers above.

Dispatch:

- **`JobBegin`**: in `LabBuildCompiler::do_job`, before the job-type switch, iterate `binder.hooks_` for `JobBegin` and call each with the job. `do_job` runs once per side of a web build (`LabBuildCompiler.cpp:3649,3656,3712-3755`), so the plugin sees the server job and then the client job. A plugin not loaded has no hook; a loaded plugin always clears, even when the job has no marked modules.
- **`IndexModule`**: in `ASTProcessor::sym_res_module`, after the `sym_res_after_link_sig_file` loop succeeds and before the body pass:
  ```cpp
  for (const auto& indexer : module->indexers) {
      auto* hook = binder.findHook(indexer, CBIFunctionType::IndexModule);
      if (!hook) continue;                       // unregistered name: inert
      std::vector<void*> decls;
      collect_public_declarations(module, decls);
      ((IndexModuleFunc) hook)(module, &decls);
  }
  ```

`collect_public_declarations` must:

- walk `module->direct_files[i].result->unit.scope.body.nodes`;
- **recurse public `NamespaceDecl` and top-level `Scope` nodes** and gate the namespace's own specifier (an internal namespace with public members must not leak);
- filter `is_linkage_public(node->specifier())`;
- v1 accepts `StructDecl`, `VariantDecl`, and `FunctionDecl`. Top-level `VarInitStatement` is **not** indexed in v1 because the CBI has no name/type accessor (F11); add the accessors before supporting ambient `var`s.

Cached plugins: appending enum values is safe, but a cached plugin binary built before `universal_cbi/build.lab` changes will not register the new hooks. The concrete mechanism is `force_recompile_plugins` / `index_function`; a cache hit without the new hooks fails lookup rather than silently skipping. Include the plugin's build.lab in its cache identity or bump the CBI ABI.

### 5.4 JS library mode and the backend flag

`do_library_job` currently never routes `emit_js`, and `do_js_job` is executable-only (requires `main()`, writes `index.mjs`, ignores `abs_path`/`ship_files`). This branch adds a real JS library mode rather than rejecting the build:

- `do_library_job`: if `options->emit_js`, call `do_js_job(job, JsJobKind::Library)`.
- `do_js_job` gains a job-kind parameter:
  - **Executable**: current behaviour (requires `main`, emits `index.mjs`, runs the program's roots).
  - **Library**: no `main()` lookup. Roots are each module's public functions/exports plus address-taken symbols; no `index.mjs` entry (a barrel is a follow-up; per-module `.mjs` imports are stable). `ship_files` are copied into `emit_js_dir`; `job->abs_path` is not used because a JS library is a directory of ES modules, not a `.so`. `check_only` lowers and diagnoses but writes nothing.
- Reachability: `MIREmitterJs` already computes reachable functions from an entry. Library mode passes a **root set** (public functions, address-taken symbols, vtable slots, async poll pairs, global initializers) instead of `main`; extend `process_js_job` to accept it.
- `target_data.js` excludes `Interpretation`; it is true for Executable and Library `emit_js`. The checker's client gate is `target_data.client || target_data.js`.
- Gate: a `library` fixture with a public function; `--emit-js <dir>`; a Node consumer imports `./<module>.mjs`, calls the function and sees the expected output; no `main` is required; `ship_files` are present; no `.so` is produced.
- LSP is unaffected.

### 5.5 Phase 4 — declarations-only modules (in scope)

A server job that wants to index `dom` needs the module included without lowering `chx_dom_*`. That is a later phase and requires all of:

- an owned `host` enum (never a `string_view` into the module-file allocator — the current `stack_protector` already has that latent use-after-free);
- a per-job `types_only` flag on `LabModule` (not public `ModuleOptions`);
- types-only keyed on `host == js` + a non-JS target, **not** on whether an indexer is registered (otherwise a plain native build of a consumer that imports `dom` breaks);
- bypassing the object cache for types-only modules (`process_module_tcc` reuses `object_path` when `has_changed == false`, before any types-only logic);
- a consumer-side refusal for calls to the host module's mangled symbols, enforced in **both** `MIREmitter.cpp` (C) and `CodegenMir.cpp` (LLVM); today only the C emitter consults `MIRTargetSupport` (F13), and the table is static with no job context. The clean shape is a per-job set of unavailable module ids carried into `MIRModule`, or `module_of_symbol` lookup against a types-only set;
- `web-split-test.sh` update (the current expectation is `failure during translation to c in the module dom`).

These are Phase 4; the client-side checkpoint (Phases 2–3) does not depend on them, but the final branch does.

---

## 6. Provider: `dom`

### 6.1 Browser spelling

Rename the Chemical-facing surface to match the browser. One spelling; no mapping table.

```chemical
public namespace dom {

public struct Document {
    public func querySelector(&self, selector : string_view) : Option<Element>
    public func querySelectorAll(&self, selector : string_view) : Option<[]Element>   // v1.1
    public func getElementById(&self, id : string_view) : Option<Element>
    public func createElement(&self, tag : string_view) : Element
    public func body(&self) : Element
    public func activeElement(&self) : Option<Element>
    public func title(&self) : string
    public func addEventListener(&self, event : string_view, handler : (e : &Event) => void)
    public func removeEventListener(&self, event : string_view, handler : (e : &Event) => void)
}

public struct Window { /* setTimeout, clearTimeout, requestAnimationFrame,
                          cancelAnimationFrame, innerWidth, innerHeight,
                          scrollY, addEventListener, removeEventListener */ }

public struct Storage { /* getItem, setItem, removeItem */ }
public struct Navigator { /* clipboard */ }
public struct Clipboard { /* writeText */ }

public func document() : Document
public func window() : Window
public func localStorage() : Storage
public func navigator() : Navigator

} // namespace dom
```

- Read-only properties (`activeElement`, `body`, `title`) are zero-arg accessors. The checker treats a member access to a zero-arg accessor as its return type.
- The raw `chx_dom_*` bindings and the JS host table are unchanged. Only Chemical names change.
- Update `dom_client`, docs and client Chemical code. The 196 JS components already use browser spelling and align.
- No `option host` in v1; `dom` is imported only on the client side.

### 6.2 v1 checks only direct members of ambient roots

The first review found that checking arbitrary chains would reject common corpus code (`document.body.classList`, `document.querySelector(...).textContent`, `dataset`, `style`, …) before `Element` is complete. v1 therefore checks:

```text
document.X        X is a direct member of Document   -> 002 if absent
window.X          X is a direct member of Window     -> 002 if absent
localStorage.X    X is a direct member of Storage    -> 002 if absent
navigator.X       X is a direct member of Navigator  -> 002 if absent
props.X           X is a declared prop               -> 001 if absent
```

Deeper chains (`document.body.classList`, `props.a.b`) are **counted** (005), not errors, in v1. This catches the target mistake (`document.somethingElse`) without a full `Element` surface. v1.1 adds chain checking, `Option<T>` unwrapping and the `Element`/`DOMRect`/`classList`/`style` surface.

### 6.3 Root coverage

The direct members the corpus uses on the roots must be declared before the corpus is expected to be clean:

- `Document`: `querySelector`, `querySelectorAll`, `getElementById`, `createElement`, `body`, `activeElement`, `title`, `documentElement`, `addEventListener`, `removeEventListener`.
- `Window`: `setTimeout`, `clearTimeout`, `requestAnimationFrame`, `cancelAnimationFrame`, `innerWidth`, `innerHeight`, `scrollY`, `addEventListener`, `removeEventListener`.
- `Storage`: `getItem`, `setItem`, `removeItem`.
- `Navigator`: `clipboard`.
- `Clipboard`: `writeText`.

`window.$__uni_*` and `__reportError` are framework internals: the checker treats a member name starting with `$__uni_` or equal to `__reportError` as an untyped edge (005), **in `resolve_member` as well as `resolve_name`**. Do not add them to `Window`.

`Math`, `JSON`, `Array`, `Object`, `Promise`, `Date`, `console`, `undefined`, `NaN`, `parseInt`, `parseFloat`, `sessionStorage` are a small built-in/known-name table: known names, unknown types (their members are counted). Reuse `hydration_safety.ch`'s list where possible (it already includes `sessionStorage`).

### 6.4 Namespace flattening

Namespace members are flattened for the ambient table: a public zero-arg `document` anywhere in the provider's public namespace tree becomes the ambient `document`. The index must:

- gate each namespace's own specifier (internal namespaces do not leak);
- treat `dom::raw`'s `chx_dom_document` as a distinct name (it is not `document`, so no collision);
- report a same-module duplicate ambient name (two public `document` declarations in the same module) as a conflict, like a cross-module duplicate.

If flattening proves surprising, the follow-up is to require ambient declarations at the provider's top level; v1 documents the flattening rule.

---

## 7. Universal plugin: index, resolution, checks

### 7.1 Index

`lang/libs/universal_cbi/src/index/index.ch` (new):

```text
AmbientEntry {
    kind      : Function                  // v1: zero-arg public functions only
    name      : std::string
    provider  : std::string_view          // module scope for diagnostics
    type      : *mut BaseType             // return type
    decl      : *mut ASTNode
}
Index {
    ambient   : std::unordered_map<std::string, AmbientEntry>
    conflicts : std::vector<Conflict>     // ambient only
}
```

Built from the declarations passed to `IndexModule`, after recursing public namespaces:

- `FunctionDecl` with zero parameters (excluding an implicit `&self`) → an ambient root whose type is the return type. This is how `dom`'s `document()`, `window()`, `localStorage()` and `navigator()` become ambient names.
- `StructDecl`/`VariantDecl` are retained for member lookup (a root's type) but are **not** ambient names. Types are keyed by the full scope path + name, not just `module + name`, so two same-named types in different namespaces of one module do not collide.
- Top-level `VarInitStatement` is not indexed in v1 (F11). Add `getName`/`getType` CBI accessors before supporting ambient `var`s.
- Only public/protected declarations are indexed; `is_linkage_public`.
- **Any marked package may provide the ambient surface.** There is no canonical-provider allow-list: if a package writes `index_with "universal"` and declares a public zero-arg `document`, that package may provide `document`. The marker is the explicit consent. Conflicts are restricted to ambient names: if two marked packages (or two declarations in one package) provide the same ambient name, the build reports a conflict at the first component that resolves it, naming both modules. v1 has no `override` keyword; the project resolves the conflict by importing only one provider or renaming the declaration. A "nearer package wins" rule is **not** part of v1; it would be a later, explicit feature.

### 7.2 Per-job lifetime and the client-side gate

- `universal_job_begin(job)` clears the index and records:
  ```text
  is_client_job = job.getTarget().client || job.getTarget().js
  ```
  (`client` is the `--web` client side; `js` is `--emit-js`.) The server job is `server=true, client=false, js=false`.
- `universal_index_module(module, declarations)` appends.
- Nothing is retained across jobs. Pointers point into the current job's AST arena; top-level declarations and non-generic struct members are job-allocated, so they are safe to retain for the job. Do not retain function bodies.
- The body check early-returns when `!is_client_job`. The server job still generates SSR; it just does not run the browser-surface check.
- The index is immutable during component checks, so per-file parallel symres can read it without locking.

### 7.3 Resolution

The checker walks `signature.js_body` at symres, using the same node kinds as `hydration_safety.ch` plus the extra kinds listed in §7.5.

```text
resolve_name(id):
    if lexical env (locals → params → props) has id: return env.type(id)
    if id is a known JS built-in or a hook name: return unknown   // counted
    if index.ambient has id: return entry.type
    if id is in the closed host-name list and no provider:  WEB-BODY-008
    return unknown                                                 // counted, not an error

resolve_member(type, name, is_root):
    if type is a struct/variant:
        member = find_member(type, name)
        if member == null and is_root: return null                  // WEB-BODY-002
        if member == null: return unknown                           // deeper chain: 005
        return field type, or function return type (zero-arg accessor included)
    return unknown
```

- `props` is seeded from `signature.propsStruct`. Use `signature.propsName`, not the literal `props`. Run the check **after** the props resolution in `sym_res.ch` (the existing `propsStruct` assignment), not before.
- Locals: `var x = e` / `state x = e` get the type of `e` when `e` is a literal, `props.field`, a known ambient value, or another typed local. Anything else is unknown.
- Arrow parameters are unknown in v1 (e.g. `(e) => e.key`), so they produce counted edges, not errors.
- Only a **direct member of an ambient root** with a missing member is an error. This is what keeps v1 from rejecting real JS.
- `$__uni_*` / `__reportError` member names are exempt in both `resolve_name` and `resolve_member`.

### 7.4 Checks

| code | rule | v1 state |
|---|---|---|
| `WEB-BODY-001` | `props.X` not declared, with the declared list | implemented (same `is_implicit_jsx_prop_name` set as call sites) |
| `WEB-BODY-002` | direct member of an ambient root does not exist, with the closest names | implemented |
| `WEB-BODY-004` | client-only global in render | already shipped (`hydration_safety.ch`) |
| `WEB-BODY-005` | untyped edge | counted everywhere in v1; strict is a later phase |
| `WEB-BODY-006` | hook conditional/inside handler | implemented |
| `WEB-BODY-008` | closed host name with no provider | implemented |
| `WEB-BODY-003` | call arity/argument types | v1.1 |
| `WEB-BODY-007` | unknown native element attribute/event | v1.1 |

- Messages name the component, the component's file/line, the offending name, and the declared members or the closest matches. One report per component + name.
- `WEB-BODY-008` fires only for the closed host list (`document`, `window`, `localStorage`, `navigator`, `fetch`, `location`, `history`, `WebSocket`, `sessionStorage`, …). Every other unknown free identifier is a counted edge. This keeps `Math`, `console`, hooks and framework globals from becoming errors.
- `WEB-BODY-005` is counted in v1. The library's strict-clean claim waits for §6.3 coverage.
- Add the `WEB-BODY-*` codes to the same place `WEB-CLIENT-001` lives (`compiler/typeverify/TypeVerify.cpp` around `:5050`) if codes are centrally enumerated.

### 7.5 Additional checker mechanics

- `BaseDefMember` already exposes `getAccessSpecifier()` via `ASTNode`; use it to hide `Document.handle` (internal). Do not add a duplicate binding.
- Methods are in `getFunctions()`; use `FunctionDeclaration.getReturnType()` and `getParams()`. **Filter generated constructors/destructors** (`init`/`drop`) so they are not treated as members.
- "Zero-arg accessor" must exclude an implicit `&self` parameter; check the parameter list, not just `size() == 0`.
- Property writes (`el.textContent = …`, `window.__reportError = …`): recognized and skipped in v1, not checked.
- `${…}` property writes are not modeled as members; skip.
- Walk the extra `JsNodeKind`s the corpus uses (`ClassDecl`, `ArrayDestructuring`, etc.) and default to "unknown child" rather than silently skipping, so a new node kind cannot hide an expression.

---

## 8. Gates

| gate | what it proves |
|---|---|
| `scripts/web-index-test.sh` (new) | `index_with` parses; a marked namespace provider is indexed; `document.title` passes; `document.titel` fails 002; no provider → 008 only for host names; a user `struct Document` in an **unmarked** module does not satisfy `document`; a marked user provider does; duplicate ambient names report a conflict; an internal namespace does not leak |
| `scripts/web-body-check-test.sh` (new) | 001, 002, 006, 008 fire with the right component/prop/member; a clean typed component does not; `Math`/`console`/hooks/`$__uni_*` do not fire 002/008; a deeper chain (`document.body.classList`) is counted, not an error |
| `./scripts/test.sh --tcc --plugins` | the 1,322 plugin tests still pass |
| `./scripts/test.sh --tcc --universal` | the 196-component corpus and WebView tests still pass |
| `scripts/web-dom-test.sh`, `web-golden-test.sh`, `web-split-test.sh`, `js-backend-test.sh` | the `dom` rename and the two-job build are intact; `web-split-test` is **unchanged** in v1 |
| client/server isolation | the server job does not run the checker; the client job clears/reuses no index; `JobBegin` ordering is correct |
| `do_js_job` Library guard | `Library + --emit-js` fails with the explicit diagnostic, not "could not find main()" |
| LSP note | the plan documents that the editor does not run the check in v1 |

Plus the workflow gates from [`component-body-typing-workflow.md`](component-body-typing-workflow.md) §5:

- `scripts/check-cbi-enum-sync.sh` — the C++/Chemical enums match.
- `scripts/web-index-test.sh` — parse/round-trip, probe, namespace flattening, duplicates.
- `scripts/web-body-check-test.sh` — 001/002/006/008 negatives, 005 positives, built-ins, `$__uni_*`, dedup, messages.
- `scripts/web-two-side-test.sh` — server skips, client runs, index isolation, native-only build.
- `scripts/web-typing-perf.sh` — build/plugin/bundle/index measurements.
- `scripts/component-typing-audit.sh` — no DOM special-case in `compiler/`, no AST reads in backends.
- `lang/tests/universal_webview/src/tests_document.ch` — the allowed `document` surface works through SSR + hydration in a real WebView.
- `.github/workflows/universal.yml` — the WebView suite runs in CI with `xvfb` + GTK/WebKit.

A rule without a failing-without-it test is not done. Never weaken an existing test.

---

## 9. Budgets

| budget | limit | measured by |
|---|---|---|
| shipped type information | **0 B / 0 runtime** | a unique debug marker (not a grep for `document`, which appears in normal JS) or a compiler-memory measurement |
| client build time | ≤ +2% on `lang/libs/components` | before/after timing |
| plugin build time | ≤ +10% | cold universal_cbi build |
| non-LLVM compiler binary | under the 5 MB ceiling | release gate |
| bundle size | unchanged | existing golden/size checks |
| index memory | O(public declarations of marked modules), freed per job | build RSS |

The measurement method (cold/warm, plugin build, index-call count, peak RSS, failure action) is in the workflow doc §7. A budget miss is reverted or recorded here with a reason, never waived.

---

## 10. Risks and mitigations

| id | risk | mitigation |
|---|---|---|
| R1 | index built before member types are linked | dispatch after `sym_res_after_link_sig_file`, not during parsing |
| R2 | plugin index leaks across jobs | `JobBegin` clears it and records the side; a two-job gate |
| R3 | duplicate providers redefine `document` | any marked package may provide; conflicts only for ambient names; report at the component; no silent last-wins |
| R4 | incomplete `Element` causes false positives | v1 checks only direct members of ambient roots; deeper chains are counted |
| R5 | JS↔Chemical spelling drift | one spelling (browser) in the provider; no mapping table |
| R6 | checker and converter disagree about the supported subset | check during/after the same lowering; one negative test per construct |
| R7 | `index_with` in a `build.lab` is ignored | v1 supports only `chemical.mod`; the converter emits `addIndexer` |
| R8 | server job breaks from `import dom` | `components` uses `import dom if client`; the checker runs only on the client/JS job |
| R9 | `do_library_job` silently produces no library | reject `Library + --emit-js` with a clear diagnostic; a real JS-library mode is separate |
| R10 | cached plugin misses the new hooks | plugin cache key includes its build.lab / CBI ABI version; a cache hit without the hooks fails lookup |
| R11 | LSP shows no diagnostic | documented v1 limitation; wiring the LSP is a follow-up |
| R12 | `ModuleFileData`/`addIndexer` stale views | add `indexers` to `clear()`; `addIndexer` copies |

---

## 11. Phases

### Phase 0 — directive, metadata, JS library mode
- `index_with` parsing, `ModuleFileData.indexers` (+ `clear()`), `LabModule.indexers`, `Module.addIndexer` (copying), converter emission.
- Implement JS library mode (§5.4): `do_library_job` → `do_js_job(Library)`, root set instead of `main`, no `index.mjs`, `ship_files`, no `.so`.
- Fix `target_data.js` to exclude Interpretation.
- Gate: `web-index-test.sh` parse/round-trip cases; a `library` fixture imported from Node and run; no `.so`; Interpretation is not JS; existing suite green.

### Phase 1 — generic indexing hooks
- `IndexModule` + `JobBegin` at the end of both enums; typedefs; dispatch points; public-namespace/scope recursion with the namespace specifier gate.
- Register the hooks in `universal_cbi/build.lab`; add a debug dump under `-v` or a tiny indexer fixture.
- Gate: `check-cbi-enum-sync.sh`; `web-index-test.sh` probe; a marked namespace module is indexed exactly once per client job; an unmarked module is not; the server job does not carry an index into the client job.

### Phase 2 — client job reads the index (no DOM yet)
- `job_begin` records `is_client_job`; the body check early-returns on the server.
- A debug/test provider package indexed in a client fixture; `document` resolves if and only if the provider is imported.
- Gate: `web-two-side-test.sh` (server skips, client runs, index isolation, native-only build); `web-index-test.sh` 008 cases.
- **Checkpoint limitation:** the gate's component is in the client-only module. A `#universal` component in a **shared** module is currently compiled by both `--web` passes and fails (`symbol is not a component` plus an `@extern` storm in `cstd`). This is a pre-existing two-pass plugin/cache limitation, not a Phase 2 index bug. Phase 4 must fix it and convert the gate to a shared component before claiming the real library is checked.

### Phase 3 — browser-spelled `dom` and the narrow checker
- Rename the surface; add the root members in §6.3.
- `index.ch`, ambient table, `resolve_name`/`resolve_member`, 001/002/006/008, 005 counting.
- `components/chemical.mod` gains `import dom if client`.
- Gate: `web-body-check-test.sh`; `web-typing-perf.sh`; `tests_document.ch` under `--universal`; `--plugins` green; `document.titel` fails; `Math`/`console` do not; deeper chains are counted; determinism repeat.
- **Done when:** typed components report unknown direct members of `document`/`window`/`localStorage`/`navigator`, and no existing component fails for an unrelated reason.

### Phase 4 — server-side indexing (in scope)
- **Fix the two-pass universal component limitation**: a `#universal` component in a shared module must compile in both `--web` passes and render on the server. Convert the `web-two-side-test.sh` fixture to a shared component; it must pass before server-side indexing is claimed.
- Declarations-only modules, `host`, per-job `types_only`, object-cache bypass, consumer-side refusal in both C and LLVM emitters, `web-split-test` update.
- Gate: `web-types-only-test.sh`; `dom_in_server` still refused at the caller; no `chx_dom_*` in the server binary; server and client produce the same component diagnostics; the shared-component two-side gate is green.
- Required for the final state; the client-side Phases 2–3 are checkpoints.

### Phase 5 — coverage, chains and strict mode
- `Element`/`DOMRect`/`classList`/`style`/`dataset`; `querySelectorAll` arrays; `Option<T>` unwrapping for chains.
- **Strict library from the start of this phase**: `WEB-BODY-005` is an error for `lang/libs/components`/CI once the corpus scan is empty. A branch is not done until strict is green.

Every phase also runs the workflow gates in [`component-body-typing-workflow.md`](component-body-typing-workflow.md) §5, records a §9 row, and satisfies the workflow's §11 definition of done. The workflow's §4 holes **H1–H25** are assigned to these phases; none may be dropped without a written decision here.

---

## 12. Decision log

| decision | resolution |
|---|---|
| directive spelling | `index_with` (single identifier, no lexer change) |
| directive location | `chemical.mod`; `build.lab` only via generated `Module.addIndexer` |
| directive order | after the import block; documented in §3.1 |
| who owns the index | the plugin; the compiler only vends marked modules and declarations |
| declaration transfer | a transient `VecRef<ASTNode>`/`std::vector<void*>`; recurse public namespaces; no new module AST accessor |
| index lifetime | per job; `JobBegin` clears and records the side; no cross-job cache |
| scope | **all phases 0–5** in one branch; Phases 2–3 (client job) are checkpoints, Phase 4/5 complete the delivery |
| checking side | Phases 2–3: client/JS job only; Phase 4: server job also checks from declarations-only `dom` |
| provider selection | any marked package may provide an ambient name; the marker is the consent; no canonical allow-list |
| duplicate policy | conflicts only for ambient names (cross- or same-module); report at the component; no silent last-wins |
| browser spelling | rename `dom`; one spelling; no mapping table |
| globals | public zero-arg functions only initially; ambient `var` needs CBI accessors (add if the DOM surface needs it) |
| depth | Phase 3: direct members of ambient roots plus `props.X`; Phase 5: chains, `Option<T>`, `Element` |
| backend flag | implement **JS library mode** in `do_library_job`/`do_js_job`; exclude Interpretation from `target_data.js` |
| untyped edges | **strict library** in Phase 5: `WEB-BODY-005` is an error for `lang/libs/components`/CI once the corpus scan is empty |
| unknown-global rule | only a closed host-name list produces 008; everything else is counted |
| `.ext` / annotations | rejected; declarations stay Chemical |
| generic `lookup_type` | rejected; the index is the only ambient source |
| LSP | explicitly not wired in v1 |

---

## 13. Open questions for the next review

1. Zero-arg function vs. top-level `var` for ambient globals: the initial phases use functions because the CBI lacks `VarInitStatement` accessors; add those accessors if Phase 5's Element surface needs a var provider.
2. Is "any marked package may provide" plus duplicate-is-a-conflict enough, or does a later release need an explicit `override`/priority rule?
3. Should `IndexModule` carry an `ASTDiagnoser*` and module location for eager conflict reporting, or keep conflicts deferred to the first component (current choice)?
4. How much of the `JsNodeKind` set must be walked to avoid silently missing `document.x` inside `${}`/JSX containers (Phase 3 vs Phase 5)?
5. What is the JS-library barrel policy: per-module `.mjs` only (chosen) or an optional `index.mjs` generated from public exports? Per-module is the v1 choice; a barrel is additive.
