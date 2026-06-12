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

CI equivalent (runs from release binary, no `--no-cache`):
```bash
./chemical lang/tests/build.lab -arg-minimal -bm -v --assertions --mode debug_complete
```

Lib tests CI:
```bash
./chemical lang/tests/build.lab -arg-test-libs -bm -v --assertions --mode debug_complete --no-cache
```

## Test structure

- `lang/tests/src/test.ch` — assertion framework (`test(name, lambda)`, `assertEquals`).
- `lang/tests/src/tests.ch` — `run_executable_tests()` lists inline tests.
- **Inline tests**: manually listed in `tests.ch` via `test(name, () => bool)`.
- **`@test` annotations**: auto-discovered by `test_runner(argc, argv)` from `test_env` lib.
- Source dirs: `basic/`, `comptime/`, `core/`, `generic/`, `libs/`, `nodes/`, `stdlib/`.
- Lib tests in `lang/tests/libs/*/src/`.

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
| `preprocess/` | C translation and visitors |
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

## Release

- Commit message must contain "releaseIt" to trigger CI release builds.
- Release workflow produces: `{platform}-{arch}.zip`, `{platform}-{arch}-tcc.zip`, `{platform}-{arch}-lsp.zip`.
