---
name: CLI Entrypoint and Targets
description: The Chemical compiler command-line entrypoints (Compiler, TCCCompiler, ChemicalLsp) — argument parsing, LabBuildCompiler wiring, the full CLI flag surface, OutputMode, output-artifact routing, embedded Clang/LLVM/LLD/TinyCC invocation, SelfInvocation, target triples (TargetData), and ModuleOptionRegistry. Load when working on the compiler CLI, adding or changing flags, output modes, target-triple handling, or embedded-tool invocation.
---

# CLI Entrypoint and Targets

This skill documents how the three compiler executables start up, parse their
command line, and route work into the `LabBuildCompiler`. It also covers target
triples (`TargetData`), the module option registry, and how the compiler
re-invokes itself and its bundled Clang/LLVM tools.

For the deeper build-pipeline internals, see `.agents/skills/build_system/SKILL.md`.

## 1. Overview: Three Binaries

All three executables are produced from the root `CMakeLists.txt` and share
`COMMON_SOURCES`:

| Binary | `main()` source | Build define | Extras linked | Backends |
|--------|-----------------|--------------|---------------|----------|
| `Compiler` | `core/targets/Compiler.cpp` | `COMPILER_BUILD CLANG_LIBS LLD_LIBS` | LLVM, Clang (driver+cc1+cc1as), LLD, TinyCC | LLVM IR codegen (`process_job_gen`) **or** C translation + embedded Clang (`--use-c`) **or** C translation + libtcc (`--use-tcc`) |
| `TCCCompiler` | `core/targets/Compiler.cpp` | `TCC_BUILD` | TinyCC (`libtcc.so`) only | C translation + libtcc (always) |
| `ChemicalLsp` | `core/targets/LSPMain.cpp` | `LSP_BUILD` | TinyCC + `lib/lsp-framework` | No codegen of its own; hosts the LSP server and can shell out to `compiler_main`/`compile_lab` |

Key facts:

- `Compiler` and `TCCCompiler` share the exact same `main()`:
  `core/targets/Compiler.cpp:6` calls `compiler_main(argc, argv)`.
- The differences are all compile-time: `Compiler` is built with
  `COMPILER_BUILD CLANG_LIBS LLD_LIBS` (`CMakeLists.txt:761`) and adds
  `${COMPILER_SOURCES}` (`CMakeLists.txt:753`); `TCCCompiler` is built with
  `TCC_BUILD` (`CMakeLists.txt:757,765`) and uses only `${COMMON_SOURCES}`.
- `ChemicalLsp` has its **own** `main()` in `core/targets/LSPMain.cpp:359`; it
  does not call `compiler_main` except for the `cc` subcommand
  (`core/targets/LSPMain.cpp:416-427`).
- Default backend selection is a compile-time default too: on `COMPILER_BUILD`,
  `LabBuildCompilerOptions::use_c=false` and `use_tcc=false`
  (`compiler/lab/LabBuildCompilerOptions.h:30-43`); on `TCC_BUILD` both are
  `true`, i.e. `TCCCompiler` always translates to C and JITs with libtcc.

## 2. Key Files

| File | Role |
|------|------|
| `core/main/CompilerMain.cpp` | The single `compiler_main()` entrypoint: option registration/parsing, subcommand dispatch, `.lab`/`.mod`/single-file routing |
| `core/main/CompilerMain.h` | Declares `int compiler_main(int argc, char *argv[])` |
| `core/targets/Compiler.cpp` | `main()` for both `Compiler` and `TCCCompiler` |
| `core/targets/LSPMain.cpp` | `main()` for `ChemicalLsp` (LSP server, builds, tests) |
| `utils/CmdUtils.h` | `CmdOption` / `CmdOptions` argument parser used by the CLI |
| `compiler/lab/LabBuildCompiler.h/.cpp` | Executes jobs, links, interprets, self-invokes tools |
| `compiler/lab/LabBuildCompilerOptions.h` | All build options, inherits `ASTProcessorOptions` |
| `compiler/lab/LabBuildContext.cpp` | `initialize_job()`, `set_job_mode()`, module creation |
| `compiler/lab/LabJobType.h` | Job kinds (Executable, Library, ToCTranslation, CBI, Interpretation, …) |
| `compiler/OutputMode.h` | `OutputMode` enum + `to_string`/`configure_emitter_opts` |
| `compiler/CodegenEmitterOptions.h` | Per-emit flags (assertions, lto, small, debug_ir, paths) |
| `compiler/CodegenOptions.h` | `fno_unwind_tables`, `fno_asynchronous_unwind_tables`, `no_pie` |
| `compiler/ASTProcessorOptions.h` | Shared processing options (target triple, exe path, debug, benchmark) |
| `compiler/lab/TargetData.h` | `TargetData` struct + `create_target_data()` / `create_jit_target_data()` |
| `ast/utils/TargetAPI.h` | `prepare_target_data(TargetData&, const std::string&)` |
| `ast/utils/GlobalFunctions.cpp` | `init_target_data` / `prepare_target_data` implementations |
| `compiler/lab/TargetConditionAPI.h` | `is_condition_enabled` / `resolve_target_condition` |
| `compiler/lab/ModuleOptionRegistry.h/.cpp` | Module-level (`chemical.mod`) option descriptors |
| `compiler/lab/LabModuleOptions.h` | C++ mirror of module options exposed to Chemical |
| `compiler/SelfInvocation.h/.cpp` | Out-of-process self-invocation for system header discovery |
| `compiler/InvokeUtils.h` | Link/compile helper signatures (`clang_link_objects`, `lld_link_objects`, …) |
| `compiler/Codegen.cpp` | Embedded clang/lld/tcc link helpers and `chemical_clang_main2`/`llvm_ar_main2` |
| `compiler/clang_driver.cpp` | Embedded Clang driver (`clang_main`, `chemical_clang_main`) |
| `compiler/clang_cc1main.cpp` | Embedded `-cc1` frontend (`cc1_main`) |
| `compiler/clang_cc1as_main.cpp` | Embedded `-cc1as` assembler (`cc1as_main`) |
| `compiler/llvm-ar.cpp` | Embedded `ar`/`ranlib`/`lib`/`dlltool` (`ChemLlvmAr_main`) |

