# AGENTS.md — Chemical Programming Language

## Language Goals & Vision

Chemical is developed for **AIs and humans**, not humans alone. AI agents are the
primary authors; humans are reviewers/auditors. The compiler must make wrong code hard
or impossible to write, with diagnostics written for an agent to act on — while keeping
code legible enough that a human can get the gist and then dig in with AI help.

**The four non-negotiable requirements:**

0. **Extreme performance** — code is no longer the bottleneck; runtime speed is.
1. **Complete memory safety** — an AI must not be able to cause a memory error.
2. **Enforceable no-panic policy** — a module can forbid panics; the compiler errors if
   any function can panic, forcing agents to report errors instead of aborting.
3. **Runs everywhere** — browser (as JS), WASM, JVM bytecode, Android, native (C + LLVM),
   and native with GC (a native binary with an embedded collector — no hosted VM).

**Toolchain constraint (binding):** the non-LLVM release compiler (`TCCCompiler` /
`chemical`) must stay **under 4 MB** (goal) with a **hard 5 MB ceiling — never more** as
the steady state, while still supporting the **whole language and every target it owns**
(C/TinyCC, JVM, JS) and being **really performant** (compile time, memory, and
generated-code quality improve together). Get there by **moving heavy, non-essential
machinery into plugins/libraries** and shrinking shared library/runtime code — never by
dropping capabilities. The **first** release may be much larger (7–15 MB) to ship fast;
after that, size + compile time + memory are reduced together. See
`lang/docs/language-goals.md` §2.

**Rust relationship:** adopt Rust-familiar **memory-safety semantics** (ownership, move
semantics, a borrow checker) so agents trained on Rust transfer their instincts — but
keep Chemical syntax (e.g. return types use `:` not `->`). This is **not** another Rust.
Reject features that cost compilation time for little verification value (e.g. Rust-style
type inference from later usage). Prefer constraining wrong programs over adding sugar.

