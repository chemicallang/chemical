---
name: Building
description: All documentation related to building and running Project or Tests
---

# Building

Its likely user is using CLion, in this case in the root dir, you'll find `cmake-build-debug` where there are two
compilers present
 - `TCCCompiler` (the compiler that embeds Tiny CC)
   - This compiler cannot create llvm ir, It translates Chemical to C and then compiles that using Tiny CC
   - This compiler is faster to build, faster at compilation but generates slower code.
 - `Compiler` (the compiler that embeds LLVM/Clang and Tiny CC)
   - This compiler can do anything TCCCompiler can do and more, It performs optimizations using LLVM. But you
   - can also tell it to use Tiny CC with flag `--use-tcc` or ask it to translate to C and then compile that using
   - clang with flag `--use-c`
   - This compiler takes time to build, slow at compilation but generates fast code

It's likely that you don't need to rebuild the compiler, because you are working on libraries (`lang/libs/<library>` or `lang/compiled/<library>`).
To properly understand building of the compilers, You should analyze the `CMakeLists.txt` in the root of the repo.

If you are working on any code that doesn't interact with LLVM/Clang then you should not compile Compiler target. you should focus
on TCCCompiler which would build faster and would compile faster.

## Configuration

```bash
# Configure with LLVM support (default)
./scripts/configure.sh

# Configure without LLVM (TCCCompiler only)
./scripts/configure.sh --no-llvm
```

## Building Compiler

There are three CMake targets: `TCCCompiler`, `Compiler`, and `ChemicalLsp`.

### Using build scripts (recommended)

```bash
./scripts/build.sh --tcc   # Build TCCCompiler only
./scripts/build.sh --llvm  # Build Compiler (LLVM/Clang backend)
./scripts/build.sh --lsp   # Build ChemicalLsp
./scripts/build.sh --all   # Build all targets
./scripts/build.sh --llvm -j 16  # Build with 16 parallel jobs
```

### Using make directly

```bash
make -C cmake-build-debug Compiler -j$(nproc)
make -C cmake-build-debug TCCCompiler -j$(nproc)
make -C cmake-build-debug ChemicalLsp -j$(nproc)
```

### Notes

- Prefer building one target at a time, not all.
- The Makefile is at `cmake-build-debug/Makefile` (configured by CLion).
- `cmake` is at `/opt/clion/bin/cmake/linux/x64/bin/cmake` (not in PATH).
- `out/host` contains LLVM/Clang Libraries — do not overwrite.
- If user doesn't have LLVM, use `./scripts/configure.sh --no-llvm` (sets `-DBUILD_COMPILER=OFF`).

## Building Tests

### Using test script (recommended)

```bash
# Build TCCCompiler, compile tests, run them
./scripts/test.sh --tcc

# Build Compiler, compile tests, run them
./scripts/test.sh --llvm

# Include library tests
./scripts/test.sh --tcc --libs

# Custom output path
./scripts/test.sh --tcc -o my_tests

# Build only (no run)
./scripts/test.sh --tcc --no-run

# Skip compiler rebuild, use existing binary
./scripts/test.sh --tcc --no-build
```

> ⚠️ **`--no-build` warning**: This flag **skips rebuilding the C++ compiler binary**.
> Any changes to `.cpp`/`.h` files **will NOT be reflected** — the previously built
> binary is used as-is. Only use `--no-build` when iterating on `.ch` test files or
> Chemical library sources **without any compiler C++ changes**. To include C++ changes,
> omit `--no-build` (or run once without it to rebuild, then you can use `--no-build`
> for subsequent iterations).

### Manual commands

```bash
# TCC backend
cmake-build-debug/TCCCompiler "lang/tests/build.lab" -o lang/tests/build/tests-tcc.exe --mode debug_quick --no-cache

# LLVM backend
cmake-build-debug/Compiler "lang/tests/build.lab" -o lang/tests/build/tests.exe --mode debug_complete --no-cache
```

### Interpretation Tests

```bash
./scripts/test.sh --tcc --interpret        # Build TCC + run interpretation tests
./scripts/test.sh --tcc --interpret --no-build  # Skip rebuild

# Manual:
./chemical lang/tests/build.lab --arg-interpret --mode debug_complete --no-cache
```

The `--arg-interpret` flag causes `build.lab` to create a `LabJobType::Interpretation` job. The compiler calls `do_interpretation_job()` in `compiler/lab/LabBuildCompiler.cpp`, which:
- Parses, symres, and typechecks `interpret/` + `common/` modules
- Initializes module-level `VarInitStmt` variables on the global scope
- Calls `main()` via the AST interpreter directly — no object code generated

### How Tests Work

Tests use a common framework at `lang/tests/common/src/test.ch` that works in both modes:

```chemical
comptime if(intrinsics::is_interpretation()) {
    intrinsics::expr_println(`${ANSI_COLOR_GREEN}Test ${total_tests + 1} [${name}] succeeded${ANSI_COLOR_RESET}`);
} else {
    printf("%sTest %d [%s] succeeded %s\n", ANSI_COLOR_GREEN, total_tests + 1, name, ANSI_COLOR_RESET);
}
```

- **Interpretation path:** `intrinsics::expr_println(expr: %expressive_string)` walks the expressive string's parts — `StringValue` literals go directly to `std::cout`, `${}` expressions are evaluated and printed via `RepresentationVisitor` with `interpret_representation = true` (no quotes).
- **Compiled path:** Standard `printf` with ANSI escape string constants.

### Compiler flags explained

