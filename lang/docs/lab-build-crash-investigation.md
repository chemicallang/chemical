# Lab build: intermittent SIGSEGV in `built_lab_file` — root cause and fix (resolved)

## TL;DR

Two ways tasks handed to the compiler's thread pool could keep running after the
function that spawned them returned:

1. **A task group whose completion signal fired too early.** `ConcurrentParsingState`
   fulfils its `all_done_promise` the moment its `outstanding` counter drops to zero.
   The pushing thread did not count itself, so the counter dropped to zero *in the
   middle of the push loop* (tasks do run concurrently while we are still pushing).
   The promise was fulfilled early, the waiting caller continued with tasks still
   running, those tasks then called `state.done_task()` on a `ConcurrentParsingState`
   whose scope had ended — and the stack slot had already been reused for
   `built_lab_file`'s `outModDependencies`.
2. **Early returns that abandoned futures.** `sym_res_module`'s four parallel phases
   (`stop_on_file_error`), `import_chemical_files_direct` and
   `import_chemical_files_direct_toks` returned as soon as a file reported errors
   *without* joining the remaining futures. Those tasks reference the `ASTProcessor`,
   the `SymbolResolver` and the three `ASTAllocator`s — all stack objects of the
   caller — so they kept writing into stack memory that had been reused by deeper
   calls.

Both are the same class of bug: **a pooled task outliving the stack state it points
at**. Symptom and signature below; fix at the end.

## Symptom

Intermittent `SIGSEGV` / `free(): invalid pointer` during lab builds that compile
compiler plugins (`universal`, `html`, ...). It surfaced as rare `--negative` suite
failures (`neg_universal_*`), because the harness captures compiler output and the
compiler crashed instead of printing the expected diagnostic.

Observed crash sites (all reached from `LabBuildCompiler::built_lab_file`):

```
std::vector<ASTFileMetaData>::size  <- mod->direct_files of a bad LabModule*
ASTProcessor::import_chemical_files_direct
  <- LabBuildCompiler::process_module_tcc
  <- LabBuildCompiler::built_lab_file          (module loop)

ASTNode::kind(this=0x0)                        <- null node
  <- LabBuildCompiler::built_lab_file          (import scan over build.lab nodes)

check_imports_for_cycles(parent_file=<worker stack address>)

free(): invalid pointer                        <- destructor of std::vector<LabModule*>
  <- std::vector<LabModule*>::~vector
  <- LabBuildCompiler::built_lab_file          (outModDependencies)
```

Different sites, one cause: corrupted stack locals.

## Signature

* Under `gdb` the crash rate jumped from ~1 in 250 runs to ~40 % — the bug is timing
  sensitive, and ptrace stops widen the window enormously.
* The corrupted word is `outModDependencies._M_start` (or `_M_finish`) — the vector's
  own pointers, in `built_lab_file`'s frame.
* The corrupted values were *small decrements* of the original pointer
  (`0x…bbf0 → 0x…bbef → 0x…bbee → 0x…bbed`), or the low 4 bytes zeroed
  (`0x555555abb660 → 0x555500000000`), or the whole word replaced by an unrelated
  heap pointer.
* A pure spin loop that only re-reads the vector's pointer (no calls, no writes)
  observed the value change underneath it — i.e. the writer was not in this thread's
  call chain.

Those three observations are the fingerprint of **`std::atomic<int>::fetch_sub` /
`store` on a dead `ConcurrentParsingState`**:

```cpp
struct ConcurrentParsingState {
    std::atomic<int> outstanding;      // 4 bytes at offset 0
    std::promise<void> all_done_promise;
    std::atomic_bool has_errors;
};
```

`done_task()` decrements `outstanding` (4-byte read-modify-write at offset 0 =>
the small decrements, the low-4-bytes writes), `set_has_errors()` stores a byte, and
`all_done_promise.set_value()` writes a word into the promise.

## Root cause, with the evidence

### The stack slot is reused

Instrumenting `built_lab_file` printed both addresses in the same run:

```
[LABDBG] parse state at 0x7ffc683065d0 (size 40)
[LABDBG] LOOP2-begin vec=0x7ffc683065d0 frame=0x7ffc68306280 ...
```

`&state == &outModDependencies`. `state` lives in an inner block that ends before
`outModDependencies` is declared, so gcc reuses the slot. Anything that writes through
a stale `ConcurrentParsingState*` after that block ends writes straight into the
vector's `_M_start` / `_M_finish` — and the destructor later calls
`free(shifted_pointer)` → `free(): invalid pointer`, or `symres` reads a
`LabModule*` built from a shifted pointer.

### The completion signal fires early

`built_lab_file` used:

```cpp
ConcurrentParsingState state;
lab_processor.import_chemical_files_recursive(pool, state, direct_files_in_lab, true, false);
auto fut = state.all_done_promise.get_future();
fut.wait();
```

