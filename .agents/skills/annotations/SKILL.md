---
name: Annotations
description: Comprehensive guide to the Chemical compiler's annotation system (`@name`, `@name(args)`) — how annotations are lexed, parsed, registered in the `AnnotationController`, stored and queried centrally, consumed by the compiler and CBI plugins, and how to add a new one. Load when working on annotations, `AnnotationController`, compiler plugins that define/read annotations, or node attribute flags such as `@extern`, `@test`, `@make`, `@deprecated`, `@no_mangle`, `@inline`, and `@volatile`.
---

# Annotations

Annotations are the `@name` / `@name(args)` attribute syntax in Chemical source. They attach compiler directives to the declaration that follows them: `@extern`, `@test`, `@make`, `@deprecated`, `@no_mangle`, `@inline`, `@volatile`, and many more.

## Overview

### Syntax

```chemical
@extern public func printf(format : *char, _ : any...) : int

@test
public func my_test(env : &mut TestEnv) { ... }

@test.timeout(60000)
public func slow_test(env : &mut TestEnv) { ... }

@direct_init
public struct RGBA8 {
    var r : u8; var g : u8; var b : u8; var a : u8
    @make func make(r_ : u8, g_ : u8, b_ : u8, a_ : u8 = 255) : RGBA8 { ... }
}
```

- An annotation starts with `@` immediately followed by an identifier-start character.
- The name may contain letters, digits, `_`, `.`, and `:` (`lexer/Lexer.cpp:724` `read_annotation_id`). This is why names like `@inline.always`, `@compiler.interface`, `@size:opt`, and `@test.timeout` work — the whole dotted/colon name is one `TokenType::Annotation` token.
- Arguments are optional and parenthesised: `@align(8)`, `@test.timeout(60000)`. The argument list is parsed with `parseExpressionOrArrayOrStruct`, so arbitrary constant expressions and comma-separated lists are accepted (`parser/statements/AnnotationMacro.cpp:32-50`).
- An annotation applies to the **next declaration node** parsed; the parser buffers them in order and flushes them with `Parser::annotate(node)`.
- An unknown annotation name is a hard parse error: `unknown annotation found '@foo'` (`parser/statements/AnnotationMacro.cpp:27-30`).

### Where annotations can appear

`Parser::parseAnnotation` is invoked from the top-level statement loop (`parser/Parser.cpp:80`), the nested-statement loop (`parser/structures/Block.cpp:22`), and struct body parsing (`parser/structures/Struct.cpp:203,216`). Buffered annotations are consumed by `Parser::annotate(node)` at every declaration construction site:

| Declaration | `annotate()` call site |
|---|---|
| Function (`func`) | `parser/structures/Function.cpp:464` |
| Struct declaration | `parser/structures/Struct.cpp:62` |
| Struct member | `parser/structures/Struct.cpp:29` |
| Union declaration | `parser/structures/Union.cpp:17,59` |
| Variant declaration / member | `parser/structures/Variant.cpp:141,19` |
| Interface declaration | `parser/structures/Interface.cpp:25` |
| Enum declaration | `parser/structures/Enum.cpp:26` |
| Impl block | `parser/structures/ImplDef.cpp:23` |
| Namespace | `parser/structures/Namespace.cpp:17` |
| `var` / `const` statement | `parser/statements/VarInitialization.cpp:187` |
| `type` alias | `parser/statements/Typealias.cpp:139` |

There is **no static "valid parent" list enforced before parsing** — an annotation is applied to whatever node `annotate()` receives next. Each handler validates the node kind at runtime and calls `parser->error(...)` when the target is wrong (see the table below).

## Key Files

