---
name: Transformers
description: Comprehensive guide to transformer jobs and the transformer pipeline in the Chemical compiler — how a `transformer_main` CBI is registered, how the target module is parsed/analyzed on demand via TransformerContext, the full CBI symbol surface, and how this differs from normal compilation and macro plugins. Load when writing a source-to-source transformer (like refgen), debugging `chemical run <transformer> <target>`, or extending TransformerContext.
---

# Transformers

A **transformer** is an external Chemical program (a `CBI` plugin) that the compiler builds and then invokes with a live `TransformerContext*` pointing at some target module. Unlike normal compilation, the transformer itself decides whether/when to parse the target, run symbol resolution and type checking, and what to do with the result. The compiler does **not** emit code for the target — it hands the transformer the module graph and AST, then gets out of the way.

> **Important:** There is **no** `LabJobType::Transformer`. A transformer run is implemented as a `LabJobType::CBI` job (the transformer itself) plus a `LabJobType::ProcessingOnly` job (the target). The "transformer job type" in the CBI enum is `CBIFunctionType::TransformerMain`. See `compiler/lab/LabJobType.h:5-57` and `compiler/cbi/model/CBIFunctionType.h:37`.

## Overview

### What a transformer is

1. The user runs `chemical run <transformer> <target.mod|.lab> [args...]`.
2. `LabBuildCompiler::run_invocation` detects the pattern (first arg is not a file, second is) and calls `run_transformer` (`compiler/lab/LabBuildCompiler.cpp:3649-3667`).
3. `run_transformer` builds the transformer as a CBI plugin into `~/.chemical/transformers/<name>` (`compiler/lab/LabBuildCompiler.cpp:4198-4291`).
4. It then builds the **target** as a `ProcessingOnly` job (parse + symres + typecheck only), flattens the module graph, and constructs a `TransformerContext`.
5. It looks up the hook `("transformer_main", CBIFunctionType::TransformerMain)` and calls it:
   ```cpp
   const auto transformer_main = (int(*)(TransformerContext*, int, char**)) transformer_main_fn;
   return transformer_main(&transformer_context, (int)argv.size() - 1, argv.data());
   ```
   (`compiler/lab/LabBuildCompiler.cpp:4345-4361`)
6. Whatever the transformer does with the context is the "output" — generating docs, emitting code, rewriting files, etc.

### How it differs from other jobs

| Aspect | Compilation / Interpretation / CBI jobs | Transformer run |
|--------|------------------------------------------|-----------------|
| Job types involved | one job type drives the whole run | `CBI` (transformer) + `ProcessingOnly` (target) |
| Target parsed eagerly? | yes, by the pipeline | **only** when the transformer calls `ctx.parseTarget()` |
| Target analyzed eagerly? | yes | **only** when the transformer calls `ctx.analyzeTarget()` |
| Codegen / link target? | yes (exe/lib/obj) | no — target is never codegen'd |
| Entry contract | `build()` returns `Module*` | `transformer_main(ctx, argc, argv)` returns `int` |
| Scope of operation | one module graph → one artifact | arbitrary program over the flattened module graph |
| Caching | module caching enabled | **disabled** for the target (`LabBuildCompiler.cpp:4298-4300`) |

### When to use a transformer

- **Source-to-source rewriting** — read the target AST, produce new Chemical/C/HTML/etc.
- **Code injection / preprocessing** — add generated members, wrap symbols, emit boilerplate.
- **Documentation generation** — the built-in `refgen` walks the AST to lazily emit API docs.
- **Analysis / indexing** — build an index of symbols, dependency graphs, call graphs.
- **Custom output formats** — anything that needs the full analyzed module graph but no executable.