and `import_chemical_files_recursive` did, per file:

```cpp
state.pushed_task();
pool.push([&state, ...](int){ import_chemical_file_recursive(...); state.done_task(); });
```

The pushing thread never counted itself. With several pool threads, a task can finish
(counter 1 → 0, **promise fulfilled**) while the pushing thread is still pushing the
remaining files. The push loop then increments the counter again, but
`all_done_promise` is already satisfied — `wait()` returns immediately, with tasks
still in flight. Those tasks call `done_task()` afterwards, on a `state` whose scope
has ended, hitting whatever now occupies that stack slot.

This also explains why it needed plugin builds: `state` is only used for build.lab
imports (`import_chemical_files_recursive`), and the CBI/plugin path runs *nested*
build.lab scripts, which delays callers and widens the race.

### The abandoned-future paths

```cpp
for(auto& f : futures) {
    auto has_errors = f.get();
    if(has_errors) {
        if(options->stop_on_file_error) return 1;   // <- remaining futures abandoned
        errored = true;
    }
}
```

`stop_on_file_error` defaults to `true`. Destroying a `std::future` from
`ctpl::thread_pool` (packaged task, not `std::async`) does **not** wait. The abandoned
tasks hold `ASTProcessor*` / `SymbolResolver*` / `ASTAllocator&` — stack objects of
the caller — and run full symres passes against them while the caller returns up the
stack, reusing that memory.

## Fix

1. **Count the pushing thread as a task** (`ASTProcessor::import_chemical_files_recursive`):

   ```cpp
   const bool is_producer = !in_task;
   if (is_producer) state.pushed_task();   // before the loop
   ...
   if (is_producer) state.done_task();     // after everything is pushed
   ```

   The counter can no longer reach zero while the producer is still pushing, so the
   promise is fulfilled exactly once, when everything really finished.
   `ConcurrentParsingState::wait()` also returns immediately on a zero counter (the
   no-tasks-pushed case), and `done_task()` guards `set_value()` with an atomic so a
   double fulfilment throws nothing.

2. **Never return with tasks in flight.** A `JoinedTasks<T>` RAII guard holds the
   futures vector, so *any* return path joins every valid future; the four symres
   phases now consume all futures and decide *after* joining
   (`join_all(futures)`); `import_chemical_files_direct` /
   `import_chemical_files_direct_toks` collect all results (marking failure) and only
   then return.

## Why it is fixed (evidence)

| Measurement | Before | After |
|---|---|---|
| gdb hunt harness, ~150 runs | 9–18 crashes | **0** |
| gdb hunt harness, second batch of 180 runs | — | **0** |
| pure-spin check detecting writes into the frame slot | frequent | **0** |
| `--tcc` / `--libs` / `--plugins` / `--negative` / `--interpret` | green (with the flake) | 2200 / 650 / 1122 / 290 / 1812 |

Differential: after the `JoinedTasks` fix alone the gdb hunt still crashed (9/150);
the producer-counting fix is what removed the crash — both are needed for correctness.

## Reproducing / regression harness

A universal-CBI module is enough (it forces nested plugin builds):

```
# chemical.mod
module neg_test
source "."
import std
import page
import universal_cbi
import html_cbi
```

Run the compiler with `--no-cache` under gdb in a loop; before the fix, ~1 run in 5
crashed, now none do (`/tmp/hunt2/` in the original session).

## Debugging notes (what worked, what misled)

* **Isolate, then bisect in-process.** Instrumenting the *suspected local* and
  comparing its value before/after each step localised the corruption to "between
  `LOOP2-begin` and the first iteration" in one run — cheap and decisive.
* **A pure spin is the fastest way to tell "another thread" from "same thread".**
  Re-reading the value in a tight loop with no calls showed it changing underneath.
* **Print addresses of suspects** (`&state`, `&vector`) — that is what exposed the
  slot reuse.
* **Hardware watchpoints on a stack slot are treacherous.** gdb stops when *any* code
  writes the address, including later calls that legitimately reuse the freed frame;
  arming and disarming around a specific window is mandatory, and a hit inside an
  unrelated chain (e.g. libc `printf`) is usually a stale watchpoint, not the writer.
* **A freed buffer's contents are rewritten by `free()` itself** (free-list pointers),
  so watching a vector's *buffer* only reports allocation churn.

## Invariants to preserve

* A function that pushes tasks to a pool **must join every future before returning on
  every path** — error paths included. `JoinedTasks<T>` exists for that.
* A task group that signals completion must count the producer as a participant, so
  its done-signal cannot fire while more tasks are still being pushed.
* Never hand a *stack* object to a pool task unless the task is joined before that
  frame dies (`ASTProcessor`, `SymbolResolver`, the `ASTAllocator`s,
  `ConcurrentParsingState`, `LabBuildContext` are all stack objects).
