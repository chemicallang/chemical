# AGENTS.md — Chemical Programming Language

## Setup (after cloning)

```bash
./scripts/setup.sh                 # Downloads libtcc, updates submodules
./scripts/setup.sh --with-llvm     # Also download prebuilt LLVM (v22.1.1)
./scripts/configure.sh             # CMake configure → cmake-build-debug/
./scripts/configure.sh --no-llvm   # BUILD_COMPILER=OFF (TCCCompiler only)
```

- `setup.sh` runs `git submodule update --init --recursive` before downloading libtcc.
- Prebuilt LLVM from `chemicallang/llvm-prebuilt`.
- LSP depends on `lib/lsp-framework` submodule.

## Build

```bash
./scripts/build.sh --tcc           # TCCCompiler (TinyCC backend)
./scripts/build.sh --llvm          # Compiler (LLVM/Clang backend)
./scripts/build.sh --lsp           # ChemicalLsp
./scripts/build.sh --all           # All targets
./scripts/build.sh --llvm -j 16    # Parallel build
```

- Build dir: `cmake-build-debug/` (CI releases use `out/build`).
- `Compiler` + `TCCCompiler` both link dynamically with TinyCC (`libtcc.so`).
- `Compiler` additionally links LLVM + Clang + LLD.
- macOS: no static linking of standard libraries.

## Build TUI

```bash
./scripts/tui.sh                   # Interactive curses menu for all scripts
./scripts/tui.sh --run myconfig    # Load saved config, run all commands headless
```

Configs saved as JSON in `scripts/tui-configs/`. Last config auto-restored.

## Test

```bash
./scripts/test.sh --tcc            # Build TCCCompiler, compile & run tests
./scripts/test.sh --llvm           # Build Compiler (LLVM), compile & run tests
./scripts/test.sh --tcc --libs     # Include library tests (html, css, js, etc.)
./scripts/test.sh --tcc --negative # Negative tests (compiler failure verification)
./scripts/test.sh --tcc --no-run   # Compile only, don't run
./scripts/test.sh --tcc --no-build # Use existing compiler binary
./scripts/test.sh --tcc --mode debug_complete  # Override compilation mode
./scripts/test.sh --tcc --cache    # Use cached objects (default: --no-cache)
./scripts/test.sh --tcc --cached-plugins  # Skip CBI plugin recompile
./scripts/test.sh --tcc --emit-c   # Emit C translation output
./scripts/test.sh --tcc --use-c    # Translate to C + compile with embedded Clang
./scripts/test.sh --tcc -g         # Debug symbols
./scripts/test.sh --tcc -o my_tests -j 8  # Custom output + parallel jobs
```

Internal defaults: `--no-cache`, `-frecompile-plugins`. Pass `--cache`/`--cached-plugins` to opt out.

> ⚠️ **`--no-build` warning**: This flag **skips rebuilding the C++ compiler binary**.
> Any changes you make to `.cpp`/`.h` files in the compiler source **will NOT be picked up** —
> the previously built binary is used unchanged. Only use `--no-build` when you are iterating
> on `.ch` test files or Chemical library sources **without any compiler C++ changes**.
> To include C++ changes, omit `--no-build` (or run once without it to rebuild first,
> then iterate with `--no-build`).

### Interpretation Tests

```bash
./scripts/test.sh --tcc --interpret        # Build TCC + run interpretation tests
./scripts/test.sh --tcc --interpret --no-build  # Skip rebuild
```

Manual:
```bash
./chemical lang/tests/build.lab --arg-interpret --mode debug_complete --no-cache
```

The `--arg-interpret` flag causes `build.lab` to create a `LabJobType::Interpretation` job instead of a compiled executable. The compiler calls `do_interpretation_job()` in `compiler/lab/LabBuildCompiler.cpp`, which:
- Parses, symres, and typechecks `interpret/` + `common/` modules
- Initializes module-level `VarInitStmt` nodes on the global scope
- Finds the `main()` function and calls it directly via the AST interpreter
- Never generates object code or links an executable

### Compiled (Runtime) Tests

```bash
# CI equivalent (runs from release binary, no `--no-cache`):
./chemical lang/tests/build.lab -arg-minimal -bm -v --assertions --mode debug_complete

# Lib tests CI:
./chemical lang/tests/build.lab -arg-test-libs -bm -v --assertions --mode debug_complete --no-cache

# Add `--emit-c` to inspect the C translation output:
./scripts/test.sh --tcc --emit-c
```

### How Tests Work

**Common test framework** (`lang/tests/common/src/test.ch`):
```chemical
@extern public func printf(format : *char, _ : any...) : int
public func test(name : *char, assert : () => bool) {
    if(assert()) {
        comptime if(intrinsics::is_interpretation()) {
            intrinsics::expr_println(`${ANSI_COLOR_GREEN}Test ${total_tests + 1} [${name}] succeeded${ANSI_COLOR_RESET}`);
        } else {
            printf("%sTest %d [%s] succeeded %s\n", ANSI_COLOR_GREEN, total_tests + 1, name, ANSI_COLOR_RESET);
        }
    }
}
```

Uses `comptime if(intrinsics::is_interpretation())` to select the `println` path (interpretation) or `printf` path (compiled executable). Both paths produce identical output.

**Interpretation path:** `intrinsics::expr_println(expr: %expressive_string)` — walks the expressive string's segment list, writes `StringValue` literals directly to `std::cout`, evaluates `${}` expression segments via `RepresentationVisitor` with `interpret_representation = true`. Defined in `ast/utils/GlobalFunctions.cpp` (class `InterpretExprPrintLn`).

**Compiled path:** `printf` with `ANSI_COLOR_*` constants — standard C printf used at runtime. ANSI escape sequences are embedded as string constants.

**`@test` annotation flow:** `intrinsics::get_tests<TestFunction>()` collects all `@test`-annotated functions. `test_runner(argc, argv)` from the `test` library dispatches them, each in a separate process via IPC.

### Test structure