| File | Purpose |
|------|---------|
| `compiler/frontend/AnnotationController.h` | `AnnotationController`, `AnnotationDefinition`, `AnnotationDefType`, `SingleMarkerMultiplePolicy`, storage maps, creation/mark/collect/query APIs |
| `compiler/frontend/AnnotationController.cpp` | All intrinsic annotation handlers (`annot_handler_*`), built-in definitions table in `initialize()`, `handle_annotation`, `mark_single` |
| `parser/statements/AnnotationMacro.cpp` | `Parser::parseAnnotation` (token → buffered annotation) and `Parser::annotate` (apply to node) |
| `parser/Parser.h` | `SavedAnnotation` struct (annotation def + args), `annotations` buffer, `parseAnnotation`/`annotate` declarations |
| `lexer/Lexer.cpp` | `read_annotation_id` — allowed annotation-name characters; emits `TokenType::Annotation` |
| `lexer/TokenType.h` | `TokenType::Annotation` enum value (`lexer/TokenType.h:146`) |
| `ast/base/AnnotationKind.h` | **Empty** legacy enum (see "Legacy model") |
| `ast/base/Annotation.h`, `ast/base/Annotation.cpp` | **Legacy/unused** node-local annotation storage |
| `ast/base/AnnotationParent.h` | **Legacy/unused** container for node-local annotations |
| `ast/base/AnnotableNode.h` | **Legacy/unused** marker base class |
| `ast/base/ASTNode.h` | `set_no_mangle` / `set_deprecated` / `set_anonymous` virtual flags (`:225-237`), `isAnnotableNode` (`:601`) |
| `compiler/cbi/bindings/AnnotationController.cpp`, `AnnotationControllerCBI.h` | CBI exposure of the controller to compiler plugins |
| `lang/libs/compiler/src/AnnotationController.ch` | Chemical CBI interface used by **plugins** to read/mark/collect annotations |
| `lang/libs/lab/src/AnnotationController.ch` | Chemical CBI interface used by **build.lab scripts** to *create* annotation definitions |
| `ast/utils/GlobalFunctions.cpp` | Intrinsics that read the controller: `get_tests`, `get_single_marked_decl_ptr` (`:1575`, `:1623`) |

## The Active Model: `AnnotationController`

All annotation state lives in a single `AnnotationController` owned by `LabBuildCompiler`/`ASTProcessor` (`compiler/ASTProcessor.h:86`, `compiler/lab/LabBuildCompiler.cpp:207,262`). It is shared across all files and jobs, and because files are parsed in parallel, its mutating operations are guarded by mutexes (`single_marker_mutex`, `marker_mutex`, `collector_mutex` — `AnnotationController.h:153-165`).

### `AnnotationDefType` — the kind of definition

`AnnotationDefType` (`AnnotationController.h:23-47`) is the real "annotation kind" enum:

| `AnnotationDefType` | Meaning | Storage | Query API |
|---|---|---|---|
| `Handler` | Calls a C++ `handler(parser, node, args)` function that mutates the node | none | `handle_annotation` |
| `SingleMarker` | Exactly one node per build may carry this name; subsequent uses obey a `SingleMarkerMultiplePolicy` | `single_marked` map (`name → CollectedAnnotation`) | `get_single_marked(name)` |
| `Marker` | Marks any number of nodes; presence is the payload | `marked` map (`{node,name} → args`) | `is_marked(node,name)`, `get_args(node,name)` |
| `Collector` | Collects nodes into an indexed collection | `collections[id].nodes` | `get_collection(id)` |
| `MarkerAndCollector` | Does both Marker and Collector work | both | both |

`AnnotationDefinition` (`AnnotationController.h:75-94`) holds a `name`, a `type`, and a `union` that is read according to `type`:
- `handler` (function pointer) — for `Handler`
- `collection_id` (`std::size_t`) — for `Collector` / `MarkerAndCollector`
- `policy` (`SingleMarkerMultiplePolicy`) — for `SingleMarker`

Reading the union member that does not correspond to `type` is undefined — each definition is initialised with only its own member (see the aggregate/designated initialisers in `AnnotationController.cpp:364-419`).

`SingleMarkerMultiplePolicy` (`AnnotationController.h:49-53`): `Override` (last wins), `Ignore` (first wins), `Error` (a second use reports an error). See `AnnotationController::mark_single` (`AnnotationController.cpp:433-453`).

### Storage & query

There is **no per-node annotation vector in the active system**. The controller is the single source of truth:

```cpp
std::unordered_map<chem::string_view, AnnotationDefinition> definitions;   // name → def
std::vector<AnnotationCollection> collections;                              // Collector buckets
std::unordered_map<MarkedAnnotatedNode, std::vector<Value*>, ...> marked;   // (node,name) → args
std::unordered_map<chem::string_view, CollectedAnnotation> single_marked;   // name → {node,args}
```

(`AnnotationController.h:129-145`.) Key-based lookups use the composite `MarkedAnnotatedNode { ASTNode* ptr; chem::string_view sv; }` with a custom hash/equality (`AnnotationController.h:97-121`).

