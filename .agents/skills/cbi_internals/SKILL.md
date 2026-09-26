---
name: CBI Internals (Compiler Side)
description: The compiler-side machinery of the Chemical Compiler Binding Interface (CBI) — the CompilerBinder/CBIData/CBIFunctionIndex model, the full CBIFunctionType hook enum and where each fires, the extern "C" bindings dispatch tables in CBI.cpp, ASTBuilder/ASTCBI/PtrVec/BatchAllocator/SourceProvider/ASTDiagnoser wrappers, the TinyCC JIT bridge and LabBuildCompiler plugin compilation/indexing, the enum-sync rule, and how to add a new CBI binding. Load when modifying the CBI model/bindings layer, adding compiler hooks or bindings, debugging plugin ABI/symbol issues, or trying to understand how the compiler calls into Chemical plugin code.
---

# CBI Internals (Compiler Side)

This skill documents the **compiler-side machinery** of the Chemical Compiler Binding
Interface (CBI): the model classes that register and look up plugin hooks, the
`extern "C"` binding layer that marshals arguments across the Chemical↔C++ boundary, and
how `LabBuildCompiler` compiles a plugin's Chemical source through the 2c backend +
TinyCC, indexes its functions, and invokes them during compilation.

> **This is NOT a guide to authoring plugins.** For the plugin *authoring* API
> (writing `@no_mangle` hook functions, `build.lab` registration, ASTBuilder usage from
> Chemical, plugin templates, debugging plugins), read
> [`.agents/skills/cbi_plugin_api/SKILL.md`](../cbi_plugin_api/SKILL.md) first.
> This skill is the layer underneath it.

## Overview

CBI lets Chemical code hook into the compiler. At the compiler level:

1. A plugin is a Chemical module compiled to **C (2c backend)** and then **JIT-compiled by
   TinyCC** (`libtcc.so`) into a `TCCState`.
2. The build script (`build.lab`) registers plugin entry positions with
   `ctx.index_cbi_fn(...)` / `ctx.index_def_cbi_fn(...)`, which push a `CBIFunctionIndex`
   onto the CBI job (`BuildContextCBI.cpp:143`).
3. When the job is linked, `LabBuildCompiler::link_cbi_job` (`compiler/lab/LabBuildCompiler.cpp:1532`)
   relocates the TCC state, stores it in a `CompilerBinder`, and resolves each indexed
   function by name via `tcc_get_symbol`, registering it as a **hook**
   (`CompilerBinder::index_function`, `parser/CompilerBinder.cpp:20`).
4. During compilation the parser/symres/codegen call `CompilerBinder::findHook(key, type)`
   and invoke the returned function pointer. The function pointer is real JIT'd plugin
   machine code; its arguments are the compiler's live C++ objects.
5. The plugin calls back into the compiler through the **bindings layer**: a fixed set of
   `extern "C"` functions in `compiler/cbi/bindings/` that are `tcc_add_symbol`'d into the
   plugin's TCC state under their Chemical-mangled names.

The compiler uses **TinyCC JIT, not libffi**. All targets (`Compiler`, `TCCCompiler`,
`ChemicalLSP`) link `libtcc` dynamically; a plugin is ordinary C (translated from
Chemical), so hooks are direct C calls sharing the compiler's allocator/ABI. (LLVM ORC JIT
is a future plan; the binding model would stay the same — `compiler_bindings/SKILL.md`.)

### Static interfaces vs. dynamic hook functions

Two different binding styles coexist:

| Style | Target | Bound how |
|-------|--------|-----------|
| **Interfaces** (`@compiler.interface`) | `BuildContext`, `Lexer`, `Parser`, `SymbolResolver`, `ASTDiagnoser`, `ASTBuilder`, `PtrVec`, `BatchAllocator`, `AnnotationController`, `SourceProvider`, `TransformerContext` | Imported into a TCC state by name; the plugin calls *compiler* methods |
| **Hooks** (`CBIFunctionType`) | `ParseMacroNode`, `SymResNode`, `ReplacementNode`, ... | The compiler calls *plugin* functions, registered via `index_cbi_fn` and looked up with `findHook` |

The interfaces are what make the plugin able to *drive* the compiler (parse a macro body,
build AST, run symbol resolution). The hooks are what make the compiler able to *call*
the plugin at the right pipeline stage.

## Key Files

| File | Role |
|------|------|
| `compiler/cbi/model/CompilerBinder.h` | `CompilerBinder` — owns `CBIData` map, `hooks_`, `interface_maps`; `registerHook`/`findHook`/`store_cbi`/`index_function` |
| `compiler/cbi/model/CBIData.h` | `CBIData` — one `TCCState*` per named plugin |
| `compiler/cbi/model/CBIFunctionIndex.h` | `CBIFunctionIndex` — `{key, fn_name, fn_type}` queued by `index_cbi_fn` |
| `compiler/cbi/model/CBIFunctionType.h` | The hook enum (16 values) |
| `compiler/cbi/model/Model.h` | Hook C typedefs + `UserLexerGetNextToken` |
| `compiler/cbi/model/ASTBuilder.h` | Base `ASTBuilder` — arena allocator + `TypeBuilder` |
| `parser/CompilerBinder.cpp` | `CompilerBinder` methods: `import_compiler_interface`, `index_function` |
| `compiler/cbi/bindings/CBI.cpp` | All per-interface symbol maps + `prepare_cbi_maps` |
| `compiler/cbi/bindings/CBIUtils.h` | `ValueSpan`, `BaseTypeSpan`, `UbigintSpan`, `take_chemical_values` |
| `compiler/cbi/bindings/ASTBuilderCBI.{h,cpp}` | Every `ASTBuilder*` construct + all AST accessors |
| `compiler/cbi/bindings/ASTCBI.h` | `extern "C"` declarations of AST/Value/Type accessors |
| `compiler/cbi/bindings/PtrVecCBI.{h,cpp}` | `std::vector<void*>` wrapper (`compiler_PtrVec_*`) |
| `compiler/cbi/bindings/BatchAllocatorCBI.{h,cpp}` | Arena allocation wrapper |
| `compiler/cbi/bindings/SourceProviderCBI.{h,cpp}` | Raw source character/UTF-8 reader |
| `compiler/cbi/bindings/ASTDiagnoserCBI.{h,cpp}` | Error reporting bridge |
| `compiler/cbi/bindings/AnnotationController.{h,cpp}` | Annotation definition/mark/collect bridge |
| `compiler/cbi/bindings/SymbolResolverCBI.{h,cpp}` | SymbolResolver + SymResLinkBody + SymbolTable bridge |
| `compiler/cbi/bindings/ParserCBI.{h,cpp}` | Parser token access + re-entrant parse entrypoints |
| `compiler/cbi/bindings/LexerCBI.{h,cpp}` | Lexer file allocator + user-lexer push/pop |
| `compiler/cbi/bindings/BuildContextCBI.{h,cpp}` | Whole `build.lab` BuildContext API |
| `compiler/cbi/bindings/LabCBIAddons.{h,cpp}` | `LabModule`/`LabJob` getters & setters |
| `compiler/cbi/bindings/TransformerContextCBI.{h,cpp}` | Transformer API |
| `compiler/cbi/bindings/lsp/LSPHooks.{h,cpp}` | Semantic tokens + folding range analyzers |
| `server/cbi/hooks.h` | `EmbeddedSemanticTokensPut`, `EmbeddedFoldingRangesPut` typedefs |
| `compiler/lab/LabBuildCompiler.cpp` | `link_cbi_job` (compile/link/index), CBI job ordering, `run_transformer` |
| `integration/libtcc/LibTcc.cpp` | `setup_tcc_state`, `prepare_tcc_state_for_jit`, `compile_c_to_tcc_state` |
| `lang/libs/lab/src/lab.ch` | `CBIFunctionType` mirror + `BuildContext.index_cbi_fn`/`index_def_cbi_fn` |
| `lang/libs/compiler/` | Chemical-side interfaces/structs the bindings satisfy |
| `lang/libs/compiler_runtime/` | `@no_mangle` stubs so shared parser code links outside a CBI build |

