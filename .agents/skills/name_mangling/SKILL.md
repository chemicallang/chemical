---
name: Name Mangling
description: Comprehensive guide to the Chemical compiler's name mangling subsystem — how declarations get unique runtime symbol names, how scoped prefixes, module/package prefixes, generic instantiation suffixes, `@extern`/`@no_mangle`, interface/impl methods and vtables are mangled, and how the LLVM and C backends consume the result. Load when working on symbol naming, generic instantiation suffixes, C interop, vtable naming, or adding a new declaration kind that has a runtime footprint.
---

# Name Mangling

The name mangler is responsible for producing **unique runtime names** for every
declaration that has a runtime footprint (functions, structs, unions, variants,
interfaces, type aliases, globals). It is a small, standalone component —
`NameMangler` — used by both backends. There is no Itanium/Windows ABI mangling here:
names are flat C identifiers built from scope/container prefixes plus a small set of
suffixes.

## Overview — Why Mangling Exists

1. **Globally unique symbols.** Chemical has no ODR-style namespacing at the object
   level. Two `struct Point` in different modules or namespaces must map to two
   distinct C symbols, so the module and container path is embedded in the name.
2. **C interop.** All backends ultimately emit C (`2c` translation for TinyCC) or C-like
   LLVM IR symbols. The mangled name must be a valid C identifier, so it only uses
   alphanumerics, `_`, and a small set of reserved prefixes (`__cgs__`, `__cfg_`).
   `@extern`/`@no_mangle` symbols deliberately bypass mangling to bind to external C
   functions by their exact name.
3. **Generic monomorphization.** Each concrete instantiation of a generic function or
   container is a separate runtime object and needs a distinct suffix
   (`__cfg_N` / `__cgs__N`).
4. **No overloading by signature.** Chemical does not mangle parameter types. A
   duplicate name in the same scope is a symbol-resolution error, not an overload.
   Scoping (module + container) is what disambiguates same-named declarations.
5. **Interface / dynamic dispatch.** Interface implementations and vtables also need
   stable, unique names; these are built from the interface, the implementing type,
   and (for methods) the override relationship.

## Key Files

| File | Purpose |
|------|---------|
| `compiler/mangler/NameMangler.h` | `NameMangler` class declaration — the public API |
| `compiler/mangler/NameMangler.cpp` | Full implementation (algorithm, suffixes, containers, vtables) |
| `ast/base/ASTNode.cpp` | `get_node_identifier()` (which nodes have a name) and `set_no_mangle()` |
| `compiler/frontend/AnnotationController.cpp` | `@extern` / `@no_mangle` / `@cpp` annotation handlers |
| `ast/structures/ModuleScope.h` | `scope_name` (org) + `module_name` carried into mangling |
| `ast/structures/FileScope.h` | File boundary; `parent()` is the `ModuleScope` |
| `compiler/lab/LabModule.h` | Populates `ModuleScope.scope_name` / `module_name` |
| `compiler/generics/GenericInstantiator.cpp` | Assigns `generic_instantiation` indices |
| `ast/structures/Generic*Decl.cpp` | Each generic sets `impl->generic_instantiation` |
| `ast/structures/MembersContainer.h` / `FunctionDeclaration.h` / `Typealias.h` | Store `generic_parent` + `generic_instantiation` |
| `ast/types/FunctionType.h` | `isExtensionFn()` — extension function detection |
| `preprocess/2c/2cASTVisitor.h` | `ToCAstVisitor::mangle(...)` wrappers writing to the C `BufferedWriter` |
| `preprocess/2c/2cASTVisitor.cpp` | C translation call sites, vtable name emission, `mangle_linked` |
| `compiler/backend/LLVM.cpp` / `ast/structures/FunctionDecl.cpp` | LLVM call sites (`ScratchString<128>` + `gen.mangler.mangle(...)`) |
| `compiler/backend/CLANG.cpp` | Separate Clang native mangling (only for `@cpp` bridging, not this subsystem) |

## The `NameMangler` API

From `compiler/mangler/NameMangler.h:26`:

