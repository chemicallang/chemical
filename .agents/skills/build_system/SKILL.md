---
name: Build System (LabBuildCompiler)
description: Comprehensive deep-dive into the Chemical build pipeline — how chemical.mod is converted to build.lab, how build scripts are JIT-compiled via TinyCC, how jobs are created and executed, and how ASTProcessor orchestrates the parallel compilation passes.
---

# Build System (LabBuildCompiler)

The Lab build system is the entry point and orchestration layer of the Chemical compiler. It reads `build.lab` or `chemical.mod` files, creates compilation jobs, manages dependencies, coordinates compiler plugins, and drives the entire multi-pass compilation pipeline.

## Full Pipeline Overview

```
User Input
    │
    ├── chemical.mod → [ModToLabConverter] → build.lab (C code string)
    │
    └── build.lab → [2c backend] → C code → [TinyCC JIT] → executable build script
                        │
                        ▼
              LabBuildCompiler::execute_job()
                        │
                        ▼
              ┌─────────────────────────────────┐
              │         Job Execution            │
              │                                  │
              │  1. Parse all source files        │
              │     (lexer → parser → AST)        │
              │     Parallel per file             │
              │                                  │
              │  2. Symbol Resolution             │
              │     (6 parallel passes)           │
              │     See ASTProcessor section      │
              │                                  │
              │  3. Type Verification             │
              │     Parallel per file             │
              │                                  │
              │  4. C Translation (2c)            │
              │     Forward decl → Type aliases   │
              │     → Declare → Implement         │
              │                                  │
              │  5. Backend Codegen               │
              │     LLVM.cpp or TinyCC            │
              │                                  │
              │  6. Link                         │
              │     Produce executable/library    │
              └─────────────────────────────────┘
```

## Phase 1: Input Loading

### chemical.mod → build.lab (ModToLabConverter)

The `ModToLabConverter` (in `compiler/lab/mod_conv/ModToLabConverter.cpp`) translates a concise `.mod` file into a full `.lab` build script.

**Input example (chemical.mod):**
```chmod
application my_app

source "src"
import "../lib_mod"
import "github.com/owner/repo" version "1.0.0"
link "mylib"
link c "helper.c"
```

**Output (generated build.lab):**
```chemical
import lab;
import std;

import "../lib_mod/build.lab" as __mod_0_stmt;

public func build(ctx : *mut BuildContext, __chx_job : *mut LabJob) : *mut Module {
    var __curr_lab_path = lab::get_my_path();
    const __chx_already_exists = ctx.get_cached(__chx_job, &__curr_lab_path);
    if(__chx_already_exists != null) { return __chx_already_exists; }

    const deps : []ModuleDependency = [
        ModuleDependency { module: __mod_0_stmt.build(ctx, __chx_job), info: null },
    ];

    const mod = ctx.new_package(ModuleType.Directory, PackageKind.Application,
        "", "my_app",
        std::span<ModuleDependency>(deps, 1));
    ctx.set_cached(__chx_job, &__curr_lab_path, mod);

    // Remote imports
    ctx.fetch_mod_dependency(__chx_job, mod, ImportRepo {
        from: "github.com/owner/repo",
        version: "1.0.0",
    });

    // Source paths
    mod.add_source("src");

    // Link statements
    mod.add_link_path("./my_libs");
    mod.add_link("mylib");
    mod.add_link_c("helper.c");

    return mod;
}
```

**Key conversion logic in `convertToBuildLab()`:**

1. **Imports for dependencies**: Each `import` statement in `.mod` becomes a `build.lab` import with a unique identifier like `__mod_N_stmt`
2. **Dependency list**: Each import generates a `ModuleDependency` entry calling the imported module's `build()` function
3. **Module creation**: `ctx.new_package()` with the correct `ModuleType` and `PackageKind`
4. **Remote imports**: `ctx.fetch_mod_dependency()` with `ImportRepo` struct
5. **Source paths**: `mod.add_source(path)` for each source directory
6. **Link statements**: `mod.add_link_path()`, `mod.add_link()`, `mod.add_link_c()` for each link directive
7. **Conditional imports**: `if(windows)` generates an `if(__chx_job.getTarget().windows)` block
8. **Symbol info**: For `import Foo from "..."` style, generates `DependencySymbolInfo` with alias and symbol list

