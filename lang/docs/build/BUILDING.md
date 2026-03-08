# Building Chemical

This guide explains how to build the Chemical Compiler for various platforms and setups.

## Prerequisites

Before building, you must run the configuration script to download the minimal required dependencies (like `libtcc`).

```bash
# Downloads libtcc for your platform
./scripts/configure.sh
```

## Common Scenarios

Depending on your environment and what you want to build, choose the appropriate scenario below.

### 1. CLion User (Without LLVM) - Building TCCCompiler Only

If you only want to build the `TCCCompiler` target (a lightweight compiler based on TinyCC) and do not want to download the massive LLVM toolchains:

1. Run `./scripts/configure.sh` to download `libtcc`.
2. In CLion, open the CMake settings for the project (`Settings` -> `Build, Execution, Deployment` -> `CMake`).
3. Under **CMake options**, add `-DBUILD_COMPILER=OFF`.
4. Reload the CMake project and build the `TCCCompiler` target.

### 2. CLion User (Windows) - Full Compiler with MinGW

If you want to build the full LLVM-backed `Compiler` using CLion and MinGW on Windows:

1. **Setup MinGW in CLion**: Ensure you have a MinGW toolchain configured in CLion (`Settings` -> `Build, Execution, Deployment` -> `Toolchains`). You **MUST** use the clang-based `llvm-mingw` toolchain, as the project requires it to build correctly. You can download it using the `scripts/mingw-toolchain.sh` script or from GitHub releases.
2. **Download Dependencies**: You need both `libtcc` and the prebuilt MinGW LLVM packages.
   ```bash
   # Download libtcc and prebuilt LLVM for MinGW (defaults to UCRT)
   ./scripts/configure.sh --with-llvm --mingw
   ```
3. Enable the CMake profile in CLion that uses your MinGW toolchain.
4. Reload CMake and build the `Compiler` target.

### 3. Visual Studio User (Windows)

If you are using Visual Studio's MSVC toolchain:

#### Option A: Full Compiler (Requires prebuilt LLVM)
1. **Download Dependencies**: Fetch `libtcc` and the prebuilt MSVC LLVM packages.
   ```bash
   # Download libtcc and prebuilt LLVM for MSVC
   ./scripts/configure.sh --with-llvm
   ```
2. Open the folder in Visual Studio or generate the Visual Studio solution via CMake.
3. CMake should automatically find the downloaded LLVM packages in `out/host`.
4. Build the `Compiler` target.

#### Option B: TCCCompiler Only (No LLVM needed)
If you don't wish to download the massive prebuilt LLVM packages, you can build the lightweight TCC-based compiler:
1. Run `./scripts/configure.sh` to download only `libtcc`.
2. Open your project in Visual Studio, and add `-DBUILD_COMPILER=OFF` to your CMake configuration options.
3. Build the `TCCCompiler` target.

### 4. CLI / Script-Only User (Windows MinGW)

If you prefer operating strictly from the command line using scripts, we provide automated toolchain and build configuration scripts:

1. **Download the MinGW Toolchain**:
   If you don't have a compiler installed, download our recommended `llvm-mingw` toolchain:
   ```bash
   ./scripts/mingw-toolchain.sh
   # Installs to toolchains/llvm-mingw/
   ```

2. **Download Dependencies**:
   ```bash
   # Download libtcc and prebuilt LLVM for MinGW
   ./scripts/configure.sh --with-llvm --mingw
   ```

3. **Configure the Project**:
   Run the MinGW configure script. It will automatically detect the toolchain downloaded in step 1.
   ```bash
   ./scripts/mingw-configure.sh
   ```
   *(Note: You can pass `--tcc-only` if you do not wish to build the full LLVM compiler).*

4. **Build the Project**:
   ```bash
   cmake --build build/mingw --target Compiler
   # Or --target TCCCompiler if you used --tcc-only
   ```

### 5. CLI / Script-Only User (macOS / Linux)

For users on macOS or Linux operating from the command line:

1. **Download Dependencies**:
   ```bash
   # Download libtcc and prebuilt LLVM for your platform
   ./scripts/configure.sh --with-llvm
   ```

2. **Configure the Project**:
   ```bash
   cmake -B build -S .
   # Use -DBUILD_COMPILER=OFF if you only want the TCCCompiler
   ```

3. **Build the Project**:
   ```bash
   cmake --build build --target Compiler
   # Or --target TCCCompiler
   ```