| Method | Purpose |
|--------|---------|
| `mangle(BufferedWriter&, ASTNode*)` | Mangle any node. Returns `false` if it has no runtime name |
| `mangle(BufferedWriter&, FunctionDeclaration*)` | Function-specific entry (handles interfaces/extension fns) |
| `mangle_non_func(BufferedWriter&, ASTNode*)` | Mangle a non-function node |
| `mangle_no_parent(BufferedWriter&, ASTNode*)` | Write only the node's own name (+ generic suffix), no parent |
| `mangle_func_parent(BufferedWriter&, FunctionDeclaration*, ASTNode*)` | Write a function's parent prefix (explicit parent) |
| `mangle_func_parent(BufferedWriter&, FunctionDeclaration*)` | Same, parent inferred from `decl->parent()` |
| `mangle_linked(BufferedWriter&, StructValue*)` | Mangle the linked struct/union of a struct value |
| `mangle_vtable_name(BufferedWriter&, InterfaceDefinition*, ExtendableMembersContainerNode*)` | Vtable symbol for a struct/variant impl |
| `mangle_vtable_name(BufferedWriter&, InterfaceDefinition*, BaseType*)` | Vtable symbol for a primitive impl |

The two backends wrap these:

```cpp
// C backend — preprocess/2c/2cASTVisitor.h:226
inline void mangle(ASTNode* node) { mangler.mangle(writer, node); }
inline void mangle(FunctionDeclaration* decl) { mangler.mangle(writer, decl); }

// LLVM backend — e.g. ast/structures/FunctionDecl.cpp:355
ScratchString<128> name_view;
gen.mangler.mangle(name_view, decl);
```

## The Mangling Algorithm

`NameMangler::mangle` dispatches on node kind (`NameMangler.cpp:398`). Functions go
through the function-specific path; everything else goes through `mangle_non_func`
(`NameMangler.cpp:168`). `mangle_non_func` only emits anything if
`node->get_node_identifier()` is non-empty (`ASTNode.cpp:335`); nodes such as `Scope`
or anonymous namespaces produce no identifier.

### Step 1 — Parent prefix (`write_mangle_parent_of`, `NameMangler.cpp:131`)

Given a node, look at `node->parent()`:

| Parent kind | What is written |
|-------------|-----------------|
| `FileScope` | The **module/package prefix** — unless `is_node_no_mangle(node)` |
| `FunctionDecl` | Nothing — local declarations have no runtime name |
| anything else | `mangle_non_func(parent)` recursively (container/namespace path) |

The module prefix comes from `write_mod_scope` (`NameMangler.cpp:118`):

```cpp
void write_mod_scope(BufferedWriter& stream, ModuleScope* mod) {
    if(!mod->scope_name.empty()) {      // scope_name = org/username
        stream << mod->scope_name;
        stream << '_';
    }
    stream << mod->module_name;         // module/repo name
    stream << '_';
}
```

So the prefix is `<scope>_<module>_` when a scope exists, otherwise `<module>_`.
`scope_name` is the GitHub org / username and `module_name` is the repo/module name
(`ast/structures/ModuleScope.h:15`, populated in `compiler/lab/LabModule.h:154`).

`write_mangle_parent_of` treats the `FileScope` case specially: `is_node_no_mangle(node)`
is checked for the **child** node (`NameMangler.cpp:97`), so a top-level `@no_mangle`
node skips the module prefix.

### Step 2 — Own name (`mangle_no_parent`, `NameMangler.cpp:64`)

| Node kind | Emitted |
|-----------|---------|
| `FunctionDecl` | function name, plus `__cfg_<N>` if `generic_instantiation != -1` |
| `StructDecl` / `UnionDecl` / `VariantDecl` / `InterfaceDecl` | `container_name()` — name plus `__cgs__<N>` if generic |
| `NamespaceDecl` | namespace name (anonymous namespaces emit nothing) |
| default | `node->get_node_identifier()` |

### Step 3 — Concatenation, no separators

The parent prefix and the node name are written **back to back** with no separator.
`mangle_non_func` (`NameMangler.cpp:168`):

```cpp
bool NameMangler::mangle_non_func(BufferedWriter& stream, ASTNode* node) {
    const auto id = node->get_node_identifier();
    if(!id.empty()) {
        write_mangle_parent_of(*this, stream, node);  // module + outer containers
        mangle_no_parent(stream, node);               // own name
        return true;
    }
    return false;
}
```

Namespaces do **not** add a trailing `_`, and container names are not separated from
their members. The only `_` characters in a non-function name come from the module
prefix. This is why `core::iterable::Iterable` becomes `core_coreiterableIterable`
(module `core_` + namespace `core` + namespace `iterable` + `Iterable`) — verified in
generated `lang/tests/build/chemical-lib-tests.dir/Translated.c`.

### Function path (`NameMangler::mangle`, `NameMangler.cpp:383`)

