---
name: Build System (LabBuildCompiler)
description: Comprehensive guide to the Lab build system — how build.lab and chemical.mod files drive compilation, jobs, plugins, caching, and dependency management.
---

# Build System (LabBuildCompiler)

The Lab build system is the entry point for the Chemical compiler. It reads `build.lab` or `chemical.mod` files, creates compilation jobs, manages dependencies, coordinates compiler plugins, and orchestrates the entire compilation pipeline.

## Architecture Overview

### CLI Entry Point

The compiler entry is in `core/main/CompilerMain.cpp`:

```cpp
// Simplified flow:
1. Parse CLI arguments → LabBuildCompilerOptions
2. Create LabBuildCompiler instance
3. Load the input file (build.lab or chemical.mod)
4. JIT-compile build.lab via TinyCC
5. Execute the build script → creates LabJob objects
6. Execute each job:
   a. Parse source files
   b. Symbol resolution
   c. Type verification
   d. Codegen (LLVM, C, or both)
   e. Link
```

### Key Files

| File | Lines | Purpose |
|------|-------|---------|
| `compiler/lab/LabBuildCompiler.h` | ~200 | Core build compiler class |
| `compiler/lab/LabBuildCompiler.cpp` | ~5000+ | Full build system implementation |
| `compiler/lab/LabBuildContext.h/.cpp` | ~300 | Module and resource management |
| `compiler/lab/LabJob.h` | ~150 | Job class — represents a compilation task |
| `compiler/lab/LabJob.cpp` | ~100 | Job implementation |
| `compiler/lab/LabJobType.h` | ~30 | Job type enum |
| `compiler/lab/LabModule.h` | ~50 | Module representation |
| `compiler/lab/LabModuleType.h` | ~20 | Module type enum |
| `compiler/lab/BackendContext.h` | ~80 | Abstract backend interface |
| `compiler/lab/LabBuildCompilerOptions.h` | ~80 | Compiler options |
| `compiler/lab/Utils.h` | ~50 | Build system utilities |
| `compiler/lab/mod_conv/ModToLabConverter.h/.cpp` | ~300 | chemical.mod → build.lab converter |
| `compiler/lab/timestamp/Timestamp.h/.cpp` | ~100 | Timestamp-based caching |
| `compiler/lab/import_model/` | Various | Remote import, dependency, version resolution |
| `compiler/lab/transformer/TransformerContext.h` | ~50 | Transformer context for build plugins |

## LabBuildCompiler Lifecycle

### 1. Initialization

```cpp
LabBuildCompiler::LabBuildCompiler(const LabBuildCompilerOptions& opts) {
    // 1. Set up ASTAllocator, type builder, target data
    // 2. Create TinyCC instance for JIT compilation
    // 3. Initialize ImportPathHandler
    // 4. Set up CompilerBinder for CBI functions
}
```

### 2. Loading Input

```cpp
// For chemical.mod → convert to build.lab first:
ModToLabConverter::convert(modPath);
// This translates the concise .mod format into a full .lab build script

// For build.lab → JIT compile directly:
LoadBuildLabFile(jobPath, labBuildCompilerOptions);
```

### 3. JIT Compilation with TinyCC

The build script is translated to C (via the 2c backend), then compiled in memory using TinyCC:

```cpp
void LabBuildCompiler::compile_build_lab(const chem::string& cCode) {
    // 1. Pass C code to TinyCC
    tcc.compile_string(cCode);
    
    // 2. Add necessary symbols for the build script API
    tcc.add_symbol("create_job", (void*)&create_job_cbi);
    tcc.add_symbol("add_dependency", (void*)&add_dependency_cbi);
    // ... many more symbols
    
    // 3. Relocate (JIT finish)
    tcc.relocate();
}
```

### 4. Job Creation

The executed build script calls CBI (Compiler Binding Interface) functions to create jobs:

```cpp
// CBI function called from build.lab:
void create_job_cbi(const char* name, int jobType, void* config) {
    // 1. Create a LabJob object
    auto job = new LabJob(name, static_cast<LabJobType>(jobType));
    
    // 2. Add it to the build compiler's job list
    build_compiler->jobs.push_back(job);
    
    // 3. Return a handle for further configuration
}
```

### 5. Job Execution

Each job goes through the full compilation pipeline:

```
LabJob → Parse sources → Symbol Resolution → Type Verify → Codegen → Link
               ↓               ↓                  ↓           ↓        ↓
         Parser.cpp      SymResLinkBody     TypeVerify.cpp  LLVM.cpp  Linker
```

## Job Types

| Job Type | Purpose |
|----------|---------|
| `LabJobType::Compilation` | Standard compilation — produces an executable or library |
| `LabJobType::Interpretation` | AST interpretation only — no codegen (for --arg-interpret) |
| `LabJobType::CBIPlugin` | Compiler plugin (macro processor) registration |
| `LabJobType::Transformer` | AST transformer for code generation |

### Compilation Job

```cpp
struct LabJob {
    chem::string name;                    // Job name
    LabJobType type;                      // Job type
    std::vector<LabModule*> modules;      // Modules to compile
    std::vector<LabJob*> dependencies;    // Dependencies
    chem::string outputPath;              // Output path
    bool isExecutable;                    // Produce executable vs library
    // ... backend selection, optimization level, etc.
};
```

### Interpretation Job

When `--arg-interpret` is passed, the build system creates an `LabJobType::Interpretation` job instead of a compilation job:

```cpp
void do_interpretation_job(LabJob* job) {
    // 1. Parse modules (interpret/ + common/)
    // 2. Symbol resolve
    // 3. Type verify
    // 4. Initialize global vars (VarInitStmt)
    // 5. Find and call main() via the AST interpreter
}
```

## Plugin System

### How Plugins Work

1. **Registration**: Build script calls `register_plugin("name")` which creates a `LabJobType::CBIPlugin` job
2. **Compilation**: The plugin's Chemical source is compiled via TinyCC JIT
3. **Hooks**: The plugin exports functions with specific names that the compiler calls:
   - `process_macro(macro_name, args)` — handles `#macro_name` annotations
   - `process_lex_token(token)` — custom lexing
   - `provide_semantic_tokens()` — for LSP

### Plugin Compilation

```cpp
void LabBuildCompiler::compile_plugin(LabJob* pluginJob) {
    // 1. Compile the plugin's Chemical source to C (2c backend)
    // 2. JIT-compile the C with TinyCC
    // 3. Add plugin symbols to the compiler binder
    // 4. Initialize the plugin (call its init function)
    
    auto cCode = generateCForPlugin(pluginJob);
    tcc.compile_string(cCode);
    tcc.add_symbol("plugin_init", pluginJob->initFunc);
    tcc.relocate();
    
    // Now the plugin's process_macro can be called during compilation
    binder.registerPlugin(pluginJob->name, pluginJob);
}
```

### Built-in Plugin Libraries

Located in `lang/libs/`:

| Plugin | Purpose |
|--------|---------|
| `html_cbi` | `#html` macro — HTML-style JSX generation |
| `css_cbi` | `#css` macro — CSS generation |
| `js_cbi` | `#js` macro — JavaScript generation |
| `universal_cbi` | `#universal` component — SSR + hydration |
| `react_cbi` | `#react` component — React-style JSX |
| `preact_cbi` | `#preact` component — Preact-style JSX |
| `solid_cbi` | `#solid` component — SolidJS-style JSX |
| `md_cbi` | Markdown processing |

## Caching System

### Timestamp-Based Cache

```cpp
struct Timestamp {
    std::filesystem::file_time_type lastModified;
    size_t fileSize;
    // ...
};
```

The cache works at multiple levels:

1. **Object file cache**: `--cache` flag enables reuse of previously compiled objects
2. **Plugin cache**: `--cached-plugins` skips recompilation of unchanged CBI plugins
3. **Module cache**: Compiled modules are cached based on their dependency hash

### Cache Invalidation

A module's cache is invalidated when:
- Any source file in the module has changed (timestamp)
- Any dependency module has been recompiled
- Compiler flags that affect codegen have changed
- Plugin versions have changed

## Dependency Management

### ImportPathHandler

The `ImportPathHandler` resolves module imports:

1. **Local imports**: `import "../lib_mod"` — resolves to file path
2. **Remote imports**: `import "github.com/owner/repo"` — downloads via git
3. **Version pinning**: `version "1.0.0"` — resolves semantic versions
4. **Orphan branches**: `orphan branch "branch"` — specific git branches
5. **Subdirectories**: `subdir "subdirectory"` — monorepo support
6. **Conditional imports**: `if windows` — platform-specific

### Conflict Resolution

When two modules depend on different versions of the same library:

```cpp
// In ImportPathHandler:
// Parse both versions as semantic versions
// If one is newer → keep newer, discard older
// If equal → keep one
// If incompatible → error
```

## Parallelization Strategies

### Current Parallelization

1. **Per-file symres**: Files within a module are symbol-resolved in parallel
2. **Generic instantiation**: Multiple instantiations can be finalized independently (with mutex for registration)

### Future Opportunities

1. **Per-module compilation**: Independent modules can be compiled in parallel
2. **Per-job execution**: Jobs with no dependency chain can run concurrently
3. **Plugin compilation**: Plugins can be compiled ahead of time, in parallel
4. **Caching lookups**: Cache checks are independent per module

## Key CBI Functions

These functions are exposed to `build.lab` scripts:

| CBI Function | Purpose |
|--------------|---------|
| `create_job(name, type)` | Create a new compilation job |
| `add_source(job, path)` | Add a source file to a job |
| `add_dependency(job, dep)` | Add a dependency between jobs |
| `set_output(job, path)` | Set output path |
| `register_plugin(name, init)` | Register a compiler plugin |
| `add_link_library(job, lib)` | Add a link-time library |
| `add_link_path(job, path)` | Add a link search path |
| `set_job_option(job, key, value)` | Set job-specific options |

## Debugging the Build System

### Common Issues

| Issue | Cause | Debug |
|-------|-------|-------|
| `TinyCC compilation error` | The generated C for build.lab has syntax errors | Use `--emit-c` to inspect generated C |
| `Symbol not found in build.lab` | Missing CBI function export | Check `tcc_add_symbol` calls |
| `Module not found` | ImportPathHandler couldn't resolve path | Check module search paths |
| `Plugin not found` | Plugin source path is wrong | Check plugin registration |
| `Cache invalidation wrong` | Timestamp comparison incorrect | Add `--no-cache` to force recompilation |
| `Linker error` | Missing symbols at link time | Check `add_link_library` calls |

### Verbose Output

Use `-v` flag for verbose output showing each build step:

```bash
cmake-build-debug/TCCCompiler "lang/tests/build.lab" -o tests.exe -v
```

### Plugin Debug Mode

```bash
--plugin-mode debug_complete  # Compile plugins in debug mode for full stack traces
```

## Extending the Build System

### Adding a New Job Type

1. Add to `LabJobType.h` enum
2. Add handling in `LabBuildCompiler::execute_job()`
3. Add CBI function for creation if needed

### Adding a New Compiler Flag

1. Add to `LabBuildCompilerOptions.h`
2. Parse in `CompilerMain.cpp` or `clang_driver.cpp`
3. Use in appropriate pipeline phase

### Adding a New Backend

1. Implement the `BackendContext` interface
2. Implement codegen visitor(s) matching the backend's output
3. Register the backend in `LabBuildCompiler::create_backend()`
