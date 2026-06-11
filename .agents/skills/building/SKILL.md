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

### Manual commands

```bash
# TCC backend
cmake-build-debug/TCCCompiler "lang/tests/build.lab" -o lang/tests/build/tests-tcc.exe --mode debug_quick --no-cache

# LLVM backend
cmake-build-debug/Compiler "lang/tests/build.lab" -o lang/tests/build/tests.exe --mode debug_complete --no-cache
```

### Compiler flags explained

- `--mode debug_quick` — quickly compile the project with debug info
- `--mode debug_complete` — full debug mode for LLVM backend
- `--no-cache` — do not rely on previously generated objects
- `--emit-c` — write the Translated.c file to the build directory
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
