# Building Chemical

This guide explains how to build the Chemical Compiler for various platforms and setups.

## Quick Start

```bash
# 0. One-time setup after cloning — downloads dependencies & initializes submodules
./scripts/setup.sh                # or ./scripts/setup.sh --with-llvm
# 1. Configure CMake
./scripts/configure.sh

# 2. Build a target
./scripts/build.sh --tcc          # TCCCompiler (fast build, no LLVM)
./scripts/build.sh --llvm         # Compiler with LLVM/Clang
./scripts/build.sh --lsp          # LSP server
./scripts/build.sh --all          # All targets

# 3. Run tests
./scripts/test.sh --tcc           # Build & run tests with TCCCompiler
./scripts/test.sh --llvm          # Build & run tests with Compiler
```

## One-Time Setup

After cloning the repository, run `setup.sh` once to download dependencies and initialize submodules:

```bash
# Downloads libtcc for your platform, initializes git submodules
./scripts/setup.sh

# Also download prebuilt LLVM (required for --llvm builds)
./scripts/setup.sh --with-llvm
```

This fetches:
- **libtcc** (TinyCC shared library) into `lib/tcc/`
- **LLVM/Clang/LLD** (with `--with-llvm`) into `out/host/`
- **Git submodules** (e.g. `lib/lsp-framework`)

## Configuration

After setup, configure CMake:

```bash
# Configure with LLVM support
./scripts/configure.sh

# Skip LLVM setup entirely (for TCC-only builds)
./scripts/configure.sh --no-llvm
```

The `--no-llvm` flag sets `-DBUILD_COMPILER=OFF` in CMake, so only TCCCompiler and ChemicalLsp targets are available.

## Building

### Using build scripts (recommended)

```bash
./scripts/build.sh --tcc           # Build TCCCompiler only
./scripts/build.sh --llvm          # Build Compiler (LLVM/Clang backend)
./scripts/build.sh --lsp           # Build ChemicalLsp
./scripts/build.sh --all           # Build all targets
./scripts/build.sh --llvm -j 16    # Build with 16 parallel jobs
```

### Using make directly

```bash
make -C cmake-build-debug Compiler -j$(nproc)
make -C cmake-build-debug TCCCompiler -j$(nproc)
make -C cmake-build-debug ChemicalLsp -j$(nproc)
```

## Testing

### Using test script (recommended)

```bash
# Build TCCCompiler, compile tests, and run them
./scripts/test.sh --tcc

# Build Compiler (LLVM), compile tests, and run them
./scripts/test.sh --llvm

# Include library tests (html, css, js, etc.)
./scripts/test.sh --tcc --libs

# Specify a custom output path
./scripts/test.sh --tcc -o /tmp/my_tests.exe

# Build test executable only, don't run
./scripts/test.sh --tcc --no-run

# Use an already-built compiler (skip rebuild)
./scripts/test.sh --tcc --no-build
```

> ⚠️ **`--no-build` warning**: This flag **skips rebuilding the C++ compiler binary**.
> Changes to `.cpp`/`.h` files in the compiler source **will NOT be reflected** —
> the previously built binary is used as-is. Only use `--no-build` when iterating on
> `.ch` test files or Chemical library sources **without any compiler C++ changes**.
> To include C++ changes, omit `--no-build` (or rebuild once without it first).

### Manual test commands

Full test suite (LLVM backend):
```bash
cmake-build-debug/Compiler lang/tests/build.lab -o lang/tests/build/tests.exe --mode debug_complete --no-cache
./lang/tests/build/tests.exe
```

TCC backend:
```bash
cmake-build-debug/TCCCompiler lang/tests/build.lab -o lang/tests/build/tests-tcc.exe --mode debug_quick --no-cache
./lang/tests/build/tests-tcc.exe
```

Library tests:
```bash
cmake-build-debug/TCCCompiler lang/tests/build.lab -o lang/tests/build/lib-tests-tcc.exe --mode debug_quick --no-cache --arg-test-libs -frecompile-plugins
```

## Build TUI

For an interactive terminal UI that wraps all of the above scripts, use `scripts/tui.sh`:

```bash
./scripts/tui.sh
```

Controls:
- **Down/Up** or **Tab/Shift+Tab** — navigate between items
- **Space** — toggle boolean options, cycle choice widgets
- **Left/Right** — cycle choice widgets
- **Enter** — run the focused section's command
- **r** — run all sections sequentially
- **s** — save current configuration to a named file
- **l** — load a saved configuration
- **q** — quit (auto-saves last config to `scripts/tui-configs/last.json`)

Headless mode — load a saved config and run all commands without showing the TUI:

```bash
./scripts/tui.sh --run myconfig
```

Configs are stored as JSON in `scripts/tui-configs/`.

## Prerequisites

- 8–16 GB RAM
- C++20 toolchain
- LLVM 22 (for Compiler target, optional)
- CMake 3.15+

### Dependencies

The CMake project expects:
- `libtcc` in `lib/tcc/` (downloaded by `scripts/configure.sh`)
- Prebuilt LLVM/Clang/LLD in `out/host/` (downloaded with `--with-llvm`)

## Common Scenarios

### 1. CLion User (Without LLVM) - Building TCCCompiler Only

If you only want to build the `TCCCompiler` target (a lightweight compiler based on TinyCC) and do not want to download the massive LLVM toolchains:

1. Run `./scripts/configure.sh --no-llvm` to download `libtcc` and set `BUILD_COMPILER=OFF`.
2. In CLion, reload the CMake project (CMake options will already contain `-DBUILD_COMPILER=OFF`).
3. Build the `TCCCompiler` target.

### 2. CLion User (Windows) - Full Compiler with MinGW

If you want to build the full LLVM-backed `Compiler` using CLion and MinGW on Windows:

1. **Setup MinGW in CLion**: Ensure you have a MinGW toolchain configured in CLion (`Settings` -> `Build, Execution, Deployment` -> `Toolchains`). You **MUST** use the clang-based `llvm-mingw` toolchain. Download it via `scripts/mingw-toolchain.sh` or from GitHub releases.
2. **Download Dependencies**:
   ```bash
   ./scripts/configure.sh --with-llvm --mingw
   ```
3. Enable the MinGW CMake profile in CLion and build the `Compiler` target.

### 3. Visual Studio User (Windows)

#### Option A: Full Compiler (Requires prebuilt LLVM)
1. **Download Dependencies**:
   ```bash
   ./scripts/configure.sh --with-llvm
   ```
2. Open the folder in Visual Studio or generate the solution via CMake.
3. Build the `Compiler` target.

#### Option B: TCCCompiler Only (No LLVM)
1. Run `./scripts/configure.sh --no-llvm`.
2. Open in Visual Studio, CMake will have `BUILD_COMPILER=OFF`.
3. Build the `TCCCompiler` target.

### 4. CLI / Script-Only User (Linux / macOS)

```bash
# Configure with LLVM
./scripts/configure.sh --with-llvm

# Build Compiler
./scripts/build.sh --llvm

# Build & run tests
./scripts/test.sh --llvm
```

For TCC-only:
```bash
./scripts/configure.sh --no-llvm
./scripts/build.sh --tcc
./scripts/test.sh --tcc
```