## 3. `CompilerMain` Flow

`compiler_main()` (`core/main/CompilerMain.cpp:339`) runs this sequence:

1. **`-cc1` fast path** (COMPILER_BUILD only) — `CompilerMain.cpp:351-356`. If
   `argv[1] == "-cc1"`, call `chemical_clang_main(argc, argv)` and return. This
   is what Clang's driver invokes when it spawns itself.
2. **Register + parse options** — `CompilerMain.cpp:358-427`. A
   `CmdOption cmd_data[]` array is registered, then
   `options.parse_cmd_options(argc, argv, 1)` (skip `argv[0]`). Positional
   arguments land in `options.arguments`.
3. **`configure` subcommand** — `CompilerMain.cpp:429-433` → `configure_exe()`
   (`CompilerMain.cpp:143`) which requests sudo/admin, sets `CHEMICAL_HOME`, and
   adds the compiler dir to `PATH`.
4. **`--version`** — `CompilerMain.cpp:435-438`.
5. **`tcc-jit` subcommand** — `CompilerMain.cpp:440-450` →
   `LabBuildCompiler::tcc_run_invocation(...)`. Available on **both** binaries.
6. **LLVM tools** (`dlltool`, `ranlib`, `lib`, `ar`) — `CompilerMain.cpp:452-472`
   (COMPILER_BUILD only) → `llvm_ar_main2(...)`.
7. **`cc` subcommand** — `CompilerMain.cpp:474-488` (COMPILER_BUILD only) →
   `chemical_clang_main2(subc)` after prepending the compiler's own executable
   path.
8. **`prepare_options` lambda** — `CompilerMain.cpp:507-585`. Maps parsed flags
   to `LabBuildCompilerOptions`. Sanitizer parsing happens here.
9. **`--help`** — `CompilerMain.cpp:587-590` → `print_help()`
   (`CompilerMain.cpp:46`).
10. **no input** — `CompilerMain.cpp:596-600` → usage + exit 1.
11. **target + bitness** — `CompilerMain.cpp:608-639`.
12. **`run` subcommand** — `CompilerMain.cpp:641-667` →
    `LabBuildCompiler::run_invocation(...)` (`LabBuildCompiler.cpp:3534`).
13. **`.mod` → `.lab` translation** — `CompilerMain.cpp:696-699` →
    `LabBuildCompiler::translate_mod_file_to_lab(...)`.
14. **`.lab` / `.mod` build** — `CompilerMain.cpp:700-782`. Creates a
    `LabBuildCompilerOptions`, `CompilerBinder`, `LocationManager`, and
    `LabBuildCompiler`. `.lab` → `compiler.build_lab_file(...)`; `.mod` →
    produces a `LabJob` (default `Executable`) and calls
    `compiler.build_mod_file(...)`. `--out *.c` translates the build script
    itself to C.
15. **Single-file compilation** (`.ch`/`.c`/`.o` inputs) —
    `CompilerMain.cpp:784-927`. Builds a `LabModule(LabModuleType::Files, ...)`,
    sets `--out-ll/-bc/-obj/-asm` paths on the module, derives the artifact from
    `-o` extension, builds a `LabJob`, then
    `compiler.do_job_allocating(&job)` and removes any temporary object file.

`prepare_options` is the authoritative CLI→options mapping; cite
`core/main/CompilerMain.cpp:507-585`.

## 4. Full CLI Flag Reference

Registered in the `cmd_data[]` array at `core/main/CompilerMain.cpp:360-424`.
Unless noted, every flag is available on both binaries. "Type" is the
`CmdOptionType` (NoValue / SingleValue / MultiValued / SubCommand).