## The Model Layer

### `CBIData` — one plugin, one TCC state

```cpp
// compiler/cbi/model/CBIData.h:8
struct CBIData {
    TCCState* module = nullptr;   // TCC module in which public functions are searched
};
```

`CompilerBinder` owns `util::unordered_string_map<CBIData> data` keyed by plugin name
(`CompilerBinder.h:42`). `store_cbi(name, state)` refuses to overwrite an existing name
(`CompilerBinder.h:88`) — this is why one plugin compiled once is reused for the whole
session, and why `build.lab` scripts guard with `ctx.contains_cbi` / `ctx.set_contains_cbi`.
`destroy_memory()` calls `tcc_delete` on every state; `~CompilerBinder` and `clear()` both
call it (`CompilerBinder.h:123-147`).

### `CBIFunctionIndex` — the registration record

```cpp
// compiler/cbi/model/CBIFunctionIndex.h:8
struct CBIFunctionIndex {
    chem::string key;        // the macro/tag name, e.g. "html"
    chem::string fn_name;    // @no_mangle C symbol, e.g. "html_parseMacroNode"
    CBIFunctionType fn_type;
};
```

`BuildContextindex_cbi_fn` (`compiler/cbi/bindings/BuildContextCBI.cpp:143`) appends one of
these to the CBI `LabJob::indexes`, but only for a `LabJobType::CBI` job and a
non-negative `func_type`. The Chemical convenience wrapper `index_def_cbi_fn`
(`lang/libs/lab/src/lab.ch:429`) passes `job.getName()` as the key, so a plugin named
`html` automatically registers under key `html`.

### `CompilerBinder` — hook registration + lookup

```cpp
// compiler/cbi/model/CompilerBinder.h:19
struct CBIFunctionKey {
    chem::string_view key;
    CBIFunctionType type;
    bool operator==(const CBIFunctionKey& o) const noexcept { return key == o.key && type == o.type; }
};
struct CBIFunctionHash {
    size_t operator()(const CBIFunctionKey& k) const noexcept {
        return std::hash<chem::string_view>{}(k.key) ^ (size_t(k.type) << 1);
    }
};
```

The hook store is a plain `std::unordered_map<CBIFunctionKey, void*, CBIFunctionHash>`
(`CompilerBinder.h:49`) plus a public `interface_maps`
(`CompilerBinder.h:55`):

```cpp
std::unordered_map<chem::string_view, std::span<const std::pair<chem::string_view, void*>>> interface_maps;
```

- `registerHook(type, key, fn)` → `hooks_.emplace({key,type}, fn)` (`CompilerBinder.h:102`).
- `findHook(key, type)` → map lookup, returns `nullptr` on miss (`CompilerBinder.h:110`).
- `index_function(index, state)` (`parser/CompilerBinder.cpp:20`) resolves
  `index.fn_name` via `tcc_get_symbol(state, index.fn_name.data())` and calls
  `registerHook`. If the symbol is missing it returns the message
  `"function with this name doesn't exist"`; `link_cbi_job` prints it and fails.
- `import_compiler_interface(interface, state)` (`parser/CompilerBinder.cpp:14`) loops the
  `{name, fnptr}` pairs and `tcc_add_symbol(state, name.data(), fnptr)`.
- `CompilerBinder::CompilerBinder()` (`parser/CompilerBinder.cpp:10`) calls
  `prepare_cbi_maps(interface_maps)`.

**`findHook` algorithm**: it is an O(1) hash lookup on the composite `(string_view key,
enum type)`. The `key` is the plugin name (for `InitializeLexer`, the `#macro` name; for
`ParseMacroNode`/`SymResNode`/`Replacement*`, the `EmbeddedNode::name`; for
`SemanticTokensPut`/`FoldingRangesPut`, the `#macro` name; for `TransformerMain`, the
literal `"transformer_main"`). Because `chem::string_view` is non-owning, the key string
must outlive the hook map — plugin names come from `CBIFunctionIndex`/`EmbeddedNode` whose
storage lives for the compilation session.

**Thread-safety**: `hooks_` and `interface_maps` have **no mutex**. This is safe by
construction: registration (`index_function`) happens during plugin linking, which is
performed **before** user modules are compiled and is single-threaded per binder; lookups
happen afterward during compilation, including from parallel per-file symres tasks (see
`SymResLinkBody::VisitEmbeddedNode` at `compiler/symres/SymResLinkBody.cpp:1746`). Treat
the maps as **write-once-then-read-only**. If you register a hook *after* parallel
compilation has started, that is a data race.

## `CBIFunctionType` — Full Hook Enum

Declared at `compiler/cbi/model/CBIFunctionType.h:5` and mirrored in Chemical at
`lang/libs/lab/src/lab.ch:96`. Each value maps to a C typedef in
`compiler/cbi/model/Model.h` (or `server/cbi/hooks.h`) and is invoked at a specific stage.