```cpp
void NameMangler::mangle(BufferedWriter& stream, FunctionDeclaration* decl) {
    if(!decl->is_no_mangle()) {
        if(decl->isExtensionFn()) {                       // first param type is the receiver
            const auto declParent = decl->params[0]->type->linked_node();
            if(declParent) {
                mangle_func_parent(stream, decl, declParent);
                mangle_no_parent(stream, decl);
                return;
            }
        }
        mangle_func_parent(stream, decl);                 // interface/struct/impl/file
    }
    mangle_no_parent(stream, decl);                       // name (+ __cfg_N)
}
```

`mangle_func_parent` (`NameMangler.cpp:284`) implements per-container rules:

| `decl->parent()` | Prefix written |
|------------------|----------------|
| `FileScope` | `<scope>_<module>_` unless `decl->is_no_mangle()` |
| `StructDecl` | `mangle_non_func(struct)` (module + struct name; static interface override handled) |
| `InterfaceDecl` | static interface → interface name; otherwise, if `interface->active_user` is set (impl context) → `<interface>_<user>_`; else interface name |
| `ImplDecl` | `<interface>_<impl-type>_` (impl for a container) or `<interface>_<primitive-type>_` (`mangle_impl_type`) |
| `FunctionDecl` | nothing (local function) |
| default | `mangle_non_func(parent)` |

### `mangle_impl_type` (`NameMangler.cpp:226`)

Used to name impl methods/vtables on primitive types (no container):

| Type | Emitted |
|------|---------|
| `int`, `u8`, `char`, ... | `int`, `u8`, `char`, ... (see `to_string`, `NameMangler.cpp:179`) |
| `float` / `double` / `bool` / `long double` | `float` / `double` / `bool` / `longdouble` |
| `string` / expressive string / `any` / `nullptr` | `str` / `expr_str` / `any` / `nullptr` |
| `*T` | `m` if mutable, then `p_`, then the inner type |
| `&T` | `m` if mutable, then `r_`, then the inner type |
| linked type | `mangle_till_file` (container path only, **no** module prefix) |

`mangle_till_file` (`NameMangler.cpp:151`) walks up from a linked node until the
`FileScope`/`FunctionDecl`, writing each node's own name — note it deliberately omits
the module prefix. So a primitive impl vtable for `*mut Point` yields `mp_Point`.

## Generic Instantiation Suffixes

Every generic instantiation carries a numeric index, assigned when the generic is
instantiated:

- **Functions** (`__cfg_N`): `GenericFuncDecl.cpp:91` sets
  `impl->generic_instantiation = (int) instantiations.size()`. `mangle_no_parent`
  appends `__cfg_<N>` (`NameMangler.cpp:69-72`). Note the suffix uses a **single**
  trailing underscore before the number: `__cfg_0`.
- **Containers** (`__cgs__N`): `GenericStructDecl.cpp:82`, `GenericUnionDecl.cpp:82`,
  `GenericInterfaceDecl.cpp:81`, `GenericVariantDecl.cpp:84`,
  `GenericTypeDecl.cpp:54`, `GenericImplDecl.cpp:81` set
  `impl->generic_instantiation = itr.first`. `container_name`
  (`NameMangler.cpp:22-30`) appends `__cgs__<N>` (double underscore around `cgs`):
  `__cgs__0`.

The index is stable only within a compilation; it depends on registration order in
the `InstantiationsContainer` (see the `generics` skill). `-1` means "not a generic
instance" and is the default (`MembersContainer.h:58`, `FunctionDeclaration.h:214`,
`Typealias.h:55`).

```chemical
func <T> identity(x : T) : T { return x }   // instantiation #0
var a = identity<int>(1)                     // C: main_identity__cfg_0

struct Box<T> { var value : T }              // instantiation #0
var b = Box<int> { value: 3 }                // C: struct main_Box__cgs__0
```

## Extern / `@no_mangle` Behaviour

Both annotations set the same `no_mangle` flag, but `@extern` additionally marks the
node as externally defined:

```cpp
// compiler/frontend/AnnotationController.cpp:186
void annot_handler_extern(Parser* parser, ASTNode* node, std::vector<Value*>& args) {
    if(!node->set_no_mangle(true)) { parser->error("couldn't make the node no_mangle"); }
    switch(node->kind()) {
        case ASTNodeKind::FunctionDecl:    node->as_function_unsafe()->set_extern(true); return;
        case ASTNodeKind::StructDecl:      node->as_struct_def_unsafe()->set_extern(true); return;
        case ASTNodeKind::InterfaceDecl:   node->as_interface_def_unsafe()->set_extern(true); return;
        case ASTNodeKind::VarInitStmt:     node->as_var_init_unsafe()->set_extern(true); return;
        default: return;
    }
}

// AnnotationController.cpp:84 — @no_mangle (and @export) only sets no_mangle
```