| Flag | Alias | Type | Meaning | Source |
|------|-------|------|---------|--------|
| `--include` | — | MultiValued | Add include path; `.ch` paths become module sources, others become C headers | CompilerMain.cpp:361,218-227 |
| `--build-dir` | `-b` (read only) | SingleValue | Build output directory (read back with alias `b`) | CompilerMain.cpp:362,692 |
| `--library` | `-l` | MultiValued | Link a system library | CompilerMain.cpp:363,898-901 |
| `cc` | — | SubCommand | Forward remaining args to embedded Clang | CompilerMain.cpp:364,474-488 |
| `configure` | — | SubCommand | Configure compiler for this OS (`CHEMICAL_HOME`, PATH) | CompilerMain.cpp:365,429-433 |
| `linker` | — | SubCommand | Forward to system linker | CompilerMain.cpp:366 |
| `tcc-jit` | — | SubCommand | Run objects via TinyCC JIT (`tcc_run`) | CompilerMain.cpp:367,440-450 |
| `ar` | — | SubCommand | Forward to embedded LLVM `ar` (COMPILER_BUILD) | CompilerMain.cpp:368,452-472 |
| `dlltool` | — | SubCommand | Forward to embedded LLVM `dlltool` (COMPILER_BUILD) | CompilerMain.cpp:369,452-472 |
| `ranlib` | — | SubCommand | Forward to embedded LLVM `ranlib` (COMPILER_BUILD) | CompilerMain.cpp:370,452-472 |
| `lib` | — | SubCommand | Forward to embedded LLVM `lib` (COMPILER_BUILD) | CompilerMain.cpp:371,452-472 |
| `run` | — | SubCommand | `chemical run <file.mod>` or `<org/repo>`; extra args passed to program | CompilerMain.cpp:372,592-667 |
| `--mode` | `-m` | SingleValue | Output mode (see §5) | CompilerMain.cpp:373,274-313 |
| `--plugin-mode` | `-pm` | SingleValue | Mode used to compile CBI plugins | CompilerMain.cpp:374,537-541 |
| `--version` | — | NoValue | Print version and exit | CompilerMain.cpp:375,435-438 |
| `--help` | — | NoValue | Print help and exit | CompilerMain.cpp:376,587-590 |
| `--minify-c` | `--minify-c` | NoValue | Minify generated C | CompilerMain.cpp:377,513 |
| `--emit-c` | `--emit-c` | NoValue | Keep/write generated C to the build dir | CompilerMain.cpp:378,514 |
| `--incremental` | `--incremental` | NoValue | Per-file C translation (`translate_to_single_file=false`) | CompilerMain.cpp:379,515-517 |
| `--keepc` | `--keepc` | NoValue | Keep C files (same as `--emit-c`) | CompilerMain.cpp:380,514 |
| `--test` | `--test` | NoValue | Set `is_testing_env=true` (target condition `test`) | CompilerMain.cpp:381,535 |
| `--benchmark` | `-bm` | NoValue | Benchmark whole compilation | CompilerMain.cpp:382,508 |
| `--benchmark-files` | `-bm-files` | NoValue | Per-file benchmark breakdown | CompilerMain.cpp:383,509 |
| `--benchmark-modules` | `-bm-modules` | NoValue | Per-module benchmark breakdown | CompilerMain.cpp:384,510 |
| `--verbose` | `-v` | NoValue | Verbose output | CompilerMain.cpp:385,490,511 |
| `--verbose-link` | `-vl` | NoValue | Verbose linker output | CompilerMain.cpp:386,512 |
| `-g` | — | NoValue | Emit debug info | CompilerMain.cpp:387,518 |
| `--ignore-errors` | `--ignore-errors` | NoValue | Continue compiling despite errors | CompilerMain.cpp:389,536 |
| `--lto` | — | NoValue | Force link-time optimization | CompilerMain.cpp:390,551-553 |
| `--assertions` | — | NoValue | Enable codegen assertions | CompilerMain.cpp:391,554-556 |
| `--tsan` | — | NoValue | Enable ThreadSanitizer (COMPILER_BUILD) | CompilerMain.cpp:392,559-561 |
| `--sanitize` | `-fsanitize` | SingleValue | Comma list: `thread,address,memory,undefined,leak,hwaddress,dataflow` (COMPILER_BUILD) | CompilerMain.cpp:393,562-580 |
| `--no-pie` | `--no-pie` | NoValue | Disable PIE | CompilerMain.cpp:394,533,581-583 |
| `--target` | `-t` | SingleValue | Target triple (default: host triple / `"native"`) | CompilerMain.cpp:395,610-639 |
| `--jobs` | `-j` | SingleValue | Thread count for parallel compilation | CompilerMain.cpp:396,677-690 |
| `--job-type` | `-jt` | SingleValue | Force job type: `exe`, `jit-exe`, `lib`, `2c`, `2ch`, `inter`, `proc` | CompilerMain.cpp:397,315-337 |
| `--jit` | `--jit` | NoValue | JIT compilation (default job type `JITExecutable`) | CompilerMain.cpp:398,669,860 |
| `--download` | `--download` | NoValue | Download-only, don't build | CompilerMain.cpp:399,772-774,865-867 |
| `--check` | `--check` | NoValue | Check-only, don't build | CompilerMain.cpp:400,775-777,868-870 |
| `--use-tcc` | `--use-tcc` | NoValue | Force TinyCC backend (implies `use_c`) | CompilerMain.cpp:401,521-526 |
| `--use-c` | `--use-c` | NoValue | Translate to C + embedded Clang | CompilerMain.cpp:402,524-526 |
| `--use-bc` | `--use-bitcode` | NoValue | Use bitcode module objects instead of `.o` | CompilerMain.cpp:403,528 |
| `--use-lld` | `--use-lld` | NoValue | Link with embedded LLD | CompilerMain.cpp:404,527 |
| `--output` | `-o` | SingleValue | Output file; extension selects artifact | CompilerMain.cpp:405,670,824-911 |
| `--resources` | `--res` | SingleValue | Override resources directory | CompilerMain.cpp:406,492-503 |
| `--ignore-extension` | — | NoValue | Ignore `-o` extension; treat as binary | CompilerMain.cpp:407,825 |
| `--no-cache` | — | NoValue | Disable module caching | CompilerMain.cpp:408,542-544 |
| `--frecompile-plugins` | `--frecompile-plugins` | NoValue | Force CBI plugin recompilation | CompilerMain.cpp:409,545-547 |
| `--out-ll` | — | SingleValue | Emit LLVM IR to path | CompilerMain.cpp:410,808-815 |
| `--out-bc` | — | SingleValue | Emit LLVM bitcode to path | CompilerMain.cpp:411,808-817 |
| `--out-obj` | — | SingleValue | Emit object file to path | CompilerMain.cpp:412,808-819 |
| `--out-asm` | — | SingleValue | Emit assembly to path | CompilerMain.cpp:413,808-821 |
| `--out-bin` | — | SingleValue | Emit binary to path | CompilerMain.cpp:414,812,842-858 |
| `--out-ll-all` | — | NoValue | Emit `.ll` for **every** module (COMPILER_BUILD) | CompilerMain.cpp:415,529 |
| `--out-asm-all` | — | NoValue | Emit assembly for every module (COMPILER_BUILD) | CompilerMain.cpp:416,530 |
| `--debug-ir` | — | NoValue | Emit (possibly invalid) IR for debugging | CompilerMain.cpp:417,548-550 |
| `-c` | — | NoValue | Compile only, no link (→ `ProcessingOnly`) | CompilerMain.cpp:418,903-904 |
| `--cbi-m` | `--cbi-m` | MultiValued | Additional CBI module names | CompilerMain.cpp:419 |
| `-fno-unwind-tables` | — | NoValue | Disable unwind tables (cleaner IR) | CompilerMain.cpp:420,531 |
| `-fno-asynchronous-unwind-tables` | — | NoValue | Disable async unwind tables | CompilerMain.cpp:421,532 |
| `--mod` | — | MultiValued | Specify module files | CompilerMain.cpp:422 |
| `--run-negative-tests` | — | NoValue | Run negative lifetime tests (DEBUG-only) | CompilerMain.cpp:423 |
| `--arg-<name>` / `-arg-<name>` | — | (passthrough) | Forwarded to `build.lab` via `ctx.has_arg`/`get_arg` | CompilerMain.cpp:743-747 |

