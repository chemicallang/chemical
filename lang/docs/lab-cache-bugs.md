# Lab build cache — reproducible bugs

The Lab build cache is enabled by default (`LabBuildCompilerOptions::is_caching_enabled`,
`translate_to_single_file`). `./scripts/test.sh` passes `--no-cache`, so most
day-to-day runs never exercise it — these bugs only appear on an *incremental*
build (an existing `build/` directory plus an edited source file).

All three were found while building an application module that depends on `regex`
(which itself instantiates `std::vector<i64>`), with the dependency served from
cache and the application re-translated.

---

## Bug 1 — `struct/union/enum already defined` (FIXED)

### Symptom

```
<string>:10607: error: struct/union/enum already defined in build/object_tcc.o
error: couldn't compile the program
```

Only when the cache is enabled; `--no-cache` always works.

### Root cause

For a cached module, the partial C file (`build/modules/<mod>/partial.2c.c`) is
appended to the translation unit, and `process_cached_module` marks the module's
*file* nodes as generated/declared. But the generic instantiations a module
registers during symbol resolution (`InstantiationsContainer::current_module_instantiations`)
do **not** live in the module's files — they are reached only through
`declare_module`'s `Declare:Generics <mod>` section. So they were never marked.

A freshly translated dependent module that references such an instantiation
(e.g. a struct with a `std::vector<i64>` field) then re-emitted the struct
definition (`early_declare_struct_def` is guarded by `has_declared`), producing a
duplicate definition in the concatenated single translation unit.

Note `--incremental` (`translate_to_single_file = false`) is affected too: the C
writer is truncated back to the declarations after each module, so every module's
translation unit carries the declarations of all previous modules.

### Fix

`process_cached_module` (`compiler/lab/LabBuildCompiler.cpp`) now also marks the
cached module's `current_module_instantiations` with
`set_generated_instantiations` + `set_defined_declarations` (TCC path).

### Reproducer

```bash
# stage 1: no std::vector use
rm -rf lang/compiled/repro_cache/build lang/compiled/repro_cache/out.exe
cmake-build-debug/TCCCompiler lang/compiled/repro_cache/chemical.mod \
    -o lang/compiled/repro_cache/out.exe --mode debug_quick      # exit 0
# stage 2: add `struct VecHolder { var items : std::vector<i64> }` + use it
cmake-build-debug/TCCCompiler lang/compiled/repro_cache/chemical.mod \
    -o lang/compiled/repro_cache/out.exe --mode debug_quick      # before fix: exit 1
```

`lang/compiled/repro_cache/` is gitignored; recreate `chemical.mod` as an
`application` importing `cstd`, `std`, `core`, `regex`, and flip `src/main.ch`
between the two stages above.

---

## Bug 2 — LLVM backend crash on an incremental cache hit (FIXED)

### Symptom

```
[lab] Compiling module 5 of 5 (repro_cache)
[lab] Building module 'repro_cache' at path 'build/modules/repro_cache/object.o'
RUNTIME ERROR: invalid memory access          # SIGSEGV
```

`Compiler` (LLVM) only, cache enabled only. Backtrace lands in
`StructDefinition::code_gen(declare=false) → ImplDefinition::code_gen_bodies →
FunctionDeclaration::code_gen_override → ... → Value::load_value →
llvm::IRBuilderBase::CreateLoad` with a bogus `llvm::Type`.

### Root cause

`process_module_gen`'s cached branch returned early **without**
`container.clear_current_module_instantiations()`. The instantiations the cached
module registered during symbol resolution therefore leaked into the next
(changed) module, which declared *and* code-generated their bodies — but those AST
nodes are owned by the cached module's allocator, which `mod_allocator->clear()`
had already released. Use-after-free of freed AST memory.

(The TCC cached branch always cleared them; the LLVM branch did not.)

### Fix

Added `processor.container.clear_current_module_instantiations();` to the LLVM
cached branch in `process_module_gen` (`compiler/lab/LabBuildCompiler.cpp`).

---

## Bug 3 — `redefinition of '__chemda_<rand>_<N>'` in cached builds (OPEN)

### Symptom

Editing a module that contains many lambdas (`lang/tests/src/tests.ch`) and
rebuilding while its dependencies are served from cache:

```
<string>:50915: error: redefinition of '__chemda_366_79' in .../object_tcc.o
error: couldn't compile the program
```

The two colliding definitions are real and both present in `Translated.c`, e.g.
one from `build/chemical-tests.dir/modules/main/partial.2c.c` (fresh) and one from
`build/chemical-tests.dir/modules/common_tests/partial.2c.c` (cached from an
earlier run), naming *different* lambda bodies.

### Root cause

Lambda functions are named `__chemda_<random(100..999)>_<lambda_num>`
(`write_lambda_function`, `preprocess/2c/2cASTVisitor.cpp`). Uniqueness relies on:

* `lambda_num` — a counter on `ToCAstVisitor` that is **not** reset per module and
  starts at `0` for the process. Cached modules emit nothing, so the first freshly
  translated module numbers its lambdas from `0` again, overlapping the numbers
  baked into the cached partials.
* `random(100, 999)` — a 900-wide prefix seeded once per process from
  `time(NULL)` (`preprocess/utils/RepresentationUtils.cpp`). Two builds get
  unrelated prefixes, so a collision needs the same prefix *and* the same number;
  with hundreds of lambdas across the cached and fresh modules this happens
  readily.

So lambda names are only unique *within one process's writer*, while a cached
build concatenates writers from different processes.

### Suggested fix

Make the name globally unique and stable, e.g. tag it with the module
(`__chemda_<module>_<N>` — numbering is already unique per module) or derive the
numeric part from `LambdaFunction::encoded_location()`. Both make a cached partial
and a fresh translation of the *same* module byte-identical, which the current
random scheme does not.

### Reproducer

```bash
./scripts/test.sh --tcc --cache                 # cold cache, exits 0
printf '\n// probe\n' >> lang/tests/src/tests.ch
./scripts/test.sh --tcc --cache --no-build      # fresh 'main', cached deps: fails
# revert the probe line afterwards
```

Caveat: this probe is noisier than bugs 1–2 — the same probe has also been seen
failing at link ("`Tcc` couldn't output file") rather than at `redefinition`, and
compiling cleanly on one run. The collisions depend on the per-process `rand()`
sequence.
