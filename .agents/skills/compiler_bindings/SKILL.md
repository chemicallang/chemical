---
name: Compiler Bindings
description: How Compiler Bindings work in Chemical, How compiler APIs are exposed for (build system, compilation and lsp)
---

# Bindings

By binding means compiler API exposed into Chemical source code, We don't mean intrinsic functions, chemical has those too, 
Those get called and interpreted, but bindings mean real functions in the compiler executable called from actual chemical source code,
We do not use libffi for this, We instead use Tiny CC. All chemical targets (Compiler, TCCCompiler, ChemicalLSP) contain libtcc
as a dependency.

> **Scope of this skill.** `compiler_bindings` is the *high-level map*: what a binding is,
> where it lives, the TinyCC mechanism, how the three binaries expose it, and the end-to-end
> registration flow. For the exhaustive, line-referenced compiler-side model
> (`CompilerBinder`/`CBIData`/`CBIFunctionIndex`, the full `CBIFunctionType` table, every
> dispatch map, the ABI marshalling helpers, and the "add a binding" checklist) read
> [`.agents/skills/cbi_internals/SKILL.md`](../cbi_internals/SKILL.md). For the
> plugin-authoring API (`build.lab`, `@no_mangle` hooks, ASTBuilder from Chemical) read
> [`.agents/skills/cbi_plugin_api/SKILL.md`](../cbi_plugin_api/SKILL.md).

## Bindings vs. intrinsics vs. hooks (one sentence each)

| Term | Direction | What it is |
|------|-----------|------------|
| **Intrinsic** | Chemical → compiler | A `intrinsics::*` function intercepted during symres/typecheck/codegen or the interpreter. Not a real C symbol call. See `intrinsics_compiler_reflection`. |
| **Binding** | plugin Chemical code → compiler executable | A real `extern "C"` function in the compiler, imported into the plugin's TCC state via `tcc_add_symbol`, called by name at runtime. |
| **Hook** | compiler → plugin Chemical code | A `@no_mangle` function in the plugin, resolved by `tcc_get_symbol` and invoked by the compiler at a pipeline stage (`CBIFunctionType`). |

A binding lets the plugin *drive* the compiler (parse a macro body, build AST, run symres);
a hook lets the compiler *call* the plugin. They are the two directions of the same CBI.

## CRITICAL: Enum Sync Rule

When adding a new enum value to a C++ enum that is exposed to CBI (compiler plugins), you MUST also add the same value to the corresponding Chemical binding file in `lang/libs/compiler/src/`. Failing to do so causes a SIGSEGV crash in all CBI plugins because the enum values are off by 1 between the C++ side and the TCC-compiled plugin side.

| C++ Enum File | Chemical Binding File |
|---------------|----------------------|
| `ast/base/ASTNodeKind.h` | `lang/libs/compiler/src/ast/base/ASTNodeKind.ch` |
| `lexer/TokenType.h` | `lang/libs/compiler/src/ChemicalTokenType.ch` |

Values inserted in the **middle** of an enum break all subsequent values. Values at the **end** can be added safely. Always verify both files have identical ordering.

The two tables above are the ones called out in `AGENTS.md` and are by far the most common
cause of crashes. In practice there are **more mirrored enums** (all of which must be kept in
sync): `CBIFunctionType` (`compiler/cbi/model/CBIFunctionType.h:5` → `lang/libs/lab/src/lab.ch:96`),
`ValueKind`, `BaseTypeKind`, `ASTAnyKind`, and `IntNTypeKind`. `cbi_internals` maintains the
full list.

### Real bugs caused by drift (from `AGENTS.md`)

- Adding `InlineAsmStmt` to `ASTNodeKind` in C++ (between `PlacementNewNode` and `EnumDecl`)
  without adding it to `ASTNodeKind.ch` made `EnumDecl` **31 in C++ but 32 in the plugin**.
  Every `switch(node.getKind())` took the wrong branch and SIGSEGV'd inside
  `ASTNodegetKind`.
- Adding `AsmKw` to `TokenType` without adding it to `ChemicalTokenType.ch` shifted `RBrace`
  by 1, breaking **all** `#html` macro parsing.