### build.lab → JIT-compiled executable

The build script itself is Chemical code that must be compiled and executed:

1. **Lex, Parse**: The build.lab is lexed and parsed like any Chemical file
2. **C Translation (2c)**: The parsed AST is translated to C code via the 2c backend
3. **TinyCC JIT**: The generated C is compiled in-memory using TinyCC
4. **Symbols exposed**: LabBuildCompiler exposes CBI functions to the build script:
   - `ctx.new_package()` — create a module
   - `ctx.set_cached()` — cache the module
   - `mod.add_source()` — add source files
   - `mod.add_link()` — add link libraries
   - `ctx.fetch_mod_dependency()` — handle remote imports
   - etc.

The compiled build script is executed by calling its `build()` function, which returns a `Module*` containing all the module's metadata.

## Phase 2: Job Creation

After the build script executes, `LabBuildCompiler` has a tree of `LabModule` objects representing the dependency graph. Each `LabModule` has:

```cpp
struct LabModule {
    chem::string name;                    // Module name
    chem::string scope_name;              // Scope name (namespace)
    LabModuleType type;                   // Directory, Files, ObjFile, CFile, CPPFile
    PackageKind package_kind;             // Application or Library
    std::vector<ModuleDependency> dependencies;  // Dependencies
    std::vector<chem::string> paths;      // Source paths or file paths
    ModuleScope module_scope;             // Scope for symbol resolution
    std::vector<ASTFileMetaData> direct_files;  // Files belonging to this module
};
```

`LabBuildCompiler::execute_job()` iterates jobs and processes each module's files through the full pipeline.

## Phase 3: ASTProcessor — The Compilation Orchestrator

`ASTProcessor` (in `compiler/ASTProcessor.h/.cpp`) is the core orchestrator that drives the compilation of each module. It owns the resolver, binder, allocators, and manages all passes.

### Key Components in ASTProcessor

| Member | Type | Purpose |
|--------|------|---------|
| `resolver` | `SymbolResolver*` | Symbol resolution |
| `loc_man` | `LocationManager&` | Source location management |
| `controller` | `AnnotationController&` | Annotation handling |
| `binder` | `CompilerBinder&` | CBI function binding |
| `path_handler` | `ImportPathHandler&` | Import resolution |
| `container` | `InstantiationsContainer&` | Generic instantiation tracking |
| `file_allocator` | `ASTAllocator` | Per-file arena allocator |
| `mod_allocator` | `ASTAllocator` | Per-module arena allocator |
| `job_allocator` | `ASTAllocator` | Per-job arena allocator |
| `mod_storage` | `ModuleStorage&` | Module storage and lookup |
| `cache` | `std::unordered_map<unsigned, ASTFileResult*>` | File result cache |
| `import_mutex` | `std::mutex` | Thread safety for imports |
| `print_mutex` | `std::mutex` | Thread safety for printing |

### The 6 Symbol Resolution Passes

`ASTProcessor::sym_res_module()` runs **6 passes** in sequence:

#### Pass 1: Top-Level Declaration (Serial)

```cpp
for(auto& file_ptr : module->direct_files) {
    file.private_symbol_range = sym_res_tld_declare_file(
        file.unit.scope.body, file.file_id, file.abs_path);
}
```

- **What**: Declares all top-level symbols (functions, structs, variants, namespaces, impls)
- **Why serial**: Declarations must happen in order to avoid race conditions on the shared symbol table
- **Output**: `SymbolRange` indicating start/end indices of file-private symbols
- **Allocator cleared**: `file_allocator.clear()` after pass

#### Pass 2: Link Signatures (Parallel per file)

```cpp
// Parallel using thread pool
for(auto& file_ptr : module->direct_files) {
    futures.emplace_back(pool.push([this, &file](int id){
        auto res = link_sig_file_task(resolver, &file);
        // merge diagnostics with mutex
        return has_errors;
    }));
}
```

- **What**: Resolves all type signatures (function parameter types, return types, struct member types, variant member types, type aliases) — does NOT enter function bodies
- **Why parallel**: Each file has its own per-file SymbolTable and diagnoser
- **Key detail**: Uses `sym_res_signature()` which creates a per-file `TopLevelLinkSignature` with its own `SymbolTable` for file-private symbols (generic type params, using-imports, aliases)
- **Output**: `SymResSignatureResult` with inline instantiations collected
- **Allocator cleared**: `file_allocator.clear()` after pass