Notes:

- `--library`/`-l` is registered **twice** (`CompilerMain.cpp:363` and `:388`);
  the map keeps one entry but both parsed lists are read via
  `options.data.find("library")` and `options.data.find("l")`.
- The `-b` alias for `--build-dir` is only read (`options.option_new("build-dir", "b")`
  at `CompilerMain.cpp:692`); it is not registered as a `CmdOption` alias, so
  `-b` is not actually accepted.
- `--arg-*` flags are passed to `LabBuildContext.build_args` after stripping the
  `arg-` prefix (`CompilerMain.cpp:743-747`); `output`, `mode`, and `test` are
  also injected (`CompilerMain.cpp:734-742`).
- The help text (`CompilerMain.cpp:46-89`) is partially stale: it mentions
  `--no-cbi`, `--no-caching`, and `--cpp-like`, which are **not** registered in
  `cmd_data[]`.

## 5. `OutputMode`

Defined in `compiler/OutputMode.h:15-55` as `enum class OutputMode : uint8_t`:

| Value | Meaning | Emitter config |
|-------|---------|----------------|
| `Debug` | Debug build with debug info (IDE default) | `is_debug=true`, `lto=false`, `small=false` |
| `DebugQuick` | Fast build, no/less debug info, no opts | `is_debug=true`, `lto=false`, `small=false` |
| `DebugComplete` | Full debug info + all assertions | `is_debug=true`, `lto=false`, `small=false`, `assertions_on=true` |
| `ReleaseFast` | Optimized release, debug info removed | `is_debug=false`, `lto=true`, `small=false` |
| `ReleaseSmall` | Optimized for size | `is_debug=false`, `lto=true`, `small=true` |
| `ReleaseSafe` | Prefer safe code over fast code | **Not handled** by `configure_emitter_opts` (see gotchas) |