- `lang/tests/common/src/test.ch` — shared test framework (`test()`, `assertEquals()`, `print_test_stats()`). Used by both interpretation and compiled modes.
- `lang/tests/common/src/main.ch` — `run_common_tests()` with basic arithmetic and loop tests.
- `lang/tests/interpret/` — interpretation-specific module (`interpret_tests`): imports `common_tests`, calls `run_common_tests()` then `print_test_stats()`. Entry point for `--arg-interpret`.
- `lang/tests/src/tests.ch` — compiled executable entry: `main()` calls `run_executable_tests()` then `test_runner()`.
- `lang/tests/src/test.ch` — deprecated; consolidated into `common/src/test.ch`.
- `lang/tests/negative/src/main.ch` — **Negative tests**: invokes the compiler via `popen`, compiles `.ch` files with invalid code, checks exit code and stderr for expected errors. Entry point for `--arg-negative`. Uses `intrinsics::get_compiler_path()`.
- **Inline tests**: manually listed in `tests.ch` via `test(name, () => bool)`.
- **`@test` annotations**: auto-discovered by `test_runner(argc, argv)` from `test_env` lib.
- Source dirs: `basic/`, `comptime/`, `core/`, `generic/`, `libs/`, `nodes/`, `stdlib/`.
- Lib tests in `lang/tests/libs/*/src/`.

### Interpretation Mode Details

**Entry:** `do_interpretation_job()` in `compiler/lab/LabBuildCompiler.cpp:4701`.

**Module-level variable initialization:** Before calling `main()`, the interpreter iterates all module dependencies and interprets `VarInitStmt` nodes (top-level `var`/`const` declarations) on the global scope. This ensures counters like `total_tests`, `tests_passed`, `tests_failed` are initialized before any test runs.

**Lambda support:** `LambdaFunction::call()` (in `ast/values/LambdaFunction.cpp`) creates an `InterpretScope`, handles `self_param`, parameters, and captures, then interprets the lambda `Scope` body. Lambda calls are dispatched from `interpret_value` in `FunctionCall.cpp`.

**Expressive string printing:** `intrinsics::expr_println` (class `InterpretExprPrintLn` in `ast/utils/GlobalFunctions.cpp`) is an intrinsic that:
1. Evaluates its single `%expressive_string` argument
2. Iterates `ExpressiveString::values` (alternating `StringValue` literals and expression nodes)
3. String literals → written directly to `std::cout`
4. Expression nodes → `evaluated_value(scope)` then printed via `RepresentationVisitor::visit()` with `interpret_representation = true` (no quotes/escapes)
5. `expr_println` appends `\n`; `expr_print` does not

**`intrinsics::is_interpretation()`:** Checks `call_scope->global->build_compiler->current_job->type == LabJobType::Interpretation`. Used inside `comptime if` to eliminate the runtime branch during codegen.

**`intrinsics::expr_print` vs `intrinsics::println`:** The old `print`/`println` are variadic (`any...`), printing each argument space-separated. The new `expr_print`/`expr_println` take a single `%expressive_string` argument, enabling backtick template strings with `${}` interpolation.

## Skills

The following skills are available in `.agents/skills/`. An AI agent should load the relevant skill(s) before working on a particular subsystem. Skills provide comprehensive, up-to-date documentation of codebase internals.

### Core Language & Syntax

| Skill | Location | Description |
|-------|----------|-------------|
| Language Syntax | `.agents/skills/chemical_source/SKILL.md` | Chemical language syntax: types, structs, functions, variants, generics, annotations, lambdas, standard library API (`std::string`, `std::vector`, `std::unordered_map`, `std::ordered_map`, `Option`, `Result`) |
| chemical.mod | `.agents/skills/chemical_mod/SKILL.md` | Module declaration syntax: package types, source paths, imports, conditional deps, remote imports, version pinning, link libraries, C file linking |

### Compiler Internals

| Skill | Location | Description |
|-------|----------|-------------|
| Symbol Resolution | `.agents/skills/symres/SKILL.md` | The full symres pipeline: `TopLevelDeclSymDeclare`, `TopLevelLinkSignature`, `GenericInstantiationPass`, `SymResLinkBody`, `SymbolTable`, `ImplementationsIndex`, `CoreNodes`, move semantics tracking, parallelization per-file |
| Generic Instantiation | `.agents/skills/generics/SKILL.md` | Monomorphization pipeline: `GenericInstantiator`, `InstantiationsContainer`, thread-safe registration with mutex + condition_variable, `FinalizeSignature`/`FinalizeBody`, `GenericInstantiationPass`, `InstantiationRequirement` |
| Parser Internals | `.agents/skills/parser_internals/SKILL.md` | Recursive descent parser: per-construct parsers, expression precedence climbing, type parsing, statement dispatch, `@annotation` system, error recovery, AST allocator |
| Type Verification | `.agents/skills/type_verification/SKILL.md` | Type checking pass: `TypeVerifier` visitor, return type checking, variable/assignment types, expression operator validation, implicit conversions, `unsatisfied_type_err` |
| Diagnostics | `.agents/skills/diagnostics/SKILL.md` | Error reporting system: `ASTDiagnoser`, `Diag` structure, severity levels, `SourceLocation` encoding, per-phase collection, parallel-safe merging, `ASTDiag.h` helpers |
| Performance | `.agents/skills/performance/SKILL.md` | Optimization patterns: arena allocation (`ASTAllocator`), SSO strings (`chem::string`), parallelization (file-level, generic instantiation), caching strategies, memory optimization, profiling tips |
| Testing Guide | `.agents/skills/testing/SKILL.md` | Test infrastructure: manual `test()` calls vs `@test` annotation dispatch, `test_env` library, `lang/tests/build.lab` wiring, inline/compiled/interpret/library tests, writing tests for compiler failures |

### Backends

| Skill | Location | Description |
|-------|----------|-------------|
| LLVM Backend | `.agents/skills/llvm_backend/SKILL.md` | LLVM IR codegen: `LLVMBackendContext`, type lowering, struct assignment (temp+destruct+memcpy), function lowering, control flow, `dso_local` gotcha, PHI nodes, debug info, parallelization |
| C Codegen (2c) | `.agents/skills/c_codegen/SKILL.md` | C translation: `ToCAstVisitor`, struct/function/variant translation, sret compound expression pattern, method chains, name generation, break/continue as goto, `BufferedWriter` |

### Build System & Plugins