#### Pass 3: Generic Instantiation (Parallel per file)

```cpp
for(auto& file_ptr : module->direct_files) {
    futures.emplace_back(pool.push([this, &file](int id){
        auto res = gen_inst_file_task(resolver, &file);
        return res.has_errors;
    }));
}
```

- **What**: Finalizes inline generic instantiations discovered during link signatures
- **Key function**: `sym_res_generic_instantiation()` in `GenericInstantiationPass.cpp`
  1. Calls `GenericTypeDecl::finalize_signature()` for each inline instantiation
  2. Calls `GenericInstantiator::FinalizeSignature()` to resolve generic type parameters
  3. Visits the scope to trigger nested generic instantiations
- **Why separate**: Generic types discovered during link signatures need to be instantiated BEFORE body resolution so that bodies can reference concrete types
- **Registration only**: This pass requires only `InstantiationRequirement::Registration` — not full body finalization
- **Allocator cleared**: `file_allocator.clear()` after pass

#### Pass 4: After Link Signature (Serial)

```cpp
for(auto& file_ptr : module->direct_files) {
    sym_res_after_link_sig_file(file.unit.scope.body, file.file_id,
        file.abs_path, file.private_symbol_range);
}
```

- **What**: Calls `sym_res_after_signature()` — handles tasks that must happen after all signatures are linked but before bodies
- **Why serial**: May modify shared resolver state
- **Allocator cleared**: `file_allocator.clear()` after pass

#### Pass 5: Link Body — Generic Decls (Parallel per file)

```cpp
for(auto& file_ptr : module->direct_files) {
    futures.emplace_back(pool.push([this, &file](int id){
        auto res = link_body_generic_decls_task(resolver, &file);
        return res.has_errors;
    }));
}
```

- **What**: Resolves the master bodies of generic declarations
- **Why separate**: Generic master bodies need to be resolved before instantiation-specific bodies
- **Function**: `sym_res_link_body_generic_decls_pass()`

#### Pass 6: Link Body — Full (Parallel per file)

```cpp
for(auto& file_ptr : module->direct_files) {
    futures.emplace_back(pool.push([this, &file](int id){
        auto res = link_body_task(resolver, &file);
        return res.has_errors;
    }));
}
```

- **What**: The main body resolution pass. Resolves ALL remaining symbols in function bodies:
  - Variable references and identifiers
  - Function calls
  - Expressions and operators
  - Type references in expressions
  - Access chains (`a.b.c()`)
  - Move semantics tracking
  - Operator overload resolution
  - Pattern matching
  - Lambdas and closures
  - Comptime blocks
- **Function**: `sym_res_link_body_pass()` which creates a per-file `SymResLinkBody` with its own `SymbolTable`, `GenericInstantiatorAPI`, and `ASTDiagnoser`
- **Allocator cleared**: `file_allocator.clear()` after pass

### The Sequential Path

`ASTProcessor::sym_res_module_seq()` is an alternative path that processes files **sequentially** (one file at a time, all passes together). This is used for the build.lab itself (small, not worth parallel overhead). It calls `declare_and_link_file()` which runs all passes in order for a single file.

### Type Verification (Parallel per file)

After symbol resolution, `ASTProcessor::type_verify_module_parallel()` runs type verification:

```cpp
for(auto& fileData : module->direct_files) {
    futures.emplace_back(pool.push([this, &fileData](int id){
        return type_verify_file_task(this, fileData.result);
    }));
}
```

Each file gets its own `ASTDiagnoser`. Results are merged after all files complete.

### C Translation

After type verification, `ASTProcessor::declare_module()` and `ASTProcessor::implement_module()` handle the 2c backend translation:

```cpp
// Phase 1: Forward declarations
for(auto& file_ptr : module->direct_files) {
    c_visitor.fwd_declare(unit.scope.body.nodes);
}

// Phase 2: Type aliases
for(auto& file_ptr : module->direct_files) {
    c_visitor.declare_type_aliases(unit.scope.body.nodes);
}

// Phase 3: Declare (prototypes only — no bodies)
for(auto& file_ptr : module->direct_files) {
    c_visitor.declare_before_translation(unit.scope.body.nodes);
}

// Phase 4: Implement (function bodies)
for(auto& file_ptr : module->direct_files) {
    c_visitor.translate_after_declaration(unit.scope.body.nodes);
    c_visitor.file_level_reset();  // Clear per-file state
}
```