| # | `CBIFunctionType` | C typedef (file:line) | When it fires | Fire site |
|---|-------------------|-----------------------|---------------|-----------|
| 0 | `InitializeLexer` | `EmbeddedLexerInitializeFn` (`Model.h:36`) | Lexing: while scanning a `#macro` token, before returning it | `lexer/Lexer.cpp:881` |
| 1 | `ParseMacroValue` | `EmbeddedParseMacroValueFn` (`Model.h:64`) | Parsing: `#macro` used as a **value** (`var x = #css{...}`) | `parser/statements/AnnotationMacro.cpp:66` (`Parser::parseMacroValue`) |
| 2 | `ParseMacroNode` | `EmbeddedParseMacroNodeFn` (`Model.h:70`) | Parsing: `#macro` used as a **node** statement | `parser/statements/AnnotationMacro.cpp:106` (`Parser::parseMacroNode`) |
| 3 | `ParseMacroTopLevelNode` | `EmbeddedParseMacroNodeTopLevelFn` (`Model.h:76`) | Parsing: macro at **top-level** position (gets an `AccessSpecifier`) | `parser/statements/LexStatement.cpp:57,112` |
| 4 | `ParseMacroMemberNode` | `EmbeddedParseMacroNodeTopLevelFn` (`Model.h:76`) | Parsing: macro at **struct/container member** position | `parser/structures/Struct.cpp:140` |
| 5 | `SymResDeclareTopLevelNode` | `EmbeddedNodeSymResDeclareTopLevel` (`Model.h:81`) | Symres **Pass 1** (top-level declare): declares symbols a macro introduces | `compiler/symres/DeclareTopLevel.cpp:333` |
| 6 | `SymResLinkSignatureNode` | `EmbeddedNodeSymResLinkSignature` (`Model.h:86`) | Symres **Pass 2** (link signatures): macro's types/signatures | `compiler/symres/LinkSignature.cpp:512` |
| 7 | `SymResLinkSignatureValue` | `EmbeddedValueSymResLinkSignature` (`Model.h:91`) | Symres **Pass 2** for `EmbeddedValue` | `compiler/symres/LinkSignature.cpp:521` |
| 8 | `SymResNode` | `EmbeddedNodeSymbolResolveFunc` (`Model.h:97`) | Symres **Pass 6** (full body): resolves identifiers inside the macro | `compiler/symres/SymResLinkBody.cpp:1746` |
| 9 | `SymResValue` | `EmbeddedValueSymbolResolveFunc` (`Model.h:103`) | Symres **Pass 6** for `EmbeddedValue`; returns `bool` | `compiler/symres/SymResLinkBody.cpp:2209` |
| 10 | `ReplacementNodeDeclare` | `EmbeddedNodeReplacementFunc` (`Model.h:113`) | Codegen declare phase: node's declaration (forward decl) | `preprocess/2c/2cASTVisitor.cpp:2768`, `compiler/backend/LLVM.cpp:1450` |
| 11 | `ReplacementNode` | `EmbeddedNodeReplacementFunc` (`Model.h:113`) | Codegen: actual node body / pointer | `preprocess/2c/2cASTVisitor.cpp:7561`, `compiler/backend/LLVM.cpp:1459` (cached at `:1434`) |
| 12 | `ReplacementValue` | `EmbeddedValueReplacementFunc` (`Model.h:119`) | Codegen: value's replacement | `preprocess/2c/2cASTVisitor.cpp:7576`, `compiler/backend/LLVM.cpp:1483` |
| 13 | `SemanticTokensPut` | `EmbeddedSemanticTokensPut` (`server/cbi/hooks.h:15`) | LSP: emits semantic tokens for a macro region | `server/analyzers/SemanticTokensAnalyzer.cpp:323` |
| 14 | `FoldingRangesPut` | `EmbeddedFoldingRangesPut` (`server/cbi/hooks.h:20`) | LSP: emits folding ranges for a macro region | `server/analyzers/FoldingRangeAnalyzer.cpp:44` |
| 15 | `TransformerMain` | *no `Model.h` typedef*; cast to `int(*)(TransformerContext*, int, char**)` | Transformer run: entry point | `compiler/lab/LabBuildCompiler.cpp:4345` then `:4358` |

Stage summary:

- **Lexing**: `InitializeLexer`
- **Parsing**: `ParseMacroValue`, `ParseMacroNode`, `ParseMacroTopLevelNode`, `ParseMacroMemberNode`
- **Symbol resolution**: `SymResDeclareTopLevelNode` (pass 1), `SymResLinkSignatureNode`/`Value` (pass 2), `SymResNode`/`Value` (pass 6)
- **Codegen** (LLVM and 2c both): `ReplacementNodeDeclare`, `ReplacementNode`, `ReplacementValue`
- **LSP**: `SemanticTokensPut`, `FoldingRangesPut`
- **Transformer**: `TransformerMain`

`ParseMacroTopLevelNode` and `ParseMacroMemberNode` share the same C typedef — the only
difference is the `AccessSpecifier` passed in. `ReplacementNodeDeclare` and
`ReplacementNode` share `EmbeddedNodeReplacementFunc`.

The `findHook` result is invoked with a C-style cast to the matching typedef, e.g.
`((EmbeddedNodeSymbolResolveFunc) found)(this, node);` at `SymResLinkBody.cpp:1748`, or
built with an `ASTBuilder` whose allocator/typeBuilder come from the caller:
`ASTBuilder builder(&gen.allocator, gen.comptime_scope.typeBuilder);`
(`compiler/backend/LLVM.cpp:1413`).

## The Bindings Layer

### Dispatch tables: `{ Chemical name → C++ function }`

`prepare_cbi_maps` (`compiler/cbi/bindings/CBI.cpp:480`) registers named maps into
`CompilerBinder::interface_maps`:

| Interface name | Array (`CBI.cpp`) |
|----------------|-------------------|
| `SourceProvider` | `SourceProviderSymMap` (`:113`) |
| `BatchAllocator` | `BatchAllocatorSymMap` (`:94`) |
| `AnnotationController` | `AnnotationControllerSymMap` (`:98`) |
| `Lexer` | `LexerSymMap` (`:130`) |
| `Parser` | `ParserSymMap` (`:137`) |
| `BuildContext` | `BuildContextSymMap` (`:24`) |
| `ASTBuilder` | `ASTBuilderSymMap` (`:160`) |
| `PtrVec` | `PtrVecSymMap` (`:151`) |
| `SymbolResolver` | `SymbolResolverSymMap` (`:401`) |
| `ASTDiagnoser` | `ASTDiagnoserSymMap` (`:464`) |
| `TransformerContext` | `TransformerContextSymMap` (`:443`) |
| `LSPAnalyzers` | `LSPAnalyzersMap` (`:469`, `#ifdef LSP_BUILD`) |