| Skill | Location | Description |
|-------|----------|-------------|
| Build System (Lab) | `.agents/skills/build_system/SKILL.md` | Lab build system: `LabBuildCompiler` lifecycle, JIT compilation via TinyCC, job types (Compilation/Interpretation/CBIPlugin/Transformer), plugin compilation, caching, dependency management, parallelization |
| Macro Codegen | `.agents/skills/macro_code_gen/SKILL.md` | How `#html`, `#preact`, `#react`, `#solid`, `#universal` macros generate code — server-side function generation, JS bundle emission |
| Compiler Bindings (CBI) | `.agents/skills/compiler_bindings/SKILL.md` | How CBI works: TinyCC binding, build process, compiler API exposure, LSP integration, lifecycle |
| Compiler Plugin API | `.agents/skills/cbi_plugin_api/SKILL.md` | Plugin development: `CompilerBinder`, `ASTBuilder` API, macro registration, individual plugin structure (html_cbi, css_cbi, etc.), debugging, testing |

### Web & Components

| Skill | Location | Description |
|-------|----------|-------------|
| Universal Components | `.agents/skills/universal/SKILL.md` | Universal component pipeline: SSR + hydration, runtime behavior, prop serialization, subscriber mutation bugs, debugging generated HTML/CSS/JS |
| Designing Web Apps | `.agents/skills/design_web_app/SKILL.md` | Web app design in Chemical: static pages, `#html`/`#css`/`#js` macros, `#universal` components, `page` library, server + client rendering |

### Compiler API (for plugin development)

| Skill | Location | Description |
|-------|----------|-------------|
| Compiler API | `.agents/skills/compiler_api/SKILL.md` | Compiler API bindings in `lang/libs/compiler/`: ASTBuilder, Lexer, Parser, SymbolResolver, SourceProvider, BatchAllocator |
| Intrinsics & Reflection | `.agents/skills/intrinsics_compiler_reflection/SKILL.md` | Compiler intrinsics: all `intrinsics::*` functions, interpreter-friendly std types (`InterpretVector`), reflection APIs for module/type/function metadata, how to add new intrinsics |
| Interpreter Internals | `.agents/skills/interpreter/SKILL.md` | AST interpreter: `InterpretScope`, move semantics (`move_clear_source`), temp struct destruction, pointer bounds, function calls, debugging interpretation tests, interpreter-friendly std data structures |

### How to load a skill

Use the `skill` tool with the skill name (the directory name):

```
skill {
  name: "symres"            # loads .agents/skills/symres/SKILL.md
}
```

The skill name is the directory name under `.agents/skills/`. For example:
- `name: "symres"` → loads `.agents/skills/symres/SKILL.md`
- `name: "build_system"` → loads `.agents/skills/build_system/SKILL.md`
- `name: "universal"` → loads `.agents/skills/universal/SKILL.md`

### Which skill to load?

| Task | Load these skills |
|------|-------------------|
| Writing Chemical code | `chemical_source` |
| Setting up a module (`chemical.mod`) | `chemical_mod` |
| Building/testing the compiler | `building` |
| **Understanding symbol resolution or fixing symres bugs** | **`symres`**, `generics` |
| **Debugging LLVM IR output** | **`llvm_backend`** |
| **Debugging C translation issues** | **`c_codegen`** |
| **Understanding parallel compilation / making compiler faster** | **`performance`** |
| **Writing a compiler plugin** | **`cbi_plugin_api`**, `compiler_api`, `compiler_bindings` |
| **Fixing generic instantiation issues** | **`generics`**, `symres` |
| **Debugging parser errors** | **`parser_internals`** |
| **Understanding type checking** | **`type_verification`** |
| **Debugging error messages / improving diagnostics** | **`diagnostics`** |
| **Understanding the build pipeline / parallelizing builds** | **`build_system`** |
| **Working on web apps** | `design_web_app`, `universal` |
| **Working on interpreter/comptime tests** | `interpreter` |
| **Writing and organizing tests** | **`testing`** |
| **Building CBI macro plugins** | `compiler_bindings`, `macro_code_gen`, `compiler_api` |
| **Extending libraries (`lang/libs/`)** | `compiler_api`, `cbi_plugin_api` |

Skills in **bold** are the new comprehensive skills. Load them for maximum context.

## Architecture

| Path | Purpose |
|------|---------|
| `ast/` | AST node definitions (values, types, structures, statements) |
| `compiler/` | Compilation pipeline |
| `compiler/backend/LLVM.cpp` | LLVM codegen (primary backend) |
| `compiler/backend/CLANG.cpp` | Clang/LLVM driver integration |
| `compiler/symres/` | Symbol resolution (semantic analysis) |
| `compiler/lab/` | Build system (Lab), `build.lab` execution |
| `compiler/cbi/` | Compiler Binding Interface (C plugins) |
| `compiler/generics/` | Monomorphization/generic instantiation |
| `compiler/typeverify/` | Second type-verification pass |
| `parser/` | Recursive descent parser |
| `lexer/` | Tokenizer |
| `preprocess/2c/` | C translation (2c) visitors and codegen |
| `core/main/CompilerMain.cpp` | CLI entrypoint |
| `server/` | LSP server implementation |
| `lang/libs/` | Standard library + CBI macro plugin libs |
| `lang/libs/compiler/` | Compiler API bindings for macros |
| `lang/tests/` | Test suite |

## Conventions

- Custom strings: `chem::string` (SSO, no `std::string`).
- Name mangling: scoped → `scope_name` prefix, generics → `__cgs__N`/`__cfg__N` suffix.
- Operator overloading: `a + b` → looks for `add(a, b)` method.
- `#macro` and `#universal` are CBI plugins compiled by TinyCC at build time.
- AST uses `ASTAllocator` arena — batch-allocated, no per-node `delete`.
- CLI entry is `LabBuildCompiler` → translates `chemical.mod` → JIT-compiles `build.lab` (TinyCC) → `LabJob` objects → parse → symres → typecheck → codegen → link.

## LLVM Backend Gotchas

### External declarations must not use `dso_local`

When a global variable is declared in another module (`submod_extern_globe_var`), do **not** set `dso_local` on the LLVM global value. The linker will reject `dso_local` + external declarations as mismatched. Relevant: `ast/statements/VarInit.cpp`, `compiler/backend/LLVM.cpp`.

### Struct assignment: temp + destruct + memcpy

The LLVM backend (`StructValue.cpp`) assigns structs via a three-step pattern:
1. **Bitwise copy** source into a stack-allocated temp
2. **Call destructor** on the destination
3. **memcpy** the temp onto the destination