- `--mode debug_quick` — quickly compile the project with debug info
- `--mode debug_complete` — full debug mode for LLVM backend
- `--no-cache` — do not rely on previously generated objects
- `--emit-c` — write the Translated.c file to the build directory
- `--arg-interpret` — run in interpretation mode (interpret AST directly, no codegen)
- `--arg-test-libs` — build library tests executable
- `-frecompile-plugins` — recompile compiler plugins

## Building Library Tests

We test most libraries in the tests above (`lang/tests/build.lab`) but some libraries like:
- `html_cbi`, `css_cbi`, `js_cbi`, `components`
- `react_cbi`, `preact_cbi`, `solid_cbi`, `universal_cbi`

These are compiler plugins, tested via a separate executable:

```bash
# Using test script
./scripts/test.sh --tcc --libs

# Manual
cmake-build-debug/TCCCompiler "lang/tests/build.lab" -o lang/tests/build/lib-tests-tcc.exe --mode debug_quick --no-cache --arg-test-libs -frecompile-plugins
```

Helpful flags:
- `--plugin-mode debug_complete` — compile plugins in debug mode for full stack traces
- `--arg-test-html`, `--arg-test-css`, etc. — individual library tests

## Building LSP

If you modify anything inside the [server](/server/) directory, note that it's part of the LSP target.

These modules are all LSP-related and require the LSP server running to verify:
- `html_ide`, `css_ide`, `js_ide`, `react_ide`, `preact_ide`, `solid_ide`, `universal_ide`, `md_ide`

```bash
# Build LSP
./scripts/build.sh --lsp
```

LSP target name is `ChemicalLsp`. Read `CMakeLists.txt` before building the LSP target.

## Developing Comptime Tests

### Test Structure

Interpret tests run via `--arg-interpret` and execute the AST interpreter directly. The entry point is:

```chemical
// lang/tests/interpret/src/main.ch
public func main() {
    run_common_tests();        // Tests 1-97 (core language features)
    run_native_common_tests(); // Pointer arithmetic, casts, comptime pointers
    print_test_stats();
}
```

| Module | Location | What it tests |
|--------|----------|---------------|
| `common_tests` | `lang/tests/common/` | Core features (arithmetic, loops, structs, variants, inc/dec) |
| `native_common_tests` | `lang/tests/native_common/` | Pointer operations, casts, comptime pointer arithmetic |
| `interpret_tests` | `lang/tests/interpret/` | Wrapper that imports both and runs `main()` |

### Comptime Test Files (in `lang/tests/src/comptime/`)

| File | Tests |
|------|-------|
| `basic.ch` | Basic comptime: sum, structs, strings, enums, constructors, `get_child_fn`, `get_line_no` |
| `features.ch` | Comptime features: bitwise ops, loops, casting, logical ops, for-in, struct mutation, destructors |
| `expressions.ch` | `comptime { }` block expressions: arithmetic in comptime blocks |
| `satisfies.ch` | `intrinsics::satisfies<T, U>()` type relationship tests |
| `is_value.ch` | `intrinsics::is_same_type()` and `is` operator type identity tests |
| `vector.ch` | `intrinsics::vector<T>()` vector operations |

### Running Tests

```bash
# Quick iteration (interpret only, skip rebuild)
./scripts/test.sh --tcc --interpret --no-build

# Full compiled test run
./scripts/test.sh --tcc --no-build

# Both in sequence
./scripts/test.sh --tcc --interpret --no-build && ./scripts/test.sh --tcc --no-build
```

### Adding a New Comptime Test

1. **Create the test source** in `lang/tests/src/comptime/` or `lang/tests/common/src/`:

   ```chemical
   // lang/tests/src/comptime/my_feature.ch
   comptime func my_feature(a : int, b : int) : int {
       return a * b + a;
   }

   func test_my_feature() {
       test("my feature works", () => {
           return my_feature(3, 4) == 15;
       });
   }
   ```

2. **Register the test** by calling the function from the appropriate runner:
   - For tests shared with runs: Add `test_my_feature();` to `run_common_tests()` in `lang/tests/common/src/main.ch`
   - For tests with pointer arithmetic: Add to `run_native_common_tests()` in `lang/tests/native_common/src/main.ch`
   - For compiled-only tests: Add to `main()` in `lang/tests/src/tests.ch`

3. **Build and test**: Use the commands above.

### Common Interpreter Pitfalls

- **Pointer bounds**: The interpreter tracks `ahead`/`behind` on PointerValues. Dereferencing past bounds returns null (not crash). Tests with pointer arithmetic reaching one-past-end may fail.
- **Struct pointers (`&raw struct_val`)**: Not supported in interpreter (returns error). Use `&mut struct_val` (ReferenceOfValue) instead, or parameter passing by reference.
- **Function references as parameters**: Functions passed as `(params) => return_type` are called via `FunctionDeclaration::call()`. If the function reference can't be resolved, a non-fatal "function not found" error appears.
- **Float/Double casts**: `float→int` and `double→int` are supported. Other float/double combinations work via standard arithmetic.
- **Destructors**: Structs with `@delete` destructors have their destructor body interpreted when the scope exits. Empty destructors work fine.

### Debugging Interpret Test Failures

1. Check the test output for `[InterpretError]` messages — these indicate exact failures
2. Look for "cannot dereference pointer" — pointer went past allocated bounds
3. Look for "function call" — a function reference couldn't be resolved
4. Look for "Operation between values" — an operation between incompatible types
5. Add `std::cerr << "[DEBUG] ..." << std::endl;` to interpreter source files to trace specific operations
6. The worst failures ("RUNTIME ERROR: invalid memory access") happen when `deref()` itself crashes — we've made this non-fatal by returning `getNullValue()` instead