Query/creation APIs (`AnnotationController.h:206-280`):

| API | Purpose |
|---|---|
| `get_definition(name)` | Look up definition by exact name (nullptr if unknown) |
| `create_collector_annotation(name, expected)` | Register a `Collector` |
| `create_marker_annotation(name)` | Register a `Marker` |
| `create_marker_and_collector_annotation(name, expected)` | Register a `MarkerAndCollector` |
| `create_single_marker_annotation(name, policy)` | Register a `SingleMarker` |
| `is_marked(node, name)` | Presence check for a `Marker` |
| `get_args(node, name)` | Annotation arguments for a marked node (nullptr if absent) |
| `get_single_marked(name)` | `CollectedAnnotation*` for a `SingleMarker` |
| `get_collection(id)` | Collection bucket |
| `handle_annotation(def, parser, node, args)` | Dispatch by `AnnotationDefType` |
| `clear_marked_or_collected()` | Clear `marked`, `single_marked`, and all collections between jobs |
| `clear()` | Full reset: wipe definitions + marks, then `initialize()` again |

`handle_annotation` (`AnnotationController.cpp:455-477`) switches on the definition type and either invokes the handler, marks, collects, or errors.

### Legacy `AnnotationKind` / `AnnotationParent` model (do not use)

The task of tracking node-local annotations is **not** what the code does today. The following are dead/vestigial and must not be extended:

- `ast/base/AnnotationKind.h` declares `enum class AnnotationKind {};` — it is **empty** (`AnnotationKind.h:7-9`). There are no built-in enum kinds.
- `Annotation` / `AnnotationParent` (`ast/base/Annotation.h`, `Annotation.cpp`, `AnnotationParent.h`) implement a vector-of-annotations-on-a-node design with `add_annotation`, `get_annotation`, `has_annotation`, `get_all`. **No class inherits `AnnotationParent`** and no node stores `annotations` through it.
- `AnnotableNode` (`ast/base/AnnotableNode.h`) is an empty `ASTNode` subclass; nothing derives from it either. `ASTNode::isAnnotableNode` (`ASTNode.h:601-603`) still lists node categories (`UsingStmt`, `VarInitStmt`, `NamespaceDecl`, `isBaseDefMember`, `isFunctionDecl`, `isMembersContainer`) and `as_annotable_node` can cast, but no annotation logic reads it.
- Note a latent bug in the dead code: `Annotation::has_annotation` calls `get_annotation(kind)` instead of `get_annotation(expected)` (`Annotation.h:49-51`).

Use `AnnotationController` instead. The only "target model" is the runtime node-kind check inside each handler.

## Built-in Annotations

All built-ins are registered in `AnnotationController::initialize()` (`compiler/frontend/AnnotationController.cpp:364-419`). Names are matched **exactly** (`@test.id` ≠ `@test`).

### Handler annotations (mutate node flags)