This pattern **breaks self-referencing pointers**. If a struct has a pointer field pointing to one of its own members (e.g., `function`'s `fn_data_ptr`), the bitwise copy produces a dangling pointer — the temp shares the same pointer, but after `memcpy` over the destination, the temp is destroyed. The destination's pointer now points to freed memory.

Fixes considered: `@reflat` annotation to skip destruct+memcpy, `@move` hook for custom move semantics, or forbidding self-referencing pointers inside value types.

### Unitialized variables and PHI nodes

LLVM requires `UndefValue` for uninitialized phis in certain lowering patterns. Check `IRBuilder::CreatePHI` usage in `LLVM.cpp` when adding new PHI-based constructs.

## Chemical Library Development Gotchas

These patterns were discovered while implementing 5 pure-Chemical libraries (archive, image, font, audio, webview).

### Struct Initialization Rules

Chemical has three struct annotation combinations that affect initialization:

| Annotations | `T{}` works? | `T.make()` works? | `T{field: val}` works? |
|---|---|---|---|
| `@direct_init` only | Yes (all fields required) | No | Yes (all fields required) |
| `@make` only | **No** | Yes | **No** |
| `@direct_init` + `@make` | Yes (all fields required) | Yes | Yes (all fields required) |
| Neither | Yes (all fields required) | No | Yes (all fields required) |

**Critical**: A struct with `@make` but **without** `@direct_init` **cannot** use `{}` syntax at all. The compiler errors with "struct has a constructor, use @direct_init to allow direct initialization". You must use `T.make()` instead.

Example:
```chemical
// types.ch
@make
public struct RGBA8 {
    var r : u8; var g : u8; var b : u8; var a : u8
    @make func make(r_ : u8, g_ : u8, b_ : u8, a_ : u8 = 255) : RGBA8 { ... }
}

// test.ch — WRONG: RGBA8{} doesn't work without @direct_init
// var px = RGBA8{}          // ERROR: struct has a constructor
// var px = RGBA8{r: 0}      // ERROR: struct has a constructor

// CORRECT:
var px = RGBA8.make(0, 0, 0, 255)   // Uses @make constructor
```

### vector Standalone Init

`vector<T>{}` aggregate init only works **inside** `@direct_init` struct init context. Standalone `var x = vector<T>{}` fails. Use the constructor instead:

```chemical
// WRONG (outside struct init):
// var file_data = vector<u8>{}

// CORRECT:
var file_data = vector<u8>()
```

### `.data()` Returns Immutable Pointer

`vector<T>.data()` returns `*T` (immutable). To write through the pointer, cast to `*mut T`:

```chemical
var sptr = vec.data() as *mut i16
sptr[0] = 10000
sptr[1] = -10000
```

### `.size()` Not `.length()`

`string` and `vector` use `.size()` — there is no `.length()` method.

### Float Literals

`0.5` is `double` in Chemical. For `float` parameters, use `0.5f`:

```chemical
public func audio_volume(audio : *mut Audio, factor : float) { ... }
audio_volume(&raw mut a, 0.5f)   // WRONG: 0.5 is double
audio_volume(&raw mut a, 0.5)    // TypeCheck error: double does not satisfy float
```

### `if` Expressions Not Inline

Chemical does **not** support `if` as an expression inline in function arguments. Extract to a variable:

```chemical
// WRONG:
// var max_samples = if(a > b) { a } else { b }

// CORRECT:
var max_samples : size_t
if(a > b) { max_samples = a } else { max_samples = b }
```

### `if` Requires `else`

Every `if` block requires an `else` block. No bare `if` without `else`.

### Variant-Captured Strings: `append_string(&msg)`

When capturing a string from a variant pattern, pass it by reference to `append_string`:

```chemical
switch(self) {
    InvalidFormat(msg) => {
        var s = string("Error: ")
        s.append_string(&msg)    // NOT append_string(msg)
        return s
    }
}
```

### `using` Declarations in Test Files

Test files at global scope need `using` declarations to access standard library types:

```chemical
using std::Result;    // For Result.Err / Result.Ok patterns
using std::string;    // For string() constructor
using std::vector;    // For vector<T>() constructor
```

### Internal Functions Not Accessible Across Packages

Functions without `public` (internal by default) cannot be called from test modules in other packages. Mark test-relevant helper functions `public`:

```chemical
// archive/src/endian.ch — WRONG: internal, can't be called from tests
// func read_u16_le(data : *u8, offset : size_t) : u16 { ... }

// CORRECT:
public func read_u16_le(data : *u8, offset : size_t) : u16 { ... }
```

### `memcpy` With Pointer Variables

When a variable is already `*mut T`, **never** use `&raw mut var` with `memcpy`. `&raw mut var` takes the address of the pointer variable itself (on the stack), not what it points to. Writing `sizeof(T)` bytes there corrupts the stack:

```chemical
func zip_find_entry(output : *mut ArchiveEntry, ...) {
    // WRONG — &raw mut output is *mut (*mut ArchiveEntry), stack corruption!
    // memcpy(&raw mut output, &raw entry, sizeof(ArchiveEntry))

    // CORRECT — output is already *mut ArchiveEntry:
    memcpy(output, &raw entry, sizeof(ArchiveEntry))
}
```

### Result Pattern in @test Functions

```chemical
@test
public func my_test(env : &mut TestEnv) {
    var result = some_func()
    if(result is Result.Err) { env.error("should succeed"); return }
    var Ok(value) = result else unreachable
    // Now use value safely
}
```

### Test File Location

Library tests go in `lang/tests/src/libs/<name>/tests.ch`, **not** `lang/tests/libs/`. The `lang/tests/src/` module is the compiled test executable's source tree.

### Building Libraries for Testing

```bash
# Compile-check a library:
cmake-build-debug/TCCCompiler lang/libs/<name>/chemical.mod -v -bm-modules

# Run all tests (including lib tests):
./scripts/test.sh --tcc
```

## C Codegen Gotchas

### Struct-returning function expression pattern

Functions returning structs are translated to C using the `(*({ ... }))` compound-expression pattern:
```c
(*({ struct Type __tmp; func(&__tmp, args...); &__tmp; }))
```
This allocates a temp, passes it as a hidden sret pointer, and dereferences the result to produce the struct value.

**Warning:** When the result is discarded (expression-statement context), `gcc -Wall` emits `-Wunused-value` on the `*` dereference. This is a pre-existing pattern throughout generated C.

### Method chains and `&self`

In a method chain like `a.b().c()`, the receiver of `c()` is the result of `b()`. When `c()` has no `&self` parameter (no receiver needed), the C codegen in `preprocess/2c/2cASTVisitor.cpp` must **not** create a receiver pointer variable. Before the fix, it created an unused `struct Type* __chx__recv__N` variable, producing "unused variable" warnings.

## Interpreter Internals (For Comptime Test Development)

The AST interpreter (`compiler/Interpreter/`) evaluates Chemical code directly without generating C or LLVM IR. It's used for:
- **Comptime evaluation**: `comptime` functions and `comptime { }` blocks run in the interpreter
- **Interpretation tests**: `--arg-interpret` runs the test suite through the interpreter

### Core Classes

| Class | File | Purpose |
|-------|------|---------|
| `InterpretScope` | `ast/base/InterpretScope.h` | Per-function/per-block scope with value map, parent chain, and pointer to `GlobalInterpretScope` |
| `GlobalInterpretScope` | `ast/base/GlobalInterpretScope.h` | Global state — holds allocator, type builder, call stack, backend context |
| `PointerValue` | `ast/values/PointerValue.h` | Simulates C pointers with `data` (void*), `behind` (bytes before), `ahead` (bytes after) for bounds checking |
| `Value` | `ast/base/Value.h` | Base class for all interpreted values (IntN, Bool, Float, String, Struct, etc.) |

### Pointer Model (`PointerValue`)

The interpreter simulates pointer semantics with bounds tracking:

```cpp
class PointerValue {
    void* data;        // Pointer to the actual data
    size_t behind;     // Bytes available BEFORE data (for backward navigation)
    size_t ahead;      // Bytes available AFTER data (for forward navigation)
};
```

- **Increment**: `data += sizeof(T)`, `behind += sizeof(T)`, `ahead -= sizeof(T)` — fails if `amount > ahead`
- **Decrement**: `data -= sizeof(T)`, `behind -= sizeof(T)`, `ahead += sizeof(T)` — fails if `amount > behind`
- **Dereference**: checks `typeSize <= ahead` — fails if the type is larger than available bytes

**Critical rule for test authors**: Pointer bounds are ENFORCED in interpretation mode but NOT in compiled mode (native C doesn't track bounds). Tests that pass pointers past element boundaries will fail in interpretation mode but work when compiled.

**Bounds safety**: When a deref fails due to bounds mismatch, the interpreter returns a null value instead of crashing. The invalid pointer access is silently tolerated to match runtime behavior.

### Reference Handling (`&mut i` vs `&raw mut i`)

The parser distinguishes between two types of address-taking:

```chemical
var j = &mut i;       // Creates ReferenceOfValue → evaluates to PointerValue (interpreter)
var k = &raw mut i;   // Creates AddrOfValue → evaluates to PointerValue (interpreter)
```

- `ReferenceOfValue::evaluated_value()` (in `ast/values/ReferenceOfValue.cpp`) creates a `PointerValue` pointing to the inner value's data field with bounds set to the type size.
- `AddrOfValue::evaluated_value()` (in `ast/values/AddrOfValue.cpp`) creates a `PointerValue` similarly but handles more types (IntN, String, Bool, Float, Double, PointerValue).

Both work identically in the interpreter — `*j = *j + 1` works for both `&mut i` and `&raw mut i`.

### How Values Are Destroyed

When an `InterpretScope` ends, its destructor calls `destroy_values()`:

```cpp
InterpretScope::~InterpretScope() {
    if(should_destruct_values) {
        destroy_values();
    }
}
```

`destroy_values()` iterates all values in the scope and for struct values with destructors (`@delete`), it creates a child scope and interprets the destructor body. The `self` parameter is removed from the child scope before destruction to prevent recursive cleanup.

**Important**: PointerValues are NOT explicitly destroyed during scope cleanup — they're allocated on the arena allocator which is batch-cleared. However, their `data` pointer becomes dangling if the pointed-to value was also arena-allocated and the arena is cleared.

### How Function Calls Work

`FunctionDeclaration::call()` (in `ast/structures/FunctionDecl.cpp`):
1. Saves and replaces `global->current_func_type`
2. Pushes the call onto `global->call_stack`
3. Creates an `InterpretScope fn_scope(global, func_allocator, global)` — parent=global, global=global
4. Declares `self`, arguments, and default-value parameters in `fn_scope`
5. Interprets the function body via `fn_scope.interpret(&body.value())`
6. Reads `fn_scope.returnValue` and restores `current_func_type`/call_stack
7. Returns the return value

### Adding a New Comptime Test

1. **Pick the right location**:
   - `lang/tests/src/comptime/` — comptime-specific features (basic.ch, features.ch, expressions.ch, etc.)
   - `lang/tests/common/src/` — tests shared between compiled and interpret modes
   - `lang/tests/native_common/src/` — tests using pointer arithmetic (interpret + compiled, NOT JVM)

2. **Write the test function**:
   ```chemical
   comptime func my_new_feature(a : int, b : int) : int {
       return a + b;
   }
   
   func test_my_feature() {
       test("my new feature works", () => {
           return my_new_feature(3, 4) == 7;
       });
   }
   ```

3. **Register in the test runner**:
   - For interpret tests: add `test_my_feature();` to `run_common_tests()` in `lang/tests/common/src/main.ch`
   - For native_common tests: add to `run_native_common_tests()` in `lang/tests/native_common/src/main.ch`
   - For compiled-only tests: add to `main()` in `lang/tests/src/tests.ch`

4. **Run the tests**:
   ```bash
   ./scripts/test.sh --tcc --interpret --no-build   # Interpret mode
   ./scripts/test.sh --tcc --no-build                # Compiled mode
   ```

### Common Pitfalls for Comptime Tests

1. **Pointer arithmetic past end of array/string**: Works in compiled mode but fails in interpreter with `cannot dereference pointer while type size is larger than bytes available`. Use `ahead`-safe operations.

2. **Taking address of struct fields (`&raw p.field`)**: Not supported in interpreter ("address of struct not supported in comptime"). Use intermediate variables or pass by reference.

3. **`void*` pointers**: Cannot be dereferenced directly in interpreter (unknown type). Cast to a concrete pointer type first.

4. **Function pointers as parameters**: When passing a function as a parameter (e.g., `test(name, myFunc)` where `assert()` calls the passed function), the function reference must be resolvable through the Identifier's `linked` field. If the function can't be linked, the error "calling a function that is not found or has no body" appears (non-fatal).

5. **Destructor bodies must be interpretable**: If a struct has an `@delete` destructor, the body must be valid Chemical code that the interpreter can evaluate. Empty destructors `{}` work fine.

6. **Float/Double to IntN casts**: The interpreter supports `float→int` and `double→int` casting. Other cast combinations may fail with "unknown operation between values".

### Test Suite Entry Points

| Mode | Entry Function | Module | File |
|------|---------------|--------|------|
| Interpret | `main()` | `interpret_tests` | `lang/tests/interpret/src/main.ch` |
| Native compiled | `main()` | `main` | `lang/tests/src/tests.ch` |
| Common (shared) | `run_common_tests()` | `common_tests` | `lang/tests/common/src/main.ch` |
| Native-common | `run_native_common_tests()` | `native_common_tests` | `lang/tests/native_common/src/main.ch` |

### What's Supported in the Interpreter

**Numbers**: `i8`–`i64`, `u8`–`u64`, `int`, `uint`, `long`, `ulong`, `float`, `double`, `bool`, `char`

**Operators**: `+`, `-`, `*`, `/`, `%`, `<<`, `>>`, `&`, `|`, `^`, `~`, `&&`, `||`, `!`, `==`, `!=`, `<`, `>`, `<=`, `>=`

**Control flow**: `if/else`, `while`, `do-while`, `for`, `for-in`, `break`, `continue`, `return`, `switch`

**Pointers**: `*`, `&raw`, `&mut`, `ptr++`, `ptr--`, `ptr + n`, `ptr - n`, `ptr - ptr`, comparison, double-deref `**ptr`

**Structs**: Construction `{ field: val }`, member access, mutation, destructors

**Generics**: Basic generic structs and functions

**Casting**: `intN→intN`, `intN→long`, `long→intN`, `float→intN`, `double→intN`, `intN→pointer`

**Not yet supported**: Full variant type matching at comptime (the else-expression path for break/continue/return/defValue IS supported — test 76 passes), struct pointers (`&raw struct_val` — use `&mut` references instead), variadic function calls, `@test` annotation dispatch

### Interpreter Move Semantics (Critical Knowledge)

The interpreter implements move semantics via `InterpretScope::move_clear_source()` — a **pointer-matching** approach that scans the scope chain for any variable pointing to the same `StructValue*` pointer and clears it (sets to `nullptr`). This prevents double-destruction when a destructible struct is moved.

#### Why Pointer-Matching Instead of AST Type Checks

The compiler may replace `VariableIdentifier` AST nodes with the resolved `StructValue` during resolution. Old code that checked `val_kind() == ValueKind::Identifier` would miss this case. The pointer-matching approach (`val == valuePtr`) works regardless of what the AST looks like.

#### The `move_clear_source()` function (in `ast/base/InterpretScope.cpp`)

```cpp
void InterpretScope::move_clear_source(Value* initializer, const chem::string_view& new_name) {
    if(!initializer || initializer->val_kind() != ValueKind::StructValue) return;
    auto structVal = initializer->as_struct_value_unsafe();
    auto ext = structVal->linked_extendable();
    if(!ext || ext->kind() != ASTNodeKind::StructDecl) return;
    auto sd = (StructDefinition*)ext;
    if(!sd->has_destructor()) return;
    InterpretScope* scanScope = this;
    while(scanScope) {
        for(auto& [name, val] : scanScope->values) {
            if(name != new_name && val == initializer) {
                val = nullptr;
                return;
            }
        }
        scanScope = scanScope->parent;
    }
}
```

Key points:
- Only runs for destructible structs (`has_destructor()`)
- Scans up the parent chain (local scope → function scope → global)
- Skips `new_name` to avoid self-clearing (e.g., `var x = x` shouldn't nullify `x`)
- Uses `new_name.empty()` to indicate "clear anything that matches" (used in function args, variant constructors, struct member inits)

#### Places that call move_clear_source()

| Location | File | `new_name` | Purpose |
|----------|------|-----------|---------|
| `VarInitStatement::interpret()` | `compiler/Interpreter/Core.cpp` | `stmt->name_view()` | `var y = x` → clears `x` (move) |
| `AssignStatement::interpret()` | `compiler/Interpreter/Core.cpp` | LHS name | `a = d` → clears `d` (move) |
| `StructValue::initialized_value()` | `ast/values/StructValue.cpp` | empty | Struct member init move |
| `FunctionCall::interpret_value()` (variant ctor) | `ast/values/FunctionCall.cpp` | empty | Variant constructor move |
| `FunctionDeclaration::call()` | `ast/structures/FunctionDecl.cpp` | empty | Function argument move |

#### Temp Struct Destruction (Bare Expressions)

Bare expressions that produce destructible structs (e.g., `create_destructible(...)`, `d.copy().copy()`) need their temps destructed. The interpreter handles this in three places:

1. **`ValueWrapperNode::interpret()`** — Wraps bare expression statements. If the value is a `FunctionCall`, evaluates it and calls `destruct_temp_struct()`. If it's an `AccessChain`, evaluates each step, collects all `FunctionCall` results, then destructs them all.

2. **`AccessChainNode::interpret()`** — Same pattern: evaluates chain step-by-step, collects temps, destructs all at end. Handles `d.copy().copy().copy()` where ALL intermediate results must be cleaned up.

3. **`AccessChain::evaluated_value()`** — When the chain starts with a FunctionCall (`create_destructible(...).data`), destructs the intermediate struct temp after extracting the field value.

#### `destruct_temp_struct()` Helper (in `compiler/Interpreter/Core.cpp`)

```cpp
static void destruct_temp_struct(InterpretScope& scope, Value* val) {
    if(!val || val->val_kind() != ValueKind::StructValue) return;
    auto structVal = val->as_struct_value_unsafe();
    auto ext = structVal->linked_extendable();
    if(!ext) return;
    auto container = ext;
    if(!container->has_destructor()) return;
    auto destructor_fn = container->destructor_func();
    if(!destructor_fn || !destructor_fn->body.has_value()) return;
    InterpretScope temp_scope(scope.global, scope.allocator, scope.global);
    temp_scope.declare("self", val);
    temp_scope.interpret(&destructor_fn->body.value());
    // Remove self so it's not destructed again when temp_scope is destroyed
    auto self_it = temp_scope.values.find("self");
    if(self_it != temp_scope.values.end()) {
        temp_scope.values.erase(self_it);
    }
}
```

Critical: The destructor runs in a **temp scope** with `self` removed after interpretation — prevents double-destruction when `temp_scope` is destroyed.

#### AssignStatement Old-Value Destruction

Assignment uses a careful 3-step approach:
1. **Save** the old LHS value pointer (before `set_value` overwrites it)
2. **Perform** assignment via `set_value()` (evaluates RHS, may take pointer to LHS)
3. **Destruct** the old LHS value (safe because RHS has already been resolved)
4. **Clear** the RHS source (move semantics)

This ordering is critical for self-referencing assignments like `x = f(x.get_ptr(), N)`, where the RHS function takes a pointer to the LHS data.

#### Array Value Destruction

`InterpretScope::destroy_values()` recursively destructs structs, including:
- `StructValue` with destructor → calls destructor body + recursively destructs members
- `ArrayValue` → destructs each element that is a struct value with a destructor
- Elements populated by `evaluated_value()` or `set_value()` — modifications through `&arr[i]` affect actual elements via `PointerValue`

### Remaining Interpretation Test Failures (as of fix session)

After fixing 16 of 32 baseline failures, the following 16 tests remain. Categorized by root cause:

| Tests | Root Cause | Difficulty |
|-------|-----------|-----------|
| 757-760 | **Array element destruction** — elements set via pointer in array aren't tracked by scope, so `destroy_values` doesn't find them | Medium |
| 798-799, 822 | **Method chain dispatch** — `get_parent_from()` returns `nullptr` for `self` in chained method calls like `d.copy().copy()`. The interpreter cannot dispatch methods through chains where the receiver is a temporary | Hard |
| 811-812 | **Self-referencing assignment** — deeper pointer/copy semantics where the destructor of the old LHS corrupts data that the new value depends on | Hard |
| 722-723, 800 | **Generic dispatch move semantics** — generic function monomorphization may not trigger the same move-semantic paths | Medium |
| 629, 831, 838 | **Edge cases** — miscellanous edge cases that need individual investigation | Varies |
| 784 | **Variant destruct** — partially fixed, a variant path may not be fully covered | Medium |

### Debugging Interpretation Test Failures

1. Check test output for `[InterpretError]` messages — exact failure locations
2. Look for `"cannot dereference pointer"` — pointer went past allocated bounds
3. Look for `"function call"` — a function reference couldn't be resolved
4. Look for `"Operation between values"` — incompatible type operation
5. Add `std::cerr << "[DEBUG] ..." << std::endl;` to interpreter source files
6. The worst failures (`"RUNTIME ERROR: invalid memory access"`) happen when `deref()` crashes — the interpreter now returns `getNullValue()` instead (non-fatal)
7. For struct destruction issues, add debug output in `InterpretScope::destroy_values()`
8. To trace value tracking, add `scope.print_values()` in key locations

### Native_common Tests

Tests in `lang/tests/native_common/src/` run AFTER common tests (as test 98+) from `run_native_common_tests()`. These tests use pointer arithmetic and have known issues in interpretation mode:
- **Pointer bounds mismatch**: When a pointer is incremented past its allocated buffer, `ahead` becomes 0 and dereferencing fails with a bounds check. The interpreter returns null instead of crashing, but tests may get wrong results.
- **Struct pointers (`&raw struct_val`)**: Not supported — returns an error from `AddrOfValue::evaluated_value()`.
- **Error visibility**: Failures in native_common tests don't produce "Test N failed" output because the error happens inside the lambda before `test()` can print. Look for `[warning]` or `[InterpretError]` messages to diagnose.

## Debugging: Isolating a Single Test Case

Running the full test suite (`./scripts/test.sh --tcc`) on every iteration is time consuming.
The test modules are large and produce very long LLVM IR and C translation outputs, making
it hard to find the exact code for a single test failure.

### The Workflow

Instead of running the full suite, **isolate the failing test** by copying its source code
(plus any types/functions it depends on) into a single stand-alone file at
`lang/compiled/temp.ch`. The `lang/compiled/` directory is in `.gitignore`,
so no files there will be committed.

You can then compile that single file directly with either the `Compiler` (LLVM) or
`TCCCompiler` (TinyCC/C translation) targets. This gives you:
- A focused LLVM IR dump for just your test case
- A focused C translation output for just your test case
- A compiled executable you can run directly
- The ability to add debug logs, recompile, and re-run quickly

### Rebuilding the Compiler First

After making changes to C++ source files (`.cpp`/`.h`), **always rebuild the compiler
binary** before compiling your test case:

```bash
# For Compiler target (LLVM/Clang backend):
./scripts/build.sh --llvm

# For TCCCompiler target (TinyCC backend):
./scripts/build.sh --tcc
```

> ⚠️ If you skip rebuilding, the old binary is used and your C++ changes are **ignored**.

### Compiling a Standalone Test File (Compiler / LLVM Backend)

Use the `Compiler` binary to compile `temp.ch` and emit LLVM IR:

```bash
cmake-build-debug/Compiler "lang/compiled/temp.ch" --out-ll-all --build-dir "lang/compiled" \
    -o "lang/compiled/temp.exe" --mode debug_complete --debug-ir -v --assertions -fno-unwind-tables
```

**Output:**
- Executable at `lang/compiled/temp.exe`
- LLVM IR at `lang/compiled/modules/main/llvm_ir.ll`

**Run the compiled executable:**
```bash
./lang/compiled/temp.exe
```

**Flag explanations:**
- `--mode debug_complete` — adds maximum debug information (omit for cleaner IR without debug metadata)
- `--debug-ir` — don't crash on potentially bad IR (LLVM's own validation may still crash)
- `--assertions` — verify the generated IR is valid
- `-fno-unwind-tables` — makes the IR cleaner (on Windows, removes unwind data above every function)
- `-v` — verbose output

### Compiling a Standalone Test File (TCCCompiler / C Translation)

Use the `TCCCompiler` binary to translate to C or produce an executable:

```bash
# Emit C translation output (file extension determines behavior):
cmake-build-debug/TCCCompiler "lang/compiled/temp.ch" -o "lang/compiled/temp.c" -v -bm-modules

# Produce an executable:
cmake-build-debug/TCCCompiler "lang/compiled/temp.ch" -o "lang/compiled/temp.exe" -v -bm-modules
```

**Flag explanations:**
- `-v` — verbose output
- `-bm-modules` — emit build module information
- `--mode debug_quick` — minimal debug info (omit for even less noise)
- Use mode `debug_complete` for maximum debug information

### Quick Iteration Loop

1. Write/update the test case in `lang/compiled/temp.ch`
2. If you changed C++ code: rebuild the compiler (see above)
3. Compile the test case with one of the commands above
4. Run the compiled executable and check the output
5. Inspect the LLVM IR or C translation output to find codegen bugs
6. Add debug logs to the compiler source, rebuild, and repeat

## Compiled Packages (`lang/compiled/`)


Each package under `lang/compiled/<name>/` is a standalone Chemical application:
- Entry: `chemical.mod` — declares `application <name>`, `source "src"`, imports.
- Build: `cmake-build-debug/TCCCompiler lang/compiled/<name>/chemical.mod -o <output> --mode debug_quick --no-cache`
- Compiled apps depend on `std`, `page`, `html_cbi`, `css_cbi`, `js_cbi`, `universal_cbi`, `net`, `json`, `fs`, and optionally `mongodb`, `totp`, `accountlib`.

### Universal Component Pipeline

`#universal ComponentName(props) { ... }` components (processed by `universal_cbi`):

1. **Definition** — JSX-based components with `state` for reactivity, event handlers, `.map()` for loops.
2. **SSR (server-side rendering)** — The component's JSX is compiled to C++ that writes HTML into `HtmlPage.pageHtml`. During SSR, `state` initializers run, conditionals evaluate, and static HTML is emitted.
3. **Hydration** — The same JSX compiles to JavaScript that runs in the browser. The SSR HTML is captured and passed to a client-side function that hydrates it in-place.
4. **Mounting** — `#html { <Component prop={value} /> }` generates:
   - Server: calls the component's C++ SSR function → writes HTML to `page`
   - Client: emits `window.$__uni_dispatch('scoped_ComponentName', element, {props})` which calls the JS version to hydrate

### How `#html {}` Blocks Work

The `#html { ... }` macro (processed by `html_cbi`):
- Parses JSX/HTML into an AST
- For native HTML elements (`<div>`, `<span>`), generates `page.append_view(...)` calls
- For universal components (`<QuizTaker>`), generates:
  1. C++ SSR function call (writes HTML to page buffer)
  2. `page.capture_html_delta_to_js(index)` — copies the SSR HTML into the JS bundle
  3. `page.truncate_html(index)` — removes SSR HTML from the page buffer (it's now in JS)
  4. Emits `window.$_uc_h(htmlString, "ComponentName", props)` into the JS bundle
- Prop values (`{expr}`) are C++ expressions evaluated at runtime

### Prop Serialization Gotchas

When passing C++ values as props to universal components via `#html { <Comp prop={value} /> }`:

**Safe types:** `string`, `string_view`, `*char`, `int`, `uint`, `i64`, `bool`, `double`
**Unsafe types:** C++ structs with `vector<>` fields — the serializer produces corrupt output

The JS serializer wraps `string` values in **single quotes** in the generated JavaScript. This means:
- `'` (single quote) in the data MUST be escaped as `\u0027` (valid JSON) — the serializer does NOT escape it
- `\` (backslash) in the data will be interpreted as a JS escape sequence — if your JSON contains `\n`, it becomes a literal newline in the JS string. You must pre-escape `\` as `\\` in your string before passing
- **Workaround:** Post-process string values with `js_string_escape()` that doubles backslashes and escapes single quotes

```chemical
func js_string_escape(view : string_view) : string {
    var out = string()
    for(var i = 0u; i < view.size(); i++) {
        var c = view.get(i)
        if(c == '\\') { out.append_view("\\\\") }
        else if(c == '\'') { out.append_view("\\u0027") }
        else { out.append(c) }
    }
    return out
}
```

### Vector Access: `.get_ptr(i)` vs `.get(i)`

For `vector<T>` where `T` has a destructor (strings, structs), **never use `.get(i)`**:
- `.get(i)` returns `T` by **bitwise copy** — internals (like string data pointers) are shared
- When the temporary is destroyed, it frees the shared pointer, causing **double-free**
- Always use `.get_ptr(i)` which returns `*mut T` (a safe pointer)
- Chemical auto-dereferences pointers for member access: `(*ptr).field` → just `ptr.field`

Pattern:
```chemical
for(var i = 0u; i < vec.size(); i++) {
    var item = vec.get_ptr(i)
    // Use item.field, item.method() — auto-dereferenced
}
```

### Json Library Notes

- `JsonParser(bufferSize, 4096)` — parses JSON with configurable buffer
- `ASTJsonHandler` — builds an AST of `JsonValue` variants
- Types are in the top-level module (directly accessible, no `json::` prefix)
- `JsonValue` variants: `Null`, `Boolean(bool)`, `Integer(i64)`, `Double(f64)`, `String(string)`, `Array(vector<JsonValue>)`, `Object(unordered_map<string, JsonValue>)`
- Pattern matching: `if(val is JsonValue.String) { var String(s) = val else unreachable }`
- Access map values: `obj_map.get_ptr(string("key"))` — returns `*mut JsonValue` or null

### MongoDB Integration

- Uses `lang/compiled/mongodb/` package
- MongoDB C driver shared libraries: `libmongoc2.so`, `libbson2.so` in the build directory
- Pool pattern: `pool.pop()` to get a client, `pool.push(&mut client)` to return it
- Document building: `mongodb::Document.new()`, `.append_utf8(key, value)`, `.append_int64(key, value)`, `.append_oid(key, &oid)`
- OID: `mongodb::OID.from_string(hex_id)`
- Queries: `collection.find(filter).first()`, `collection.insert_one_with_id(&doc)`, `collection.delete_one(&filter)`

## Release