Each entry is `{ "<chemical-mangled-name>", (void*) <C++fn> }`. A plugin only sees the
interfaces its module declared. A module requests interfaces via `chemical.mod`
`interface Foo` statements (converted by `ModToLabConverter` into
`ctx.add_compiler_interface(mod, "Foo")`, `ModToLabConverter.cpp:383`), or via
`build.lab`. `BuildContextadd_compiler_interface` (`BuildContextCBI.cpp:58`) looks up the
map and stores the span on `LabModule::compiler_interfaces`; `link_cbi_job` then imports
them (`LabBuildCompiler.cpp:1564`).

### Naming convention of `extern "C"` functions

The C++ symbols are named `<Type><member>` (or `<Type>_<free_function>` for PtrVec), e.g.:

- `LexergetFileAllocator`, `LexersetUserLexer`, `LexerunsetUserLexer` (`LexerCBI.h:13`)
- `ParsergetTokenPtr`, `ParserparseExpression` (`ParserCBI.h:33`)
- `ASTBuildermake_int_value`, `ASTBuildermake_embedded_node` (`ASTBuilderCBI.h:127`)
- `ASTNodegetKind`, `ASTNodechild`, `ValuegetKind`, `BaseTypegetKind` (`ASTCBI.h:54-70`)
- `PtrVec_get`, `PtrVec_push` (`PtrVecCBI.h:11`)
- `BatchAllocatorallocate_size` (`BatchAllocatorCBI.h:13`)

The map string is the **Chemical mangled name**, which for a type in the `compiler`
module is `compiler_` + the C++ symbol name (module-name prefix — see
`annot_handler_compiler_interface`, `compiler/frontend/AnnotationController.cpp:74-82`:
*"now we put module name prefix in the symbols we provide to the module using cbi"*).
Other prefixes: `lab_` (BuildContext/LabModule/LabJob/AnnotationController),
`transformer_` (TransformerContext), `ide_` (LSP analyzers). Note a few methods that add a
space-separated suffix live under a *different* C name than the map key suggests, e.g.
`FunctionCallNodeadd_generic_arg` proxies to the last chain call
(`ASTBuilderCBI.cpp:764`), and `ASTBuildermake_bitwise_not` is declared but not in the
`ASTBuilderSymMap` (it is reachable through the ASTBuilder interface only if listed).

There is **no automatic reflection-based binding**: adding a binding means adding the C
function, a declaration, and one map entry (see the checklist below). If a symbol is not
in the map, the plugin's TCC link fails with an undefined symbol.

### Crossing the boundary: arguments and returns

CBI functions use only C-compatible ABI-shaped types. The conventions:

- **Output string views** are written through a pointer:
  `void FunctionDeclarationgetName(chem::string_view* view, FunctionDeclaration* decl)`
  (`ASTCBI.h:214`). The plugin passes a `&std::string_view` (address of an uninitialized
  local) and the callee placement-constructs/writes it.
- **Slices/spans** use small POD structs with a raw pointer + `size`:
  `ValueSpan`/`BaseTypeSpan`/`UbigintSpan` (`CBIUtils.h:15-28`), `NodeSpan`
  (`ASTBuilderCBI.h:18`), `ModuleSpan`/`StringSpan`/`StringViewSpan`/
  `ModuleDependencyCBISpan`/`FileTokensSpanCBI` etc. (`BuildContextCBI.h:17`,
  `TransformerContextCBI.h:20`).
- **`take_chemical_values(std::vector<Value*>&, ValueSpan*)`**
  (`compiler/cbi/bindings/ASTBuilderCBI.cpp:105`) copies a raw `ValueSpan` into a
  `std::vector<Value*>` for the compiler's internal APIs. Used by
  `AnnotationController*` (`AnnotationController.cpp:27`). `take_chemical_nodes` is the
  `NodeSpan` analogue (`ASTBuilderCBI.cpp:116`).
- **Enums** cross as `int` (`spec`/`specifier`, `func_type`, `int type`) and are cast with
  `static_cast<...>` at the boundary (`LabCBIAddons.cpp:9`, `BuildContextCBI.cpp:145`).
- **Structs of flags** (e.g. `FuncDeclAttributesCBI`, `ASTCBI.h:14`) are packed field-by-field
  by `FunctionDeclarationgetAttributes`/`setAttributes`.
- **Location** is a `uint64_t` encoded `SourceLocation` (`.encoded`), obtained via
  `ParsergetEncodedLocation` (`ParserCBI.cpp:11`) or accessor functions.
- **`PtrVec`** wraps `std::vector<void*>` so Chemical can index/iterate compiler vectors:
  `PtrVec_get/_set/_push/_erase/_data/_size` (`PtrVecCBI.cpp:6-28`), exposed to Chemical
  as `compiler_PtrVec_*`.
- **Return conventions**: primitives by value; `bool` results are real `bool`; "optional
  pointer" returns `nullptr` (e.g. `AnnotationControllergetDefinition` may return null,
  `AnnotationController.cpp:6`). Functions returning a struct by value are lowered by the
  2c backend using the sret compound-expression pattern (see `c_codegen` skill), and at
  the CBI boundary the C++ implementation literally returns by value (e.g.
  `SymbolResolvergetJobBuilder` is `void`, writing into an out `ASTBuilder*`, because
  `ASTBuilder` is not trivially copyable across TCC: `SymbolResolverCBI.cpp:88`).

`libtcc` support lives in `integration/libtcc/LibTcc.cpp`: `setup_tcc_state` (`:164`) picks
`TCC_OUTPUT_MEMORY` for JIT, `prepare_tcc_state_for_jit` (`:214`) is a hook for heap
interop, and `compile_c_to_tcc_state` (`:227`) compiles a C string into a state.

## `ASTBuilder` Model + `ASTBuilderCBI`

The base model (`compiler/cbi/model/ASTBuilder.h:9`) is intentionally tiny:

```cpp
class ASTBuilder {
public:
    ASTAllocator* allocator;
    TypeBuilder& typeBuilder;
    template<typename T> FORCE_INLINE T* allocate() {
        static_assert(std::is_base_of<ASTAny, T>::value, "T must derived from ASTAny");
        return (T*) (void*) allocator->allocate_size(sizeof(T), alignof(T));
    }
};
```

Every plugin that receives a hook gets an `ASTBuilder` value constructed on the stack by
the caller (`ASTBuilder builder(&allocator, typeBuilder);` in
`parser/statements/AnnotationMacro.cpp:69` or `ASTBuilder builder(&gen.allocator,
gen.comptime_scope.typeBuilder)` in `LLVM.cpp:1413`). The builder's allocator determines
the AST node lifetime (file / module / job arena).

`ASTBuilderCBI` exposes the full construction surface. It is large; the categories
(`ASTBuilderCBI.h`):

- **Allocation/cleanup**: `ASTBuilderallocate_with_cleanup`, `ASTBuilderstore_cleanup`
  (`:25`). Used for objects that need a C++ destructor when the arena is cleared.