**Known blockers on the "runs everywhere" path (mostly unsolved):** `#js`/universal
components emit raw unchecked JavaScript; no TypeScript-equivalent DOM/web type
declarations; no JavaScript backend; borrow checking makes DOM-heavy code harder; GC
platforms (JVM) vs. native semantics; **a native GC mode (a Go-like "ignore
allocation, still a native binary" mode with an embedded collector) — its semantics,
boundary, and implementation are open** (see `lang/docs/language-goals.md` §6.7);
**generic impls (`impl<T> Trait for MyType<T>`) are not supported** — `impl <T> ...`
is rejected and parameterized impls do not resolve methods, so generic types cannot
participate uniformly in protocols (§6.8); and the open problem of one syntax across
divergent platform memory models.

**Proposal roadmap (2026-10):** the large-feature proposals live in `lang/docs/proposals/`
(open `review.md`). Approved: generic impls (`0003`), borrow checker (`0004`), no-panic
effects (`0005`), native GC (`0006`), JavaScript backend (`0007`), web/DOM types (`0008`),
async completion (`0013`), generic-container JSON serialization (`0014`) — implemented after
the MIR consolidation work. WASM (`0009`) is deferred to TinyCC's direction; JVM (`0010`) is
reconsidered now that GC is approved, and Android/cross-compilation (`0011`) waits on a
working GC model. **No approved feature may
regress compiler compile time, peak memory, or generated-code quality**; shared machinery
goes in MIR and backends stay thin.

Full detail: [`lang/docs/language-goals.md`](lang/docs/language-goals.md). Load the
[`language_goals`](.agents/skills/language_goals/SKILL.md) skill before making
language-design or roadmap decisions.

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
./scripts/test.sh --tcc --plugins     # Include compiler plugin tests (html, css, js, etc.)
./scripts/test.sh --tcc --libs        # Library test suite (bcrypt, uuid, json, fs, crypto, audio, ...)
./scripts/test.sh --tcc --regexp      # Extensive regex library test suite (dedicated)
./scripts/test.sh --tcc --async       # Async/await suite
./scripts/test.sh --tcc --universal  # Universal component tests (#universal_test) in a real WebView
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
./scripts/test.sh --tcc -bt      # Run under GDB -batch, print backtrace on crash
./scripts/test.sh --tcc -bt-full # Full GDB backtrace with registers, disasm, locals
```

**Run every suite & get a summary table** (fresh-clone / new-machine sanity check):

> ⚠️ **AI agents: do NOT run `--all`.** It runs every suite and takes a very long
> time (the `tls` suite alone runs for minutes). It exists only for a human doing
> a new-machine sanity check. Agents must run the single suite relevant to their
> change (e.g. `--tcc`, `--libs`, `--regexp`, `--async`, `--webview`) and stop.

```bash
./scripts/test.sh --all            # all suites on TCCCompiler (tls skipped)
./scripts/test.sh --all --include-tls   # also run the slow tls suite
./scripts/test.sh --llvm --all     # all suites on Compiler (LLVM)
./scripts/test.sh --all -v         # stream each suite's output live
```

`--all` builds the compiler once, then runs main, interpret, negative, plugins,
async, libs, regexp, process, server and webview in turn, printing a per-suite result and
a final table (`SUITE / STATUS / TOTAL / PASSED / FAILED / TIME`). **The slow
`tls` suite is skipped by default**; pass `--include-tls` to include it (the run
tells you this at the top and in the summary). Full per-suite logs are kept in
`lang/tests/build/all-logs/`. Exits non-zero if any suite failed. Note that some
suites are environment-sensitive (`tls` needs python3 with the `cryptography`
module plus `fuser`, since it cross-checks the port against python's
`cryptography` and manages its own server processes; `webview` needs GTK3 + a
display).

> 🐛 **`-bt` / `-bt-full`**: Wraps the test in `gdb -batch` mode. On crash, prints backtrace.
> `-bt` gives `bt full`; `-bt-full` adds thread info, registers, `x/16i $pc`, locals, and args.
> Implies `-g`. Works in compiled, interpret, and negative modes.

Internal defaults: `--no-cache`, `-frecompile-plugins`. Pass `--cache`/`--cached-plugins` to opt out.

### Running Specific Tests

Use `--test-ids` to run specific `@test`-annotated functions by their numeric ID, and `--test-names` to run by function name:

```bash
# Run by ID (comma-separated)
./scripts/test.sh --tcc --no-build --skip-sequential --test-ids 1073741823,1073741824

# Run by name (comma-separated)
./scripts/test.sh --tcc --no-build --skip-sequential --test-names font_create_empty_works,test_hex_encode_upper
```

The `--skip-sequential` flag skips all inline tests (manually called in `run_executable_tests()`) and only dispatches `@test`-annotated functions:

```bash
# Only @test functions, no inline tests
./lang/tests/build/tests.exe --skip-sequential

# With test ID filter
./lang/tests/build/tests.exe --skip-sequential --test-ids 1073741823
```

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
- Parses, symres, and typechecks every module of the job
- Lowers each module's functions and globals into one shared MIR program
- Finds `main()` and runs it on `mir::MIRInterpreter` (the same MIR the code generators consume)
- Never generates object code or links an executable

### Compiled (Runtime) Tests

```bash
# CI equivalent (runs from release binary, no `--no-cache`):
./chemical lang/tests/build.lab -arg-minimal -bm -v --assertions --mode debug_complete

# Lib tests CI:
./chemical lang/tests/build.lab -arg-test-plugins -bm -v --assertions --mode debug_complete --no-cache

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
- Source dirs: `basic/`, `comptime/`, `core/`, `generic/`, `compiler_plugins/`, `nodes/`, `stdlib/`.
- Compiler plugin tests in `lang/tests/compiler_plugins/*/src/`.
- Library tests (bcrypt, uuid, json, datetime, fs, path, encoding, crypto,
  compression, osrand, mime, audio, font, archive, image + `integration/`) live in
  `lang/tests/libs/` — a standalone module (`chemical.mod` + `main.ch` entry) dispatched
  via `--arg-test-libs` / `./scripts/test.sh --tcc --libs`. They are **not** part of the
  main test suite; keep essential libraries (e.g. anything the language itself relies on,
  like `std`/`core`/`atomic` tests under `lang/tests/src/stdlib/`) in the main suite.
- Dedicated library suites (run independently so the main `--tcc` suite stays fast and environment-specific suites can run alone):
  - `lang/tests/regexp/` (`chemical.mod` + `main.ch`) — the extensive `regex` library suite (literals, dot/escapes, quantifiers, classes, anchors, groups/alternation, captures, find/replace/split, compile errors and realistic patterns). All test files are flat in the module directory and use the shared `helpers.ch` assertions (`rx_match`, `rx_find`, `rx_replace`, `rx_capture_pair`, ...). Run with `./scripts/test.sh --tcc --regexp` (included in `--all` after `libs`).
  - `lang/tests/process/` (`chemical.mod` + `src/`) — `process` **and** `environment` library tests. Run with `./scripts/test.sh --tcc --process`.
  - `lang/tests/webview/` (`chemical.mod` + `src/`) — `webview` library tests (display-independent API only; requires GTK3 + WebKit2GTK to link/run). Run with `./scripts/test.sh --tcc --webview`.
  - `lang/tests/universal_webview/` (`chemical.mod` + `src/`) — `#universal_test` universal-component tests: each test SSR-renders an inline fixture with the production pipeline and runs raw JS steps in a real WebView. Entry is `universal_test_runner(argc, argv)` (not `test_runner`); discovery is a `universal_test` collector annotation + `intrinsics::get_universal_tests<UTFunction>()`. Requires a display (use `xvfb-run` headless). Run with `./scripts/test.sh --tcc --universal`. Load the `universal_testing` skill.
  - See `## Chemical Library Development Gotchas` → *Uninitialized Variables Require the `unsafe` Keyword* for the migration rule that applies to all of these test sources.

### Interpretation Mode Details

**Entry:** `do_interpretation_job()` in `compiler/lab/LabBuildCompiler.cpp`.

**There is no AST statement interpreter.** Interpretation lowers the job's
modules to one shared MIR program and runs it on `mir::MIRInterpreter`:

1. Every module is parsed, symres'd and typechecked.
2. `ASTProcessor::interpret_module` lowers the module's function bodies and
   globals into the shared MIR program (collect mode).
3. `main()` is found, its mangled name resolved exactly as the lowerer interned
   it, and `MIRInterpreter::run` executes it. No object code is generated and no
   executable is linked.

`interpretation_mode` is **off** for the job, so
`comptime if(intrinsics::is_interpretation())` takes the compiled branch: an
interpreted run must produce byte-identical output to the compiled binary. A
compiled-vs-interpreted difference is a `MIRInterpreter` bug, not a language
limitation.

Lowering-time `comptime func` calls run on the same engine through
`mir::MIRComptime`, which declines anything the engine does not reproduce
exactly. What still folds on the AST is expression-only: symres-time `comptime
if` conditions, array lengths and `const` initializers, evaluated by
`Value::evaluated_value` plus the compiler-intrinsic `FunctionDeclaration::call`
overrides (`ast/utils/GlobalFunctions.cpp`). Do not add statement execution
there — new capability goes into MIR. Full design:
`lang/docs/mir-interpretation-plan.md`; load the `interpreter` skill.

## Skills

The following skills are available in `.agents/skills/`. An AI agent should load the relevant skill(s) before working on a particular subsystem. Skills provide comprehensive, up-to-date documentation of codebase internals.

### Language Goals & Vision

| Skill | Location | Description |
|-------|----------|-------------|
| Language Goals & Vision | `.agents/skills/language_goals/SKILL.md` | The language's direction: AI-first + human-readable, Rust-familiar memory safety (borrow checking), strict type and memory safety, module-level no-panic enforcement, rejecting compile-time-expensive sugar (Rust-style type inference), and the cross-platform "runs everywhere" ambition (native C/LLVM, WASM, JavaScript/browser, JVM, Android). Canonical doc: `lang/docs/language-goals.md` |

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
| Testing Guide | `.agents/skills/testing/SKILL.md` | Test infrastructure: manual `test()` calls vs `@test` annotation dispatch, `test_env` library, `lang/tests/build.lab` wiring, inline/compiled/interpret/library tests, writing tests for compiler failures, and **streaming/combining multiple test runners** (`test_report`, `MultiTestRunner`) |
| Components E2E Testing | `.agents/skills/components_e2e/SKILL.md` | Browser E2E suite for the components/universal libraries (`lang/compiled/components-e2e`, GitHub: chemicallang/components-e2e): how to run, write, and debug Playwright tests against the real SSR→hydration→interaction pipeline |

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
| Benchmark Dashboard | `.agents/skills/benchmark_dashboard/SKILL.md` | The benchmark/release analytics system: gh-pages data model (manifest/daily/release JSON), `bench-*.sh` collection scripts, benchmark workflows, and the static dashboard (6 views incl. Failures, shadcn-style design, stability invariants, Node test harness) |

### Web & Components

| Skill | Location | Description |
|-------|----------|-------------|
| Universal Components | `.agents/skills/universal/SKILL.md` | Universal component pipeline: SSR + hydration, runtime behavior, prop serialization, subscriber mutation bugs, debugging generated HTML/CSS/JS |
| Universal Component Testing | `.agents/skills/universal_testing/SKILL.md` | `#universal_test` native WebView tests: macro syntax (inline fixture + raw JS `<script>` steps), injected JS API, isolation/filters, discovery intrinsic + `universal_test` runner library, implementation gotchas |
| Designing Web Apps | `.agents/skills/design_web_app/SKILL.md` | Web app design in Chemical: static pages, `#html`/`#css`/`#js` macros, `#universal` components, `page` library, server + client rendering |

### Compiler API (for plugin development)

| Skill | Location | Description |
|-------|----------|-------------|
| Compiler API | `.agents/skills/compiler_api/SKILL.md` | Compiler API bindings in `lang/libs/compiler/`: ASTBuilder, Lexer, Parser, SymbolResolver, SourceProvider, BatchAllocator |
| Intrinsics & Reflection | `.agents/skills/intrinsics_compiler_reflection/SKILL.md` | Compiler intrinsics: all `intrinsics::*` functions, interpreter-friendly std types (`InterpretVector`), reflection APIs for module/type/function metadata, how to add new intrinsics |
| Interpreter Internals | `.agents/skills/interpreter/SKILL.md` | Interpretation and comptime after the MIR migration: `MIRInterpreter`, `MIRComptime`, the residual AST expression folder (`evaluated_value`, `InterpretScope`, `PointerValue`), and debugging `--interpret`/comptime failures |
| JSON Serialization | `.agents/skills/json_serialization/SKILL.md` | JSON via the `json` runtime library (`JsonValue`, `JsonEncoder`/`JsonDecoder`, `TypeDecoder<T>` dispatch) and the `json_cbi` `#json(Struct)` macro that generates `std::Serializer`/`std::Deserializer` impls — full pipeline, CBI bindings it relies on, and codegen/symres gotchas |

### How to load a skill

Use the `skill` tool with the skill name (the directory name):

```
skill {
  name: "symres"            # loads .agents/skills/symres/SKILL.md
}
```

The skill name is the directory name under `.agents/skills/`. For example:
- `name: "language_goals"` → loads `.agents/skills/language_goals/SKILL.md`
- `name: "symres"` → loads `.agents/skills/symres/SKILL.md`
- `name: "build_system"` → loads `.agents/skills/build_system/SKILL.md`
- `name: "universal"` → loads `.agents/skills/universal/SKILL.md`

### Which skill to load?

| Task | Load these skills |
|------|-------------------|
| **Making language-design / roadmap / feature-fit decisions** | **`language_goals`** |
| **Working on memory safety / borrow checker / no-panic direction** | **`language_goals`**, `type_verification`, `symres` |
| **Working on cross-platform backends (JS/WASM/JVM/browser) or DOM/web type declarations** | **`language_goals`**, plus the relevant backend skill |
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
| **Working on the benchmark dashboard / analytics workflows** | **`benchmark_dashboard`** |
| **Testing components in a real browser / debugging hydration or runtime bugs** | **`components_e2e`**, `universal`, `testing` |
| **Writing universal component tests in the native WebView** | **`universal_testing`**, `universal`, `testing` |
| **Building CBI macro plugins** | `compiler_bindings`, `macro_code_gen`, `compiler_api` |
| **Extending libraries (`lang/libs/`)** | `compiler_api`, `cbi_plugin_api` |
| **Working on the `json` library / the `#json` macro (`json_cbi`)** | **`json_serialization`**, `chemical_source`, `cbi_plugin_api` |
| **Writing a macro that generates impls via EmbeddedNodes (like `#json`)** | **`json_serialization`** (worked example), `macro_code_gen`, `cbi_plugin_api` |
| **Implementing TLS/HTTPS** | `chemical_source`, `testing`, `building` |
| **Integrating TLS with HTTP** | `chemical_source`, `testing` |

Skills in **bold** are the new comprehensive skills. Load them for maximum context.

## Architecture

| Path | Purpose |
|------|---------|
| `ast/` | AST node definitions (values, types, structures, statements) |
| `compiler/` | Compilation pipeline |
| `compiler/CodegenMir.cpp` | MIR → LLVM emission (types, function bodies, globals, DWARF) — the LLVM backend |
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
| `lang/libs/json/` | JSON runtime: `JsonValue`, parser, `JsonEncoder`/`JsonDecoder`, `TypeDecoder<T>` decode dispatch, `__non_gen_se_repl` |
| `lang/libs/json_cbi/` | `#json(Struct)` macro plugin: auto-generates `std::Serializer`/`std::Deserializer` impls |
| `lang/libs/test_report/` | Shared test reporting: `TestOutcome`, `TestReporter` interface, `ConsoleReporter`, `TestRunnerHandle` + `MultiTestRunner` for combining runners (see `lang/docs/test-reporting.md`) |
| `lang/libs/compiler/` | Compiler API bindings for macros |
| `lang/tests/` | Test suite |

## Async / Await — Remaining Work

Async/await is shipped and integrated across `lang/libs`, but several compiler and
platform limitations were **worked around, not fixed**. Before touching async
internals, read [`lang/docs/async-remaining-work.md`](lang/docs/async-remaining-work.md):
an actionable, one-item-at-a-time worklist with symptom, root cause, workaround,
and definition of done for each item.

Pending items (recommended fix order):

1. **AC** — async closures parsed but not lowered

**B15-W, B20, B23, B24, B25, B26, POSIX-EPOLL, TLS-VT and WIN-IOCP are fixed**
— see the worklist. (B15-W was the LLVM `debug_complete` debug-info limitation:
the async lowering now emits a `DISubprogram` per generated function and
`CoroSplit` gives each clone its own. POSIX-EPOLL replaced the `select(2)` fd
reactor with persistent `epoll(7)` on Linux and `kqueue(2)` on macOS/BSD
(`select` kept as fallback), removing the `FD_SETSIZE` ~1024 fd cap. TLS-VT
replaced the async TLS `spawn_blocking` bridge with a shared `Transport` vtable:
the record layer/handshake are coroutines, the sync API drives them with
`block_on` over a blocking transport, and the async API installs a non-blocking
transport that suspends on fd readiness. WIN-IOCP wired Windows `net` async
(`accept`/`recv`/`send`) to a process-wide IOCP completion port with a dispatcher
thread instead of `spawn_blocking`; `AcceptEx` is resolved via the bundled
`mswsock.def`.)

Design/assessment: [`async-await-design.md`](lang/docs/async-await-design.md),
[`async-library-integration.md`](lang/docs/async-library-integration.md).

## Conventions

- **No backend may read the AST — every backend works from MIR.** The AST is a front-end artifact (parser, symres, type-check, `MIRLowerer`); no code generator may touch it. Every backend (`MIREmitter` for C, `compiler/CodegenMir.cpp` for LLVM, and any JVM/JS/WASM backend later) is a dispatcher over MIR only: types and layout from `MIRTypeTable`/`MIRLayout`, calling convention from `MIRABI`, per-function facts from `MIRFacts`, names/mangling/linkage and constants from the MIR module, declarations from MIR declaration tables (`MIRDeclPrinter` + `MIRGlobal` today; `MIRTypeDecl`/`MIRPrototype` as they land). No `ast/` include, no `ASTNode*`/`BaseType*`/`StructDefinition*`/`FunctionDeclaration*` parameter, no source-node walk, no re-deriving layout/ABI/declarations/destructor rules/lifetime from source form. A fact a backend needs that MIR does not publish is **added to MIR**, never fetched from the AST — that is what keeps a new target a thin dispatcher + printer instead of another front end. Only the lowering boundary (`MIRTypeBuilder`, `MIRLowerer`) and the front-end declaration collection pass may see the AST. Statements: `lang/docs/mir-implementation-plan.md` §0 rule 9, `lang/docs/mir-design.md` §16, `compiler/mir/README.md`.
- Custom strings: `chem::string` (SSO, no `std::string`).
- Name mangling: scoped → `scope_name` prefix, generics → `__cgs__N`/`__cfg__N` suffix.
- Operator overloading: `a + b` → looks for `add(a, b)` method.
- `#macro` and `#universal` are CBI plugins compiled by TinyCC at build time.
- AST uses `ASTAllocator` arena — batch-allocated, no per-node `delete`.
- CLI entry is `LabBuildCompiler` → translates `chemical.mod` → JIT-compiles `build.lab` (TinyCC) → `LabJob` objects → parse → symres → typecheck → codegen → link.

## build.lab Script Helpers

The `build.lab` JIT-compiled build scripts have access to these helpers from the `lab::` namespace:

| Helper | Purpose |
|--------|---------|
| `lab::run(cmd)` | Run a shell command (returns output) |
| `lab::curr_dir()` | Get current working directory |
| `lab::rel_path_to(path)` | Get path relative to the build script location |
| `lab::mkdir(path)` | Create directory |
| `lab::rm(path)` | Remove file/directory |
| `lab::cp(src, dst)` | Copy file |
| `lab::exists(path)` | Check if path exists |
| `lab::read_file(path)` | Read file contents |
| `lab::write_file(path, content)` | Write content to file |
| `lab::glob(pattern)` | Find files matching pattern |

## Complete Annotation Reference

All built-in annotations recognized by the compiler:

| Annotation | Target | Purpose |
|------------|--------|---------|
| `@extern` | func, struct, var | External definition (no mangle, linked from elsewhere) |
| `@test` | func | Marks test function (auto-discovered by `test_runner`) |
| `@deprecated` | func, struct, etc. | Marks item as deprecated (optionally with message: `@deprecated("use new_func")`) |
| `@no_mangle` | func | Don't mangle the function name |
| `@override` | func (top-level) | Let a top-level function coexist with a same-named function from a **different module** (no duplicate-symbol error). Used for an application `main` overriding a library's test `main`. Still errors for same-module duplicates, or when both sides are `@no_mangle`/`@extern`. Does **not** remove the library's tests. See [`annotations` skill](.agents/skills/annotations/SKILL.md#top-level-override) |
| `@static` | func (in struct) | Static member function (no `self` parameter) |
| `@implicit` | func parameter | Implicit parameter (not passed explicitly by caller) |
| `@constructor` | func | Marks a constructor method (replaces default init) |
| `@inline` | func | Hint to inline the function |
| `@noinline` | func | Prevent inlining of the function |
| `@make` | struct | Enables `T.make()` constructor; **prevents** `T{}` syntax unless `@direct_init` is also present |
| `@direct_init` | struct | Enables `T{}` and `T{field: val}` aggregate initialization |
| `@allow_zeroed` | struct, union, variant | Allows `zeroed<T>()` even when the type has a constructor/destructor |
| `@no_drop` | struct, union, variant | Suppresses all drop glue (automatic and explicit `@delete`); used by `ManuallyDrop`/`MaybeUninit` |
| `@delete` | func (in struct) | Destructor — called when value goes out of scope or is deleted |
| `@volatile` | var, const | Marks variable as volatile — prevents compiler optimization of loads/stores |
| `@compiler.interface` | interface | Marks a CBI interface exposed to the compiler (used in `build.lab` for build API) |

**Critical `@make` + `@direct_init` interaction:**

| Annotations | `T{}` works? | `T.make()` works? | `T{field: val}` works? |
|---|---|---|---|
| `@direct_init` only | Yes (all fields required) | No | Yes |
| `@make` only | **No** | Yes | **No** |
| `@direct_init` + `@make` | Yes (all fields required) | Yes | Yes |
| Neither | Yes (all fields required) | No | Yes |

> A struct with `@make` but WITHOUT `@direct_init` **cannot** use `{}` syntax at all. Use `T.make()` instead.

## Language Features Quick Reference

### Expressive Strings (Backtick Templates)

```chemical
var name = "World"
var msg = `Hello ${name}!`           // String interpolation
println(`2 + 2 = ${2 + 2}`)         // Expression interpolation
```

Backtick strings evaluate `${}` expressions inline. Used with `expr_println` in interpretation mode.

### Loop Expressions

```chemical
var result = loop {
    if(done) { break value }
    // ...
}
```

`loop { break value }` acts as an expression that evaluates to `value`.

### For-In Loops

```chemical
for(var x in collection) { ... }        // Forward iteration
for(var x in collection reversed) { ... } // Reverse iteration
for(var x, i in collection) { ... }     // With index
```

Requires implementing `core::iterable::Iterable<T, Cursor>`. For `reversed`, also implement `ReversibleIterable`.

### `using` Declarations

```chemical
using closed_bro::bring_me_in;      // Single symbol
using namespace all_closed;          // All symbols from namespace
```

### Defer

**Not yet implemented.** Chemical does not support `defer`. Use `@delete` destructors for RAII-style cleanup.

### `zeroed<T>()`

Creates a zero-initialized value of type T:

```chemical
var buf : [256]char = zeroed        // Array zeroed at declaration
var s = zeroed<MyStruct>()          // Zero-initialized struct (no constructor needed)
```

### `sizeof()` and `alignof()`

```chemical
var size = sizeof(int)              // 4 bytes
var align = alignof(double)         // 8 bytes
var struct_size = sizeof(MyStruct)  // Size of struct
var arr_size = sizeof(my_array)     // Total array size
```

### Inline Assembly

Chemical supports GCC extended inline assembly via the `asm` keyword:

```chemical
// Simple form — template only
asm("nop")

// Extended form — with outputs, inputs, clobbers
var old_val : u64 = 0
var success : bool = false
asm("lock cmpxchgq %3, %2\n\tsete %1"
    : "=a"(old_val), "=q"(success), "+m"(*ptr)   // outputs
    : "r"(desired), "0"(old_val)                   // inputs
    : "cc", "memory")                              // clobbers

// Compiler barrier (prevents reordering, no hardware fence)
asm("" ::: "memory")
```

**Syntax**: `asm("template" : outputs : inputs : clobbers)`

- **Outputs**: `"constraint"(expr)` — where asm writes. `"=a"` = rax, `"=q"` = any GP reg, `"+m"` = read/write memory
- **Inputs**: `"constraint"(expr)` — values passed to asm. `"r"` = any reg, `"0"` = same reg as operand 0, `"m"` = memory
- **Clobbers**: `"memory"`, `"cc"` — what asm may trash

**Critical pattern**: `asm(...)` is a **statement**, not an expression. Use output operands to write to local variables. For use in comptime functions, wrap in `@retained` helper + `%runtime_value()`:

```chemical
@retained func my_asm_helper(ptr : *mut u64) : u64 {
    var val : u64 = 0
    asm("movq %1, %0" : "=r"(val) : "m"(*ptr) : "memory")
    return val
}

public comptime func atomic_load(x : %runtime<*u64>) : u64 {
    comptime if(intrinsics::get_backend_name() == "C") {
        return %runtime_value(my_asm_helper(x)) as u64
    }
}
```

## Standard Library Quick Reference

### Core Types

| Type | Description | Key Methods |
|------|-------------|-------------|
| `std::string` | Heap-allocated string | `append()`, `append_view()`, `append_string()`, `copy()`, `size()`, `to_view()` |
| `std::string_view` | Lightweight string view (no ownership) | `size()`, `get(i)`, implicit from `*char` |
| `std::vector<T>` | Dynamic array | `push()`, `get(i)`, `get_ptr(i)`, `size()`, `reserve()`, `clear()`, `data()` |
| `std::unordered_map<K,V>` | Hash map (non-deterministic order) | `insert()`, `get_ptr()`, `contains()`, `erase()`, `size()`, `clear()` |
| `std::ordered_map<K,V>` | Insertion-order-preserving hash map | Same as `unordered_map` + `for-in` preserves insertion order |
| `std::function<sig>` | Capturing lambda container | Required for lambdas that capture variables |
| `std::Option<T>` | Optional value | `Some(value)`, `None()` |
| `std::Result<T,E>` | Error handling | `Ok(value)`, `Err(error)` |
| `std::concurrent` | Threading utilities | `sleep_ms(ms)` |

### `std::function` for Capturing Lambdas

```chemical
var x = 10
var lambda : std::function<(p : int) => int> = |x|(p : int) : int => {
    return p + x
}
```

Without `std::function`, capturing lambdas cannot capture variables.

### `std::concurrent`

```chemical
std::concurrent.sleep_ms(100u)   // Sleep for 100 milliseconds
```

## Compiler Intrinsics Reference

All intrinsics are called via `intrinsics::name()`. Load the `intrinsics_compiler_reflection` skill for full details.

### Print/Debug

| Intrinsic | Signature | Purpose |
|-----------|-----------|---------|
| `intrinsics::print` | `print(value : any...)` | Print values (space-separated) |
| `intrinsics::println` | `println(value : any...)` | Print + newline |
| `intrinsics::expr_print` | `expr_print(expr : %expressive_string)` | Print backtick template |
| `intrinsics::expr_println` | `expr_println(expr : %expressive_string)` | Print backtick template + newline |

### Mode Detection

| Intrinsic | Returns | Purpose |
|-----------|---------|---------|
| `intrinsics::is_interpretation()` | `bool` | True if running in interpretation mode |
| `intrinsics::is_runtime()` | `bool` | True if running in codegen (not comptime) |
| `intrinsics::is_comptime()` | `bool` | True if inside a comptime evaluation |

### Type Reflection

| Intrinsic | Signature | Purpose |
|-----------|-----------|---------|
| `intrinsics::satisfies<T, U>()` | `bool` | True if T satisfies interface U |
| `intrinsics::is<T, U>()` | `bool` | True if T and U are identical types |
| `intrinsics::type_to_string<T>()` | `string` | Human-readable type name |
| `intrinsics::size(value)` | `size_t` | Size of string or array |
| `intrinsics::defined(name)` | `bool` | True if build definition is set |
| `intrinsics::supports(feature)` | `bool` | True if backend supports the feature |

### Source Location

| Intrinsic | Returns | Purpose |
|-----------|---------|---------|
| `intrinsics::get_raw_location()` | `u64` | Encoded SourceLocation of call site |
| `intrinsics::get_line_no()` | `int` | Line number of call site |
| `intrinsics::get_char_no()` | `int` | Character offset of call site |
| `intrinsics::get_current_file_path()` | `*char` | File path of current source file |

### Module/Build

| Intrinsic | Returns | Purpose |
|-----------|---------|---------|
| `intrinsics::get_module_name()` | `string` | Module name |
| `intrinsics::get_module_dir()` | `string` | Module directory path |
| `intrinsics::get_build_dir()` | `string` | Build output directory |
| `intrinsics::get_compiler_path()` | `*char` | Path to compiler binary |
| `intrinsics::get_target()` | `string` | Target triple (e.g., "x86_64-linux-gnu") |
| `intrinsics::version()` | `string` | Compiler version |

### Function Reflection

| Intrinsic | Signature | Purpose |
|-----------|-----------|---------|
| `intrinsics::get_child_fn<T>(name)` | `*T` | Pointer to child function by name |
| `intrinsics::get_marked_decls<T>(name)` | `[]T` | Array of declarations with given annotation |

## Interpretation (MIR) Notes

The MIR interpreter executes exactly the MIR the code generators consume, so
there is no separate "interpreter-friendly" feature set any more: if a program
symres/typechecks, the engine runs it. Known engine limits are coroutine
(`Async*`) opcodes and `DynamicCall`/vtables, which report
`[lab] error: MIR interpreter: ...` instead of executing; extend
`MIRInterpreter` (and `MIRTests`) when you need them.

The AST layer only folds expressions at symres time (`comptime if` conditions,
array lengths, `const` initializers) and evaluates compiler intrinsics. Pointer
bounds are still simulated there (`PointerValue` tracks `behind`/`ahead`), so a
comptime pointer that walks outside its allocation fails to fold rather than
producing a value.

## TLS / HTTP Integration Patterns

### TLS Library (`lang/libs/tls/`)

The TLS library is a port of mbedTLS to Chemical. Key APIs:
- `tls::ssl_init(ssl_ptr)` — initialize an SSLContext (must be called after allocation)
- `tls::ssl_config_init(endpoint_type)` — create SSLConfig (returns by value)
- `tls::ssl_set_config(ssl_ptr, &raw mut config)` — attach config to context
- `tls::ssl_set_hostname(ssl_ptr, hostname)` — set SNI hostname (call before handshake)
- `tls::tls_connect(ssl_ptr, host, port)` — dial + handshake in one call
- `tls::ssl_handshake(ssl_ptr)` — perform TLS handshake
- `tls::ssl_read(ssl_ptr, buf, len)` / `tls::ssl_write(ssl_ptr, data, len)` — encrypted I/O
- `tls::ssl_close_notify(ssl_ptr)` — send polite TLS close_notify alert
- `tls::ssl_free(ssl_ptr)` — free handshake params + close socket (sets transport_connected=false)
- `tls::x509_verify_hostname(cert, hostname)` — verify hostname against cert CN
- `tls::load_system_ca_bundle()` — load system CA certs from /etc/ssl/certs/

### Memory Management

For structs **without** a `@constructor` (like `SSLContext`), use `malloc` + `ssl_init`:
```chemical
var ssl_ptr = malloc(sizeof(tls::SSLContext)) as *mut tls::SSLContext
tls::ssl_init(ssl_ptr)
```

Cleanup requires calling `ssl_free` (frees handshake + closes socket) then `unsafe { dealloc }`:
```chemical
tls::ssl_close_notify(ssl_ptr)    // Polite TLS shutdown
tls::ssl_free(ssl_ptr)           // Free resources + close socket
unsafe { dealloc ssl_ptr }        // Free memory
```

> ⚠️ Never mix `new`/`delete` with `malloc`/`unsafe { dealloc }`. `delete` calls the `@delete` destructor which also closes the socket — calling `delete` after `ssl_free` would double-close (though `ssl_free` sets `transport_connected=false` as a safeguard).

### Adding HTTPS Support to HTTP

To make an HTTP library support `https://`, follow this pattern:

1. **Add `tls_ctx: *mut tls::SSLContext` field** to `Body` and `ResponseWriter` structs (default `null` for plain HTTP)
2. **Dispatch I/O**: In `body_recv`, check `tls_ctx != null` → call `tls::ssl_read` else `net::recv_all`. In writer, use a `send_data` helper that calls `tls::ssl_write` or `net::send_all`.
3. **Parser**: Add an optional `tls_ctx: *mut tls::SSLContext = null` parameter to response reader; pass it to the Body.
4. **Client**: In `request()`, detect `https://` scheme → malloc + ssl_init → set_hostname → tls_connect → verify hostname → ssl_write → read with tls_ctx. Body destructor handles cleanup (close_notify → ssl_free → dealloc).
5. **Error paths**: On failure, `ssl_free + unsafe { dealloc }` for TLS, `close_socket` for plain.

### Namespace Access

Without `using namespace http;`, you must fully qualify nested namespaces:
```chemical
var srv = http::server::Server(cfg)          // NOT server::Server
var u = http::URL::parse(url_str)            // NOT URL::parse
var client = http::Client()                  // NOT Client
```

With `using namespace http;` (as in `net_test.ch`), unqualified names work.

## Compiler API Gotchas

### CRITICAL: Enum Sync Rule for CBI Plugins

**When adding a new enum value to a C++ enum that is exposed to CBI (compiler plugins), you MUST also add the same value to the corresponding Chemical binding file.** Failing to do so causes a SIGSEGV crash in all CBI plugins because the enum values are off by 1 between the C++ side and the TCC-compiled plugin side.

| C++ Enum File | Chemical Binding File |
|---------------|----------------------|
| `ast/base/ASTNodeKind.h` | `lang/libs/compiler/src/ast/base/ASTNodeKind.ch` |
| `lexer/TokenType.h` | `lang/libs/compiler/src/ChemicalTokenType.ch` |

Values inserted in the **middle** of an enum break all subsequent values. Values at the **end** can be added safely. Always verify both files have identical ordering.

**Why this happens**: CBI plugins are compiled by TCC from Chemical source. They import the `compiler` library, which contains `.ch` files defining enum values that mirror the C++ enums. The TCC-compiled plugin code uses these Chemical enum values in switch statements and comparisons. If the Chemical enum is missing a value that was added to the C++ enum, every value after the insertion point is shifted by 1, causing wrong branches and crashes.

**Real bug examples**:
- Adding `InlineAsmStmt` to `ASTNodeKind` in C++ without adding it to `ASTNodeKind.ch` → all `getKind()` comparisons off by 1 → SIGSEGV in `ASTNodegetKind`
- Adding `AsmKw` to `TokenType` in C++ without adding it to `ChemicalTokenType.ch` → `RBrace` off by 1 → all `#html` macro parsing fails

## LLVM Backend Gotchas

### External declarations must not use `dso_local`

When a global variable is declared in another module (`submod_extern_globe_var`), do **not** set `dso_local` on the LLVM global value. The linker will reject `dso_local` + external declarations as mismatched. Relevant: `ast/statements/VarInit.cpp`, `compiler/CodegenMir.cpp` (`mir_function` / `mir_declare_globals`).

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

### Uninitialized Variables and the `unsafe` Marker

A variable may be declared **without an initializer** using plain `var x : Type` (or
`const x : Type`). No `unsafe` keyword is written on the declaration. The compiler performs
**definite-assignment analysis**: such a variable is *uninitialized* until it is first
assigned, and accessing it before that is an error unless the access is wrapped in `unsafe`.

> The old `unsafe var` / `unsafe const` declaration syntax was **removed**. Writing
> `unsafe var x : Type` is now a hard parser error.

```chemical
// OK — has an initializer
var x : int = 0
var buf : [256]char = zeroed

// OK — uninitialized, declared without `unsafe`
var cmd : char[2048]
var s : [32]u16
```

**Accessing an uninitialized variable is an error** (the compiler would otherwise try to
read or destruct garbage memory). Wrap the access in `unsafe(...)` (for an expression) or
`unsafe { }` (for a block):

```chemical
var s : [32]u16
// ERROR: taking a pointer to an uninitialized variable
var p = &raw mut s
// FIX:
var p = unsafe(&raw mut s)

var c : Container          // Container has a destructor
// ERROR: writing a field of an uninitialized (destructor-bearing) struct
c.field = 5
// FIX:
unsafe { c.field = 5 }
```

Rules enforced by the definite-assignment pass (merged into `compiler/typeverify/TypeVerifier`):
- A **full assignment** `x = value` (whole variable, not a member/index) is treated as
  *first initialization*. It initializes the variable and the code generators / interpreter
  correctly skip destroying the previous (garbage) value.
- **Reading** an uninitialized variable whose type has a destructor → error
  (`use of uninitialized variable 'x' before it is initialized (use of)`).
- **Taking its address** `&raw mut x` or **reference** `&mut x` → these are **not flagged**
  by the definite-assignment pass (they are legitimate C interop patterns for taking addresses
  of uninitialized memory).
- **Writing a member or index** `x.field = ...` / `x[i] = ...` on an uninitialized
  *destructor-bearing* type → also **marks the variable as initialized** (supporting manual
  struct initialization patterns like `var c : Container; unsafe { c.field = 5 }`).
- Branches: a variable is only "definitely initialized" if **every** path assigns it (e.g.
  both sides of an `if/else`).

`unsafe(expr)` is a **compile-time-only safety marker**. It does **not** change runtime
behaviour — wrapping a value in `unsafe(...)` never suppresses or alters its destructor, and
a `return unsafe(x)` is still a normal move (the moved-from local is not destroyed; the
caller's copy is). Use it only to assert "I am intentionally touching uninitialized memory".

Exemptions:
- Global `@extern` declarations are exempt (defined elsewhere).
- A `var`/`const` declared **inside an `unsafe { }` block** is exempt.
- **Struct member declarations are exempt** — fields inside a `struct`/`class`/`variant` body
  must never carry `unsafe` (it is invalid there).

Compiler errors you may see:
```
[TypeVerify] error: use of uninitialized variable 'x' before it is initialized (use of)
[TypeVerify] error: use of uninitialized variable 'x' before it is initialized (taking address of)
```

When migrating or writing code, do **not** add `unsafe` to the declaration. Instead, either
give the variable an initializer, or wrap the specific *access* of the uninitialized value in
`unsafe(...)`. When fixing a whole suite, prefer the `edit` tool for individual statements;
only a generated helper script may bulk-apply changes, and that script MUST skip `var` inside
struct/variant/enum bodies (struct fields are exempt).

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

Library tests go in `lang/tests/libs/<name>/` (standalone `chemical.mod` suite, run with `./scripts/test.sh --tcc --libs`), **not** `lang/tests/compiler_plugins/` and not `lang/tests/src/` — the `lang/tests/src/` module is the main test executable's source tree and should only contain tests for essential language/stdlib features.

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

## Comptime & Interpretation Internals

| Component | File | Purpose |
|-----------|------|---------|
| `MIRInterpreter` | `compiler/mir/MIRInterpreter.{h,cpp}` | Single-threaded, AST-free engine over a populated `MIRModule`; `prepare()` builds dense side tables (types, field offsets, symbols, constants, globals); API is `run` / `call` / `call_sret` |
| `MIRComptime` | `compiler/mir/MIRComptime.{h,cpp}` | Lowering-time comptime driver: lowers a `comptime func` (and everything it reaches) to MIR, runs it, bridges the result back to an AST `Value*`; declines what the engine does not reproduce |
| `MIRInterpProgram` | `compiler/mir/MIRInterpProgram.h` | The shared MIR program/module an interpretation job lowers into |
| `Value::evaluated_value` | `ast/base/Value.cpp`, `ast/values/*.cpp` | Residual **expression** folding used at symres time only |
| `InterpretScope` | `ast/base/InterpretScope.h` | Value map for parameter binding plus arithmetic folding (`evaluate`); no statements |
| `PointerValue` | `ast/values/PointerValue.{h,cpp}` | Comptime pointer with `behind`/`ahead` bounds |
| `GlobalInterpretScope` | `ast/base/GlobalInterpretScope.h` | Per-job global state: `typeBuilder`, `target_data`, `backend_context`, diagnoser |
| Compiler intrinsics | `ast/utils/GlobalFunctions.cpp` | `FunctionDeclaration::call` overrides that produce `intrinsics::*` values |

`FunctionDeclaration::call` binds `self`/params into an `InterpretScope` and
folds a body that is a single `return <expr>` (or a bare expression); any other
body is reported, because real comptime functions are lowered and run by
`MIRComptime`. `LambdaFunction::call` likewise only reports that lambda bodies
are evaluated by the engine.

### Test Suite Entry Points

| Mode | Entry Function | Module | File |
|------|---------------|--------|------|
| Interpret | `main()` | `interpret_tests` | `lang/tests/interpret/src/main.ch` |
| Native compiled | `main()` | `main` | `lang/tests/src/tests.ch` |
| Common (shared) | `run_common_tests()` | `common_tests` | `lang/tests/common/src/main.ch` |
| Native-common | `run_native_common_tests()` | `native_common_tests` | `lang/tests/native_common/src/main.ch` |

### Debugging Interpretation / Comptime Failures

1. `./scripts/test.sh --tcc --interpret` — the interpretation suite (`-bt` adds a
   GDB backtrace).
2. `./cmake-build-debug/MIRTests` — engine smoke tests; extend them with any new
   opcode.
3. `[lab] error: MIR interpreter: <message>` — the engine declined or trapped;
   the message names the function/opcode.
4. `MIRDump` (`compiler/mir/MIRDump.{h,cpp}`) prints the lowered MIR — the
   fastest way to see what the engine is asked to run.
5. A comptime diagnostic at a call site means `MIRComptime` declined and the AST
   folder could not fold (usually a body with statements or an unsupported
   value kind).

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

### Isolating Tests That Depend on Libraries

Many test failures involve library code (TLS, audio, HTTP, etc.) that a bare `temp.ch` cannot import.
For those, create a **standalone module with a `chemical.mod`** that imports the required
dependencies:

```bash
mkdir -p lang/compiled/my_test/src
```

**`lang/compiled/my_test/chemical.mod`** — use `application` with imports:
```
application my_test
source "src"
import cstd
import std
import audio                    # add whatever libs the test needs
import tls
import net
```

**`lang/compiled/my_test/src/main.ch`** — your test code:
```
public func main() : int {
    // ... test code that uses audio, tls, etc. ...
    return 0
}
```

**Compile with the LLVM backend:**
```bash
cmake-build-debug/Compiler "lang/compiled/my_test/chemical.mod" \
    -o "lang/compiled/my_test/main.exe" \
    --mode debug_complete --assertions
```

The Compiler resolves imports from `lang/libs/` automatically. No special flags needed.

**Run:**
```bash
./lang/compiled/my_test/main.exe
```

**Inspect LLVM IR (add `--out-ll-all`):**
```bash
cmake-build-debug/Compiler "lang/compiled/my_test/chemical.mod" \
    -o "lang/compiled/my_test/main.exe" \
    --mode debug_complete --assertions \
    --out-ll-all --build-dir "lang/compiled/my_test/build/"

# IR is at: lang/compiled/my_test/build/modules/<name>/llvm_ir.ll
```

**Get a backtrace on crash:**
```bash
gdb -batch -ex run -ex bt -ex "info registers" -ex "x/16i \$pc" \
    ./lang/compiled/my_test/main.exe
```

This approach gives you:
- Full library access (no manual code copying)
- Fast iteration (no full test suite rebuild)
- Focused LLVM IR for just your module
- GDB backtraces for SIGSEGV crashes in library-dependent tests

## Isolating Library-Dependent Failures (Standalone Module)

When a test failure depends on library code (TLS, audio, etc.), see the
[Debugging: Isolating a Single Test Case](#debugging-isolating-a-single-test-case)
section above under **Isolating Tests That Depend on Libraries**.

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

**Critical rule:** You CANNOT split a `#html` block across multiple blocks. An element opened in one `#html` block must be closed in the same block.

### `@{}` Escape Syntax for Dynamic Content

Use `@{...}` to escape from JSX/HTML mode back to Chemical code inside `#html` blocks. Inside the escape, you can write any Chemical statement (loops, conditionals, variable declarations). Use nested `#html { }` blocks inside the escape to emit HTML elements.

```chemical
// Looping over data to generate HTML elements
var items = // ... array of data from DB
var idx : size_t = 0
var count = items.size()

#html {
    <div class="grid-3">
        @{while(idx < count) {
            var item = items.get_ptr(idx)
            var name = // extract field
            var id = // extract field
            var link = std::string("/detail/")
            link.append_string(&int_to_string(id))
            idx = idx + 1
            #html {
                <Card><CardBody>
                    <CardTitle>{name}</CardTitle>
                    <Link href={link}>View</Link>
                </CardBody></Card>
            }
        }}
    </div>
}
```

```chemical
// Conditional rendering
#html {
    <div>
        @{if(has_data) {
            <div class="grid-3">...</div>
        } @else {
            <div>No data available</div>
        }}
    </div>
}
```

**Key rules:**
- Always use `@{}` to write Chemical logic inside `#html` blocks
- The index variable must be incremented BEFORE the nested `#html { }` block
- Each `@{}` block must be self-contained

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