Mapping is performed by `configure_emitter_opts()` in `compiler/Codegen.cpp:1597-1626`;
the string form is `to_string(OutputMode)` in `compiler/lab/LabBuildCompiler.cpp:1431-1447`.

Parsing (`get_output_mode`, `CompilerMain.cpp:274-313`): accepts `debug`,
`debug_quick`, `debug_complete`, `release`/`release_fast`, `release_small`.
An unknown value prints a warning and falls back to `Debug`. In DEBUG builds, if
no `--mode` is given the default is `DebugQuick` (`CompilerMain.cpp:304-311`);
otherwise `Debug`.

`OutputMode` does **not** itself select the artifact. It configures
optimization/debug/lto/assertions. The artifact is selected by `-o` extension,
`--out-*`, or `--job-type` (see §6).

## 6. Output Artifact Routing

For single-file inputs, the compiler derives the artifact in
`core/main/CompilerMain.cpp:807-911`:

| Selection | Artifact | Where |
|-----------|----------|-------|
| `--out-ll <p>` / `.ll` | LLVM IR (`module.llvm_ir_path`) | CompilerMain.cpp:808-833 |
| `--out-bc <p>` / `.bc` | LLVM bitcode (`module.bitcode_path`) | CompilerMain.cpp:809-835 |
| `--out-obj <p>` / `.o` | Object file (`module.object_path`) | CompilerMain.cpp:810-829 |
| `--out-asm <p>` / `.s` | Assembly (`module.asm_path`) | CompilerMain.cpp:811-831 |
| `--out-bin <p>` / default `-o` | Executable binary (`bin_out`) | CompilerMain.cpp:812,836-858 |
| `-o x.c` | `LabJobType::ToCTranslation` (Chemical → C) | CompilerMain.cpp:905-907 |
| `-o x.ch` | `LabJobType::ToChemicalTranslation` (C → Chemical) | CompilerMain.cpp:908-910 |
| `-c` or no `bin_out` | `LabJobType::ProcessingOnly` (no link) | CompilerMain.cpp:903-904 |
| (no `-o`) | `a.exe` on Windows, `a` elsewhere | CompilerMain.cpp:842-849 |
| `--out-ll-all` / `--out-asm-all` | Emit per-module IR/asm (COMPILER_BUILD) | CompilerMain.cpp:529-530 |

The `JobType` override (`--job-type`/`-jt`) is resolved by
`getJobTypeFromOpt` (`CompilerMain.cpp:315-337`) with values `exe`, `jit-exe`,
`lib`, `2c`, `2ch`, `inter`, `proc`.

`LabJobType` (`compiler/lab/LabJobType.h:5-56`) values: `Executable`,
`JITExecutable`, `Library`, `ToCTranslation`, `ToChemicalTranslation`,
`ProcessingOnly`, `CBI`, `Intermediate`, `Interpretation`.

## 7. Embedded Clang / LLVM / LLD & SelfInvocation

### Clang frontend & driver

- `Compiler` embeds the full Clang driver, `-cc1` frontend, and `-cc1as`
  assembler.
- A `-cc1` invocation of the compiler forwards to:
  `compiler_main` → `chemical_clang_main` (`CompilerMain.cpp:351-356`) →
  `clang_main(..., {argv[0], nullptr, false})` (`compiler/clang_driver.cpp:483-486`).
- `clang_main()` (`clang_driver.cpp:243`) handles `-cc1` by calling `cc1_main`
  (`clang_cc1main.cpp:219`) and `-cc1as` by calling `cc1as_main`
  (`clang_cc1as_main.cpp:670`) — see `ExecuteCC1Tool` at `clang_driver.cpp:211-241`.
- `--cc ...` forwards to embedded Clang via `chemical_clang_main2`
  (`CompilerMain.cpp:474-488`), implemented in `compiler/Codegen.cpp:1673-1679`.
- The Clang linker wrapper is `clang_link_objects()` (`Codegen.cpp:1862-1963`):
  it builds `[exe_path, -target <triple>, -resource-dir=…, -g?, -no-pie?, -v?,
  -fsanitize=…, <objects>, -L…, -l…, -shared?, -o <out>, -Wl,-rpath…]` and calls
  `chemical_clang_main2`. `compile_c_file_to_object()` (`Codegen.cpp:1965-2004`)
  does the same for C→`.o`.

### LLD

- `Compiler` embeds LLD drivers for ELF/COFF/MachO/MinGW/WASM
  (`Codegen.cpp:1697-1701`).
- `lld_link_objects()` (`Codegen.cpp:1740`) builds the link command, and
  `invoke_lld()` (`Codegen.cpp:1715-1734`) picks the driver by triple:
  `ld64.lld` (Darwin), `lld-link` (Windows), else `ld.lld`.
- Selected with `--use-lld` (`CompilerMain.cpp:527`).

### LLVM ar/ranlib/lib/dlltool

- `compiler/llvm-ar.cpp` exposes `ChemLlvmAr_main(int, char**)` at
  `llvm-ar.cpp:1545` (`llvm_ar_main(argc, argv, {argv[0], nullptr, false})`).