**Why it happens:** the plugin is TCC-compiled from Chemical, and the enum values it embeds
are read from the `.ch` mirror. A missing value shifts every later integer on the plugin
side, so comparisons against compiler-provided `int` kinds silently disagree.

### How to sync safely

1. Add the value in the **same position** in both files (don't reorder).
2. Prefer appending at the end when possible — existing plugins never reference a new value,
   so an append is backward compatible.
3. Verify explicit values and implicit increments match (`ASTNodeKind` matches through
   `InlineAsmStmt`/`UnresolvedDecl`; `TokenType`/`ChemicalTokenType` share the
   `EndOfFile = 30000` base).

## Key Files

### Compiler-side model (`compiler/cbi/model/`)

| File | Role |
|------|------|
| `compiler/cbi/model/CompilerBinder.h` | `CompilerBinder` — owns `data` (plugin name → `CBIData`), `hooks_` (indexed plugin functions), `interface_maps`; `store_cbi` (`:88`), `registerHook` (`:102`), `findHook` (`:110`), `clear`/`destroy_memory` (`:123`/`:137`), `~CompilerBinder` (`:145`) |
| `compiler/cbi/model/CBIData.h` | `CBIData` — one `TCCState* module` per named plugin (`:8`) |
| `compiler/cbi/model/CBIFunctionIndex.h` | `CBIFunctionIndex` — `{key, fn_name, fn_type}` record queued by `index_cbi_fn` (`:8`) |
| `compiler/cbi/model/CBIFunctionType.h` | The hook enum (`:5`), 16 values, mirrored in `lab.ch:96` |
| `compiler/cbi/model/Model.h` | The C hook typedefs (`EmbeddedLexerInitializeFn` `:36` … `EmbeddedValueReplacementFunc` `:119`) + `UserLexerGetNextToken` |
| `compiler/cbi/model/ASTBuilder.h` | Base `ASTBuilder` — `ASTAllocator* allocator` + `TypeBuilder& typeBuilder` + `allocate<T>()` (`:9`) |

### Compiler-side bindings (`compiler/cbi/bindings/`)

| File | Role |
|------|------|
| `compiler/cbi/bindings/CBI.cpp` | All per-interface dispatch tables (`BuildContextSymMap` `:24`, `BatchAllocatorSymMap` `:94`, `AnnotationControllerSymMap` `:98`, `SourceProviderSymMap` `:113`, `LexerSymMap` `:130`, `ParserSymMap` `:137`, `PtrVecSymMap` `:151`, `ASTBuilderSymMap` `:160`, `SymbolResolverSymMap` `:401`, `TransformerContextSymMap` `:443`, `ASTDiagnoserSymMap` `:464`, `LSPAnalyzersMap` `:469`) + `prepare_cbi_maps` (`:480`) |
| `compiler/cbi/bindings/CBI.h` | Declares `prepare_cbi_maps` (`:10`) |
| `compiler/cbi/bindings/CBIUtils.h` | `ValueSpan` (`:15`), `BaseTypeSpan` (`:20`), `UbigintSpan` (`:25`), `take_chemical_values` (`:30`) |
| `.../ASTBuilderCBI.{h,cpp}` | Every `ASTBuilder*` constructor + all AST accessors (impl `:552` `ASTBuildermake_function`, `:618+` accessors) |
| `.../ASTCBI.h` | `extern "C"` declarations of AST/Value/Type accessors |
| `.../ParserCBI.{h,cpp}` | Parser token access + re-entrant parse entrypoints (`ParsergetAnnotationController` `ParserCBI.cpp:15`) |
| `.../LexerCBI.{h,cpp}` | User-lexer push/pop + file allocator |
| `.../SymbolResolverCBI.{h,cpp}` | `SymbolResolver`/`SymResLinkBody`/`SymbolTable` bridge (out-param builders `:88-98`) |
| `.../PtrVecCBI.{h,cpp}` | `std::vector<void*>` wrapper (`PtrVec_get` `:6` … `PtrVec_size` `:28`) |
| `.../BatchAllocatorCBI.{h,cpp}` | Arena allocation wrapper |
| `.../SourceProviderCBI.{h,cpp}` | Raw char + UTF-8 reader for custom lexers |
| `.../ASTDiagnoserCBI.{h,cpp}` | Error reporting bridge |
| `.../AnnotationController{,CBI}.{h,cpp}` | Annotation definition/mark/collect bridge |
| `.../BuildContextCBI.{h,cpp}` | The whole `build.lab` BuildContext API (`add_compiler_interface` `:58`, `build_cbi` `:120`, `index_cbi_fn` `:143`) |
| `.../LabCBIAddons.{h,cpp}` | `LabModule`/`LabJob` getters & setters |
| `.../TransformerContextCBI.{h,cpp}` | Transformer API |
| `.../lsp/LSPHooks.{h,cpp}` | Semantic-token + folding-range analyzers (`#ifdef LSP_BUILD`) |

### Chemical-side binding sources

| File | Role |
|------|------|
| `lang/libs/compiler/chemical.mod` | Lists the interfaces a module may request: `SourceProvider`, `BatchAllocator`, `AnnotationController`, `Lexer`, `Parser`, `ASTBuilder`, `PtrVec`, `SymbolResolver`, `ASTDiagnoser` |
| `lang/libs/compiler/src/ASTBuilder.ch` | The `@compiler.interface ASTBuilder` (`:627`) and every `make_*`/`get_*_type` method (`make_function` `:886`) |
| `lang/libs/compiler/src/Parser.ch` | `@compiler.interface Parser` (`:2`) + `getAnnotationController` (`:9`), `parseExpression` (`:17`) |
| `lang/libs/compiler/src/Lexer.ch` | `Lexer` interface (user-lexer hooks) |
| `lang/libs/compiler/src/SymbolResolver.ch`, `SymResLinkBody.ch`, `SymResLinkSignature.ch`, `SymbolTable.ch` | Symres-side interfaces |
| `lang/libs/compiler/src/PtrVec.ch` | `PtrVec` interface + `VecRef<T>` convenience wrapper |
| `lang/libs/compiler/src/ASTDiagnoser.ch`, `BatchAllocator.ch`, `SourceProvider.ch`, `AnnotationController.ch` | Remaining interfaces |
| `lang/libs/compiler/src/ast/base/ASTNode.ch`, `Value.ch`, `BaseType.ch` | Body-less methods on AST structs that resolve to `compiler_ASTNodegetKind` etc. |
| `lang/libs/compiler/src/ast/base/*Kind.ch`, `ChemicalTokenType.ch` | The enum mirrors (see Enum Sync Rule) |
| `lang/libs/lab/src/lab.ch` | `CBIFunctionType` (`:96`), `BuildContext` (`:136`), `LabJob` (`:77`), `LabJobCBI` (`:92`), `index_cbi_fn` (`:212`), `index_def_cbi_fn` (`:429`) |
| `lang/libs/compiler_runtime/src/RuntimeSymRes.ch` | `@no_mangle` stubs (e.g. `compiler_ASTNodegetKind` `:139`) so non-CBI code links; **not** the real implementations |
| `lang/libs/lab/src/lab.ch` + `ModToLabConverter.cpp:383` | Generates `ctx.add_compiler_interface(mod, "Foo")` for each `interface Foo` in a `chemical.mod` |

### TinyCC integration

| File | Role |
|------|------|
| `integration/libtcc/LibTcc.cpp` | `tcc_new_state` (`:17`), `setup_tcc_state` (`:164`, picks `TCC_OUTPUT_MEMORY` for JIT), `prepare_tcc_state_for_jit` (`:214`), `compile_c_to_tcc_state` (`:227`), `tcc_link_objects` (`:342`) |
| `integration/libtcc/LibTccInteg.h` | Public declarations (`:26-51`) |
| `lib/tcc/libtcc.so` | The shared library every target links (`CMakeLists.txt:145-158`) |
| `parser/CompilerBinder.cpp` | `import_compiler_interface` (`:14`, `tcc_add_symbol`), `index_function` (`:20`, `tcc_get_symbol`) |

## Basic overview

- Compiler translates user code (chemical) to C
- Compiler compiles that C code in memory using Tiny CC
- Compiler symbols are added using `tcc_add_symbol`
- Functions can be called inside the compiled program

## Where does it happen ?

- During the build process
  - We translate user's `build.lab` or `chemical.mod` into C, then we JIT compile it using Tiny CC
  - A build API is exposed to `build.lab`, which user uses to explain his dependencies and so on...
- During the compilation process
  - Inside the `build.lab`, User can advertise plugins to the compiler, which can be used during the compilation process
  - For example, macro processors are used to handle certain macros.
  - A compiler API is exposed to the plugin, which includes hooks for lexing, parsing and semantic analysis...
- During the LSP process
  - We expose some functions to compiler plugins to also provide semantic highlighting and folding ranges.
  - The current API is limited but there are plans to expand it.

## Source Code

These files contain the heart of how bindings interact, inside the Compiler

- [CBI.cpp](/compiler/cbi/bindings/CBI.cpp)
    - This file contains all the functions with the exact names they are being exposed
- [LibTcc.cpp](/integration/libtcc/LibTcc.cpp)
    - Tiny CC code which compiles the C code
- [LabBuildCompiler.cpp](/compiler/lab/LabBuildCompiler.cpp)
    - Entire build system implementation, compilation process
    - Search within it `LabBuildCompiler::link_cbi_job` (the function that links a compiler plugin)
- [BuildContextCBI.cpp](/compiler/cbi/bindings/BuildContextCBI.cpp)
    - Build context implementation, this provides the build API
- [lab.ch](/lang/libs/lab/src/lab.ch)
    - BuildContext binding written in chemical source code that is exposed to user
- [CompilerBinder.h](/compiler/cbi/model/CompilerBinder.h)
    - Used as the handler for binding of functions into compiler plugins

> **Correction (kept for accuracy):** `CBI.cpp` does **not** contain the binding
> implementations. It contains the `{ Chemical-mangled-name → C++ function }` dispatch tables
> (`*SymMap`) plus `prepare_cbi_maps` (`CBI.cpp:480`). The actual `extern "C"` functions live
> in the sibling `*CBI.cpp` files (`ParserCBI.cpp`, `ASTBuilderCBI.cpp`, `LexerCBI.cpp`, …).

## The Full Mechanism

### 1. How a Chemical bindings file is compiled by TinyCC

A module declares the compiler interfaces it wants in its `chemical.mod`
(`interface Parser`, `interface ASTBuilder`, …). When the module is built (or when it is a
plugin's CBI job), the 2c backend emits C in which each interface method call is a plain C
function call to the **Chemical-mangled name** (e.g. `compiler_ParsergetAnnotationController`).
Because that symbol is undefined in the emitted C, TCC must be told about it before
relocating:

1. `CompilerBinder::CompilerBinder()` calls `prepare_cbi_maps(interface_maps)`
   (`parser/CompilerBinder.cpp:10`), which registers every `*SymMap` under its interface name.
2. When the module (`chemical.mod`) is lowered by `ModToLabConverter`, each `interface Foo`
   becomes `ctx.add_compiler_interface(mod, "Foo")` (`ModToLabConverter.cpp:383-386`).
   `BuildContextadd_compiler_interface` (`BuildContextCBI.cpp:58`) looks the name up in
   `binder.interface_maps` and appends the matching span to `module->compiler_interfaces`.
3. At link time, `link_cbi_job` walks every dependency module's `compiler_interfaces` and
   calls `CompilerBinder::import_compiler_interface(span, state)`
   (`LabBuildCompiler.cpp:1564-1569` → `parser/CompilerBinder.cpp:14`), which loops and does
   `tcc_add_symbol(state, name.data(), fnptr)` for each pair. Now the emitted C call resolves
   to the compiler's live C++ function.
4. `tcc_relocate(state)` patches the code; the state is stored via
   `binder.store_cbi(cbiName, state)` (`LabBuildCompiler.cpp:1572`/`:1581`) so it is deleted
   exactly once at session teardown.

`build.lab` itself is compiled the same way (`tcc_compile_string` at
`LabBuildCompiler.cpp:3121-3145`), then the compiler looks up its entry with
`tcc_get_symbol(state, "chemical_lab_build")` and calls it with a `LabBuildContext*`
(`LabBuildCompiler.cpp:3370`/`:3383`).

### 2. Symbol lookup and call

- **Plugin code calling a binding:** resolved at relocate time via `tcc_add_symbol` (above).
- **Compiler calling a plugin hook:** `link_cbi_job` iterates `cbiJob->indexes` and calls
  `binder.index_function(index, state)` (`LabBuildCompiler.cpp:1595`), which does
  `tcc_get_symbol(state, index.fn_name.data())` and `registerHook(index.fn_type, key, sym)`
  (`parser/CompilerBinder.cpp:20-26`). Later, `findHook(key, type)` is an O(1) hash lookup on
  `(chem::string_view key, CBIFunctionType)` (`CompilerBinder.h:110`).
- The returned `void*` is cast back to the correct C typedef from `Model.h`/`server/cbi/hooks.h`
  and invoked (e.g. `((EmbeddedNodeSymbolResolveFunc) found)(this, node)` at
  `compiler/symres/SymResLinkBody.cpp:1748`).

### 3. ABI and signature conventions (`extern "C"`, PtrVec, ValueSpan, sret)

Every binding is declared `extern "C"` (see the `extern "C" { ... }` blocks in
`ParserCBI.h:31`, `ASTBuilderCBI.h:17`) so names are unmangled and match the Chemical-mangled
string in the map. Types crossing the boundary must be C-compatible:

- **Strings/slices** are passed as small POD structs or out-params: `chem::string_view*` is
  written through a pointer (`void ParsergetCurrentFilePath(chem::string_view* view, Parser*)`,
  `ParserCBI.h:43`); slices use `ValueSpan`/`BaseTypeSpan`/`UbigintSpan`
  (`CBIUtils.h:15-28`) or `NodeSpan` (`ASTBuilderCBI.h:18`). `take_chemical_values`
  (`ASTBuilderCBI.cpp:105`) materializes a `ValueSpan*` back into `std::vector<Value*>`.
- **Enums** cross as `int` and are `static_cast` at the boundary
  (`BuildContextCBI.cpp:145`).
- **Collections** are exposed as raw `std::vector<T*>*` so Chemical can wrap them in
  `PtrVec` (`PtrVec_get/_set/_push/_erase/_data/_size`, `PtrVecCBI.cpp`). `PtrVec` is the
  bridge that lets Chemical index/iterate compiler vectors safely.
- **sret / non-trivial returns:** functions that return a C++ object which is not trivially
  copyable do **not** return by value. Instead they use an **out-parameter**:
  `void SymbolResolvergetFileBuilder(ASTBuilder* out_builder, SymbolResolver*)` placement-news
  the builder into the caller-provided slot (`SymbolResolverCBI.cpp:88-98`). For ordinary
  Chemical functions returning a struct by value, the 2c backend lowers the call with the
  `(*({ struct T __tmp; f(&__tmp, ...); &__tmp; }))` compound-expression pattern (see the
  `c_codegen` skill); at the CBI boundary this corresponds to the C++ side returning by value.
- **Location** is a `uint64_t` encoded `SourceLocation` (`ParsergetEncodedLocation`,
  `ParserCBI.cpp:11`).

### 4. Lifetime of pointers handed to plugins

- AST nodes created via `ASTBuilder` are allocated from the **builder's** `ASTAllocator*`
  (`model/ASTBuilder.h:12`); the builder handed to a hook is constructed with the *caller's*
  allocator (e.g. `ASTBuilder builder(&allocator, typeBuilder)` at
  `parser/statements/AnnotationMacro.cpp:69`, or the codegen gen allocator at
  `compiler/backend/LLVM.cpp:1413`). Match the arena to the pipeline stage.
- Compiler objects passed in (`Parser*`, `Lexer*`, `SymbolResolver*`, the live
  `ASTDiagnoser*`) are **borrowed references to live compiler state** — usable only for the
  duration of the call, never stored. An `EmbeddedNode::data_ptr` that outlives its arena
  becomes dangling.
- The plugin's TCC state owns its own JIT'd code and data; `CompilerBinder::clear()` deletes
  every TCC state (`CompilerBinder.h:137`), so any hook pointer cached after a clear is
  invalid.

## How the three binaries expose bindings

All three targets link `libtcc` (`CMakeLists.txt:145-158`) and include the full
`COMMON_SOURCES`, which contain `compiler/cbi/bindings/` and `parser/CompilerBinder.cpp`.
They differ only in which definitions change the surface:

| Binary | Target / definition | Interface surface exposed |
|--------|---------------------|---------------------------|
| `Compiler` | `core/targets/Compiler.cpp`, `-DCOMPILER_BUILD -DCLANG_LIBS -DLLD_LIBS` (`CMakeLists.txt:753`, `:761`) | All CBI maps **plus** LLVM backend. The LLVM codegen call-sites (`compiler/backend/LLVM.cpp:1450-1483`) fire `Replacement*` hooks, and `#ifdef COMPILER_BUILD` sections in `BuildContextCBI.cpp` expose the LLVM `ar` helpers. |
| `TCCCompiler` | same entry, `-DTCC_BUILD` (`CMakeLists.txt:757`, `:765`) | All CBI maps, no LLVM. `process_job_tcc`/`link_cbi_job` still compile plugins with TinyCC even during compilation. |
| `ChemicalLsp` | `core/targets/LSPMain.cpp`, `-DLSP_BUILD` (`CMakeLists.txt:665`, `:768`) | All CBI maps **plus** `LSPAnalyzersMap` (`CBI.cpp:469`, registered `:493-495`), `compiler/cbi/bindings/lsp/LSPHooks.cpp`, and the `#ifdef LSP_BUILD` block in `add_compiler_interface` (`BuildContextCBI.cpp:63-65`). |

Each creates a `CompilerBinder` and passes it into a `LabBuildCompiler`:
`CompilerBinder binder; LabBuildCompiler compiler(loc_man, binder, &opts, threadCount);`
(`core/main/CompilerMain.cpp:706-708` and `:788-790`). `ChemicalLsp`'s `WorkspaceManager`
owns a `binder` (`server/WorkspaceManager.h`) so LSP builds reuse compiled plugins.

## Registration flow: `build.lab` → `index_cbi_fn` → `CompilerBinder`

```
chemical.mod `interface Foo`
      │  ModToLabConverter.cpp:383
      ▼
build.lab  ctx.add_compiler_interface(mod, "Foo")
      │  BuildContextadd_compiler_interface  (BuildContextCBI.cpp:58)
      ▼
LabModule::compiler_interfaces  ← span of {name, fnptr} from CompilerBinder::interface_maps

build.lab  ctx.build_cbi("html")
      │  BuildContextbuild_cbi → LabBuildContext::build_cbi (LabBuildContext.cpp:211)
      ▼
LabJob(LabJobType::CBI)  ── process_job_tcc ──► 2c → C → TCC object

build.lab  ctx.index_def_cbi_fn(cbi, "html_parseMacroNode", CBIFunctionType.ParseMacroNode)
      │  lab.ch:212 index_cbi_fn → BuildContextindex_cbi_fn (BuildContextCBI.cpp:143)
      ▼
LabJob::indexes  +=  CBIFunctionIndex{"html", "html_parseMacroNode", ParseMacroNode}

link job  ──►  LabBuildCompiler::link_cbi_job (LabBuildCompiler.cpp:1532)
      ├─ setup_tcc_state(TCC_OUTPUT_MEMORY)                          (:1542)
      ├─ tcc_add_file for each object                               (:1550)
      ├─ prepare_tcc_state_for_jit                                  (:1559)
      ├─ import_compiler_interface for each dep interface           (:1564, tcc_add_symbol)
      ├─ tcc_relocate                                               (:1572)
      ├─ binder.store_cbi(cbiName, state)                           (:1581)
      └─ for each index: binder.index_function(index, state)        (:1595, tcc_get_symbol)
                          → registerHook(type, key, sym)  (CompilerBinder.h:102)

compilation  ──►  binder.findHook(key, type)  (CompilerBinder.h:110)
```

`index_def_cbi_fn` derives the key from the job name
(`ctx.index_cbi_fn(job, job.getName(), name, type)`, `lab.ch:429`), so a plugin named `html`
registers under key `html` automatically.

## Worked example: `ParsergetAnnotationController`

This is the binding a macro plugin uses to create/collect annotations (e.g. `#json`'s marker
annotation). End to end:

1. **C++ implementation** — `compiler/cbi/bindings/ParserCBI.cpp:15`
   ```cpp
   AnnotationController* ParsergetAnnotationController(Parser* parser) {
       return &parser->controller;
   }
   ```
2. **C declaration** — `compiler/cbi/bindings/ParserCBI.h:37`
   ```cpp
   extern "C" AnnotationController* ParsergetAnnotationController(Parser* parser);
   ```
3. **Dispatch map entry** — `compiler/cbi/bindings/CBI.cpp:140`, in `ParserSymMap`:
   ```cpp
   {"compiler_ParsergetAnnotationController", (void*) ParsergetAnnotationController},
   ```
   The map key is the **Chemical-mangled** name: module `compiler` prefix + `Parser` type +
   `getAnnotationController` member. `prepare_cbi_maps` registers this map under the interface
   name `"Parser"` (`CBI.cpp:486`).
4. **Chemical interface declaration** — `lang/libs/compiler/src/Parser.ch:9`:
   ```chemical
   @compiler.interface
   public interface Parser {
       func getAnnotationController(&self) : *mut AnnotationController
       ...
   }
   ```
5. **Requested by the plugin module** — `lang/libs/compiler/chemical.mod` lists
   `interface Parser`; `ModToLabConverter.cpp:383` turns that into
   `ctx.add_compiler_interface(mod, "Parser")`, and `link_cbi_job` imports the map's symbols
   into the plugin's TCC state.
6. **Called from plugin Chemical code** — e.g. `lang/libs/css_cbi/src/styled.ch` and
   `lang/libs/universal_cbi/src/react/macro.ch` do `const controller = parser.n()` where the
   method name resolves to `parser.getAnnotationController()`. Inside `#json`'s
   `build.lab`, `ctx.getAnnotationController()` is the `BuildContext` counterpart.
7. At runtime the plugin's C call resolves to the C++ function above, returning a pointer to
   the compiler's live `parser->controller`.

The same shape applies to `ASTBuildermake_function`
(`ASTBuilderCBI.cpp:552` / `.h:209` / `CBI.cpp:267` / `ASTBuilder.ch:886`): declaration →
implementation → map entry → Chemical interface → caller, with the `ASTBuilder*` receiver
supplied by the compiler (the hook's argument, or an out-param builder from
`SymbolResolvergetFileBuilder`).

## Exposed APIs

- [lab.ch](/lang/libs/lab/src/lab.ch)
    - BuildContext binding written in chemical source code that is exposed to user
- [compiler lib](/lang/libs/compiler)
    - Compiler API like Lexer, Parser, SymbolResolver are here
    - The most important file is [ASTBuilder](/lang/libs/compiler/src/ASTBuilder.ch) which allows to create chemical AST nodes, values and types
- LSP bindings
    - [ide lib](/lang/libs/ide/) FoldingRangeAnalyzer, SemanticTokenScopes, SemanticTokensAnalyzer and more...
    - [minlsp lib](/lang/libs/minlsp)
- [transformer lib](/lang/libs/transformer)
  - Transformer API helps transform chemical AST to other stuff
  - For example, we use transformer API in [refgen](/lang/libs/refgen) to create API documentation in HTML for Chemical

## The Compiler Lifecycle

This lifecycle takes into account all the processes mentioned above. This is very simplified and does not cover all the details.

- Compiler translates user's `build.lab` to C and then JIT compiles it and expose a build API to it
- User's `build.lab` uses the build API to explain dependencies, module graph and jobs
- User's `build.lab` uses the compiler API to advertise plugins as well
- Compiler compiles the plugins before building anything, so it can call them when parsing actual user code
- Compiler initiates the build, executes user's jobs, compiles the modules, calls the plugins when macros are encountered
- Compiler plugins parse their code, usually convert to our AST and ask our compiler to compile it
- Compiler generates assets (executables, dynamic libs, whatever user asked)

## Gotchas

- **ABI/signature mismatch.** Hook typedefs in `Model.h:36-119` and `server/cbi/hooks.h` are
  the contract; a wrong signature still links and corrupts the stack because the C++ side
  blindly casts `void*` to the typedef before calling (`SymResLinkBody.cpp:1748`). Keep
  bindings C-compatible: no default args, no references-by-value of non-POD types, no
  `std::vector`/`ASTBuilder` returned by value.
- **Enum drift** — the #1 crash cause. See the Enum Sync Rule above; sync the value in the
  same position in both files, and remember there are more than two mirrors.
- **Name exactness.** The map key must exactly equal the Chemical-mangled symbol the plugin
  references. A typo yields either `"function with this name doesn't exist"` when indexing a
  hook (`parser/CompilerBinder.cpp:23`) or a TCC undefined-symbol link error for a binding.
- **Pointer lifetime.** Arena-allocated AST nodes die with their arena; live compiler objects
  are borrowed for the call only. Never stash an `ASTBuilder`-produced pointer past its stage,
  and never keep a hook pointer after `CompilerBinder::clear()` deletes the TCC state
  (`CompilerBinder.h:137`).
- **Plugin caching / recompilation.** `store_cbi` refuses to overwrite a name
  (`CompilerBinder.h:88`), so plugins are compiled once per session. Use
  `--frecompile-plugins` (sets `force_recompile_plugins`, disables CBI job caching at
  `LabBuildCompiler.cpp:1617`) when iterating on plugin C++/Chemical code.
- **Thread-safety.** `hooks_` and `interface_maps` have no mutex: registration happens
  single-threaded during plugin linking, before parallel per-file compilation; lookups then
  happen concurrently (including from parallel symres, `SymResLinkBody.cpp:1746`). Treat the
  maps as write-once-then-read-only; registering concurrently with `findHook` is a data race.
- **TCC limitations.** TinyCC supports a restricted C dialect — it is fine for the 2c-generated
  plugin code and the binding shims, but not a general C++ toolchain. `prepare_tcc_state_for_jit`
  (`LibTcc.cpp:214`) is currently a no-op hook for heap interop; all targets link the shared
  `libtcc.so` and must find it relative to the executable (`CMakeLists.txt:960`, copied at
  configure time).
- **`#ifdef LSP_BUILD`.** `LSPAnalyzersMap` (`CBI.cpp:469`) and `LSPHooks.cpp` exist only in
  `ChemicalLsp`; `SemanticTokensPut`/`FoldingRangesPut` are only looked up there.
- **Non-CBI consumers need stubs.** Outside a CBI build, the body-less methods in
  `lang/libs/compiler/src/ast/...` link against `@no_mangle` stubs in
  `lang/libs/compiler_runtime/src/RuntimeSymRes.ch` (e.g. `:139`), not the real CBI
  implementations. Don't confuse the two.

## Adding a new binding (short version)

Full checklists live in `cbi_internals` ("Adding a New CBI Binding / Hook"). Summary:

1. Implement the `extern "C"` function in the matching `compiler/cbi/bindings/*CBI.cpp`
   (POD args, out-params for non-trivial returns, `chem::string_view*`/spans for strings).
2. Declare it in the paired header's `extern "C" { ... }` block.
3. Add a `{ "chemical-mangled-name", (void*) fn }` entry to the relevant `*SymMap` in
   `CBI.cpp`. Forgetting this is the usual "undefined symbol" cause.
4. Declare the Chemical side under `lang/libs/compiler/src/` (or `lab.ch`/`transformer/`) and,
   for a new interface, list it in `chemical.mod`.
5. Test with `./scripts/test.sh --tcc --plugins` or
   `cmake-build-debug/TCCCompiler <mod> -o out -v -frecompile-plugins`.

## Related Skills

- **Compiler Plugin API** (`.agents/skills/cbi_plugin_api/SKILL.md`) — How to develop compiler plugins: ASTBuilder API, macro registration, plugin structure, debugging
- **CBI Internals** (`.agents/skills/cbi_internals/SKILL.md`) — The compiler-side model and bindings layer in depth: `CompilerBinder`/`CBIData`/`CBIFunctionIndex`, the full `CBIFunctionType` table, every `*SymMap`, ABI marshalling, and the add-a-binding checklist
- **Build System (LabBuildCompiler)** (`.agents/skills/build_system/SKILL.md`) — How plugins are compiled and loaded during the build process
- **Building** (`.agents/skills/building/SKILL.md`) — Building the three targets and running the plugin/test suites
- **Performance** (`.agents/skills/performance/SKILL.md`) — Optimization patterns, parallel compilation of plugins

## Future

In the Future, We may use LLVM ORC JIT instead of Tiny CC, but the binding system will remain the same.
