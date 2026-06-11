# AGENTS.md — Chemical Programming Language

## Configuration

```bash
./scripts/configure.sh            # Configure with LLVM support
./scripts/configure.sh --no-llvm  # Configure without LLVM (TCCCompiler only)
```

## Build (Compiler in C++20, Makefile in `cmake-build-debug`)

### Using scripts (recommended)

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

- The Makefile is at `cmake-build-debug/Makefile` (configured by CLion).
- `cmake` is at `/opt/clion/bin/cmake/linux/x64/bin/cmake` (not in PATH); do **not** run `cmake ..` manually.
- Both `Compiler` (LLVM/Clang backend) and `TCCCompiler` (TinyCC) link dynamically with TinyCC (`libtcc.so`).
- `Compiler` additionally links with LLVM + Clang + LLD to emit executables.
- LLVM version 22; prebuilt LLVM assets from `chemicallang/llvm-prebuilt`.
- `configure.sh` downloads `libtcc` (and optionally LLVM with `--with-llvm`).
- macOS: no static linking of standard libraries.
- LSP depends on `lib/lsp-framework` submodule.

## Running Tests

### Using test script (recommended)

```bash
./scripts/test.sh --tcc           # Build TCCCompiler, compile & run tests
./scripts/test.sh --llvm          # Build Compiler, compile & run tests
./scripts/test.sh --tcc --libs    # Include library tests
./scripts/test.sh --tcc -o my_tests  # Custom output path
./scripts/test.sh --tcc --no-run  # Build test executable only, don't run
./scripts/test.sh --tcc --no-build  # Use existing compiler binary
```

### Full test suite (LLVM backend, development):
```bash
./compiler_bin lang/tests/build.lab -arg-minimal -bm -v --assertions --mode debug_complete --no-cache
```
- `compiler_bin` is whichever `Compiler`/`TCCCompiler` executable you built.
- Output: `lang/tests/build/tests.exe` (runs inline tests + `@test` annotation tests).
- If LLVM code changes, clear IR cache: `rm lang/tests/build/chemical-tests.dir/modules/main/llvm_ir.ll`

### CI test command:
```bash
./chemical lang/tests/build.lab -arg-minimal -bm -v --assertions --mode debug_complete
```
(CI releases strip `--no-cache` and accepts cached results.)

### Lib tests (html, css, js, md, react, preact, solid, universal):
```bash
./chemical lang/tests/build.lab -arg-test-libs -bm -v --assertions --mode debug_complete --no-cache
```
Or individual: `-arg-test-html`, `-arg-test-css`, `-arg-test-js`, etc.

## Test Structure

- `lang/tests/src/test.ch` — lightweight assertion framework (`test(name, lambda)`, `assertEquals`).
- `lang/tests/src/tests.ch` — `run_executable_tests()` calls all inline test functions.
- **Inline tests**: manually listed in `tests.ch`, use `test(name, () => bool)`.
- **`@test` annotation tests**: auto-discovered by `test_runner(argc, argv)` from `test_env` lib.
- Test source dirs: `basic/`, `comptime/`, `core/`, `generic/`, `libs/`, `nodes/`, `stdlib/`.
- Lib tests live in `lang/tests/libs/*/src/`.

## Architecture

### Key directories
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
| `preprocess/` | C translation and visitors |
| `core/main/CompilerMain.cpp` | CLI entrypoint |
| `server/` | LSP server implementation |
| `lang/libs/` | Standard library + CBI macro plugin libs |
| `lang/tests/` | Test suite |

### Compilation flow
1. CLI → `LabBuildCompiler` → translates `chemical.mod` → `build.lab` (in memory)
2. `build.lab` is compiled by TinyCC (JIT), produces `LabJob` objects
3. Jobs executed: parse → symbol resolve → type check → codegen → link
4. AST uses arena allocators (`ASTAllocator`) — batch-allocated, no per-node `delete`

### Key conventions
- Custom strings: `chem::string` (SSO, no `std::string`).
- Name mangling in `compiler/mangler/NameMangler.cpp` — scoped symbols get `scope_name` prefix, generics get `__cgs__N`/`__cfg__N` suffixes.
- Operator overloading: `a + b` → looks for `add(a, b)` method.
- `#macro` and `#universal` are CBI plugins compiled by TinyCC at build time.
- Commit trigger for releases: message contains "releaseIt".

## Testing Gotchas

- `--no-cache` forces recompilation; omit for CI (uses timestamp caching).
- `--assertions` enables runtime assertion checks in generated code.
- `--mode debug_complete` is for LLVM; TCC uses `--mode debug`.
- `-bm` enables benchmark output.
- `-v` is verbose, `-vl` is verbose link (link-time verbosity).