| Annotation | Valid target(s) | Semantics | Handler (file:line) |
|---|---|---|---|
| `@inline` | function | `InlineStrategy::InlineHint` | `AnnotationController.cpp:20` |
| `@inline.always` | function | `InlineStrategy::AlwaysInline` | `AnnotationController.cpp:29` |
| `@noinline` / `@inline.no` | function | `InlineStrategy::NoInline` | `AnnotationController.cpp:38` |
| `@compiler.inline` | function | `InlineStrategy::CompilerInline` | `AnnotationController.cpp:47` |
| `@size:opt` | function | `InlineStrategy::OptSize` | `AnnotationController.cpp:56` |
| `@size:min` | function | `InlineStrategy::MinSize` | `AnnotationController.cpp:65` |
| `@compiler.interface` | interface | `set_extern(true)` + `set_is_static(true)` (`node->kind()==InterfaceDecl`) | `AnnotationController.cpp:74` |
| `@no_mangle` / `@export` | node with `set_no_mangle` | `node->set_no_mangle(true)` | `AnnotationController.cpp:84` |
| `@constructor` / `@make` | function | `func->set_constructor_fn(true)` | `AnnotationController.cpp:90` |
| `@delete` | function | `func->set_delete_fn(true)` (destructor) | `AnnotationController.cpp:99` |
| `@unsafe` | function | `func->set_unsafe(true)` | `AnnotationController.cpp:108` |
| `@stdcall` | function | `func->set_std_call(true)` | `AnnotationController.cpp:117` |
| `@dllimport` | function | `func->set_dll_import(true)` | `AnnotationController.cpp:126` |
| `@dllexport` | function | `func->set_dll_export(true)` | `AnnotationController.cpp:135` |
| `@no_init` | struct | `struct->set_no_init(true)` | `AnnotationController.cpp:144` |
| `@anonymous` | struct/union/variant | `node->set_anonymous(true)` | `AnnotationController.cpp:153` |
| `@const` | — | No-op (TODO, not implemented) | `AnnotationController.cpp:159` |
| `@retained` | function, struct, union, variant, interface | `set_is_body_retained(true)` (keeps body for reflection) | `AnnotationController.cpp:163` |
| `@extern` | function, struct, interface, `var` | `set_no_mangle(true)` then `set_extern(true)`; other kinds silently accepted | `AnnotationController.cpp:186` |
| `@implicit` | function | `func->set_implicit(true)` | `AnnotationController.cpp:208` |
| `@direct_init` | struct | `struct->set_direct_init(true)` | `AnnotationController.cpp:217` |
| `@thread_local` | top-level `var`/`const` | `varInit->set_thread_local(true)` | `AnnotationController.cpp:226` |
| `@volatile` | `var` or struct member | `set_volatile(true)`; rejects args and other targets | `AnnotationController.cpp:234` |
| `@maxalign` | struct or struct member | required alignment `16` (64-bit) / `8` (32-bit) | `AnnotationController.cpp:254` |
| `@no_return` | function | `func->set_noReturn(true)` (`noreturn`) | `AnnotationController.cpp:270` |
| `@cpp` | function | `set_cpp_mangle(true)` + `set_no_mangle(true)` (C++ mangle TODO) | `AnnotationController.cpp:279` |
| `@static` | interface | `interface->set_is_static(true)` | `AnnotationController.cpp:291` |
| `@deprecated` | node with `set_deprecated` | `node->set_deprecated(true)` | `AnnotationController.cpp:300` |
| `@align(N)` | struct or struct member | required alignment from first constant integer arg; errors if absent/zero | `AnnotationController.cpp:306` |
| `@allow_zeroed` | struct/union/variant | `container->allow_zeroed = true` via `get_master_members_container()` | `AnnotationController.cpp:327` |
| `@non_dyn` | interface | `interface->set_non_dynamic(true)` | `AnnotationController.cpp:336` |
| `@partial_instantiate` | generic type alias | `alias->generic_parent->is_partial_instantiate = true` | `AnnotationController.cpp:344` |
| `@never_destructed` | `var`/`const` | `varInit->set_never_destructed(true)` | `AnnotationController.cpp:356` |

Many flags are declared on the base `ASTNode` as virtuals and overridden per node: `set_deprecated` (`ASTNode.cpp:459`), `set_anonymous` (`ASTNode.cpp:517`), `set_no_mangle` (`ASTNode.cpp:545`).

### Test-related annotations

| Annotation | Type | Semantics | Source |
|---|---|---|---|
| `@test` | `Collector` | Collects the function into collection 0; read by `intrinsics::get_tests<TestFunction>()` | Registered in the inline constructor `AnnotationController.h:174`; consumed `GlobalFunctions.cpp:1674-1675` |
| `@test.before_each` | `SingleMarker` (Override) | One function run before each test | `AnnotationController.cpp:406`; consumed `lang/libs/test/src/runner.ch:85` |
| `@test.after_each` | `SingleMarker` (Override) | One function run after each test | `AnnotationController.cpp:407`; consumed `lang/libs/test/src/runner.ch:86` |
| `@test.id(N)` | `Marker` | Overrides numeric test ID | `AnnotationController.cpp:410`; consumed `GlobalFunctions.cpp:1695` |
| `@test.name("…")` | `Marker` | Overrides display name | `AnnotationController.cpp:411`; consumed `GlobalFunctions.cpp:1712` |
| `@test.group("…")` | `Marker` | Test group string | `AnnotationController.cpp:412`; consumed `GlobalFunctions.cpp:1720` |
| `@test.pass_on_crash` | `Marker` | Treat non-zero exit as pass on crash | `AnnotationController.cpp:413`; consumed `GlobalFunctions.cpp:1773` |
| `@test.ignore` | `Marker` | Reserved; registered but **not read** by the current test runner | `AnnotationController.cpp:414` |
| `@test.timeout(ms)` | `Marker` | Per-test timeout in ms (default 10000) | `AnnotationController.cpp:415`; consumed `GlobalFunctions.cpp:1741` |
| `@test.retry(N)` | `Marker` | Retry count (clamped 0…999999) | `AnnotationController.cpp:416`; consumed `GlobalFunctions.cpp:1750` |
| `@test.benchmark` | `Marker` | Run as a benchmark | `AnnotationController.cpp:417`; consumed `GlobalFunctions.cpp:1778` |