Generic instantiations are translated between the module files:
```cpp
c_visitor.fwd_declare(container.get_current_module_instantiations());
c_visitor.declare_before_translation(container.get_current_module_instantiations());
c_visitor.translate_after_declaration(container.get_current_module_instantiations());
container.clear_current_module_instantiations();
```

## File Import and Caching

### ASTFileResult Cache

The `cache` map in ASTProcessor stores parsed file results:
```cpp
std::unordered_map<unsigned, ASTFileResult*> cache;
```

When a file is first imported, it's:
1. Lexed and parsed into an `ASTFileResult`
2. Cached by file ID for reuse
3. Import statements in the file trigger recursive imports
4. Import resolution is handled by `figure_out_direct_imports()`

### Recursive Import Pattern

`import_chemical_file_recursive()` handles recursive imports:
1. Lex and parse the current file
2. Print diagnostics
3. Call `figure_out_direct_imports()` to find local file imports
4. Recursively import those files via thread pool
5. Uses `ConcurrentParsingState` to track completion (atomic task counter)

### Debug Feature: File Shuffling

In DEBUG builds, files within a directory are **shuffled** before compilation:

```cpp
#ifdef DEBUG
    shuffle_files(filePaths);
#endif
```

This randomly reorders files to expose compilation order bugs. A seed is printed so failures can be reproduced:
```
File order seed: 12345678 (set FILE_ORDER_SEED to reproduce)
```

## Job Types in LabBuildCompiler

### Compilation Job

Standard compilation — produces executable or library:
1. Parse sources
2. Symbol resolve (via ASTProcessor)
3. Type verify
4. C translation (2c)
5. Backend codegen (LLVM or TinyCC)
6. Link

### Interpretation Job

When `--arg-interpret` is passed:
```cpp
int LabBuildCompiler::do_interpretation_job(LabJob* job) {
    // 1. Parse interpret/ + common/ modules
    // 2. Symbol resolve
    // 3. Type verify
    // 4. Initialize module-level VarInitStmt on global scope
    // 5. Find main() and call it via AST interpreter
    // No codegen, no linking
}
```

### CBI Plugin Job

Compiles a compiler plugin:
```cpp
int LabBuildCompiler::link_cbi_job(LabJobCBI* cbiJob, std::vector<LabModule*>& dependencies) {
    // 1. Parse plugin source
    // 2. Symbol resolve
    // 3. Type verify
    // 4. C translation
    // 5. TinyCC JIT compile
    // 6. Register plugin in CompilerBinder
}
```

## Thread Safety Architecture

### What's Serial
- Top-level declaration (Pass 1)
- After-link-signature (Pass 4)
- Module scope start/end
- Main function no_mangle marking

### What's Parallel
- File importing (lex + parse)
- Link signatures (Pass 2)
- Generic instantiation (Pass 3)
- Generic decl body linking (Pass 5)
- Full body linking (Pass 6)
- Type verification

### Synchronization Points
- **`print_mutex`**: Guards diagnostic output — each parallel task acquires it before printing
- **`import_mutex`**: Guards the file cache during recursive imports
- **`generic_inst_reg_mutex`**: Guards generic instantiation registration maps (recursive mutex)
- **`implsIndex.index_mutex`**: A `std::shared_mutex` — shared_lock for reads, unique_lock for writes

### Allocator Strategy
```cpp
file_allocator.clear();  // Called after each pass — frees temporary AST nodes
mod_allocator;            // Lifetime = module compilation
job_allocator;            // Lifetime = entire job
ast_allocator;            // Lifetime = entire compilation session
```

## Key Files Reference