`set_no_mangle` is implemented per-kind in `ASTNode.cpp:545` (function, interface,
struct, union, variant, typealias, var). `@cpp` also sets `no_mangle` on the wrapped
function (`AnnotationController.cpp:284`).

`is_node_no_mangle` (`NameMangler.cpp:97`) enumerates which kinds can be no-mangle.
When true:

- Top-level nodes skip the module prefix (`write_mangle_parent_of` FileScope case,
  `NameMangler.cpp:136-139`).
- Functions skip **all** parent mangling and emit only their bare name
  (`NameMangler.cpp:384`).

**How `@extern` avoids collisions:** external symbols intentionally keep their exact C
name (e.g. `c_function`, `printf`). Chemical declarations normally always carry a
`<module>_` prefix, so a Chemical `c_function` in module `main` becomes
`main_c_function` and cannot clash with the external `c_function`.

> The LLVM backend has a related gotcha: external global variables must **not** carry
> `dso_local`, or linking fails with a mismatched-declaration error. See the
> `llvm_backend` skill and `ast/statements/VarInit.cpp`.

## Interfaces, Impls and Vtables

Interface method mangling is the most involved branch (`NameMangler.cpp:284`). It
exists because an interface method declaration and each concrete override must all get
unique names, while dynamic dispatch still needs the interface's own function symbol.

- **Interface-declared method** (no active impl user): mangled as
  `<interface><method>` — e.g. `main_Greetergreet`. This is the symbol a call through
  an interface dispatches on.
- **Impl method** where the interface is non-static and the impl is a `StructDecl`
  (`NameMangler.cpp:306`): `mangle_non_func(struct)` then the method name — e.g.
  `main_Persongreet`.
- **Impl declaration** (`ImplDecl`, `NameMangler.cpp:317`): if the impl target is a
  members container, it resolves the **base** function in the interface (following
  inheritance via `base_func->parent()`), then writes
  `<interface>_<impl-container>_`, then the method name — e.g.
  `main_Greeter_Person_greet`. If no base function is found (an extra method on the
  impl), it still writes `<interface>_<container>_`.
- **Static interfaces** (`interface->is_static()`) use the interface name directly as
  the function prefix. Serialization-style static interfaces (`core::stream::Stream`)
  behave this way.
- **Primitive impls** (no members container, `NameMangler.cpp:351`): writes the impl
  container parent, then `mangle_impl_type(impl type)`, then `_` and the method name.

**Vtable symbols** are emitted by `mangle_vtable_name` (`NameMangler.cpp:409`):

```cpp
void mangle_vtable_name(stream, InterfaceDefinition* interface, ExtendableMembersContainerNode* def) {
    mangle(stream, interface);   // e.g. main_Greeter
    mangle(stream, def);         // e.g. main_Person   (back to back, no separator)
}
```

That produces `main_Greetermain_Person` for `impl Greeter for Person`. For a primitive
impl (`mangle_vtable_name` with `BaseType*`, `NameMangler.cpp:414`) the second part is
`_` + `mangle_impl_type`. The C backend additionally wraps the vtable **type** as
`__chx_<mangled>_vt_t` (`preprocess/2c/2cASTVisitor.cpp:3024`), and the C backend
emits the vtable via `visitor.mangler.mangle_vtable_name(...)`
(`2cASTVisitor.cpp:684`, `:3145`). The LLVM backend calls the same API in
`InterfaceDefinition.cpp:241/268` and `compiler/backend/LLVM.cpp:1881`.

## Extension Functions

An extension function's receiver is its first parameter, so its "container" is the
**type of that parameter**, not its lexical parent. `NameMangler::mangle` detects this
with `FunctionType::isExtensionFn()` (`ast/types/FunctionType.h:96`) and uses
`decl->params[0]->type->linked_node()` as the parent (`NameMangler.cpp:386`). This
produces the same prefix a method would get:

```chemical
struct Point { var x : int }                       // module main, namespace geo
func (p : &geo::Point) scale(factor : int) : int   // C: main_geoPointscale
```