- **Embedded nodes/values**: `make_embedded_node` (`:29`), `make_top_level_embedded_node`
  (`:31`), `make_embedded_value` (`:33`). These are how a macro wraps the Chemical AST it
  built plus an opaque `data_ptr` and resolve/replacement callbacks. Implementation at
  `ASTBuilderCBI.cpp:127`, `:142`, `:158` — each also copies `NodeSpan`/`ValueSpan` into
  `chemical_nodes`/`chemical_values`.
- **Types**: `make_any_type`, `make_array_type`, `get_i8_type`…`get_u128_type`,
  `get_char_type`…`get_ulonglong_type`, `make_double_type`, `make_float_type`,
  `make_func_type`, `make_generic_type[_with_args]`, `make_linked_type`,
  `make_linked_value_type`, `make_literal_type`, `make_ptr_type`,
  `make_reference_type`, `make_string_type`, `make_void_type`, `make_dynamic_type`
  (`ASTBuilderCBI.h:35-89`).
- **Values**: `make_access_chain`, `make_value_wrapper`, `make_addr_of_value`,
  `make_reference_of_value`, `make_array_value`, all `make_*_value` scalar/`string`/
  `struct`/`null`/`sizeof`/`offsetof`/`expression`/`casted`/`dereference`/
  `function_call`/`index_op`/`is`/`lambda`/`captured_variable`/`block` constructors,
  `make_identifier`, `make_variant_case[_variable]`, `make_bitwise_not`,
  `make_negative_value`, `make_not_value` (`ASTBuilderCBI.h:91-177`).
- **Nodes/decls**: `make_access_chain_node`, `make_function_call_node`, every statement
  (`assignment`/`break`/`continue`/`destruct`/`return`/`typealias`/`using`/`varinit`),
  every structure (`scope`/loops/`enum_decl`+member/`function`+param/
  `generic_param`/`if_stmt`/`impl_def`/`interface_def`/`namespace`/`struct_def`+member/
  `union_def`/`unsafe_block`/`variant_def`+member+param), and the pattern-match pair
  (`ASTBuilderCBI.h:93-237`, impl `ASTBuilderCBI.cpp:718`).
- **Mutations**: `FunctionCalladd_generic_arg`, `FunctionDeclarationsetAttributes`,
  `FunctionDeclarationsetAccessSpecifier`, `StructDefinitionadd_member`/`add_function`,
  `EnumDeclarationadd_member`, `VariantMemberadd_param`,
  `IfStatementadd_else_body`/`add_else_if`, `BlockValuesetCalculatedValue`,
  `StructValueadd_value`, `PatternMatchExprset_expression`/`set_else_unreachable`,
  `PatternMatchExpradd_param_name` (`ASTCBI.h`).

**Important:** `make_pattern_match_node` (`ASTBuilderCBI.cpp:722`) moves the configured
fields from the temporary `PatternMatchExpr` into the node's embedded expression and
re-points each `PatternMatchIdentifier::matchExpr` at `&node->value`, because codegen keys
its `local_allocated` map by the `PatternMatchExpr` it visits (see the comment at
`ASTBuilderCBI.cpp:724-737`). Building a pattern-match in two shots without this transfer
produces identifiers that emit as bare C names.

On the Chemical side these are methods on the `@compiler.interface` struct `ASTBuilder`
(`lang/libs/compiler/src/ASTBuilder.ch:627`); helper structs for every AST kind are
declared in the same file (`PatternMatchExpr`, `PatternMatchExprNode`, `VariantCase`, …).

## `ASTCBI` / `ASTNode` Accessors

`ASTCBI.h` (implementation in `ASTBuilderCBI.cpp:618+`) lets Chemical code read and mutate
existing AST. Highlights:

- `ASTAnygetAnyKind`, `ValuegetKind`, `ASTNodegetKind`, `BaseTypegetKind`,
  `IntNTypeget_intn_type_kind` (`ASTCBI.h:54-72`, impl `ASTBuilderCBI.cpp:622-664`). All
  return `int`; the Chemical side compares against the mirrored `ASTNodeKind`/`ValueKind`/
  `BaseTypeKind` enums.
- `ASTNodegetParent`, `ASTNodechild(node, name)` (`ASTCBI.h:66-68`).
- Type navigation: `LinkedTypegetLinkedNode`, `GenericTypegetLinkedType`,
  `Pointer`/`Reference`/`Dynamic`/`LiteralTypegetChildType`, `ArrayTypegetElementType`/
  `getArraySize`, `TypealiasStatementgetActualType`, `FunctionTypeget_params`/
  `getReturnType` (`ASTCBI.h:74-94`).
- Containers return `std::vector<T*>*` so Chemical can wrap them in `PtrVec`:
  `ScopegetNodes`, `*get_body`, `Struct`/`Enum`/`VariantDefinitiongetMembers`/`getFunctions`,
  `AccessChainget_values`, `ArrayValueget_values`, `FunctionCallget_args`,
  `LambdaFunctionget_params`/`get_body` (`ASTCBI.h:96-230`).
- Names/attributes and generic declarations: `FunctionDeclarationgetName`,
  `*getAttributes`/`setAttributes`, `ASTNodegetAccessSpecifier`,
  `BaseGenericDeclgetGenericParams`, `Generic*DeclgetMasterImpl`,
  `GenericTypegetArgumentCount`/`getArgumentType`/`getArgumentLocation`,
  `GenericTypeParametergetName`/`getDefaultType` (`ASTCBI.h:214-275`).

Chemical declares the accessors as methods on `ASTNode`/`Value`/`BaseType` structs in
`lang/libs/compiler/src/ast/base/` (`ASTNode.ch:3-12`, `Value.ch:3-7`, `BaseType.ch:3`).
Those methods have no Chemical body; in a CBI build they resolve to the imported
`compiler_ASTNodegetKind` etc. symbols. Outside a CBI build,
`lang/libs/compiler_runtime/src/RuntimeSymRes.ch` supplies `@no_mangle` stubs (e.g.
`compiler_ASTNodegetKind` at `:139`) so shared parser code links. **Do not confuse these
stubs with the real implementations** — the real one is `ASTNodegetKind` in
`ASTBuilderCBI.cpp:644`, exposed under the map key `compiler_ASTNodegetKind`
(`CBI.cpp:287`).

## CBI Wrapper Classes

### `BatchAllocator`

The compiler's `Lexer` owns a `BatchAllocator` (`LexergetFileAllocator`,
`LexerCBI.cpp:6`). `BatchAllocatorallocate_size` (`BatchAllocatorCBI.cpp:6`) calls
`allocate_released_size(obj_size, alignment)`. Plugins use it to allocate AST nodes that
live in the file arena. The `ASTBuilder` methods already allocate through the builder's
`allocator`, so direct use is mostly for raw buffers/opaque `data_ptr`s.