- `llvm_ar_main2()` (`Codegen.cpp:1681-1687`) is the CLI bridge.
- Dispatched by the `ar`/`ranlib`/`lib`/`dlltool` subcommands
  (`CompilerMain.cpp:452-472`).

### TinyCC JIT / self-invocation

- `launch_tcc_jit_exe()` (`LabBuildCompiler.cpp:1491-1530`) is a true
  **self-invocation**: it builds argv
  `[<this compiler exe>, --mode, <mode>, <objects...>, tcc-jit]` and spawns
  itself. The spawned process hits the `tcc-jit` branch
  (`CompilerMain.cpp:440-450`) → `tcc_run_invocation()`
  (`LabBuildCompiler.cpp:1449-1486`), which assembles a `TCCState`, adds the
  object files, and calls `tcc_run(state, argc, argv)`.
- `compiler/SelfInvocation.h/.cpp` provides a different self-invocation path:
  `system_headers_path(arg0)` (`SelfInvocation.cpp:50-103`) runs
  `<self> cc -v -c -xc++ /dev/null` (or `nul` on Windows) via
  `invoke_capturing_out()` (`SelfInvocation.cpp:9-48`, using `popen`/`_popen`) and
  parses the `#include <...> search starts here:` block to discover system
  headers. `header_abs_path()` / `headers_dir()` then resolve header locations.

## 8. Target Support

`core/targets/` contains the **executable entrypoints** (`Compiler.cpp`,
`LSPMain.cpp`). Target-triple *data* lives in `compiler/lab/TargetData.h` and
`ast/utils/TargetAPI.h`.

### Representation

`TargetData` (`compiler/lab/TargetData.h:7-62`) is a flat struct of booleans,
grouped as:

- **Compilers**: `c`, `tcc`, `clang`
- **Job kind**: `cbi`, `lsp`, `test`
- **Modes**: `debug`, `debug_quick`, `debug_complete`, `release`,
  `release_safe`, `release_small`, `release_fast`
- **Environments**: `posix`, `gnu`
- **Platforms**: `is64Bit`, `little_endian`, `big_endian`, `windows`, `win32`,
  `win64`, `isLinux`, `macos`, `freebsd`, `isUnix`, `android`, `cygwin`,
  `mingw32`, `mingw64`, `emscripten`, `musl`
- **Architectures**: `x86_64`, `x86`, `i386`, `arm`, `aarch64`, `powerpc`,
  `powerpc64`, `riscv`, `riscv32`, `riscv64`, `s390x`, `wasm32`, `wasm64`

`create_target_data()` (`TargetData.h:68-206`) is a `consteval` host build via
preprocessor macros. `create_jit_target_data()` (`TargetData.h:215-220`)
forces `c=true, tcc=true` for jobs whose generated C is always consumed by
TinyCC (build.lab scripts, `chemical.mod` build files, CBI plugins) regardless
of the outer compiler backend.

### Parsing and propagation

1. CLI: `--target`/`-t` is read at `CompilerMain.cpp:612-617` (COMPILER_BUILD
   default `llvm::sys::getDefaultTargetTriple()`) or `CompilerMain.cpp:626-633`
   (TCC default `"native"`).
2. Bitness: `Codegen::is_arch_64bit(target)` (`Codegen.cpp:242-248`, uses
   `llvm::Triple::isArch64Bit`) on Compiler; host macros on TCC
   (`CompilerMain.cpp:634-638`).
3. Stored in `LabBuildCompilerOptions.target_triple` /
   `ASTProcessorOptions.target_triple` (`ASTProcessorOptions.h:68`) and
   `ASTProcessorOptions.is64Bit` (`ASTProcessorOptions.h:63`).
4. `LabBuildContext::initialize_job()` calls
   `prepare_target_data(job->target_data, target_triple)`
   (`compiler/lab/LabBuildContext.cpp:66-68`).
5. `prepare_target_data()` (`ast/utils/GlobalFunctions.cpp:3109-3130`) clears
   host-derived flags, normalizes the triple, and dispatches to
   `init_target_data()` (`GlobalFunctions.cpp:2987-3107`) which uses
   `llvm::Triple` on COMPILER_BUILD. The `TCC_BUILD` branch
   (`GlobalFunctions.cpp:3134-3251`) parses the triple by splitting on `-`.
6. `initialize_job()` also sets backend flags (`use_c`/`use_tcc`/job-kind) and
   `set_job_mode()` translates `OutputMode` into the `debug*`/`release*` bits
   (`LabBuildContext.cpp:12-64`).

### Conditions

`def`/`if` target conditions are resolved by `is_condition_enabled()`
(`compiler/lab/LabJob.cpp:7-97`) against `TargetData`, e.g. `if tcc`,
`if clang`, `if windows`, `if linux`, `if is64Bit`, `if x86_64`, `if musl`,
`if release_fast`. `resolve_target_condition()` (`LabJob.cpp:99-127`) handles
`&&`/`||`/negation over these. Conditions are also declared as `def` values in
`declare_def_values()` (`GlobalFunctions.cpp:3259+`).