If you only need to react to a `#macro` at a specific site, a **macro plugin** (ParseMacroNode/SymResNode/ReplacementNode) is the right tool — see [Macro hooks vs. a transformer job](#macro-hooks-vs-a-transformer-job).

## Key Files

| File | Purpose |
|------|---------|
| `compiler/lab/transformer/TransformerContext.h` | The `TransformerContext` struct — job, compiler, processor, flattened modules, token map (`:14-44`) |
| `compiler/lab/LabBuildCompiler.cpp` | `run_transformer` (`:4198-4363`), invocation detection (`:3649-3667`), `get_transformers_cache_dir` (`:4190`) |
| `compiler/lab/LabBuildCompiler.h` | `run_transformer` declaration (`:603`), `process_modules` (`:345-355`) |
| `compiler/cbi/bindings/TransformerContextCBI.h/.cpp` | `extern "C"` implementations of every context method and module/file accessor |
| `compiler/cbi/bindings/CBI.cpp` | `TransformerContextSymMap` (`:443-462`), `interface_maps.emplace("TransformerContext", ...)` (`:492`) |
| `compiler/cbi/model/CBIFunctionType.h` | `TransformerMain` enum value (`:37`) |
| `compiler/cbi/model/CompilerBinder.h` / `parser/CompilerBinder.cpp` | `registerHook` (`:102`), `findHook` (`:110`), `index_function` (`:20`) |
| `compiler/cbi/bindings/BuildContextCBI.cpp` | `BuildContextindex_cbi_fn` — validates the job is CBI and stores the index (`:143-150`) |
| `lang/libs/transformer/` | The `transformer` module: `chemical.mod` + `TransformerContext.ch` + interface declarations |
| `lang/libs/refgen/` | Reference transformer: build.lab registration + `transformer_main` + full generator |
| `lang/libs/lab/src/lab.ch` | Chemical mirror of `CBIFunctionType` (`:96-113`), `index_cbi_fn` (`:212`), `index_def_cbi_fn` (`:429`) |
| `compiler/ASTProcessor.h/.cpp` | `import_chemical_files_direct_with_tokens` (`:216`/`:1095`), `determine_module_files` (`:275`/`:133`), `sym_res_module` (`:490`/`:430`), `type_verify_module_parallel` (`:508`/`:775`) |
| `lang/libs/compiler/src/PtrVec.ch` | `VecRef<T>` used by `getFlattenedModules` (`:20-46`) |
| `lang/libs/compiler/src/Token.ch` | `Token { type, value, position }` (`:2-14`) |
| `lang/libs/compiler/src/LocationData.ch` | `LocationData` returned by `decodeLocation` (`:1-15`) |

## The transformer job lifecycle

### 1. Creation & registration (inside the transformer's `build.lab`)

A transformer is a normal Chemical library whose `build.lab` registers a `TransformerMain` hook with `ctx.index_cbi_fn`. The `transformer` module must be a dependency so the compiler interface is imported.

`lang/libs/refgen/build.lab:8-18`:
```chemical
public func build(ctx : *mut BuildContext, __chx_job : *mut LabJob) : *mut Module {
    const curr_file_path = lab::get_my_path()
    const __chx_already_exists = ctx.get_cached(__chx_job, curr_file_path);
    if(__chx_already_exists != null) { return __chx_already_exists; }
    const deps : []ModuleDependency = [ /* std, fs, compiler, transformer */ ];
    const mod = ctx.new_package(ModuleType.Directory, PackageKind.Library, "", "refgen", std::span<ModuleDependency>(deps, 4));
    ctx.set_cached(__chx_job, curr_file_path, mod)
    ctx.add_path(mod, lab::rel_path_to("src").to_view());
    // key MUST be "transformer_main" — it is looked up verbatim by run_transformer
    ctx.index_cbi_fn(__chx_job as *mut LabJobCBI, "transformer_main", "transformer_main", CBIFunctionType.TransformerMain)
    return mod;
}
```

- `index_cbi_fn(job, key, fn_name, type)` requires `job->type == LabJobType::CBI` (`compiler/cbi/bindings/BuildContextCBI.cpp:143-150`) — it records a `CBIFunctionIndex{key, fn_name, fn_type}` on `job->indexes` (`compiler/lab/LabJob.h:158`).
- `index_def_cbi_fn(job, name, type)` is a convenience that uses `job.getName()` as the key (`lang/libs/lab/src/lab.ch:429-431`).
- After the CBI is built, `link_cbi_job` resolves the function symbol by name in the TCC state and calls `binder.registerHook(type, key, sym)` (`compiler/lab/LabBuildCompiler.cpp:1593-1600`, `parser/CompilerBinder.cpp:20-27`).
- `run_transformer` then retrieves it with `binder.findHook("transformer_main", CBIFunctionType::TransformerMain)` (`compiler/lab/LabBuildCompiler.cpp:4345`).

### 2. Inputs

`run_transformer` has two inputs:

- **`transformer`** — a name (`"refgen"` is resolved as a *native* transformer from `lang/libs/`; any other value is treated as a remote `org/repo` and fetched into `~/.chemical/transformers/<name>`) — `compiler/lab/LabBuildCompiler.cpp:4217-4252`.
- **`target`** — a `.mod` or `.lab` path/identifier. It is built into a `ProcessingOnly` job and never linked (`:4293-4306`).
- **`args`** — the extra CLI words are forwarded to `transformer_main` as `argv` (after the transformer name), exactly like a `main()` receives `argv` (`:4351-4361`).

### 3. Context construction

`run_transformer` creates a fresh interpreter + processor environment for the target and hands ownership of the flattened module list to the context:

```cpp
GlobalInterpretScope global(other_job.mode, other_job.target_data, nullptr, this, *job_allocator, type_builder, loc_man);
InstantiationsContainer instContainer;
SymbolResolver resolver(binder, global, path_handler, controller, instContainer, coreNodes, implsIndex, ...);
ASTProcessor processor(path_handler, options, mod_storage, controller, loc_man, &resolver, binder, type_builder, instContainer, ...);
create_or_rebind_container(this, global, resolver, other_job.target_data);
auto outMods = flatten_dedupe_sorted(other_job.dependencies);
for(const auto mod : outMods) { processor.determine_module_files(mod); }
TransformerContext transformer_context(&other_job, this, &processor, std::move(outMods));
```
(`compiler/lab/LabBuildCompiler.cpp:4315-4342`)

- `flatten_dedupe_sorted` recursively flattens and de-duplicates the dependency graph, children first (`compiler/lab/LabBuildCompiler.cpp:141-163`).
- `determine_module_files` populates each module's `direct_files` (without parsing) from its paths/directories (`compiler/ASTProcessor.cpp:133-200`).
- **Parsing is lazy.** The files are *not* parsed until `ctx.parseTarget()` is called.

### 4. Execution

The transformer's `transformer_main` handler runs inside the TCC-JIT'd plugin. A typical flow:

```chemical
@no_mangle
public func transformer_main(ctx : *TransformerContext, argc : int, argv : **char) : int {
    // 1. parse with comments preserved (optional)
    if(!ctx.parseTarget(true)) { return 1; }
    // 2. resolve symbols + typecheck
    if(!ctx.analyzeTarget()) { return 1; }
    // 3. walk modules/files
    var deps = ctx.getFlattenedModules();
    // ... inspect AST via file scopes, tokens, locations ...
    return 0;
}
```
(`lang/libs/refgen/src/main.ch:1-104`)

### 5. Output

The compiler imposes **no output**. The transformer returns an `int` exit code which becomes `run_transformer`'s return value (`compiler/lab/LabBuildCompiler.cpp:4361`). `refgen` writes HTML to `./docs` using `fs::write_text_file`; another transformer could mutate the AST, emit Chemical source, print a report, etc.

## `TransformerContext` API

C++ declaration: `compiler/lab/transformer/TransformerContext.h:14-44`. All methods are exported as `extern "C"` and registered under the `TransformerContext` compiler interface (`compiler/cbi/bindings/CBI.cpp:443-462`, `:492`).

Chemical binding: `lang/libs/transformer/src/TransformerContext.ch:1-16`. Because the interface lives in module scope `transformer`, exported symbol names are prefixed `transformer_...` (e.g. `transformer_TransformerContextparseTarget`).

| Method (Chemical) | C++ symbol | Purpose |
|-------------------|-----------|---------|
| `getTargetJob(&self) : *LabJob` | `TransformerContextgetTargetJob` | The target `ProcessingOnly` job. Use `getName()`, `getBuildDir()`, `getMode()`, `getTarget()`, etc. (`TransformerContextCBI.cpp:12-14`) |
| `parseTarget(&self, keep_comments : bool) : bool` | `TransformerContextparseTarget` | Lex+parse every direct file of every flattened module. If `keep_comments` is true, comment/whitespace tokens are retained and copied into the job allocator. Returns false on any parse failure. Does **not** process imports recursively. (`TransformerContextCBI.cpp:16-27`) |
| `analyzeTarget(&self) : bool` | `TransformerContextanalyzeTarget` | Runs the full symbol-resolution pipeline (`sym_res_module`) then type checking (`type_verify_module_parallel`) for each flattened module. Returns false on the first failure. Call after `parseTarget`. (`TransformerContextCBI.cpp:29-45`) |
| `getFlattenedModules(&self) : *mut VecRef<Module>` | `TransformerContextgetFlattenedModules` | Pointer to the de-duplicated, topologically-sorted module list. Iterate with `.size()` / `.get(i)`. (`TransformerContextCBI.cpp:47-49`) |
| `getFileTokens(&self, fileId : uint) : std::span<Token>` | `TransformerContextgetFileTokens` | The token stream for a file. Only comment tokens survive if `parseTarget(true)` was used. Returns a null/empty span for unknown file ids. (`TransformerContextCBI.cpp:51-58`) |
| `decodeLocation(&self, encoded : ubigint) : LocationData` | `TransformerContextdecodeLocation` | Decode an AST node's encoded source location into `{fileId, lineStart, charStart, lineEnd, charEnd}`. (`TransformerContextCBI.cpp:60-67`) |

### Supporting interfaces

`TransformerModule` (`lang/libs/transformer/src/transformer.ch:8-15`) extends `Module` (`lang/libs/lab/src/lab.ch:28-47`):

| Method | C++ symbol | Purpose |
|--------|-----------|---------|
| `getFiles(&self) : std::span<ASTFileMetaData>` | `ModulegetFiles` | All direct files (pointer + size). (`TransformerContextCBI.cpp:69-71`) |
| `getFileCount(&self) : uint` | `ModulegetFileCount` | Count of direct files. (`:73-75`) |
| `getFile(&self, index : uint) : *mut ASTFileMetaData` | `ModulegetFile` | One file by index. (`:77-79`) |
| `getDependencyCount(&self) : uint` | `ModulegetDependencyCount` | Number of direct dependencies of the module. (`:81-83`) |
| `getDependency(&self, index : uint) : *mut TransformerModule` | `ModulegetDependency` | A direct dependency module. (`:85-87`) |
| `getName(&self) : std::string_view` | (from `Module`) | Module name — used as the doc folder name in refgen. |

`ASTFileMetaData` (`lang/libs/transformer/src/transformer.ch:1-6`):

| Method | C++ symbol | Purpose |
|--------|-----------|---------|
| `getFileId(&self) : uint` | `FileMetaDatagetFileId` | File id, matches `getFileTokens`/`decodeLocation`. (`TransformerContextCBI.cpp:89-91`) |
| `getAbsPath(&self) : std::string_view` | `FileMetaDatagetAbsPath` | Absolute source path. (`:93-95`) |
| `getFileScope(&self) : *mut FileScope` | `FileMetaDatagetFileScope` | The parsed `FileScope` (null before `parseTarget`). (`:97-100`) |

`TransformerFileScope` (`lang/libs/transformer/src/transformer.ch:17-20`) extends `FileScope`:

| Method | C++ symbol | Purpose |
|--------|-----------|---------|
| `getBody(&self) : *mut Scope` | `FileScopegetBody` | The top-level `Scope`; iterate its `getNodes()` (`VecRef<ASTNode>`). (`TransformerContextCBI.cpp:102-104`) |

`LocationDataCBI` is marshalled into the Chemical `LocationData` (`compiler/cbi/bindings/TransformerContextCBI.h:12-18`, `lang/libs/compiler/src/LocationData.ch:1-15`).

## Integration with `process_modules()` and the dependency graph

`run_transformer` does **not** go through `process_modules` for the target. Instead:

1. It builds the transformer CBI via `do_job(&transformer_job)` (`LabBuildCompiler.cpp:4275`). `do_job` dispatches `CBI` to `process_modules` (`:231-232`), which selects `process_job_tcc` (`LabBuildCompiler.h:345-355`) and eventually `link_cbi_job` (`:1532`) to JIT-compile and register the transformer.
2. It builds the target as a `ProcessingOnly` job and flattens its graph (`:4293-4342`).
3. It creates its **own** `ASTProcessor`, `SymbolResolver`, `InstantiationsContainer`, and `GlobalInterpretScope` for the target — completely isolated from the transformer build (`:4315-4324`).

The job dependency graph still matters because `flatten_dedupe_sorted` (`:4330`) returns the transitive closure of the target job's `dependencies`, sorted children-first. `TransformerContext::flattened_mods` is this list. The transformer can traverse it linearly or walk `getDependency(i)` edges to reconstruct the graph (refgen does both — it emits a Mermaid graph via `add_module_deps`, `lang/libs/refgen/src/generator.ch:688-734`).

> **Dependency graph ordering:** use `flattened_mods` for "all modules" and `getDependency` for edges. Do not assume the list order equals the dependency edges.

## Worked example: a minimal transformer

Directory layout:
```
lang/libs/my_transform/
├── build.lab
└── src/
    └── main.ch
```

`lang/libs/my_transform/build.lab` — register the hook. Note the dependency on `@transformer/build.lab`:
```chemical
import lab;
import std;
import "@std/build.lab" as __std
import "@compiler/build.lab" as __compiler
import "@transformer/build.lab" as __transformer

public func build(ctx : *mut BuildContext, __chx_job : *mut LabJob) : *mut Module {
    const curr_file_path = lab::get_my_path()
    const cached = ctx.get_cached(__chx_job, curr_file_path);
    if(cached != null) { return cached; }

    const deps : []ModuleDependency = [
        ModuleDependency { module: __std.build(ctx, __chx_job), info: null },
        ModuleDependency { module: __compiler.build(ctx, __chx_job), info: null },
        ModuleDependency { module: __transformer.build(ctx, __chx_job), info: null },
    ];
    const mod = ctx.new_package(ModuleType.Directory, PackageKind.Library, "", "my_transform", std::span<ModuleDependency>(deps, 3));
    ctx.set_cached(__chx_job, curr_file_path, mod);
    ctx.add_path(mod, lab::rel_path_to("src").to_view());
    ctx.index_cbi_fn(__chx_job as *mut LabJobCBI, "transformer_main", "transformer_main", CBIFunctionType.TransformerMain)
    return mod;
}
```
(Modeled on `lang/libs/refgen/build.lab:1-18`.)

`lang/libs/my_transform/src/main.ch` — the entry hook. The exact signature is fixed by the call site cast at `LabBuildCompiler.cpp:4358`:
```chemical
@no_mangle
public func transformer_main(ctx : *TransformerContext, argc : int, argv : **char) : int {
    if(!ctx.parseTarget(true)) {
        printf("parse failed\n");
        return 1;
    }
    if(!ctx.analyzeTarget()) {
        printf("analysis failed\n");
        return 1;
    }

    var mods = ctx.getFlattenedModules();
    var i = 0u;
    while(i < mods.size()) {
        var mod = mods.get(i) as *TransformerModule;
        printf("module %s has %d files\n", mod.getName().data(), mod.getFileCount());

        var f = 0u;
        while(f < mod.getFileCount()) {
            var meta = mod.getFile(f);
            var scope = meta.getFileScope();
            if(scope != null) {
                var body = scope.getBody();
                if(body != null) {
                    var nodes = body.getNodes();
                    if(nodes != null) {
                        printf("  %s: %d top-level nodes\n", meta.getAbsPath().data(), nodes.size());
                    }
                }
            }
            f++;
        }
        i++;
    }
    return 0;
}
```

Invoke (second arg must end in `.mod`/`.lab` so detection at `LabBuildCompiler.cpp:3655-3666` fires):
```bash
chemical run my_transform path/to/target/chemical.mod
```

For a real-world, full traversal + token-based doc-comment extraction example, read `lang/libs/refgen/src/generator.ch`:
- `index_module` (`:464-491`) — iterate files → scope → nodes → recursive index.
- `generate_file_docs` (`:771-852`) — pulls tokens via `ctx.getFileTokens(file_id)` and locations via `ctx.decodeLocation`.
- `find_comment_before` (`:4-41`) — walks the token stream backwards to find doc comments.
- `add_module_deps` (`:688-717`) — reconstructs the dependency graph edges.

## Macro hooks vs. a transformer job

Both are CBI plugins built with TCC, but they operate at completely different granularity.

| | Macro hooks (`ParseMacroNode` / `SymResNode` / `ReplacementNode`) | Transformer job (`TransformerMain`) |
|--|----------------------------------------------------------------|--------------------------------------|
| Trigger | the parser/analyzer/codegen encounters a specific `#macro` node you own | the user runs `chemical run <transformer> <target>` |
| Granularity | one `EmbeddedNode` value at one source site | the entire flattened module graph |
| Called from | parser?, `SymResLinkBody.cpp:1746`, `LLVM.cpp:1434` / `2cASTVisitor.cpp:7561` | `LabBuildCompiler.cpp:4361` |
| Input | `Parser*`, `SymResLinkBody*`, `ASTBuilder*`, `EmbeddedNode*` | `TransformerContext*`, `argc`, `argv` |
| Output | an `ASTNode*`/`Value*` substituted in place | an exit code; arbitrary side effects |
| Target parsing | happens as part of the normal compile | lazy, explicit via `ctx.parseTarget()` |
| Target analysis | normal pipeline | explicit via `ctx.analyzeTarget()` |
| Registration key | macro name (e.g. `"html"`) | `"transformer_main"` |
| Runs during | parsing / symres / codegen of a module | after the transformer program is JIT'd and invoked |
| Applies to | code that uses `#macro` | any `.mod`/`.lab` target | 

Key files for macro hooks: `compiler/cbi/model/CBIFunctionType.h:5-37`, `compiler/symres/SymResLinkBody.cpp:1746`, `compiler/backend/LLVM.cpp:1414-1483`, `preprocess/2c/2cASTVisitor.cpp:7561`. See the [Compiler Plugin API (CBI)](../cbi_plugin_api/SKILL.md) skill.

Rule of thumb: **macro = rewrite one node inline; transformer = a whole program over an entire analyzed codebase.** A transformer does not need to hook the parser at all.

## Gotchas

### Caching

`run_transformer` explicitly disables caching for the target, since transforms must always see fresh source:
```cpp
options->is_caching_enabled = false;
options->is_build_lab_caching_enabled = false;
```
(`compiler/lab/LabBuildCompiler.cpp:4298-4300`). The **transformer plugin itself** is still subject to normal plugin caching (`get_cached` in its `build.lab`; rebuild with `--frecompile-plugins`). If edits to the transformer source don't take effect, force plugin recompilation.

### `parseTarget` does not resolve imports

`parseTarget` calls `import_chemical_files_direct_with_tokens` (`compiler/ASTProcessor.cpp:1095-1141`), which lexes/parses only the **direct files already registered in each module's `direct_files`**. It does not recursively follow `import` statements. If the target's AST depends on imported files, make sure those modules are part of the flattened dependency graph (they normally are, via `flatten_dedupe_sorted`).

### `keep_comments` and token lifetime

Comment/whitespace tokens are only emitted by the lexer when `keep_comments` is true (`lexer/Lexer.cpp:906-915`). More importantly, comment token string data is backed by a memory-mapped source that dies once the input source closes, so the parser copies comment strings into the **job allocator** (`compiler/ASTProcessor.cpp:1388-1396`). Always call `parseTarget(true)` if you intend to read comments from `getFileTokens`; otherwise the tokens are still returned but comments are absent.

### Order of operations

`getFileScope()` returns null before `parseTarget`, and `analyzeTarget` requires a prior `parseTarget`. Always: `parseTarget` → `analyzeTarget` → traverse. `refgen` does exactly this (`lang/libs/refgen/src/main.ch:54-64`).

### The transformer is TCC-compiled

The transformer is compiled to C and JIT'd with TinyCC (`link_cbi_job`). It links against the `compiler`, `lab`, `std`, and `transformer` libraries' CBI interfaces. Anything it calls must be available in that TCC state; unsupported runtime features will fail at JIT/link time.

### Enum-sync concerns

Transformer plugins consume C++ enums exposed through Chemical bindings. If the C++ and Chemical orderings diverge, `switch`/comparisons silently take wrong branches (SIGSEGV or wrong output). Enums to keep in sync:

- `CBIFunctionType` — C++ `compiler/cbi/model/CBIFunctionType.h:5-38` vs Chemical `lang/libs/lab/src/lab.ch:96-113`. `TransformerMain` is the **last** value; append-only additions are safe, middle insertions are not.
- `ASTNodeKind` — `ast/base/ASTNodeKind.h` vs `lang/libs/compiler/src/ast/base/ASTNodeKind.ch`. Transformers switch on `node.getKind()` extensively (refgen does at `generator.ch:144-202`).
- `TokenType` — `lexer/TokenType.h` vs `lang/libs/compiler/src/ChemicalTokenType.ch`.

**Known divergence to be aware of:** `LabJobType` in C++ (`compiler/lab/LabJobType.h:5-57`) contains an `Intermediate` value between `CBI` and `Interpretation`, but the Chemical mirror (`lang/libs/lab/src/lab.ch:49-58`) omits it. Consequently `LabJobType.Interpretation` is `7` in Chemical but `8` in C++. Any build script/transformer comparing `job.getType()` against `Interpretation` must account for this. Add the missing value to keep them aligned.

### Parallelization

- `parseTarget` parses files concurrently through `compiler->pool` (`compiler/lab/LabBuildCompiler.cpp:4318` via `TransformerContextparseTarget`, which calls `import_chemical_files_direct_with_tokens`).
- `analyzeTarget` runs `sym_res_module` (which internally parallelizes its per-file passes) then `type_verify_module_parallel` (`TransformerContextCBI.cpp:29-45`).
- The transformer's own traversal (`transformer_main`) runs on the calling thread; the plugin may spawn threads but must respect the interpreter/allocator state it is handed. Prefer sequential traversal.
- Allocator lifetimes: `job_allocator`/`mod_allocator`/`file_allocator` are cleared before `do_job` on the target and again after the transformer runs (`LabBuildCompiler.cpp:4264-4284`). Do not retain pointers to AST nodes, token strings, or module data after `transformer_main` returns.

### Build directory

The transformer's build dir is centralized at `~/.chemical/transformers/<name>` (`get_transformers_cache_dir`, `LabBuildCompiler.cpp:4190-4195`), and the target job's `build_dir` is set to that same path (`:4296`). The target is never emitted anywhere; if the transformer wants to write output (refgen writes `./docs`), it does so itself relative to the user's CWD.

## Related Skills

- [Compiler Plugin API (CBI)](../cbi_plugin_api/SKILL.md) — plugin registration, `CompilerBinder`, macro hooks, enum-sync rules
- [Build System (Lab)](../build_system/SKILL.md) — `LabJob`, `LabModule`, `process_modules`, the `build.lab` API
- [Compiler API](../compiler_api/SKILL.md) — AST node bindings used to traverse the target
- [Compiler Intrinsics & Reflection](../intrinsics_compiler_reflection/SKILL.md) — metadata access available to plugins