### `PtrVec`

`PtrVec` is the bridge for `std::vector<void*>` (`PtrVecCBI.cpp`). Chemical's `PtrVec.ch`
(`lang/libs/compiler/src/PtrVec.ch`) declares the `@compiler.interface PtrVec` with
`_get/_set/_push/_erase/_data/_size`; the CBI exposes them as `compiler_PtrVec_*`
(`CBI.cpp:151`). Use this to walk `getMembers()`, `get_args()`, etc.

### `SourceProvider`

`SourceProviderCBI.cpp` exposes raw character reads and UTF-8 codepoint handling for
custom lexers: `increment`, `readCharacter`, `readCodePoint`, `utf8_decode_peek`,
`incrementCodepoint`, `eof`, `peek`, `increment(char)`, `getLineNumber`,
`getLineCharNumber`, `readWhitespaces`, `hasNewLine`, `readNewLineChars`,
`readWhitespacesAndNewLines` (`SourceProviderCBI.h:11-41`). The Chemical wrapper adds
higher-level helpers in `lang/libs/compiler/src/SourceProviderUtils.ch`.

### `Lexer` / user lexers

`LexerCBI` (`LexerCBI.cpp`) exposes `getFileAllocator`, `setUserLexer`,
`unsetUserLexer`, `getEmbeddedToken`. Plugins (html/js/css/md) implement a user lexer and
push it with `setUserLexer(instance, subroutine)`; the compiler's lexer calls the
subroutine via `UserLexerGetNextToken` (`Model.h:46-58`). `setUserLexer` saves the prior
one on a stack (`LexerCBI.cpp:10-17`), and `unsetUserLexer` pops it (`:19-30`).

### `Parser`

`ParserCBI` exposes token access and **re-entrant parsing** used by macro parsers:
`getTokenPtr`, `getEncodedLocation`, `getAnnotationController`, `getIs64Bit`,
`getParentNodePtr`, `getCurrentFilePath`, `parseExpression`,
`parseExpressionOrArrayOrStruct`, `parseNestedLevelStatement`, `parseType`, `error_at`
(`ParserCBI.h:33-51`). These delegate to `Parser::parseExpression(allocator, ...)` etc.,
so the plugin parses using the same parser/allocator as the compiler.

### `ASTDiagnoser`

`ASTDiagnosererror(diagnoser, msg, loc)` (`ASTDiagnoserCBI.cpp:6`) calls
`diagnoser->error(...)`. This is how macros report compile-time errors from replacement /
symres hooks. `ASTDiagnoserwarning(diagnoser, msg, loc)` calls `diagnoser->warn(...)` and
backs the Chemical-side `diagnoser.warning(...)`; a warning does **not** fail the build, so
negative tests assert it with `expect_compile_output_contains` (not
`expect_compile_error_*`). Both are registered in `CBI.cpp`'s `ASTDiagnoserSymMap`. The
`diagnoser` passed to replacement hooks is the **live codegen diagnoser**, not a throwaway
(see `Model.h:105-113` comment), so diagnostics surface through the normal pipeline.

### `AnnotationController`

`AnnotationController.cpp` bridges custom annotation definitions. Plugins create
definitions (`createSingleMarkerAnnotation`, `createMarkerAnnotation`,
`createCollectorAnnotation`, `createMarkerAndCollectorAnnotation`), mark/collect nodes
(`markSingle`, `mark`, `collect`, `markAndCollect`), and query state
(`isMarked`, `getDefinition`, `handleAnnotation`). `ValueSpan` args are materialized via
`take_chemical_values` (`:27`). The `@compiler.interface` annotation itself is handled by
`annot_handler_compiler_interface` (`compiler/frontend/AnnotationController.cpp:74`),
which marks interfaces extern+static so `verify_interface_implementation` skips them
(`compiler/typeverify/TypeVerify.cpp:1211-1214`).

### `TransformerContext`

`TransformerContextCBI.cpp` exposes the transformer API: `getTargetJob`, `parseTarget`,
`analyzeTarget` (runs `sym_res_module` + `type_verify_module_parallel`, `:29`),
`getFlattenedModules`, `getFileTokens`, `decodeLocation`, plus `LabModule`/`FileMetaData`
accessors. A transformer registers its entry as `transformer_main` with
`CBIFunctionType.TransformerMain` (`lang/libs/refgen/build.lab:16`); `run_transformer`
finds it (`LabBuildCompiler.cpp:4345`) and calls it with `(TransformerContext*, argc,
argv)` (`:4358`).

### LSP hooks

Compiled only `#ifdef LSP_BUILD` (`CBI.cpp:20`, `:468`). `LSPHooks.cpp` bridges
`SemanticTokensAnalyzer` and `FoldingRangeAnalyzer`. Their typedefs live in
`server/cbi/hooks.h` (not `Model.h`) and their interface map is `LSPAnalyzersMap`
(`CBI.cpp:469`), only registered for LSP builds (`CBI.cpp:493-495`). Both are also indexed
under `SemanticTokensPut`/`FoldingRangesPut`.

## Build-Side Integration

### CBI job creation and ordering

`build.lab` calls `ctx.build_cbi(name)` and `ctx.put_job_before(cbi, user_job)`
(`cbi_plugin_api/SKILL.md`). `LabBuildContext::build_cbi`
(`compiler/lab/LabBuildContext.cpp:214`) creates a `LabJob(LabJobType::CBI, ...)`.
CBI jobs are executed **before the final job**: `run_invocation` loops over `executables`
and does each `LabJobType::CBI` job (`LabBuildCompiler.cpp:3928-3935`), and
`run_transformer` does the same before running a transformer (`:4254-4262`). This ordering
guarantees all hooks are registered before user modules compile.

### Compiling a plugin: 2c + TinyCC

`process_modules` dispatches a CBI job to `process_job_tcc` (`LabBuildCompiler.h:345`,
TCC is used even on the LLVM build for CBI/JIT jobs — see `use_c`). `process_job_tcc`:
parses/symres/typechecks the plugin's Chemical sources through the normal passes
(`process_module_tcc_bm`), emits C via the `ToCAstVisitor`, compiles it to an object, and
then calls `link_cbi_job` (`LabBuildCompiler.cpp:1893`).

`link_cbi_job` (`LabBuildCompiler.cpp:1532`):

1. `setup_tcc_state(exe_path, "", jit=true, to_tcc_mode(options))` — creates the TCC state
   in JIT (`TCC_OUTPUT_MEMORY`) mode (`:1542`).