### Backend handoff

- LLVM: `Codegen::module_init()` sets
  `module->setTargetTriple(llvm::Triple(llvm::Triple::normalize(target_triple)))`
  and `setDataLayout(TargetMachine->createDataLayout())`
  (`compiler/Codegen.cpp:264-266`).
- Clang/LLD: the triple is passed as `-target <triple>` (`Codegen.cpp:1874-1878`
  for links, `Codegen.cpp:1976-1980` for C objects).
- TinyCC does not support target triples (`LabBuildContext.cpp:51-54`).

## 9. `ModuleOptionRegistry`

`compiler/lab/ModuleOptionRegistry.h` + `.cpp` register the set of allowed
module-level options parsed from `chemical.mod`. The registry is a lazy
singleton (`get_option_registry()`, `ModuleOptionRegistry.cpp:44-51`) built by
`init_module_option_registry()` (`ModuleOptionRegistry.cpp:22-38`):

| Option key | Kind | Allowed | Source |
|------------|------|---------|--------|
| `safety` | Boolean | true/false | ModuleOptionRegistry.cpp:26 |
| `checks.bounds` | Boolean | true/false | ModuleOptionRegistry.cpp:27 |
| `checks.overflow` | Boolean | true/false | ModuleOptionRegistry.cpp:28 |
| `checks.null` | Boolean | true/false | ModuleOptionRegistry.cpp:29 |
| `optimization_level` | Integer | 0–3 | ModuleOptionRegistry.cpp:32 |
| `safe_mode` | String | `off`, `warn`, `enforce` | ModuleOptionRegistry.cpp:36 |
| `stack_protector` | String | `none`, `standard`, `strong`, `all` | ModuleOptionRegistry.cpp:37 |

Supporting types: `ModOptionValue` / `ModOptionValueKind` (parsed value),
`ModFileOption` (parsed option + source location), `OptionDescriptor` with
factories `boolean()`, `integer(min,max)`, `floating(min,max)`,
`string_val(allowed)`, `any_string()`, and validators `value_kind_matches()` /
`validate_allowed()` (`ModuleOptionRegistry.h:15-200`).

The C++ layout consumed by codegen/CBI is `LabModuleOptions`
(`compiler/lab/LabModuleOptions.h:24-30`) which mirrors `lang/libs/lab/src/lab.ch
:: ModuleOptions` — keep field order/names/types in sync.

## 10. `Compiler` vs `TCCCompiler` Entry Paths

Both call the same `compiler_main()`, so the split is by `#ifdef`:

| Aspect | `Compiler` (`COMPILER_BUILD`) | `TCCCompiler` (`TCC_BUILD`) |
|--------|-------------------------------|-----------------------------|
| `-cc1` handling | Yes (`CompilerMain.cpp:351-356`) | No |
| `cc` subcommand | Yes (`:474-488`) | No |
| `ar`/`ranlib`/`lib`/`dlltool` | Yes (`:452-472`) | No |
| Default target triple | `llvm::sys::getDefaultTargetTriple()` (`:616`) | `"native"` (`:626`) |
| Bitness | `Codegen::is_arch_64bit` (`:623`) | host macros (`:634-638`) |
| `--out-ll-all` / `--out-asm-all` | Yes (`:529-530`) | No |
| `--use-lld`, `--use-bc`, `--no-pie`, sanitizers | Yes (`:527-533,559-583`) | No |
| Default `use_c` / `use_tcc` | `false` / `false` (`LabBuildCompilerOptions.h:30-43`) | `true` / `true` |
| Job processing | `use_c ? process_job_tcc : process_job_gen` (`LabBuildCompiler.h:345-355`) | `process_job_tcc` only (`:352-354`) |
| Embedded Clang in link | `use_embedded_clang()` true unless TCC job/`--use-tcc` (`LabBuildCompiler.h:328-339`) | `false` → libtcc link (`:335-338`) |
| `configure` / `run` / `tcc-jit` | Yes | Yes |

The backend dispatch for a job:

```
process_modules(job)
  COMPILER_BUILD: use_c(job) ? process_job_tcc(job) : process_job_gen(job)
  TCC_BUILD:      process_job_tcc(job)
```

where `use_c(job)` is `options->use_c || is_tcc_job(type)` and
`is_tcc_job` covers `CBI`, `ToCTranslation`, `JITExecutable`
(`LabBuildCompiler.h:285-301`). So build.lab/CBI/JIT jobs are **always** C+TCC
even on the LLVM `Compiler`.

## 11. Gotchas

- **Help text lies.** `--no-cbi`, `--no-caching`, `--cpp-like`
  (`CompilerMain.cpp:79-81`) are documented but not registered in `cmd_data[]`.
- **`-b` is not an accepted alias.** Only `--build-dir` is registered
  (`CompilerMain.cpp:362`); the `"b"` second argument at `:692` is a stale read.
- **`ReleaseSafe` is unreachable via `--mode`.** `get_output_mode`
  (`CompilerMain.cpp:274-313`) never returns it, and `configure_emitter_opts`
  (`Codegen.cpp:1597-1626`) has no `ReleaseSafe` case — under `DEBUG` it throws,
  otherwise the options keep their constructor defaults.
