# Lab build: intermittent SIGSEGV in `built_lab_file` (open investigation)

Status: **open**. No verified root cause or fix yet. This document records the
reproduction, the exact corruption signature, and what has already been *ruled
out*, so the next attempt does not repeat the same dead ends.

## Symptom

In `DEBUG` builds, compiling a module that imports a macro plugin
(`universal_cbi` / `html_cbi`) intermittently segfaults (or aborts, rc `134`).
It surfaces in the `--negative` suite as
`neg_universal_missing_prop_names_the_component` /
`neg_universal_unknown_component_names_the_symbol` failing, because the
compiler dies instead of printing the expected diagnostic. It looks like a flake
because it is file-order dependent, not because it is benign.

### Under gdb the crash is *frequent*

Running the compile under `gdb -batch` makes it reproduce in **1–7 attempts**
instead of ~1 in 250 (gdb disables ASLR, and the slowdown shifts thread timing).
That is the cheapest way to get a crash on demand, and it is itself a strong
signal that the bug is **timing/layout sensitive**. Keep gdb in the loop.

Typical run (abridged):

```
$ TCCCompiler chemical.mod --no-cache -o out.exe
File order seed: 8493191457391920518 (set FILE_ORDER_SEED to reproduce)
...
[lab] Building CBI 'universal'
[lab] Building module cstd (1 / 9)
...
Thread 1 "TCCCompiler" received signal SIGSEGV, Segmentation fault.
#0 std::vector<ASTFileMetaData>::size (this=0x555555ab9a01a0)
#1 ASTProcessor::import_chemical_files_direct (ASTProcessor.cpp:714)
#3 LabBuildCompiler::process_module_tcc (LabBuildCompiler.cpp:809)
#5 LabBuildCompiler::built_lab_file (LabBuildCompiler.cpp:2988)   <- module loop
```

There are **three** distinct crash sites, all reached from `built_lab_file`:

```
#0 std::vector<ASTFileMetaData>::size (this=0x555555ab95c001a0)
#1 ASTProcessor::import_chemical_files_direct (ASTProcessor.cpp:714)
#3 LabBuildCompiler::process_module_tcc (LabBuildCompiler.cpp:818)
#5 LabBuildCompiler::built_lab_file (LabBuildCompiler.cpp:3001)   <- module loop
```

```
#0 ASTNode::kind (this=0x0)                    <- null node
#1 LabBuildCompiler::built_lab_file (LabBuildCompiler.cpp:2863)
```
`:2862`/`:2863` is the loop that scans a build.lab's AST for its `import`
statements:

```cpp
for(const auto node : file.result->unit.scope.body.nodes) {   // :2862
    if(node->kind() != ASTNodeKind::ImportStmt) {              // :2863
```

so `file.result` or the `nodes` vector inside it is already bad.

And, from a CBI build's 2c translation, a stale `BaseType`:

```
#0 ASTNode::kind (this=0x2160f26a311f625c)
#1 BaseType::isStructLikeType
#3 accept_func_return (2cASTVisitor.cpp:728)
```

## Reproduction harness

Without gdb the crash is rare (~1 in 250 compiles) but cheap to hit in bulk — one
compile is under a second. Harness used (keep it out of the repo; `/tmp` is fine),
plus a driver that loops `gdb -batch --args $COMPILER chemical.mod --no-cache -o
out.exe` per directory and stops on `received signal SIGSEGV`.

```bash
mkdir -p /tmp/hunt/m{1..6}
for i in 1 2 3 4 5 6; do
  cat > /tmp/hunt/m$i/chemical.mod <<'EOF'
module neg_test
source "."
import std
import page
import universal_cbi
import html_cbi
EOF
  printf '#universal Host(props) {\n    return <NoSuchComponent />\n}\npublic func main() : int {\n    return 0\n}\n' \
    > /tmp/hunt/m$i/src.ch
done
# then loop `$COMPILER chemical.mod --no-cache -o out.exe` in each dir until rc >= 128
```

Measured baseline on the current tree: **~10 crashes per ~2400 compiles**
(6 parallel workers x up to 400 runs, two rounds: 6 + 4).

Because `DEBUG` shuffles per-module file order (see below), always note the
`File order seed:` line of a crashing run — it reproduces the same ordering.

## The exact corruption signature

**Pointers are byte-shifted.** In the `process_module_tcc` crash the `LabModule*`
is exactly `real << 8`, and the bad address being read is that value plus the
module's field offset:

```
mod   = 0x555555ab95c00000      # real LabModule* was 0x555555ab95c0
files = 0x555555ab95c001a0      # = (real << 8) + offsetof(LabModule, direct_files)
```

`real << 8` is exactly what you get by reading 8 bytes **one byte before** the
pointer. Instrumenting `built_lab_file`'s dependency loop showed the local
`auto outModDependencies = flatten_dedupe_sorted(mod_dependencies);` being shifted
in place, mid-function, by exactly that much:

```
[LAB-DEPS] ... n=4 0x592e599f25c0 0x592e599f2970 0x592e599f2090 0x592e599f00e0 \
    self=0x7ffc49741b60 beg=0x592e599f1bf0 end=0x592e599f1c10 cap=0x592e599f1c10
[LAB-VEC]  ... n=4 self=0x7ffc49741b60 beg=0x592e599f1bf0 end=0x592e599f1c10 ...   <- still fine
[LAB-USE]  i=0 slot=0x592e599f1bf0 mod=0x592e599f25c0                             <- fine
[LAB-USE]  i=1 slot=0x592e599f1bf5 mod=0x2e599f2970000059                         <- corrupt!
```

At `i=1` the slot address is `beg + 5` instead of `beg + 8`; the value read is
the original pointer shifted by 5 bytes. Other runs differ only in *how much*:

* `beg = 0x...bf0` -> `0x...bef` (low byte `0xf0 -> 0xef`, **-1**)
* `beg = 0x...bf0` -> `0x...bed` (low byte `0xf0 -> 0xed`, **-3**)

In every case **only the lowest byte of `std::vector::_M_start` changes**
(`0xf0 -> 0xef` is `beg - 1`, `0xf0 -> 0xed` is `beg - 3`), between iterations of
the loop — i.e. while `process_module_tcc_bm` runs for module N, the frame's
vector object has already been damaged. `beg - 1` explains both symptoms: the
first element then reads as `real << 8`, and `data()` becomes 8-byte-misaligned
(`data=0x...bef`).

This is a **single-byte write into `built_lab_file`'s stack frame at
`self + 0`**, not a heap use-after-free: `size()` (`_M_finish - _M_start`) still
reports 4, and `capacity`/`end` look sane. Consequences: `n` and the element
values disagree, the loop reads a bogus `LabModule*` (crash in
`process_module_tcc`), and one run's `[LAB-DEPS]` line printed hundreds of
garbage values including raw ASCII path fragments — consistent with reading
memory a `std::vector<chem::string>` of paths now occupies.

## Ruled out

* **`ModuleStorage::clear()` destroying modules still referenced by a build.**
  The initial hypothesis was that a nested build's `clear()` frees `LabModule`s
  that an enclosing build's `outModDependencies` still points at. Instrumentation
  showed only **one** `mod_storage.clear()` in a whole crashing run (the top-level
  app clear, before the build function is called and before any job runs), and no
  nested `build_module_build_file_no_alloc`. Making `clear()` *retire* modules
  instead of destroying them (`modules` moved to a `retired_modules` vector, only
  the index dropped) changed the measured crash rate from 10/2400 to 9/2400 —
  i.e. **no effect**. That hypothesis is wrong for this crash. (It may still be a
  latent hazard worth hardening separately, but it is not the cause here.)
* **A data race on `LabModule`.** Three ThreadSanitizer runs (1.5 GB of reports)
  contain no race involving `LabModule`, `add_dependency` or `get_dependencies`.
  The races TSan *does* report are the known benign ones on AST nodes
  (`FunctionDeclaration::set_has_usage`, `ASTNode::kind` during generic
  instantiation) — and note they are reported under
  `ASTProcessor::sym_res_module`'s pool tasks, i.e. **concurrent symres across
  modules**, which is designed behaviour.
* **`nameBuffer[50]` in `built_lab_file`.** Its writes are `snprintf`-bounded, and
  the block runs *after* the dependency loop anyway, so it cannot explain
  corruption observed during the loop.
* **The DEBUG file-order shuffle itself.** `shuffle_files`
  (`compiler/ASTProcessor.cpp`) only reorders a module's own file list
  deterministically from the printed seed; it is a debug aid exposing
  order-dependent bugs, not a bug.

## Open leads (in the order I would try next)

1. **Watch the frame byte — but for *every* `built_lab_file` invocation.**
   Arming a hardware watchpoint on `*(unsigned char*)g_lab_watch_target` (the
   vector object's first byte, not `_M_start`'s target) at the first
   `built_lab_file` invocation, then disabling the breakpoint, caught **nothing**
   in 360 gdb runs — because the crashing call is a *different* invocation of
   `built_lab_file`. Re-arm on every hit of the creation line (publish the
   address from the frame via a global) and let the watchpoint be re-pointed; or
   key the arming on the invocation whose `path_view` is the app's
   `chemical.mod`. This is the one experiment that names the writer.
2. **Believe the timing signal.** Crashes are ~50x more likely under gdb. That
   points at either a thread race or a stack-layout-sensitive bug. Two concrete
   candidates to test by inspection/build configuration:
   * build with `-O0 -fno-omit-frame-pointer` and a *fixed* stack layout (no
     ASLR: `setarch -R`) to see if the crash becomes deterministic; then bisect
     by addressing `built_lab_file`'s frame contents.
   * Since `ASTProcessor::sym_res_module`, `import_chemical_files_*` and
     `type_verify_module*` all fan out to `ctpl::thread_pool`, check every one of
     them actually *waits* for its futures before returning — an unwaited task
     surviving into a later phase would explain both the timing sensitivity and
     writes into reused memory/frames.
3. **Compare backends.** Run the same bulk loop with the LLVM backend
   (`./scripts/test.sh --llvm`) — if the crash disappears there, the lead is TCC
   JIT/relocation or the 2c path rather than the front end.
4. **Allocator clears inside nested builds.** `build_module_from_mod_file` ends
   with `mod_allocator->clear(); file_allocator->clear();`, and
   `do_allocating` installs/tears down per-job allocators. Verify whether a
   nested build can clear an allocator an enclosing build's AST still lives in
   (this would explain the `ASTNode::kind(this=0x0)` and bad-`BaseType` sites,
   which are reads of AST that should be live).

## Recommended shape of the fix ("isolated nested builds")

The build system shares one mutable build state (`mod_storage`, `coreNodes`,
`implsIndex`, `current_job`, `executables`, the three allocators) across the
top-level build, nested `build.lab`/`chemical.mod` builds and every job. The
structural fix is to give each nested build its own state and restore the
enclosing one on exit — a `BuildScope` RAII object around the nested entry points
(`built_lab_file`, `build_lab_file_no_alloc`, `build_module_build_file_no_alloc`,
`local_or_remote_project_to_module`, `run_transformer`) that swaps the above
fields and merges the nested build's modules into the parent storage on exit so
job dependency pointers stay valid. Do **not** land this without the harness
above as a regression gate; it is a large-surface change.