2. Adds the plugin's object files with `tcc_add_file` (`:1550`).
3. `prepare_tcc_state_for_jit` (`:1559`).
4. For each flattened dependency module, imports the compiler interfaces that module
   requested: `CompilerBinder::import_compiler_interface(interface, state)` (`:1564-1569`)
   → `tcc_add_symbol` for every `{name, fnptr}` in the interface map.
5. Adds the job's library search paths (`tcc_add_library_path`) and **link libraries**
   (`tcc_add_library`) to the state before relocating (`:1571-1585`).
   This is required for any plugin whose dependency graph declares `link "…"` in a
   `chemical.mod`: e.g. `refgen → fs → encoding → crypto → osrand`, and
   `osrand/chemical.mod` declares `link "bcrypt" if windows` and calls `BCryptGenRandom`.
   Without this step `tcc_relocate` aborts with
   `tcc: error: unresolved reference to 'BCryptGenRandom'` and
   `[lab] error: failed to relocate cbi '…'`. (The `build.lab` JIT path does the same at
   `:3163-3174`.)
6. `tcc_relocate(state)` (`:1586`).
7. `binder.store_cbi(cbiName, state)` (`:1595`) — ownership of the TCC state transfers to
   the binder (so it is eventually `tcc_delete`d exactly once).
8. For each `CBIFunctionIndex` in the job, `binder.index_function(index, state)` (`:1609`)
   resolves `fn_name` via `tcc_get_symbol` and registers it in `hooks_`.

The cached-job path also links CBI jobs (`:1724-1729`), so even a fully cached build
reproduces hook registration.

### The build.lab itself

`build.lab` is translated to C and JIT-compiled the same way
(`LabBuildCompiler::built_lab_file`, `compiler/lab/LabBuildCompiler.cpp:2697`/`:3264`; TCC
path at `:3119-3174`): setup state, `tcc_compile_string`, import required interfaces, add
library paths/libs, then `tcc_relocate`. The BuildContext interface
(`BuildContextSymMap`, `CBI.cpp:24`) is what the build script sees as `ctx`. Both
`build.lab` and plugins share `libtcc` and the same interface-map import mechanism.

### The Chemical-side interface

`lang/libs/lab/src/lab.ch` mirrors the C++ model:

- `CBIFunctionType` at `:96-113` (must match `CBIFunctionType.h`).
- `BuildContext.index_cbi_fn` at `:212` (the raw binder call).
- `index_def_cbi_fn` at `:429` (derives `key = job.getName()`).
- `BuildContext`, `Module`, `LabJob`, `LabJobCBI` are `@compiler.interface` interfaces
  (`:28`, `:76`, `:91`, `:135`).

`lang/libs/compiler/` declares the compiler API types (interfaces + `ASTBuilder` +
AST/value/type structs); `lang/libs/compiler/chemical.mod` lists the interfaces a module
can request (`SourceProvider`, `BatchAllocator`, `AnnotationController`, `Lexer`, `Parser`,
`ASTBuilder`, `PtrVec`, `SymbolResolver`, `ASTDiagnoser`). `lang/libs/compiler_runtime/`
imports `compiler` and provides `@no_mangle` stubs for non-CBI consumers.

## CRITICAL: Enum Sync Rule

**When you add a value to a C++ enum that is exposed to CBI, you MUST add the same value,
at the same position, to its Chemical mirror.** Mismatched ordering shifts every
subsequent value on the plugin side (the plugin compares/`switch`es on ints), taking wrong
branches and dereferencing garbage — typically a `SIGSEGV` deep inside plugin code.

| C++ enum (source of truth) | Chemical mirror |
|-----------------------------|-----------------|
| `ASTNodeKind` (`ast/base/ASTNodeKind.h:14`) | `lang/libs/compiler/src/ast/base/ASTNodeKind.ch:1` |
| `TokenType` (`lexer/TokenType.h:5`) | `lang/libs/compiler/src/ChemicalTokenType.ch:1` |
| `CBIFunctionType` (`compiler/cbi/model/CBIFunctionType.h:5`) | `lang/libs/lab/src/lab.ch:96` |
| `ValueKind` (`ast/base/ValueKind.h`) | `lang/libs/compiler/src/ast/base/ValueKind.ch` |
| `BaseTypeKind` | `lang/libs/compiler/src/ast/base/BaseTypeKind.ch` |
| `ASTAnyKind` | `lang/libs/compiler/src/ast/base/ASTAnyKind.ch` |
| `IntNTypeKind` (`ast/base/IntNTypeKind.h`? see `ASTBuilderCBI`/`ASTBuilder.ch:45`) | `lang/libs/compiler/src/ASTBuilder.ch:45` |

Rules:

- Values inserted **in the middle** break every value after them — sync immediately.
- Values appended at the **end** are safe for existing plugins (they never reference the
  new value) but still must be mirrored for new code.
- Both files must have identical declaration/initializer ordering (same explicit values,
  same implicit increments). `ASTNodeKind.h` and `ASTNodeKind.ch` currently match exactly
  through `InlineAsmStmt` (`ASTNodeKind.h:45` / `ASTNodeKind.ch:32`) and `UnresolvedDecl`
  (`:92` / `:79`). `TokenType.h` and `ChemicalTokenType.ch` match from `EndOfFile = 30000`
  through `ConstKw`/`WhereKw` and the `IndexKwStart/End` range helpers
  (`ChemicalTokenType.ch:241-242`).

### Real bugs caused by drift (from `AGENTS.md`)

- Adding `InlineAsmStmt` to `ASTNodeKind` in C++ (between `PlacementNewNode` and
  `EnumDecl`) without adding it to `ASTNodeKind.ch` made `EnumDecl` 31 in C++ but 32 in
  the plugin; `switch(node.getKind())` took wrong branches and crashed in `ASTNodegetKind`.
- Adding `AsmKw` to `TokenType` without adding it to `ChemicalTokenType.ch` shifted
  `RBrace` by 1, breaking all `#html` macro parsing.

## Gotchas

- **ABI/signature mismatches.** Hook typedefs in `Model.h` (`:36-119`) and
  `server/cbi/hooks.h` are the contract. A wrong signature still links at the machine level
  and corrupts the stack; the *C++ side* blindly casts `void*` to the typedef before
  calling (`SymResLinkBody.cpp:1748`).
- **`extern "C"` + name exactness.** The map key must exactly equal the Chemical-mangled
  symbol the plugin references. A typo means `tcc_get_symbol` returns null for hooks
  (`"function with this name doesn't exist"`) or the TCC link fails for bindings.
