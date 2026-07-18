---
name: Testing Guide
description: Comprehensive guide to the Chemical compiler test infrastructure — how tests are organized, written, and executed. Covers the test framework, @test annotation dispatch, test_env and test libraries, and how compiler plugins get tested via lang/tests/build.lab.
---

# Testing Guide

The Chemical compiler has a multi-layered test infrastructure. Tests can be inline (called manually), annotation-based (auto-dispatched via `@test`), or library-level (for compiler plugins). Understanding these layers is essential for writing effective tests.

## Test Architecture Overview

```
lang/tests/
├── build.lab                  # Master build script — wires everything together
├── common/                    # Shared tests (run in both interpret and compiled modes)
│   ├── chemical.mod
│   └── src/
│       ├── test.ch            # test(), assertEquals(), print_test_stats()
│       └── main.ch            # run_common_tests()
├── interpret/                 # Interpretation-only tests
│   ├── chemical.mod
│   └── src/main.ch            # Entry point for --arg-interpret
├── native_common/             # Pointer arithmetic tests (interpret + compiled)
│   ├── chemical.mod
│   └── src/main.ch            # run_native_common_tests()
├── src/                       # Compiled-only tests
│   ├── chemical.mod
│   ├── tests.ch               # run_executable_tests(), main()
│   ├── basic/                 # Basic language feature tests
│   ├── comptime/              # Compile-time feature tests
│   ├── core/                  # Core language tests
│   ├── generic/               # Generic instantiation tests
│   ├── stdlib/                # Standard library tests
│   └── libs/                  # Tests that depend on specific libraries
├── libs/                      # Library-specific tests (html, css, js, universal, md)
│   ├── html/
│   ├── css/
│   ├── js/
│   ├── universal/
│   ├── md/
│   └── runner/
└── submod2/, submod3/         # Sub-module tests for module import/export
```

## Two Test Dispatch Mechanisms

### 1. Manual Inline Tests

Tests are written as regular functions and called explicitly:

```chemical
// lang/tests/common/src/main.ch
public func run_common_tests() {
    test("arithmetic addition", () => { return 2 + 2 == 4 });
    test("string equality", () => { return "hello" == "hello" });
    // Every test function is called explicitly here
}
```

**Flow:**
1. `run_common_tests()` is called from the module's entry point
2. Inside, each `test(...)` call is made explicitly
3. `test()` is defined in `common/src/test.ch`

### 2. `@test` Annotation Dispatch

Tests annotated with `@test` are automatically discovered and dispatched:

```chemical
@test
func my_auto_test() : bool {
    return 2 + 2 == 4
}
```

**Flow:**
1. `intrinsics::get_tests<TestFunction>()` (defined in `compiler/Interpreter/Core.cpp`) collects all `@test`-annotated functions
2. `test_runner(argc, argv)` from the `test_env` library dispatches them
3. Each test function runs in a **separate process** via IPC
4. Test results are communicated back to the parent process

```chemical
// In lang/tests/src/tests.ch:
public func main(argc : int, argv : **char) : int {
    if(argc > 1) {
        run_executable_tests()           // Manual inline tests
    } else {
        // Default: run @test annotated functions
    }
    test_runner(argc, argv)              // Auto-dispatched @test functions
    print_test_stats()
}
```

## The Test Framework (`lang/tests/common/src/test.ch`)

### Core Functions

```chemical
var total_tests = 0;
var tests_passed = 0;
var tests_failed = 0;

public func test(name : *char, assert : () => bool) {
    if(assert()) {
        comptime if(intrinsics::is_interpretation()) {
            intrinsics::expr_println(`${ANSI_COLOR_GREEN}Test ${total_tests + 1} [${name}] succeeded${ANSI_COLOR_RESET}`);
        } else {
            printf("%sTest %d [%s] succeeded %s\n", ANSI_COLOR_GREEN, total_tests + 1, name, ANSI_COLOR_RESET);
        }
        tests_passed++;
    } else {
        comptime if(intrinsics::is_interpretation()) {
            intrinsics::expr_println(`${ANSI_COLOR_RED}Test ${total_tests + 1} [${name}] failed${ANSI_COLOR_RESET}`);
        } else {
            printf("%sTest %d [%s] failed %s\n", ANSI_COLOR_RED, total_tests + 1, name, ANSI_COLOR_RESET);
        }
        tests_failed++;
    }
    total_tests++;
}

public func print_test_stats() {
    var color = tests_failed > 0 ? ANSI_COLOR_RED : ANSI_COLOR_GREEN;
    comptime if(intrinsics::is_interpretation()) {
        intrinsics::expr_println(`Total ${total_tests} ${ANSI_COLOR_GREEN}Passed ${tests_passed}${ANSI_COLOR_RESET} ${color}Failed ${tests_failed}${ANSI_COLOR_RESET}`);
    } else {
        printf("Total %d %sPassed %d%s %sFailed %d%s", total_tests, ...);
    }
}
```

