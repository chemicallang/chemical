---
name: Building
description: All documentation related to building and running Project or Tests
---

# Building

Its likely user is using CLion, in this case in the root dir, you'll find `cmake-build-debug` where there are two
compilers present
 - `TCCCompiler.exe` (the compiler that embeds Tiny CC)
   - This compiler cannot create llvm ir, It translates Chemical to C and then compiles that using Tiny CC
   - This compiler is faster to build, faster at compilation but generates slower code.
 - `Compiler.exe` (the compiler that embeds LLVM/Clang and Tiny CC)
   - This compiler can do anything TCCCompiler can do and more, It performs optimizations using LLVM. But you
   - can also tell it to use Tiny CC with flag `--use-tcc` or ask it to translate to C and then compile that using
   - clang with flag `--use-c`
   - This compiler takes time to build, slow at compilation but generates fast code

It's likely that you don't need to rebuild the compiler, because you are working on libraries (`lang/libs/<library>` or `lang/compiled/<library>`).
Linux binaries don't have `.exe` extension.
To properly understand building of the compilers, You should analyze the `CMakeLists.txt` in the root of the repo.

If you are working on any code that doesn't interact with LLVM/Clang then you should not compile Compiler target. you should focus
on TCCCompiler which would build faster and would compile faster.

## Building Compiler

There are three targets in `CMakeLists.txt`, Your job is to configure the project. Prefer Visual Studio toolchain on Windows.

Analyzing the `CMakeLists.txt` would help, do not build all the targets, Build one target at most.

If you are having trouble finding a toolchain, You should inform the user. Then also analyze the scripts in scripts directory in root of the repo.

You should look at `PATH` or other environment variables to find where Visual Studio toolchain is located and whats the version.

Please note that `out/host` contains LLVM/Clang Libraries, Do not overwrite these with any build configuration you choose.

If user doesn't have LLVM, You'll need `-DBUILD_COMPILER=OFF`

If building for the first time, read `lang/docs/build/BUILDING.md`

## Building Tests

The command used to build tests using the compiler is 
```bash
cmake-build-debug/TCCCompiler.exe "lang/tests/build.lab" -o lang/tests/build/tests-tcc.exe --mode debug_quick --no-cache --emit-c
```

The `--mode debug_quick` means quickly compile the project, It will include debug information for the translated C
The `--no-cache` means do not rely on previously generated objects, even if files haven't changed.
The `--emit-c` means write the `Translated.c` file to the build directory

After compiling the tests, You should run the binary `lang/tests/build/tests-tcc.exe`

You can change the build directory using `--build-dir my_build_dir`, If you don't provide an explicit build directory, `build`
directory would created right next to the `build.lab` (sibling), exactly named `build`.

If you would like to test the LLVM/Clang based compiler, run the tests using the other executable

```bash
cmake-build-debug/Compiler.exe "lang/tests/build.lab" -o lang/tests/build/tests.exe --mode debug_quick --no-cache --emit-c
```

## Building Library Tests

We test most libraries in the tests above (`lang/tests/build.lab`) But some libraries like 
- `html_cbi`
- `css_cbi`
- `js_cbi`
- `components`
- `react_cbi`
- `preact_cbi`
- `solid_cbi`
- `universal_cbi`

These libraries are basically compiler plugins, For these we generate a separate executable, Because building these takes
time, we don't want to take time building language tests.

```bash
cmake-build-debug/TCCCompiler.exe "lang/tests/build.lab" -o lang/tests/build/lib-tests-tcc.exe --mode debug_quick --no-cache --arg-test-libs -frecompile-plugins --emit-c
```

- `--arg-test-libs` means pass `test-libs` as an argument to `build.lab` file, which then builds a lib tests executable 
- `-frecompile-plugins` means recompile compiler plugins like `html_cbi`, without this it won't build, it is basically `--no-cache` for plugins

Very helpful to know

- `--plugin-mode debug_complete` means compile the plugins in debug mode, this embeds full debug info and therefore helpful if a crash happening in plugin code

If you have just modified a compiler plugin, If it crashes, you should compile it in plugin mode to see the stack trace.