- **Enum drift** — see above. This is the #1 source of plugin crashes.
- **Lifetime of CBI pointers.** `ASTBuilder`-created nodes are arena allocated; file-arena
  nodes are cleared after each pass while module/job arenas live longer. An
  `EmbeddedNode::data_ptr` outliving its arena becomes dangling. The builder handed to a
  hook uses the *caller's* allocator, so match lifetime to stage.
- **`CompilerBinder::clear()`** resets `hooks_` *and* deletes all TCC states
  (`CompilerBinder.h:137`) — any cached hook pointer afterward is invalid.
- **`store_cbi` refuses duplicates** (`CompilerBinder.h:88-92`); stale plugins persist for
  the session unless the compiler is recreated, hence `--frecompile-plugins` sets
  `force_recompile_plugins` and disables CBI job caching (`LabBuildCompiler.cpp:1617`).
- **Thread-safety**: register hooks before parallel compilation, then only read them. Do
  not `registerHook` concurrently with `findHook`.
- **`#ifdef LSP_BUILD`**: `LSPAnalyzersMap` (`CBI.cpp:469`, registered at `:493`) exists
  only in LSP builds; `SemanticTokensPut`/`FoldingRangesPut` are only looked up there.
- **Non-trivial C++ by value.** Do not try to pass `std::vector`, `ASTBuilder`, or other
  non-POD C++ types by value across the boundary; use out-parameters (the builder getters
  are `void ... (ASTBuilder* out, ...)`, `SymbolResolverCBI.cpp:88-98`) or POD spans.
- **`ASTBuilder.allocator`** is `BatchAllocator*` in Chemical
  (`lang/libs/compiler/src/ASTBuilder.ch:631`) but `ASTAllocator*` in C++
  (`model/ASTBuilder.h:12`); `compiler_runtime` casts between them (`RuntimeSymRes.ch:150`).
- **`make_bitwise_not`** is declared in `ASTBuilderCBI.h:141` but is *not* in
  `ASTBuilderSymMap` (`CBI.cpp:160-399`). If a plugin needs it, add the map entry first.
- **`FunctionCallNodeget_args`/`add_generic_arg`** operate on the *last* value of the
  chain (`ASTBuilderCBI.cpp:760-767`), assuming the chain ends in a `FunctionCall`.

## Adding a New CBI Binding / Hook

### A. New compiler interface method (a binding the plugin calls)

1. **Implement the `extern "C"` function** in the right wrapper file, e.g.
   `compiler/cbi/bindings/ASTBuilderCBI.cpp` or `.../ParserCBI.cpp`. It must be
   C-compatible: POD args, no C++ default args, return by value or via out-param. Use
   `chem::string_view*`/spans for strings/slices.
2. **Declare it** in the matching header inside `extern "C" { ... }`
   (`ASTBuilderCBI.h`, `ParserCBI.h`, `SymbolResolverCBI.h`, …).
3. **Add a dispatch entry** to the relevant `*SymMap` in
   `compiler/cbi/bindings/CBI.cpp`, keyed by the Chemical-mangled name (usually
   `compiler_<Type><member>`, `lab_...`, `transformer_...`, `ide_...`). Forgetting this is
   the most common cause of "undefined symbol" at plugin link time.
4. **Declare the Chemical side** (method/struct field) in the appropriate file under
   `lang/libs/compiler/src/` (or `lang/libs/lab/src/lab.ch` for build context,
   `lang/libs/transformer/` for transformer). For a struct/interface type, add the
   `@compiler.interface` annotation / list it in `chemical.mod`.
5. If it returns a C++ collection, expose it as `<Type>get_values(...) :
   *mut VecRef<T>` returning `std::vector<T*>*` (see `ASTCBI.h:96-118`) and iterate it in
   Chemical via the `VecRef`/`PtrVec` helper.
6. **Test**: compile a plugin that calls the new method
   (`./scripts/test.sh --tcc --plugins`, or a standalone `build.lab`). Non-LSP hooks can be
   smoke-tested with `cmake-build-debug/TCCCompiler <mod> -o out -v -frecompile-plugins`.

### B. New compiler→plugin hook (`CBIFunctionType`)

1. **Add the enum value** to `compiler/cbi/model/CBIFunctionType.h` (append at the end
   unless you update every mirror).
2. **Mirror it** in `lang/libs/lab/src/lab.ch` `CBIFunctionType` at the same position
   (enum-sync rule; this enum is also exposed to `build.lab`).
3. **Define a typedef** in `compiler/cbi/model/Model.h` (or `server/cbi/hooks.h` for LSP)
   describing the exact ABI.
4. **Invoke it** at the right pipeline site with
   `binder.findHook(key, CBIFunctionType::YourHook)` and cast to the typedef, guarding for
   `nullptr` and reporting a diagnostic when missing (follow
   `SymResLinkBody.cpp:1745-1752`).
5. **Index it** from a plugin's `build.lab` via `ctx.index_def_cbi_fn(cbi, "<fn>", ...)`
   (or `ctx.index_cbi_fn` with an explicit key).
6. **Implement the plugin function** with `@no_mangle public func` and the exact typedef
   signature.
7. **Test**: add/extend a plugin under `lang/libs/<x>_cbi/` plus its test wiring and run
   `./scripts/test.sh --tcc --plugins`. Add a negative/ABI test if the hook signature is
   non-trivial.

### C. New compiler-invocable AST accessor (read-side)

1. Implement `int <Type>getKind(...)`-style function in `ASTBuilderCBI.cpp`.
2. Declare it in `ASTCBI.h`.
3. Add the `compiler_...` map entry to `ASTBuilderSymMap` (`CBI.cpp:160`).
4. Declare the Chemical method on the struct in `lang/libs/compiler/src/ast/...`.
5. If non-CBI code also links that struct, add a `@no_mangle` stub to
   `lang/libs/compiler_runtime/src/RuntimeSymRes.ch`.

## Related Skills

- **Compiler Plugin API (CBI)** (`.agents/skills/cbi_plugin_api/SKILL.md`) — the
  authoring-facing guide: plugin templates, `build.lab` registration, ASTBuilder from
  Chemical, built-in plugin inventory, plugin tests.
- **Compiler Bindings** (`.agents/skills/compiler_bindings/SKILL.md`) — high-level
  explanation of TinyCC bindings across build, compilation, and LSP.
- **Build System (LabBuildCompiler)** (`.agents/skills/build_system/SKILL.md`) — job
  types, `process_modules`, the 6 symres passes, and where hooks fire within them.
- **Compiler API** (`.agents/skills/compiler_api/SKILL.md`) — the Chemical-side compiler
  API types (`ASTBuilder.ch`, AST structs).
- **Macro Codegen** (`.agents/skills/macro_code_gen/SKILL.md`) — how existing plugins turn
  macros into generated code via replacement hooks.
