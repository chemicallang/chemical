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
│   └── compiler_plugins/      # Tests that depend on specific libraries
├── compiler_plugins/          # Library-specific tests (html, css, js, universal, md)
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
    // Check for --arg-test-plugins → build library test executable
    if(ctx.has_arg("test-plugins")) {
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

Each library test module has its own `build.lab` in `lang/tests/compiler_plugins/<name>/`.

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

### Option B2: `@test` with `TestEnv` (for library tests)

Library tests in `lang/tests/src/libs/` use this pattern. The `TestEnv` provides `env.error("msg")` for failure reporting:

```chemical
@test
public func my_lib_test(env : &mut TestEnv) {
    var result = my_lib_func()
    if(result is Result.Err) { env.error("should succeed"); return }
    var Ok(value) = result else unreachable
    if(value != 42) { env.error("expected 42") }
}
```

**Key differences from plain `@test`:**
- Takes `env : &mut TestEnv` parameter (not `: bool` return)
- Uses `env.error("msg"); return` for failure (not assertions)
- Functions must be `public` (called from test runner in different package)
- Test file needs `using std::Result;` if using `Result.Err`/`Result.Ok` patterns

**Test file location**: `lang/tests/src/libs/<name>/tests.ch`

#### HTTPS Error Path & Integration Test Patterns

For testing HTTP-TLS integration (`http::Client` with `https://` URLs), follow these patterns:

**URL parsing tests** (no network needed):
```chemical
@test
public func https_url_parsing_works(env : &mut TestEnv) {
    var u = http::URL::parse(string_view("https://example.com"))
    if(u is std::Option.None) { env.error("should parse"); return }
    var Some(url) = u else unreachable
    if(!url.scheme.equals_with_len("https", 5)) { env.error("scheme should be https") }
    if(url.port != 443u) { env.error("default https port should be 443") }
}
```

**Error path tests** (connection refused / invalid host):
```chemical
@test
public func https_connection_refused_returns_error(env : &mut TestEnv) {
    var client = http::Client()
    var res = client.get(string_view("https://127.0.0.1:49999/nonexistent"))
    if(res is Result.Ok) {
        env.error("https connection to closed port should fail")
    }
}
```

**Server lifecycle pattern** (start server, make requests, clean up):
```chemical
@test
public func https_server_test_pattern(env : &mut TestEnv) {
    // 1. Configure server
    var cfg = http::server::ServerConfig()
    cfg.addr = std::string::make_no_len("127.0.0.1:49998")
    var srv = http::server::Server(cfg)
    
    // 2. Add routes
    srv.router.add("GET", "/", ||(req, res) => {
        res.write_string(std::string::make_no_len("response"))
    })
    
    // 3. Start async and wait for startup
    var thread = srv.serve_async(49998u)
    std.concurrent.sleep_ms(100u)
    
    // 4. Make client requests
    var client = http::Client()
    var res = client.get(string_view("http://127.0.0.1:49998/"))
    // ... verify response ...
    
    // 5. Clean up
    srv.shutdown()
    thread.join()
}
```

**Key conventions:**
- When writing tests in `lang/tests/src/libs/tls/tests.ch` (no `using namespace http;`), fully qualify: `http::server::Server`, `http::URL`, `http::Client`
- When writing tests in `lang/tests/src/libs/net/net_test.ch` (has `using namespace http;`), `server::Server` works directly
- Use high port numbers (49xxx range) to avoid conflicts with other tests
- Keep test names prefixed with the feature area (e.g., `https_` for HTTPS tests)
- Always call `srv.shutdown()` + `thread.join()` in that order for clean server teardown

```bash
# Run all lib tests:
./scripts/test.sh --tcc

# Build + run from scratch:
cmake-build-debug/Compiler lang/tests/build.lab --build-dir lang/tests -o lang/tests/test.exe --mode debug_complete --no-cache -v
./lang/tests/test.exe
```

### Option C: Library Test (for Compiler Plugins like html_cbi, css_cbi)

These are different from standard library tests — they test CBI compiler plugins:

1. **Create the test module** at `lang/tests/compiler_plugins/<name>/`:
   ```
   lang/tests/compiler_plugins/<name>/
   ├── chemical.mod
   ├── build.lab            # Calls testLibsRunner's test infrastructure
   └── src/
       └── main.ch          # Test functions
   ```

2. **Wire in the master build.lab** — add import and build call in `lang/tests/build.lab`

3. **Run with**:
   ```bash
   ./scripts/test.sh --tcc --plugins
   ```

### Option D: Negative Tests (Compiler Failure Verification)

Negative tests verify the compiler correctly rejects invalid code. They work by spawning
the compiler binary via `popen`, compiling a `.ch` source file, and checking the exit code
and stderr for expected error substrings. This is the Chemical source equivalent of the
C++ tests in `core/tests/NegativeLifetimeTests.cpp`.

#### Structure

```
lang/tests/negative/
├── src/
│   └── main.ch              # Test runner — invokes compiler via popen
```

No `chemical.mod` is needed in the negative test package — the test runner lives in the
main test module and is built as a separate executable.

#### Build system wiring

The negative test executable is built separately and gated behind `--arg-negative`:

```chemical
// In lang/tests/build.lab — inside the build() function:
if(ctx.has_arg("negative")) {
    const neg_job = ctx.build_exe("chemical-negative-tests")
    ctx.add_module(neg_job, core_module)
    ctx.add_module(neg_job, cstd_module)
    ctx.add_module(neg_job, std_module)
    ctx.add_module(neg_job, fs_module)
    ctx.add_module(neg_job, neg_tests_mod)
    ctx.define(neg_job, &definition)
    return neg_job
}
```

The negative test source module is declared as:
```chemical
const neg_tests_mod = LabModuleSource {
    name: "negative_tests",
    source_dir: "lang/tests/negative/src"
}
```

#### Running

```bash
./scripts/test.sh --tcc --negative        # Build + run negative tests
./scripts/test.sh --tcc --negative --no-build  # Skip rebuild
```

The `--negative` flag in `scripts/test.sh` translates to `--arg-negative` and uses a
separate output path (`lang/tests/build/negative-tests-tcc.exe`) to avoid overwriting
the main test executable.

#### How the test runner works

The runner in `lang/tests/negative/src/main.ch` does:

1. Creates a temp directory under `/tmp/chemical_neg_tests/`
2. For each test case, writes a `chemical.mod` and `test.ch` with intentionally invalid code
3. Invokes the compiler via `popen(cmd, "r")` and captures stderr
4. Checks the exit code (non-zero for expected errors) and stderr for expected error substrings
5. Cleans up the temp directory

#### Key API used in the test runner

| API | Source | Notes |
|-----|--------|-------|
| `intrinsics::get_compiler_path()` | Compiler intrinsic | Returns `*char` — path to the compiler binary |
| `popen(cmd, "r")` | C stdlib (`cstd`) | Opens a pipe to a command; returns `*mut FILE` |
| `pclose(pipe)` | C stdlib | Closes pipe, returns exit status |
| `fgets(buf, size, pipe)` | C stdlib | Reads a line from a pipe |
| `sprintf(buf, fmt, ...)` | C stdlib | Format string into a buffer |
| `mkdir(path, mode)` | C stdlib | Create a directory |
| `fwrite`, `fopen`, `fclose` | C stdlib | File I/O for writing temp files |

#### Important pointer gotcha in the test runner

Array indexing `arr[i]` produces a **reference** (`&char`), not a raw pointer (`*char`).
To pass array elements to C functions expecting `*char` or `*mut char`, use `&raw` or `&raw mut`:

```chemical
var cmd : char[2048]
sprintf(&raw mut cmd[0], "%s \"%s\"", compiler, mod_path)  // ✅ *mut char
var pipe = popen(&raw cmd[0], "r")                         // ✅ *char (read-only)

// WRONG — produces &char, not *char:
// sprintf(&cmd[0], ...)   // TypeCheck error: &char does not satisfy *mut char
// popen(&cmd[0], ...)     // TypeCheck error: &char does not satisfy *char
```

#### Writing a new negative test

1. Add a new `var ch_N = "..."` string literal containing the invalid Chemical source
2. Call `run_single_negative_test()` with the source and expected error info:
   - `expect_error: true` — test passes when compiler exits non-zero and stderr contains expected text
   - `expect_error: false` — test passes when compiler exits zero (compiles successfully)
   - `expected_sub` — substring to search for in stderr (empty string `""` means any error is fine)
3. Wrap in `test("name", ...)` call

#### Example

```chemical
var ch_new = "struct Foo {\n    func broken(&self) : ??? {\n    }\n}\n"
test("broken_return_type_errors",
    run_single_negative_test("/tmp/chemical_neg_tests", "broken_return_type",
        mod, ch_new, true, "TypeCheck"))
```

## Writing Tests for Compiler Failure

There are two approaches to testing that the compiler correctly rejects invalid code:

### Approach 1: Negative Tests (Preferred)

Use the negative test infrastructure described in **Option D** above. These tests
spawn the compiler binary, compile a `.ch` file with intentionally invalid code, and
check that the expected errors appear. Run with:

```bash
./scripts/test.sh --tcc --negative
```

This is the Chemical-source equivalent of the C++ tests in `core/tests/NegativeLifetimeTests.cpp`.
See **Option D** for the full API reference and patterns.

### Approach 2: Comptime Intrinsics (Limited)

For simpler cases, you can use comptime intrinsics:

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

### Debugging Test Crashes with GDB Backtraces

When a test crashes with `RUNTIME ERROR: invalid memory access`, use `-bt` or `-bt-full` flags to get a stack trace:

```bash
# Basic backtrace (bt full)
./scripts/test.sh --tcc --bt

# Full backtrace with registers, disassembly, locals, args
./scripts/test.sh --tcc --bt-full

# For interpretation tests:
./scripts/test.sh --tcc --interpret --bt
```

The `-bt` flag wraps the test executable (or compiler in interpretation mode) with:
```bash
gdb -batch -ex "run" -ex "bt full" --args <program>
```

The `-bt-full` flag adds thread info, registers, instruction disassembly at `$pc`, and local/arg variable values.

> Both flags imply `-g` automatically. Works in all modes (compiled, interpret, negative).

> ⚠️ However, `-bt` only catches crashes in the **parent process**. The `@test` runner dispatches tests via `posix_spawnp` subprocesses, so child-process crashes are NOT caught by `-bt`. For those, use the standalone module approach below.

### Isolating Library-Dependent Test Failures

When a test failure depends on library code (TLS, audio, HTTP, etc.), a bare `.ch` file
cannot import them. Create a **standalone module with a `chemical.mod`** instead:

```bash
mkdir -p lang/compiled/my_test/src
```

**`lang/compiled/my_test/chemical.mod`**:
```
application my_test
source "src"
import cstd
import std
import tls           # add whatever libs the failure requires
import audio
import net
```

**`lang/compiled/my_test/src/main.ch`** — reproduce the exact failing code path:
```chemical
@extern
public func printf(format : *char, _ : any...) : int

public func main() : int {
    // mirror the failing @test code here
    var ssl : tls::SSLContext
    tls::ssl_init(&raw mut ssl)
    // ...
    printf("Done\n")
    return 0
}
```

**Compile and run:**
```bash
cmake-build-debug/Compiler "lang/compiled/my_test/chemical.mod" \
    -o "lang/compiled/my_test/main.exe" \
    --mode debug_complete --assertions

./lang/compiled/my_test/main.exe
```

**Get a GDB backtrace on SIGSEGV:**
```bash
gdb -batch -ex run -ex bt -ex "info registers" -ex "x/16i \$pc" \
    ./lang/compiled/my_test/main.exe
```

**Inspect the LLVM IR:**
```bash
cmake-build-debug/Compiler "lang/compiled/my_test/chemical.mod" \
    -o "lang/compiled/my_test/main.exe" \
    --mode debug_complete --assertions \
    --out-ll-all --build-dir "lang/compiled/my_test/build/"

# IR at: lang/compiled/my_test/build/modules/<name>/llvm_ir.ll
```

This approach gives you:
- **Full library access** — no manual code copying
- **Fast iteration** — no need to rebuild the full test suite
- **Focused LLVM IR** — only your module's codegen, easy to inspect
- **GDB backtraces** — the standalone executable runs in a single process, so the parent-process `gdb -batch` approach works for all crashes

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
./scripts/test.sh --tcc --plugins                   # All library tests
```


### Individual Library Tests

```bash
# Using --arg-test-<name> CLI args:
./scripts/test.sh --tcc --plugins                   # Tests with --arg-test-html etc internally
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
│   └── libs/               # Standard library @test tests (archive, image, audio, font)
│       ├── archive/tests.ch
│       ├── image/tests.ch
│       ├── audio/tests.ch
│       └── font/tests.ch
├── common/src/             # Shared tests (interpret + compiled)
├── native_common/src/      # Pointer tests (interpret + compiled)
├── interpret/src/          # Interpretation-only entry point
├── compiler_plugins/       # CBI plugin tests (html, css, js, universal, md)
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