The `get_tests` intrinsic materialises the collected `@test` functions into an array of values (id, name, group, function pointer, file, timeout, retry, returns_bool, pass_on_crash, benchmark, source line/char) in `ast/utils/GlobalFunctions.cpp:1623-1796`.

### Plugin-defined annotations

CBI plugins register their own annotations from `build.lab`. Examples:

- `component` (marker) — `lang/libs/universal_cbi/build.lab:31`, `lang/libs/css_cbi/build.lab:30`, `lang/libs/js_cbi/build.lab:30`; read by `universal_cbi`/`html_cbi`/`css_cbi` `sym_res` passes.
- `serializable` (marker) — `lang/libs/json_cbi/build.lab:25`.

```chemical
// lang/libs/universal_cbi/build.lab
const controller = ctx.getAnnotationController()
controller.createMarkerAnnotation(std::string_view("component"))
```

Plugins then mark generated nodes at parse time and later test membership:

```chemical
// lang/libs/universal_cbi/src/react/macro.ch:107-117
const controller = parser.getAnnotationController()
const definition = controller.getDefinition("component")
...
controller.mark(node, definition, std::span<*mut Value>(args))

// lang/libs/universal_cbi/src/sym_res/sym_res_root.ch:41
if(!controller.isMarked(compNode, "component")) { ... }
```

## Parser Flow

1. The lexer emits a `TokenType::Annotation` whose `value` includes the leading `@` (`lexer/Lexer.cpp:861-868`).
2. `Parser::parseAnnotation` (`parser/statements/AnnotationMacro.cpp:19-52`):
   - strips the leading `@`, looks up the definition with `controller.get_definition(name_view)`,
   - errors `unknown annotation found '...'` if absent,
   - appends a `SavedAnnotation { AnnotationDefinition&, std::vector<Value*> }` (`parser/Parser.h:42-48`) to the parser's `annotations` buffer (`parser/Parser.h:411`),
   - if `(` follows, parses comma-separated argument expressions into `SavedAnnotation::arguments`.
3. When a declaration node is created, `Parser::annotate(node)` (`parser/statements/AnnotationMacro.cpp:54-60`) iterates the buffered annotations, calls `controller.handle_annotation(annot.definition, this, node, annot.arguments)`, then clears the buffer.

Because the buffer is flushed on the next declaration, annotations accumulate if you place several before one declaration:

```chemical
@inline
@no_mangle
func f() { ... }
```

## `@make` + `@direct_init` (Struct Initialization)

`@constructor` and `@make` run the **same handler** `annot_handler_constructor` (`AnnotationController.cpp:90`), setting `is_constructor_fn` on a **function** (`node->as_function()` — applying either to a non-function errors "couldn't make the function constructor"). `@make` is a synonym of `@constructor`; in practice the constructor function is named `make`, so `T.make(...)` resolves to it (`MembersContainer::default_constructor_func` requires `is_constructor_fn` + no params, `MembersContainer.cpp:617`). `@direct_init` applies to the **struct** and sets `is_direct_init` (`AnnotationController.cpp:217`).

The struct-literal syntax allowed depends on whether the struct has a constructor function (any `@constructor`/`@make` function, conventionally named `make`) and whether the struct has `@direct_init`:

| Constructor (`@make`/`@constructor` fn) | `@direct_init` on struct | `T{}` works? | `T.make()` works? | `T{field: val}` works? |
|---|---|---|---|---|
| Yes | only | Yes (all fields required) | n/a (no `@make`) | Yes (all fields required) |
| Yes | no | **No** | Yes | **No** |
| Yes | yes | Yes (all fields required) | Yes | Yes (all fields required) |
| No | any | Yes (all fields required) | No | Yes (all fields required) |