The receiver must be a reference (symres emits
`receiver in extension function must always be a reference` otherwise).

## Name Collision Handling / Disambiguation

There is no automatic numeric de-duplication beyond the generic suffixes. Uniqueness is
structural:

| Collision source | Disambiguator |
|------------------|---------------|
| Same name in different modules | module prefix `<scope>_<module>_` |
| Same name in different namespaces/containers | container path concatenated before the name |
| Multiple generic instantiations | `__cfg_N` / `__cgs__N` index |
| Same-named method on different containers | container name prefix |
| Interface method vs. impl override | `<interface><method>` vs `<interface>_<impl>_<method>` |
| External C symbol | `@extern`/`@no_mangle` pins the exact name |

A duplicate name **within the same scope** is rejected by symbol resolution, not
resolved by mangling (`compiler/symres/DeclareTopLevel.cpp:302` notes top-level
declaration does not itself check duplicates; the shadow-checking declaration path
handles it). Consequently:

- Chemical does **not** support parameter-type overloading: two `func foo(a : int)` and
  `func foo(a : float)` in the same scope is an error.
- `@no_mangle` on a method drops its container/module prefix, so two no-mangle methods
  with the same name in different types will collide. Use it only for genuine C entry
  points.

## How Mangling Interacts with Modules and Imports

Symbol resolution imports declarations from other modules by their AST identity, not by
their mangled name. Mangling only happens at codegen. Therefore:

- An imported function is code-generated in its **defining** module and keeps that
  module's prefix; the importing module references the same symbol.
- The module prefix always uses the **defining** module's `scope_name`/`module_name`,
  carried on its `ModuleScope` node (`ast/structures/ModuleScope.h:15`). Updating a
  module's names updates the scope node (`LabModule.h:167`).
- A module's own top-level namespace can repeat the module name (the standard library
  wraps code in `namespace std`/`namespace core`), yielding names like
  `std_stdResult__cgs__0` and `core_coreiterableIterable__cgs__0`.
- Only modules whose `specifier()` is `Public` are externally declared; the LLVM
  backend bails out for non-public declarations before mangling
  (`FunctionDecl.cpp:361-371`).

## Worked Examples (source-verified against generated C)

The following were confirmed by compiling a probe with
`cmake-build-debug/TCCCompiler` and inspecting the emitted C. The probe lived in a
single-file module named `main` (so `<module>_` = `main_`).

| Chemical declaration | Mangled runtime symbol |
|----------------------|------------------------|
| `func foo()` in module `main` | `main_foo` |
| `func foo()` in module `math`, scope `acme` | `acme_math_foo` |
| `namespace geo { func distance() }` | `main_geodistance` |
| `struct Point` inside `namespace geo` | `main_geoPoint` |
| `func Point.sum(&self)` | `main_geoPointsum` |
| extension `func (p : &Point) scale(...)` | `main_geoPointscale` |
| `func <T> identity(...)`, instance 0 | `main_identity__cfg_0` |
| `struct Box<T>`, instance 0 | `main_Box__cgs__0` |
| `std::Result<T,E>`, instances 0..N | `std_stdResult__cgs__0` ... |
| `core::iterable::Iterable<...>`, instance 0 | `core_coreiterableIterable__cgs__0` |
| interface method `Greeter.greet` (dispatch symbol) | `main_Greetergreet` |
| `impl Greeter for Person { func greet }` | `main_Greeter_Person_greet` |
| vtable for `Greeter`/`Person` | `main_Greetermain_Person` |
| primitive impl for `*mut Point` vtable part | `mp_Point` |
| `@extern func c_function(a : int)` | `c_function` |
| `@no_mangle func raw_name()` | `raw_name` |
| application entry `main` | `main` (auto `@no_mangle`, `ASTProcessor.cpp:633`) |

## Gotchas

1. **No separator between containers and members.** `struct Point { func sum }` →
   `Pointsum`, not `Point_sum`. Do not assume underscores; the only guaranteed `_` in
   a plain name comes from the module prefix. (The `c_codegen` skill's older
   `Namespace_foo` example is not what the current mangler emits.)
2. **`__cfg_N` vs `__cgs__N` differ.** Functions use `__cfg_` (single trailing `_`),
   containers use `__cgs__` (double). Mixing them up silently breaks symbol lookup.
3. **Generic index stability.** `N` is a registration-order index, not derived from the
   type arguments. It should be treated as opaque and only compared within one build.