- **`--out-ll`/`--out-bc`/`--out-obj`/`--out-asm` are absolute-path outputs**
  and are only applied to the single-file module (`CompilerMain.cpp:808-821`);
  they are not the same as `.lab`/`.mod` build outputs.
- **Output mode ≠ artifact.** `--mode` never selects object/IR/executable;
  the `-o` extension or `--job-type` does.
- **`.lab`/`.mod` builds set `is_caching_enabled=false` implicitly?** No — only
  the single-file path forces `compiler_opts.is_caching_enabled = false`
  (`CompilerMain.cpp:794`). `.lab`/`.mod` builds honour `--no-cache`.
- **TinyCC cannot cross-compile.** The triple is deliberately ignored for TCC
  jobs (`LabBuildContext.cpp:51-54`); `--target` has no effect there.
- **Build.lab / CBI always use TCC.** `create_jit_target_data()` forces
  `c=true, tcc=true` (`TargetData.h:215-220`) so libraries never emit C that
  libtcc can't parse (e.g. `__float128`).
- **Host leakage risk.** `prepare_target_data` wipes host-derived flags before
  parsing an explicit triple (`GlobalFunctions.cpp:3115-3122`); otherwise
  `source ... if linux` would wrongly fire when targeting Windows.
- **`OutputMode::ReleaseSafe` bypass.** Keeping the enum but never mapping it
  means the mode silently degrades.
- **Self-invocation depends on `options->exe_path`.** The path is captured from
  `getExecutablePath()` at startup (`CompilerMain.cpp:703,785`) and stored in
  `ASTProcessorOptions.exe_path` (`ASTProcessorOptions.h:78`); a moved/renamed
  compiler binary breaks Clang/libtcc self-invocation.

### Extension Checklist

**Adding a new CLI flag:**

1. Add a `CmdOption` entry to `cmd_data[]` in
   `core/main/CompilerMain.cpp:360-424` with the right `CmdOptionType`.
2. Read it in `prepare_options` (`CompilerMain.cpp:507-585`) and store it in
   `LabBuildCompilerOptions` (add the field to
   `compiler/lab/LabBuildCompilerOptions.h` or a sub-options struct).
3. If it must reach the frontend/codegen, thread it through
   `ASTProcessorOptions` / `CodegenOptions` / `CodegenEmitterOptions`.
4. If it's a `--arg-*` passthrough, no C++ change is needed — it is
   auto-forwarded (`CompilerMain.cpp:743-747`).
5. Update `print_help()` (`CompilerMain.cpp:46-89`) — and keep the registered
   set in sync so help doesn't lie.

**Adding a new output mode:**

1. Add the enumerator to `OutputMode` (`compiler/OutputMode.h:15-55`).
2. Add a case in `to_string()` (`LabBuildCompiler.cpp:1431-1447`).
3. Add a case in `configure_emitter_opts()` (`Codegen.cpp:1597-1626`) and in
   `is_debug`/`is_release` helpers (`OutputMode.h:57-67`).
4. Teach `get_output_mode()` to parse its string (`CompilerMain.cpp:274-313`).
5. Teach `set_job_mode()` to set the corresponding `TargetData` bit
   (`LabBuildContext.cpp:12-20`) and expose it in
   `is_condition_enabled` (`LabJob.cpp:7-97`) if it should be queryable.

**Adding a new target:**

1. Add a boolean to `TargetData` (`compiler/lab/TargetData.h:7-62`).
2. Set it in `create_target_data()` for the host and in `init_target_data()`
   (`GlobalFunctions.cpp:2987-3107`) plus the TCC parser
   (`GlobalFunctions.cpp:3134-3251`).
3. Add a name mapping in `is_condition_enabled` (`LabJob.cpp:7-97`) and a
   `def` value in `declare_def_values()` (`GlobalFunctions.cpp:3259+`).
4. If the backend needs it, pass the triple through `clang_link_objects` /
   `compile_c_file_to_object` / `Codegen::module_init` (already generic).
5. Add tests that exercise `if <new_target>` conditions on the `.lab`/`.mod`
   side.

## Related Skills

- **Build System (LabBuildCompiler)** (`.agents/skills/build_system/SKILL.md`) —
  full pipeline, job types, CLI arg flow, extension routing.
- **Building** (`.agents/skills/building/SKILL.md`) — scripts and targets.
- **Compiler Bindings** (`.agents/skills/compiler_bindings/SKILL.md`) — TinyCC
  JIT and CBI registration used by `link_cbi_job` / `tcc_run_invocation`.
- **chemical.mod** (`.agents/skills/chemical_mod/SKILL.md`) — module options
  consumed by `ModuleOptionRegistry`.
- **C Codegen** (`.agents/skills/c_codegen/SKILL.md`) / **LLVM Backend**
  (`.agents/skills/llvm_backend/SKILL.md`) — the backend behind `process_job_tcc`
  / `process_job_gen`.