Critical rule: a struct with a constructor function but **without** `@direct_init` **cannot** use `{}` syntax at all — use `T.make(...)`. The compiler emits `struct with name '…' has a constructor, use @direct_init to allow direct initialization` (`compiler/symres/LinkSignature.cpp:501`, `compiler/symres/SymResLinkBody.cpp:3107`).

`@delete` (`AnnotationController.cpp:99`) marks the destructor function called on scope exit / `delete`; `struct->has_destructor()` (`StructDefinition.h:204`) returns true when a `@delete` function exists.

## CBI Plugin Access

The controller is exposed to CBI plugins through three entry points, all returning `AnnotationController*`:

| Accessor (C++ binding) | Chemical API | Scope |
|---|---|---|
| `ParsergetAnnotationController` (`compiler/cbi/bindings/ParserCBI.cpp:15`) | `Parser.getAnnotationController()` (`lang/libs/compiler/src/Parser.ch:9`) | parsing plugins (define + mark) |
| `SymbolResolvergetAnnotationController` (`compiler/cbi/bindings/SymbolResolverCBI.cpp:12`) | `SymbolResolver.getAnnotationController()` (`lang/libs/compiler/src/SymbolResolver.ch:4`) | symres plugins (read marks) |
| `BuildContextgetAnnotationController` (`compiler/cbi/bindings/BuildContextCBI.cpp:21`) | `BuildContext.getAnnotationController()` (`lang/libs/lab/src/lab.ch:138`) | `build.lab` scripts (create definitions) |

The CBI functions themselves live in `compiler/cbi/bindings/AnnotationController.cpp` (`extern "C"`), with prototypes in `AnnotationControllerCBI.h` and dispatch entries in `compiler/cbi/bindings/CBI.cpp:99-102,140,402`:

- `AnnotationControllergetDefinition`
- `AnnotationControllercreateSingleMarkerAnnotation` / `createMarkerAnnotation` / `createCollectorAnnotation` / `createMarkerAndCollectorAnnotation`
- `AnnotationControllermarkSingle` / `mark` / `collect` / `markAndCollect`
- `AnnotationControllerhandleAnnotation`
- `AnnotationControllerisMarked`

Arguments cross the CBI boundary as `ValueSpan*` and are unpacked with `take_chemical_values` (`AnnotationController.cpp:26-54`).

Plugin-side chemical interfaces:
- `lang/libs/lab/src/AnnotationController.ch` — creation API (`createSingleMarkerAnnotation`, `createMarkerAnnotation`, `createCollectorAnnotation`, `createMarkerAndCollectorAnnotation`).
- `lang/libs/compiler/src/AnnotationController.ch` — read/mark API (`getDefinition`, `markSingle`, `mark`, `collect`, `markAndCollect`, `handleAnnotation`, `isMarked`).

> Enum-sync warning: if you add or reorder an `AnnotationDefType` / `SingleMarkerMultiplePolicy` value, keep any exposed Chemical binding in sync. The general CBI rule (see AGENTS.md) is that enum values must match exactly between the C++ header and the `.ch` binding; mid-enum insertions shift all later values and cause crashes. The annotation controller currently exposes only these two enums by integer policy value (`createSingleMarkerAnnotation(..., policy : int)`), so update `SingleMarkerMultiplePolicy` consumers and the lab binding together.

## Adding a New Annotation

Checklist:

1. **Register the definition** in the `definitions` map inside `AnnotationController::initialize()` (`compiler/frontend/AnnotationController.cpp:364-419`).
   - Intrinsic flag: add `{ "my_annot", { annot_handler_my_annot, "my_annot", AnnotationDefType::Handler } }`.
   - Marker / Single marker / Collector: use the appropriate aggregate initialiser, or create it from a plugin's `build.lab`.