4. **`@no_mangle` on a nested node does not strip outer prefixes.** Only the
   `FileScope` parent check consults `no_mangle`; nested containers still get their
   parent prefix (`NameMangler.cpp:131-149`). Functions are the exception: a
   `no_mangle` function emits only its bare name, dropping even its struct/interface
   prefix.
5. **`@no_mangle` includes `@export` and `@cpp`.** `@export` maps to the same handler
   (`AnnotationController.cpp:377`) and `@cpp` sets `no_mangle` too (`:285`).
6. **The main entry point is force-mangled off.** During module processing the compiler
   finds `main` and calls `set_no_mangle(true)` so it links as the C entry point
   (`compiler/ASTProcessor.cpp:633`, `LabBuildCompiler.cpp:3488`).
7. **`mangle_linked` uses the referenced type only as a fallback.** When a
   `StructValue`'s definition is not linked, it falls back to the raw struct/union type
   name without the container path (`NameMangler.cpp:49-61`).
8. **Anonymous namespaces emit nothing** in `mangle_no_parent`
   (`NameMangler.cpp:85`), so their children rely entirely on outer scope names — do not
   place uniquely-named declarations in anonymous namespaces expecting them to be
   discoverable.
9. **`get_node_identifier` returns an empty view for unsupported kinds**
   (`ASTNode.cpp:371`), and `mangle_non_func` returns `false` for them. A new node kind
   without an identifier entry will silently produce an empty symbol.
10. **LLVM external globals must not be `dso_local`** — a linking issue adjacent to
    `@extern` mangling (see `llvm_backend`).

## Checklist — Adding a New Declaration Kind with a Runtime Footprint

1. **Does it have a name?** Add a case to `ASTNode::get_node_identifier()`
   (`ast/base/ASTNode.cpp:335`) returning the node's name view, otherwise the mangler
   emits nothing.
2. **Can it be `@no_mangle`/`@extern`?** Add a case to `ASTNode::set_no_mangle()`
   (`ASTNode.cpp:545`) and to `is_node_no_mangle()` (`NameMangler.cpp:97`) if it should
   be able to bypass mangling. Add the storage field + getter/setter on the node's
   attrs (follow `StructDefinition.h`, `FunctionDeclaration.h`).
3. **Own-name emission.** Add a case to `NameMangler::mangle_no_parent`
   (`NameMangler.cpp:64`) if it needs a special suffix or a container-style name
   (`container_name`). Reuse `container_name` for generic containers so `__cgs__N` is
   applied.
4. **Parent prefix.** If it can be nested (namespace/struct/impl), decide how
   `write_mangle_parent_of`/`mangle_func_parent` should treat it. For methods, follow
   the `StructDecl`/`ImplDecl` cases.
5. **Generic instantiation.** If it is generic, add `generic_parent` +
   `generic_instantiation` fields (see `MembersContainer.h:53-58`) and set
   `impl->generic_instantiation` in the corresponding `Generic*Decl.cpp` `instantiate_*`
   method, mirroring `GenericStructDecl.cpp:81` / `GenericFuncDecl.cpp:90`.
6. **Dynamic dispatch / vtables.** If it generates a vtable, wire it through
   `mangle_vtable_name` in both backends (`2cASTVisitor.cpp:684`, `LLVM.cpp:1881`).
7. **Primitive impls.** If it can be an impl target for a primitive type, ensure
   `mangle_impl_type` (`NameMangler.cpp:226`) supports it, or add a case.
8. **`@extern` semantics.** If external linkage matters, add the `set_extern` case in
   `annot_handler_extern` (`AnnotationController.cpp:186`).
9. **Verify.** Compile a probe with `TCCCompiler -o out.c` and grep the emitted C for
   the expected symbol; then verify the LLVM backend still links (`./scripts/build.sh
   --llvm` and run the test suite). Do not add parameter-type mangling — Chemical
   resolves overloads by scope, not signature.

## Related Skills

- **Generics** (`.agents/skills/generics/SKILL.md`) — how `generic_instantiation`
  indices are assigned and how concrete instantiations are created.
- **Symbol Resolution** (`.agents/skills/symres/SKILL.md`) — how names are declared,
  scoped, and checked for duplicates before mangling.
- **C Codegen (2c)** (`.agents/skills/c_codegen/SKILL.md`) — where mangled names are
  written into the translated C.
- **LLVM Backend** (`.agents/skills/llvm_backend/SKILL.md`) — how mangled names become
  LLVM function/global symbols (including the `dso_local` external-declaration gotcha).