| File | Lines | Role |
|------|-------|------|
| `compiler/lab/LabBuildCompiler.cpp` | ~5000+ | Full build system: job creation, execution, link, interpretation, plugin compilation |
| `compiler/lab/LabBuildCompiler.h` | ~200 | Core build compiler class declaration |
| `compiler/ASTProcessor.cpp` | ~1500 | The pass orchestration: sym_res_module, type_verify_module, declare/implement module |
| `compiler/ASTProcessor.h` | ~200 | ASTProcessor class declaration |
| `compiler/lab/mod_conv/ModToLabConverter.cpp` | ~300 | chemical.mod → build.lab conversion |
| `compiler/lab/mod_conv/ModToLabConverter.h` | ~50 | ModToLabConverter header |
| `compiler/lab/LabJob.cpp` | ~100 | Job implementation |
| `compiler/lab/LabBuildContext.cpp` | ~300 | Module and resource management |
| `compiler/lab/LabBuildContext.h` | ~200 | LabBuildContext header |
| `compiler/lab/LabModule.h` | ~50 | Module structure |
| `compiler/lab/ModuleStorage.h` | ~50 | Module storage and lookup |

## The build.lab API (Lab Module)

The `lab` module (`lang/libs/lab/src/lab.ch`) exposes the build system API to `build.lab` scripts. Every `build.lab` file imports this module and calls its API to define modules, dependencies, and jobs.

### Key Interfaces

#### `BuildContext` Interface

The primary interface for build scripts. Available as `ctx` in the `build()` function:

```chemical
@compiler.interface
public interface BuildContext {
    // Module creation
    func new_package(&self, type : ModuleType, package_kind : PackageKind,
        scope_name : &std::string_view, name : &std::string_view,
        dependencies : std::span<ModuleDependency>) : *mut Module

    // Caching
    func get_cached(&self, job : *LabJob, path : &std::string_view) : *mut Module
    func set_cached(&self, job : *LabJob, path : &std::string_view, module : *mut Module)

    // Module management
    func add_path(&self, module : *mut Module, path : &std::string_view)
    func add_dependency(&self, job : *mut LabJob, module : *mut Module, info : *mut DependencySymbolInfo)
    func add_module(&self, job : *mut LabJob, module : *mut Module)
    func put_job_before(&self, newJob : *mut LabJob, existingJob : *mut LabJob)

    // Linking
    func link_system_lib(&self, job : *mut LabJob, name : &std::string_view, module : *mut Module = null)
    func add_lib_search_path(&self, job : *mut LabJob, path : &std::string_view, module : *mut Module = null)
    func add_object(&self, job : *LabJob, path : &std::string_view)

    // Job creation (returns new LabJob*)
    func build_exe(&self, name : &std::string_view) : *mut LabJob
    func run_jit_exe(&self, name : &std::string_view) : *mut LabJob
    func build_dynamic_lib(&self, name : &std::string_view) : *mut LabJob
    func build_interpretation(&self, name : &std::string_view) : *mut LabJob
    func build_cbi(&self, name : &std::string_view) : *mut LabJobCBI
    func translate_to_c(&self, name : &std::string_view, output_path : &std::string_view) : *mut LabJob
    func translate_to_chemical(&self, module : *mut Module, output_path : &std::string_view) : *mut LabJob

    // Remote imports
    func fetch_mod_dependency(&self, job : *mut LabJob, mod : *mut Module,
        dep : &ImportRepo, strategy : ConflictResolutionStrategy = ...) : bool
    func fetch_job_dependency(&self, job : *mut LabJob,
        dep : &ImportRepo, strategy : ConflictResolutionStrategy = ...) : bool

    // CLI argument handling
    func has_arg(&self, name : &std::string_view) : bool
    func get_arg(&self, name : &std::string_view) : std::string_view
    func define(&self, job : *LabJob, name : &std::string_view) : bool
    func undefine(&self, job : *LabJob, name : &std::string_view) : bool

    // Testing
    func set_environment_testing(&self, job : *mut LabJob, value : bool)

    // Compiler interfaces (for CBI plugins)
    func add_compiler_interface(&self, module : *mut Module, interface : &std::string_view) : bool
    func contains_cbi(&self, key : &std::string_view) : bool
    func index_cbi_fn(&self, job : *mut LabJobCBI, key : &std::string_view,
        fn_name : &std::string_view, fn_type : CBIFunctionType) : bool

    // Tool invocation
    func build_path(&self) : std::string_view
    func invoke_ar(&self, string_arr : std::span<std::string_view>) : int
    func invoke_ranlib(&self, string_arr : std::span<std::string_view>) : int
}
```

#### `LabJob` Interface