2. **Write the handler** (for `Handler` type) next to the others in `AnnotationController.cpp`. Validate `node->kind()` and call `parser->error(...)` on wrong targets. Arguments arrive in `std::vector<Value*>& args`; inspect `val_kind()` / `get_number()` / `get_the_string()` (see `annot_handler_align` at `:306` and `annot_handler_volatile` at `:234`).
3. **Add/confirm the node accessor** the handler calls (e.g. `set_my_flag` on the relevant node class — often on the base `ASTNode` virtuals at `ASTNode.h:225-237`, or on `StructDefinition`/`FunctionDeclaration`/`VarInit`).
4. **Consume the flag/mark** where it matters: symres, type verification, interpreter, LLVM backend (`compiler/backend/LLVM.cpp`), or C codegen (`preprocess/2c/`). For markers, read via `controller.is_marked(node, "name")`; for single markers `get_single_marked(name)`; for collectors `get_collection(id)`.
5. **Ensure `annotate()` runs for the target node.** If the annotation attaches to a new node type, add `annotate(node)` at that construction site (see the table above).
6. **Test** with an isolated `lang/compiled/temp.ch` or a `@test`-annotated function; a bad name is a parse error and a bad target is a `parser->error`.
7. **No enum change needed** for string-named annotations — names are strings, so adding `@my_annot` does not touch `AnnotationKind`. Only update CBI enum bindings if you add a new `AnnotationDefType`/policy value.

## Gotchas

- **Unknown annotation is fatal at parse time.** `@foobar` → `unknown annotation found '@foobar'` (`AnnotationMacro.cpp:27-30`). Register it first.
- **Name matching is exact and case-sensitive**, including punctuation. `@test` (collector) is distinct from `@test.id`, `@test.timeout`, etc.
- **Annotation names may contain `.` and `:`** because `read_annotation_id` explicitly allows them (`Lexer.cpp:724-739`); `@inline.always` and `@size:opt` are single tokens.
- **Buffered-until-next-node semantics.** An annotation with no following declaration is not applied; consecutive annotations all attach to the next declaration.
- **Target checks are per-handler, not a central schema.** `@volatile` errors unless applied to a `VarInitStmt` or `StructMember`; `@align` errors off structs/members; `@maxalign` rejects arguments. Always read the handler for the real rules.
- **`@extern` silently accepts some targets.** After `set_no_mangle`, the `switch` only handles `FunctionDecl`, `StructDecl`, `InterfaceDecl`, `VarInitStmt`; other node kinds fall through with no error (`AnnotationController.cpp:190-205`).
- **`@const` is a no-op** (`AnnotationController.cpp:159-161`) — do not rely on it.
- **The legacy `Annotation`/`AnnotationParent`/`AnnotableNode` classes are dead code.** Do not add annotation fields to nodes through `AnnotationParent`; the controller is authoritative.
- **State is cleared between jobs.** `LabBuildCompiler` calls `controller.clear_marked_or_collected()` after a job (`LabBuildCompiler.cpp:262`); marks/collections do not survive across compilation units.
- **Concurrency.** Files are parsed in parallel; use the controller's thread-safe `create_*` / `mark*` / `collect*` APIs rather than touching the private maps.
- **Plugin definitions are per-build.** `component` is registered in `build.lab`; if a plugin's `build.lab` did not run (e.g. missing dependency), `getDefinition("component")` returns null and marking is skipped (`universal_cbi/src/react/macro.ch:110-113`).

## Worked Examples

Simple flag annotation:

```chemical
@extern public func malloc(size : usize) : *mut void

@no_mangle
public func my_entry() : int { return 0 }

@volatile
var shared_flag : int = 0
```

Function strategies and constructor/destructor:

```chemical
@inline
func fast() : int { return 1 }

@direct_init
public struct RGBA8 {
    var r : u8; var g : u8; var b : u8; var a : u8
    @make func make(r_ : u8, g_ : u8, b_ : u8, a_ : u8 = 255) : RGBA8 { ... }
}

struct Widget {
    var handle : *mut void
    @constructor func Widget() { }
    @delete func delete(&mut self) { }
}
```

Test metadata (all markers/interceptors read by the test runner):

```chemical
@test
@test.id(1073741823)
@test.name("font_create_empty_works")
@test.group("font")
@test.timeout(60000)
public func font_create_empty_works(env : &mut TestEnv) { ... }

@test.before_each
func setup(env : &mut TestEnv) { ... }
```

Plugin defines and reads its own marker:

```chemical
// build.lab
ctx.getAnnotationController().createMarkerAnnotation(std::string_view("component"))

// parse/transform pass
const controller = parser.getAnnotationController()
const definition = controller.getDefinition("component")
controller.mark(node, definition, std::span<*mut Value>(args))

// later symres pass
if(!controller.isMarked(compNode, "component")) { /* not a component */ }
```