**Key design:** Uses `comptime if(intrinsics::is_interpretation())` to select between:
- **Interpretation path**: `intrinsics::expr_println()` with expressive strings (backtick `${}` templates)
- **Compiled path**: Standard `printf()` with ANSI escape codes

## The `test_env` Library (`lang/libs/test_env/`)

The `test_env` library provides the infrastructure for `@test` annotation dispatch:

```chemical
// Key functions:
func test_runner(argc : int, argv : **char) {
    // 1. Uses intrinsics::get_tests<TestFunction>() to collect all @test functions
    // 2. Spawns a child process for each test via IPC
    // 3. Waits for all tests to complete
    // 4. Collects results
}
```

The `test_runner` function:
- Dispatches each `@test`-annotated function in a **separate process**
- Uses IPC to communicate pass/fail results back to the parent
- Enables isolation: one test crashing doesn't affect others
- Parallel execution: tests can run concurrently

## The `test` Library (`lang/libs/test/`)

The `test` library provides the `TestFunction` struct and related types:

```chemical
// TestFunction — the type used by @test-annotated functions
// Collected by intrinsics::get_tests<TestFunction>()
// Each function must return bool (true = pass, false = fail)
```

## How Tests Are Wired in `lang/tests/build.lab`

The master build script (`lang/tests/build.lab`) is the central wiring point:

### Module Creation Flow

```chemical
func build(ctx : *mut AppBuildContext) : *mut LabJob {
    // Check for --arg-test-libs → build library test executable
    if(ctx.has_arg("test-libs")) {
        return test_lib_exe(ctx, "all")
    }

    // Check for --arg-interpret → build interpretation job
    if(ctx.has_arg("interpret")) {
        const interp_job = ctx.build_interpretation("chemical-interpret")
        var common_module = get_common_mod(ctx, interp_job)
        var native_common_mod = get_common_native_mod(ctx, interp_job)
        const interp_mod = ctx.directory_app_module("", "interpret_tests", interp_path, [native_common_mod, common_module])
        ctx.add_module(interp_job, interp_mod)
        ctx.define(interp_job, &definition)
        return interp_job
    }

    // Default: build compiled test executable
    const exe_job = ctx.build_exe("chemical-tests")
    ctx.add_module(exe_job, get_main_module(ctx, exe_job))
    ctx.define(exe_job, &definition)
    return exe_job
}
```

### Module Dependency Chain

```
chemical-tests (executable)
├── ext_module (C file: ext/file.c)
├── ext_cpp_module (CPP file: ext/file2.cpp)
├── submod (Chemical sub-module)
├── submod2_module (from submod2/build.lab)
├── submod3_module (from submod3/build.lab)
├── std_module (@std)
├── core_module (@core — provides core interfaces)
├── bcrypt_module (@bcrypt)
├── uuid_module (@uuid)
├── json_module (@json)
├── atomic_module (@atomic)
├── datetime_module (@datetime)
├── regex_module (@regex)
├── fs_module (@fs)
├── net_module (@net)
├── docgen_module (@docgen)
├── test_module (@test)
├── test_env_module (@test_env — provides test_runner)
├── common_mod (common/tests — shared tests)
└── native_common_mod (native_common/ — pointer tests)
```

### Library Test Wiring

Library tests (for compiler plugins like `html_cbi`, `css_cbi`, etc.) are built separately:

```chemical
func test_lib_exe(ctx : *mut AppBuildContext, which : &std::string_view) : *mut LabJob {
    const exe_job = ctx.build_exe("chemical-libs-tests")
    ctx.set_environment_testing(exe_job, true)
    switch(which) {
        "html" => { ctx.add_module(exe_job, htmlTestMod.build(ctx, exe_job)) }
        "css"  => { ctx.add_module(exe_job, cssTestMod.build(ctx, exe_job)) }
        // ... etc
        default => {
            // Build ALL library tests
            ctx.add_module(exe_job, htmlTestMod.build(ctx, exe_job))
            ctx.add_module(exe_job, cssTestMod.build(ctx, exe_job))
            ctx.add_module(exe_job, jsTestMod.build(ctx, exe_job))
            ctx.add_module(exe_job, mdTestMod.build(ctx, exe_job))
            ctx.add_module(exe_job, universalTestMod.build(ctx, exe_job))
        }
    }
    ctx.add_module(exe_job, testLibsRunner.build(ctx, exe_job))
    return exe_job
}
```

Each library test module has its own `build.lab` in `lang/tests/libs/<name>/`.

## Adding a New Test

### Option A: Inline Test (Manual)

1. **Create your test function** in an appropriate file:
   ```chemical
   // lang/tests/common/src/arithmetic.ch
   func test_new_feature() {
       test("my new feature works", () => {
           return my_func(3, 4) == 7;
       });
   }
   ```

2. **Register the test** — call it from the appropriate runner:
   - `run_common_tests()` in `lang/tests/common/src/main.ch` — for interpret + compiled
   - `run_native_common_tests()` in `lang/tests/native_common/src/main.ch` — for pointer tests
   - `run_executable_tests()` in `lang/tests/src/tests.ch` — for compiled-only tests