Represents a compilation job:
```chemical
public interface LabJob {
    func getType(&self) : LabJobType
    func getName(&self) : std::string_view
    func getAbsPath(&self) : std::string_view
    func getBuildDir(&self) : std::string_view
    func getStatus(&self) : LabJobStatus
    func getTargetTriple(&self) : std::string_view
    func getMode(&self) : OutputMode
    func getTarget(&self) : &TargetData
    func setAbsPath(&self, path : std::string_view)
}
```

#### `Module` Interface

Represents a compiled module:
```chemical
public interface Module {
    func getType(&self) : ModuleType
    func getScopeName(&self) : std::string_view
    func getName(&self) : std::string_view
    func getBitcodePath(&self) : std::string_view
    func setBitcodePath(&self, path : &std::string_view)
    func getObjectPath(&self) : std::string_view
    func setObjectPath(&self, path : &std::string_view)
    // ... etc
}
```

### Key Enums

```chemical
public enum ModuleType { File, CFile, CPPFile, ObjFile, Directory }
public enum PackageKind { Library, Application }
public enum LabJobType {
    Executable, JITExecutable, Library, ToCTranslation,
    ToChemicalTranslation, ProcessingOnly, CBI, Interpretation
}
public enum LabJobStatus { Pending, Launched, Success, Failure }
public enum OutputMode : int {
    Debug, DebugQuick, DebugComplete,
    ReleaseFast, ReleaseSmall, ReleaseSafe
}
```

### Common Patterns in build.lab

#### Pattern 1: Standard Executable
```chemical
import lab

func build(ctx : *mut AppBuildContext) : *mut LabJob {
    const exe_job = ctx.build_exe("my_app")
    const deps = [/* dependency modules */]
    const mod = ctx.directory_app_module("", "main", "src", deps)
    ctx.add_module(exe_job, mod)
    return exe_job
}
```

#### Pattern 2: Remote Dependency
```chemical
ctx.fetch_mod_dependency(job, mod, ImportRepo{
    from : "github.com/owner/repo",
    version : "v1.0",
    location : intrinsics::get_raw_location()
})
```

#### Pattern 3: CBI Plugin
```chemical
const cbi_name = std::string_view("my_plugin")
const cbi_job = ctx.build_cbi(&cbi_name)
ctx.add_module(cbi_job, my_plugin_mod)
```

#### Pattern 4: Conditional Build
```chemical
if(ctx.has_arg("interpret")) {
    const interp_job = ctx.build_interpretation("my-interp")
    // ... add modules
    return interp_job
}
```

#### Pattern 5: Build Path Helpers (in `lab` namespace)
```chemical
// Path to the current build.lab's directory
lab::curr_dir()

// Absolute path relative to current build.lab
lab::rel_path_to("src")

// Build directory paths
ctx.build_job_dir_path("job_name")
ctx.build_mod_file_path("job", "scope", "mod", "file")
ctx.build_llvm_ir_path("job", "scope", "mod")

// Common helpers
const mod = ctx.directory_app_module("", "main", lab::rel_path_to("src"), deps)
```

### How Tests Are Wired (lang/tests/build.lab Example)

See the [Testing Guide](./.agents/skills/testing/SKILL.md) for how `lang/tests/build.lab` wires:
- Interpretation tests (`--arg-interpret`)
- Compiled tests (default)
- Library tests (`--arg-test-libs`)
- Individual library tests (`--arg-test-html`, `--arg-test-css`, etc.)

The test wiring uses `ctx.build_interpretation()`, `ctx.build_exe()`, `set_environment_testing()`, `has_arg()`, and `define()` to configure the build environment.

## Related Skills

- **Symbol Resolution** (`.agents/skills/symres/SKILL.md`) — Detailed symres pipeline
- **Generic Instantiation** (`.agents/skills/generics/SKILL.md`) — Generic monomorphization
- **C Codegen** (`.agents/skills/c_codegen/SKILL.md`) — 2c translation backend
- **Performance** (`.agents/skills/performance/SKILL.md`) — Optimization and parallelization patterns
- **Compiler Bindings** (`.agents/skills/compiler_bindings/SKILL.md`) — CBI and TinyCC integration
- **Testing Guide** (`.agents/skills/testing/SKILL.md`) — How tests are wired and executed