3. **Source the file** — ensure the file is in the module's source path

### Option B: `@test` Annotation (Auto-Dispatched)

1. **Annotate your function**:
   ```chemical
   @test
   func my_auto_test() : bool {
       return 2 + 2 == 4
   }
   ```

2. **No registration needed** — `intrinsics::get_tests<TestFunction>()` discovers it automatically

3. **Ensure dependencies** — the `test_env` module must be imported in the build

### Option C: Library Test (for Compiler Plugins)

1. **Create the test module** at `lang/tests/libs/<name>/`:
   ```
   lang/tests/libs/<name>/
   ├── chemical.mod
   ├── build.lab            # Calls testLibsRunner's test infrastructure
   └── src/
       └── main.ch          # Test functions
   ```

2. **Wire in the master build.lab** — add import and build call in `lang/tests/build.lab`

3. **Run with**:
   ```bash
   ./scripts/test.sh --tcc --libs
   ```

## Writing Tests for Compiler Failure

Tests that verify the compiler correctly rejects invalid code:

```chemical
// Pattern: test that a specific code causes a compile error
func test_type_error() {
    // The key is that the test function itself never gets compiled with bad code
    // Instead, use intrinsics or comptime to trigger specific errors
    comptime {
        // This checks that intrinsics::error() works
        // intrinsics::error("this should fail")
    }
}
```

For testing compile-time errors from the C++ side, add diagnostics checks in the compilation pipeline and verify `has_errors()` returns true.

## Running Tests

### Interpretation Tests

```bash
./scripts/test.sh --tcc --interpret              # Build + run
./scripts/test.sh --tcc --interpret --no-build   # Skip rebuild
```

### Compiled Tests

```bash
./scripts/test.sh --tcc                          # Build + run
./scripts/test.sh --tcc --no-run                 # Build only
./scripts/test.sh --tcc --no-build               # Use existing binary
```

### Library Tests

```bash
./scripts/test.sh --tcc --libs                   # All library tests
```

### Individual Library Tests

```bash
# Using --arg-test-<name> CLI args:
./scripts/test.sh --tcc --libs                   # Tests with --arg-test-html etc internally
```

### Manual Test Execution

```bash
# Interpret:
./chemical lang/tests/build.lab --arg-interpret --mode debug_complete --no-cache

# Compiled:
cmake-build-debug/TCCCompiler lang/tests/build.lab -o tests.exe --mode debug_quick --no-cache
./tests.exe
```

## Test Directory Structure Reference

```
lang/tests/
├── src/                    # Compiled-only tests
│   ├── basics/             # Basic language features
│   ├── comptime/           # Compile-time features
│   │   ├── basic.ch        # Sum, structs, strings, enums, constructors
│   │   ├── features.ch     # Bitwise ops, loops, casting, for-in, destructors
│   │   ├── expressions.ch  # Comptime block expressions
│   │   ├── satisfies.ch    # Type relationship tests
│   │   ├── is_value.ch     # Type identity tests
│   │   └── vector.ch       # Vector operations
│   ├── core/               # Core language tests
│   ├── generic/            # Generic tests
│   ├── stdlib/             # Standard library tests
│   ├── nodes/              # AST node tests
│   └── libs/               # Library-dependent tests
├── common/src/             # Shared tests (interpret + compiled)
├── native_common/src/      # Pointer tests (interpret + compiled)
├── interpret/src/          # Interpretation-only entry point
├── libs/                   # Library/plugin tests
│   ├── html/src/           # html_cbi plugin tests
│   ├── css/src/            # css_cbi plugin tests
│   ├── js/src/             # js_cbi plugin tests
│   ├── universal/src/      # universal_cbi plugin tests
│   ├── md/src/             # md_cbi plugin tests
│   └── runner/             # Library test runner
```

## Best Practices

1. **Always test both modes**: Write tests that work in both interpretation and compiled modes
2. **Use `comptime if(is_interpretation())`** for mode-specific code
3. **Keep tests simple**: Each test should verify ONE behavior
4. **Name tests descriptively**: Use names like `"arithmetic addition works"` not `"test1"`
5. **Prefer inline tests** for new features (easier to debug)
6. **Use `@test` annotations** for integration tests (isolation via separate processes)
7. **Avoid pointer arithmetic** in shared tests (works differently in interpreter)
8. **Test compiler failures**: Add tests that verify the compiler correctly rejects invalid input
9. **Use the single-test debugging workflow**: Copy failing test to `lang/compiled/temp.ch`

## Related Skills

- **Build System** (`.agents/skills/build_system/SKILL.md`) — How tests are built: `build.lab` wiring, interpretation jobs, library tests
- **Interpreter Internals** (`.agents/skills/interpreter/SKILL.md`) — How interpretation tests run, move semantics, debugging
- **Intrinsics & Reflection** (`.agents/skills/intrinsics_compiler_reflection/SKILL.md`) — `is_interpretation()`, `expr_println()`, `get_tests()`, and other intrinsics used in tests
