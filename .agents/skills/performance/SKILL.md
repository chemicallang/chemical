---
name: Performance Optimization
description: Comprehensive guide to performance optimization patterns in the Chemical compiler — arena allocation, string handling, parallelization, caching, memory layout, profiling, and anti-patterns.
---

# Performance Optimization

The Chemical compiler is designed for speed. This document describes the key performance optimization patterns used throughout the codebase and strategies for further improvements.

> **Companion skills:** [Build System (Lab)](../build_system/SKILL.md) covers the
> orchestration/pass pipeline this document optimizes; [AST Framework](../ast_framework/SKILL.md)
> covers the node hierarchy and the arena allocator in depth; [Generics](../generics/SKILL.md)
> covers generic instantiation and its synchronization; [Benchmark Dashboard](../benchmark_dashboard/SKILL.md)
> covers the historical timing data collected in CI. Everything here is grounded in source
> with `file:line` references — verify against those before relying on a claim.

## Design Philosophy

1. **Compile-time performance first** — the compiler should be as fast as possible
2. **Parallelize everything** — exploit multi-core CPUs at every pipeline stage
3. **Virtual dispatch with kind-switch dispatch** — nodes are C++ class hierarchies
   (`ASTAny` declares virtual methods such as `any_kind()`), but hot traversal switches on a
   1-byte `ASTNodeKind`/`ValueKind`/`BaseTypeKind` enum rather than `dynamic_cast`
4. **Arena allocation** — no per-node `new`/`delete`; batch deallocation, but destructors
   **are** run at arena clear (see correction below)
5. **Minimal copying** — use `chem::string_view`, move semantics, in-place mutation
6. **Memory-mapped input** — source files are read via mmap, not copied into buffers
7. **Cache aggressively** — timestamped object/partial-C caches avoid recompiling unchanged modules

## 1. Arena Allocation (ASTAllocator / BatchAllocator)

The most impactful optimization. All AST nodes are allocated from arena allocators.

```cpp
// ast/base/BatchAllocator.h:16 — pointer-bump arena
class BatchAllocator {
    std::vector<char*> heap_memory;                 // :69  batches
    std::size_t heap_batch_size;                    // :73  bytes per batch
    std::size_t initial_heap_batch_size;            // :78  restored by clear()
    std::size_t heap_offset;                        // :83  current position
    std::shared_ptr<std::mutex> allocator_mutex;    // :88  shared across moves
public:
    template<typename T> T* allocate_released();                        // :41
    char* allocate_released_size(size_t obj_size, size_t alignment);    // :49
    char* allocate_str(const char* data, std::size_t size);             // :54
    char* reserve_heap_storage();                                       // :99
    char* object_heap_pointer(std::size_t obj_size, std::size_t alignment); // :104
};

// ast/base/ASTAllocator.h:20 — tracked arena
class ASTAllocator final : public BatchAllocator {
    std::vector<ASTAny*> ptr_storage;               // :97
    std::vector<ASTCleanupFunction> cleanup_fns;    // :105
public:
    template<typename T> FORCE_INLINE T* allocate() {                  // :46
        static_assert(std::is_base_of<ASTAny, T>::value);
        return (T*) (void*) allocate_size(sizeof(T), alignof(T));
    }
    char* allocate_size(std::size_t obj_size, std::size_t alignment);  // :56
    char* allocate_with_cleanup(std::size_t, std::size_t, void* fn);   // :64
    void clear();                                                      // :84
    ~ASTAllocator();                                                   // :89
};
```

### How the bump works

`object_heap_pointer` (`ast/base/ASTAllocator.cpp:128`) is the hot path:

- Alignment must be a power of two (`assert` at `:129`); the next offset is
  `(heap_offset + alignment - 1) & ~(alignment - 1)` (`:131`).
- If the object fits in the current batch (`:133`), the pointer is just
  `heap_memory.back() + aligned_heap_offset` and `heap_offset` is bumped (`:134-136`).
- Otherwise a new batch is reserved (`:144`); a single object **larger than the batch size**
  grows `heap_batch_size` to that object's size first (`:140-143`) so it always fits.
- Allocation is serialized by `allocator_mutex` in both the tracked
  (`allocate_size`, `:153-158`) and released (`allocate_released_size`, `:167-170`) paths.
  A shared `allocator_mutex` (via `std::shared_ptr`, `BatchAllocator.h:88`) means moved/copied
  allocators keep serializing against each other.

`BatchAllocator::allocate_str` (`ast/base/ASTAllocator.cpp:172`) copies bytes plus a NUL into
the arena — used for interned strings and comment tokens (`ASTProcessor.cpp:1392`).

### Destructor tracking — AST nodes are NOT POD

> **Correction to an earlier version of this skill.** A previous revision claimed AST nodes
> are "POD-like (no virtual destructors needed)" with "no destructor calls." That is **wrong**
> and must not be propagated. `ASTObjects` are a C++ class hierarchy: `ASTAny` declares a
> `virtual ~ASTAny()` (`ast/base/ASTAny.h:62`), so every node, value, and type carries a
> vtable. `ASTAllocator` deliberately records every allocated pointer in `ptr_storage` and
> calls `ptr->~ASTAny()` virtually when the arena is cleared:

```cpp
// ast/base/ASTAllocator.cpp:96
void ASTAllocator::destruct_ptr_storage() {
    for (const auto ptr: ptr_storage) {
        ptr->~ASTAny();          // virtual destructor — nodes are not trivially destructible
    }
    ptr_storage.clear();
}
```

Tracked vs untracked entry points:

| Entry point | Tracked? | On `clear()` / destructor |
|-------------|----------|---------------------------|
| `ASTAllocator::allocate<T>()` / `allocate_size()` | Yes, `store_ptr` (`ASTAllocator.h:69`) | Calls `ptr->~ASTAny()` (`ASTAllocator.cpp:96-101`) |
| `ASTAllocator::allocate_with_cleanup()` | Yes, custom `cleanup_fn` (`ASTAllocator.cpp:160-165`) | Runs the registered cleanup (`:103-108`) |
| `BatchAllocator::allocate_released<T>()` / `allocate_released_size()` | No | Memory only; caller must destruct |
| `BatchAllocator::allocate_str()` | No | Memory only; no destructor |

This is why the compiler supports `@delete` destructors on Chemical-defined types: those
destructors are invoked when a node is destroyed at arena clear. It also means raw
`memcpy`/`memset` over an `ASTAllocator`-tracked node is unsafe in general.

### `clear()` — reuse, don't reallocate

`ASTAllocator::clear()` (`ast/base/ASTAllocator.cpp:63`) takes the lock, runs destructors and
cleanups, then keeps the **first** heap block and frees the rest (`:72-76`), resets
`heap_offset = 0` (`:78`) and restores `heap_batch_size = initial_heap_batch_size` (`:81`).
The result is that a per-file allocator can be reused across every pass with almost no
allocation churn — `file_allocator.clear()` is called after every symres pass
(`ASTProcessor.cpp:491`, `:522`, `:554`, `:574`, `:623`).

### Batch sizes and lifetime tiers

| Allocator | Batch size | Lifetime | Source |
|-----------|-----------|----------|--------|
| `global_allocator` | 100 KB | Whole compiler session (`TypeBuilder` flyweights, `nullValue`) | `LabBuildCompiler.cpp:183` |
| `job_allocator` | 100 KB | One build job | `LabBuildCompiler.cpp:3201-3204` |
| `mod_allocator` | 100 KB | One module | `LabBuildCompiler.cpp:3202-3205` |
| `file_allocator` | 100 KB | One file; cleared/reused between passes | `LabBuildCompiler.cpp:3203-3206` |
| LSP/workspace allocator | 10 KB | Editor workspace/analysis | `server/build/Build.cpp:86`, `server/WorkspaceManager.cpp:42` |
| Ad-hoc parser allocator | 10 KB | Build.lab/mod parsing | `LabBuildCompiler.cpp:2401` |

`ptr_storage`/`cleanup_fns` reserve `PTR_VEC_SIZE = 1000` up front
(`ast/base/ASTAllocator.h:110`, constructor `ASTAllocator.cpp:22-23`) so the common case
never reallocates the tracking vectors.

**Implications:**

- **Fast allocation**: simple pointer bump — 3-10x faster than `malloc` for small objects.
- **Cache-friendly**: contiguous allocation improves locality.
- **Freeing is O(blocks) + O(tracked nodes)**, not O(live nodes): the whole arena is freed at
  once, but `clear()` still walks `ptr_storage` to run destructors.
- **Multiple allocators serve different lifetimes** (table above) — never hold a raw
  `ASTAny*` past the `clear()` of the allocator that owns it.

## 2. Custom String Types (`chem::string` / `chem::string_view`)

The compiler avoids `std::string` on hot paths.

```cpp
// std/chem_string.h:12
constexpr int STR_BUFF_SIZE = 16;

// std/chem_string.h:17 — tagged union of three representations
struct string {
    union {
        struct { const char* data; size_t length; } constant;             // '0' — non-owning view
        struct { char* data; size_t length; size_t capacity; } heap;      // '2' — owned heap
        struct { char buffer[STR_BUFF_SIZE]; unsigned char length; } sso; // '1' — inline
    } storage;
    char state;
};
```

**Key properties:**

- **Small String Optimization (SSO)**: strings shorter than `STR_BUFF_SIZE` (16) live inline
  (`state == '1'`); literals are non-owning views (`state == '0'`, `chem_string.h:57-69`).
  `move_const_to_buffer` promotes a literal into SSO without a heap allocation (`:162-173`),
  and only promotes to heap (`state == '2'`) when it outgrows the buffer (`:175-187`).
- **No COW**: copy-on-write is unpredictable; Chemical strings copy eagerly (`copy()`/`substring`).
- **Move semantics**: `_own` steals the heap pointer and nulls the source (`:79-99`);
  `operator=` frees an existing heap buffer then steals (`:105-111`).
- **Heterogeneous hashing**: `std::hash<chem::string>` is a hand-rolled FNV-1a with
  `is_transparent = void` and matching `std::equal_to`, so a `chem::string_view` can be looked
  up in a map keyed by `chem::string` without constructing a `std::string`
  (`std/chem_string.h:551-583`). `chem::string::to_chem_view()` (`:466`) provides the view.
- **`chem::string_view`** is the zero-copy argument/lookup type used throughout
  (`std/chem_string_view.h`); prefer `const chem::string_view&` parameters.

### Move-only, non-copyable

`chem::string(string&)` is deleted (`std/chem_string.h:77`) — a `chem::string` must be moved,
not copied implicitly. This is intentional: it forces call sites to be explicit about
ownership and prevents accidental full-string copies in hot loops.

## 3. Parallelization Patterns

### The thread pool

The compiler uses the header-only `ctpl::thread_pool` (`lib/ctpl/ctpl.h:71`) with
`std::thread::hardware_concurrency()` workers by default
(`compiler/lab/LabBuildCompiler.cpp:190`); an explicit count can be passed to the constructor
(`LabBuildCompiler.h:107`, `LabBuildCompiler.cpp:175-184`). Tasks are submitted with
`pool.push(fn, args...)` which returns a `std::future` (`lib/ctpl/ctpl.h:173`). The pool uses
a mutex-guarded queue and a condition variable (`ctpl.h:238-246`).

### Module-Level Parallelism

Modules are independent and can be processed in parallel conceptually, though the current
driver processes a job's module DAG sequentially per module while parallelizing *within* a
module. Remote imports are downloaded in parallel **waves**
(`LabBuildCompiler.cpp:4755-4777`): each wave submits `download_remote_import` through
`pool.push` (`:4766`) and joins on the futures.

### File-Level Parallelism (Current)

Importing (lex + parse) and most symbol-resolution passes fan out **per file**:

| Stage | Parallel? | Site |
|-------|-----------|------|
| Lex + parse direct files | Yes | `ASTProcessor.cpp:709-752` (`pool.push` at `:737`) |
| Recursive imports | Yes, with deadlock avoidance | `ASTProcessor.cpp:804-864` (`:856`) |
| Top-level declare | **No** (serial) | `ASTProcessor.cpp:475-488` |
| Link signatures | Yes | `ASTProcessor.cpp:499-511` (`:501`) |
| Generic instantiation | Yes | `ASTProcessor.cpp:533-543` (`:535`) |
| After-link-signature | **No** (serial) | `ASTProcessor.cpp:559-571` |
| Link generic-decl bodies | Yes | `ASTProcessor.cpp:581-591` (`:583`) |
| Full link bodies | Yes | `ASTProcessor.cpp:602-612` (`:604`) |
| Type verification | Yes | `ASTProcessor.cpp:775-802` (`:781`) |
| C translation (2c) | **No** — one mutable `ToCAstVisitor` | `LabBuildCompiler.cpp` `process_module_tcc` |

Each parallel unit gets its own `ASTDiagnoser`, then results are merged with `print_mutex`
guards (`ASTProcessor.h:122`, used at `ASTProcessor.cpp:504`, `:538`, `:586`, `:607`, `:794`).

**Why top-level declare is serial:** declarations mutate the shared `SymbolTable` and must run
in source order to keep private-symbol ranges consistent. Link-signature and later passes build
per-file `SymbolTable`s layered on top (see [Symbol Resolution](../symres/SKILL.md)).

**Deadlock avoidance in recursive imports:** when already inside a pool task, imports are
processed sequentially in the current thread rather than pushed to the pool
(`ASTProcessor.cpp:846-860`), because pushing nested tasks that wait on sub-tasks can exhaust
the pool. Completion is tracked with `ConcurrentParsingState`, whose `outstanding` counter and
`all_done_promise` use `std::atomic` and a `std::promise` (`ASTProcessor.h:56-75`).

### Generic Instantiation Parallelism

Registration of generic instantiations is guarded by a **recursive** mutex
`SymbolResolver::generic_inst_reg_mutex` (`compiler/symres/SymbolResolver.h:150`), locked in
`GenericInstantiator.cpp:673` and `LinkSignature.cpp:733`.

Signature finalization is coordinated with a **single shared condition variable and status
mutex** across all generic declarations (`InstantiationsContainer.h:64-67`):

```cpp
// compiler/generics/GenericInstantiator.cpp:704
void GenericInstantiator::waitSignatureFinalized(BaseGenericDecl* decl, size_t index) {
    auto& status_mutex = container.getInstantiationStatusMutex();
    auto& cv = container.getInstantiationCv();
    std::unique_lock<std::mutex> lock(status_mutex);
    // self-deadlock guard: the thread that registered this instantiation must not wait on itself
    if(decl->instantiation_statuses[index].builder_thread == std::this_thread::get_id()) {
        return;
    }
    cv.wait(lock, [decl, index]() {
        return decl->instantiation_statuses[index].status == InstantiationStatus::SignatureFinalized;
    });
}
// :724 notifySignatureFinalized sets the status then cv.notify_all();
```

The `builder_thread` field (`ast/structures/BaseGenericDecl.h:26`) records which thread must
finalize a given instantiation, preventing recursive generic types (e.g. `JsonValue` containing
`std::vector<JsonValue>`) from deadlocking against themselves (`GenericInstantiator.cpp:709-718`).

`InstantiationsContainer` also **deduplicates** instantiations by key, so each unique type-argument
combination is finalized once (`InstantiationsContainer.h:56`, `registerInstantiation` `:97`), and
tracks per-file registrations so a file edit can invalidate exactly its own instantiations
(`removeInstantiationsFor(unsigned int fileId)`, `:158`). See [Generics](../generics/SKILL.md).

### Serialization points to watch

- `print_mutex` (`ASTProcessor.h:122`) — diagnostic printing; lock held only around the print.
- `import_mutex` (`ASTProcessor.h:117`) — guards the `ASTFileResult` cache during recursive imports.
- `mod_storage_mutex` (`LabBuildCompiler.h:117`) — guards `ModuleStorage`.
- `download_mutex` + `download_cv` (`LabBuildCompiler.h:122-124`) — dedupe concurrent downloads of the same directory.
- `generic_inst_reg_mutex` (`SymbolResolver.h:150`) — recursive; held briefly.
- `inst_status_mutex` + `instantiation_cv` (`InstantiationsContainer.h:64-67`) — condvar wait path.
- `ImplementationsIndex::index_mutex` — a `std::shared_mutex` (shared reads / unique writes).
- `CTranslator::translation_mutex` (`compiler/ctranslator/CTranslator.h:61`) — C translation is a
  single mutable translator; it is not run concurrently, so it does not scale across files.

### Future Parallelization Opportunities

| Stage | Current | Future |
|-------|---------|--------|
| Lexing | Parallel per file (current) | — |
| Parsing | Parallel per file (current) | — |
| Top-level decl | Serial across files | Parallel with scoped locking |
| Link signatures | Parallel per file | More granular |
| Generic instantiation | Parallel per instantiation | More granular parallel |
| Link bodies | Parallel per file | Parallel per function |
| Type verification | Parallel per file (current) | — |
| C codegen | Serial per module/function | Parallel per function (needs per-thread visitor state) |
| LLVM codegen | Serial per module/function | Parallel per function (thread-safe contexts) |
| Linking | Serial | Linker can be parallel |

## 4. Caching Strategies

### Object / partial-C cache

The compiler caches generated artifacts per module and validates them with a **timestamp file**
written to the build directory:

```
Cache key = output mode + [(absolute source path, file size, last_write_time), ...]
Cache hit = same file count, same mode, and every file's size + mtime match
Cache value = object file (.o) or partial C/header files
```

Implementation: `save_mod_timestamp` / `compare_mod_timestamp` in
`compiler/lab/timestamp/Timestamp.cpp`. `save` sorts files for deterministic order, then writes
count, mode, and per-file `(size, mtime)` (`:14-39`); `compare` checks count and mode, then
stats each file and compares size + mtime with a fast size-mismatch short-circuit
(`:59-106`). Timestamp paths are `timestamp.dat` (LLVM/obj) or `timestamp_tcc.dat` (TCC)
under the module's build dir (`LabBuildCompiler.cpp:426-431`).

What gets cached:

- **Multi-file (`!single_file`)**: the module **object file** (`.o`) and a
  `partial.2h.c` declarations/header snippet (`get_partial_h_path`, `:440`).
- **Single-file**: the module's C body snippet `partial.2c.c` (`get_partial_c_path`, `:434`),
  appended into the merged translation unit.
- **Fully translated C**: `Translated.c` (`get_translated_c_path`, `:446`).

`has_module_changed` (`LabBuildCompiler.cpp:496-511`) returns `true` immediately when caching is
disabled, otherwise `has_module_changed_recursive` (`:441-493`): it checks the cached artifact
exists, recursively marks the module changed if any **dependency** changed, then consults the
timestamp file. Dependency invalidation propagates through `LabModule::has_changed`
(`:489-492`). On a hit, `process_cached_module` (`:680`) marks structs/variants as already
declared and instantiations as generated so they are not emitted twice, and the module's
`mod_allocator` is cleared (`:924-933`).

### `ASTFileResult` cache

Parsed files are stored by file id in
`std::unordered_map<unsigned int, std::unique_ptr<ASTFileResult>> cache`
(`compiler/ASTProcessor.h:128`). Recursive imports check this map under `import_mutex`
(`ASTProcessor.cpp:823-844`) so a file reached by multiple import paths is parsed once.

### `ModuleStorage`

`ModuleStorage` (`compiler/lab/ModuleStorage.h:10`) owns every `LabModule` (`:16`) and an
`std::unordered_map<std::string, LabModule*>` index keyed by `"scope:module"`
(`:22`, `build_format` `:26`, `find_module` `:72`, `insert_module` `:56`). This is lookup
caching for import resolution, not on-disk caching.

### Cache flags

| Flag | Effect | Source |
|------|--------|--------|
| `--no-cache` | Sets `is_caching_enabled = false`; `has_module_changed` always returns true | `core/main/CompilerMain.cpp:545`/`:812`, `LabBuildCompiler.cpp:498-505` |
| `--cache` | Test-harness opt-in; scripts default to `--no-cache` | `scripts/test.sh` |
| `--frecompile-plugins` | Sets `force_recompile_plugins = true`; disables CBI plugin cache | `CompilerMain.cpp:545-546`, `LabBuildCompiler.cpp:1617` |
| `--cached-plugins` | Test-harness flag that removes `-frecompile-plugins` (skip CBI recompile) | `scripts/test.sh:35`/`:114` |

Job caching is per-job-type (`LabBuildCompiler.cpp:1617`, `:1918`):

```cpp
const auto job_caching = !job->attrs.check_only
    && (is_job_cbi ? (options->force_recompile_plugins == false) : options->is_caching_enabled);
```

The **build.lab** itself is JIT-compiled and cached as an object file; its timestamp is saved
after successful compilation (`LabBuildCompiler.cpp:3116`) and the cached object is relocated
at next launch (`:2806-2937`). Remote-import downloads live in a centralized commands cache dir
(`get_commands_cache_dir`, `:3520`).

### Cache invalidation summary

- Source `last_write_time` or size change (`Timestamp.cpp:100`).
- Added/removed source files (file-count mismatch, `Timestamp.cpp:65`).
- Output mode change (`Timestamp.cpp:69`).
- Dependency recompilation (`has_changed` propagation, `LabBuildCompiler.cpp:483-492`).
- Plugin recompilation (`--frecompile-plugins`).

## 5. Memory Optimization

### Zero-copy input via mmap

`FileInputSource` memory-maps source files (`stream/FileInputSource.h:74`) with a heap fallback
buffer only if mapping fails (`:89`). The lexer reads directly from the mapped bytes, so source
text is never copied into an intermediate buffer. **Watch out:** token values are
`chem::string_view`s into that mapping; that is why comment tokens are copied onto the
`job_allocator` when persistent tokens are requested (`ASTProcessor.cpp:1388-1396`).

### Move diagnostics, don't copy

Diagnostics and per-pass results are moved (not copied) between phases:

```cpp
// ASTProcessor.cpp:1147-1149
auto moved1 = std::move(result.lex_diagnostics);
auto moved2 = std::move(result.parse_diagnostics);
```

`type_verify_file_task` likewise `std::move`s the local `ASTDiagnoser`'s diagnostics into the
result (`ASTProcessor.cpp:771`), and the main diagnoser appends moved ranges.

### Arena strings for long-lived token text

When tokens must outlive the mmap (e.g. LSP/semantic tokens), the compiler allocates the text on
the arena with `allocate_str` (`ASTProcessor.cpp:1392`). Long-lived name strings use
`allocate_released_size` on the `job_allocator` (`LabBuildCompiler.cpp:3055`), which is a raw
bump with no destructor tracking — appropriate for `char` data.

### TypeBuilder flyweight + pointer-identity types

`TypeBuilder` (`ast/base/TypeBuilder.h:13`) allocates exactly one instance of each primitive and
shared type once, handing out stable pointers. `GlobalBaseType` therefore compares by pointer
identity and returns `this` from `copy()` (`ast/base/BaseType.h:781-800`), avoiding both
allocation and deep comparison for primitives.

### Flat scope storage

The interpreter's `ScopeValueMap` is an insertion-ordered flat vector that only builds a hash
index past `INDEX_THRESHOLD = 12` (`ast/base/ScopeValueMap.h:32`, `:39`), avoiding a heap node
per variable in hot interpreter loops.

### `chem::string` vs `std::string`

Use `chem::string` for owning compiler strings (SSO, move-only, FNV hash) and
`chem::string_view` for parameters/keys. `std::string` still appears at boundaries (filesystem
paths, CLI options, error formatting) where the standard library API wins.

### Field budget on hot nodes (measured)

High-frequency AST nodes are allocated once per occurrence in *every* module, so a field that
only a rare syntactic form uses is pure tax. Real example: a `std::vector<TypeLoc>` field
added to `VariableIdentifier` for the `ident<int>` generic-function-reference feature grew
**every** identifier by 24 bytes — 96 → 72 once it moved to a dedicated node. The vector was
only ever non-empty for a handful of identifiers in a whole build.

`gdb` prints the exact layout, including padding waste, from a debug binary — no rebuild or
instrumentation needed:

```bash
gdb -batch -ex 'print sizeof(VariableIdentifier)' \
           -ex 'ptype /o VariableIdentifier' cmake-build-debug/TCCCompiler
```

`ptype /o` marks holes (`XXX  2-byte hole`, `XXX  7-byte padding`), which is how you spot that
a field ordering is costing more than its declared size. Identifiers are also allocated
number-of-chain-segments times (3 per `a.b.c`), so the multiplier is larger than "one per
expression". See [AST Framework → Transparent wrapper values](../ast_framework/SKILL.md) for
the pattern that avoids the tax: keep the hot node lean and give the rare form its own node.

### Compact AST nodes

AST nodes use a 1-byte kind enum for discrimination:

- `ASTAnyKind`, `ASTNodeKind`, `ValueKind`, `BaseTypeKind` are `uint8_t` enum classes.
- Pointers to children rather than owning containers; `std::vector` only for variable-length
  children.
- **Note:** the earlier claim "No virtual table if not needed" was misleading — every
  `ASTAny`-derived type has a vtable because `ASTAny` declares virtuals (`ast/base/ASTAny.h:33`,
  `:62`). The optimization is that **dispatch in visitors uses the kind enum**, not `dynamic_cast`
  or virtual `accept()` (see [AST Framework](../ast_framework/SKILL.md)).

### `vector<T>` vs pointer vectors

Prefer vectors of values where possible to keep accesses contiguous and avoid pointer chasing.
Where nodes must be addressed stably across passes, pointer vectors (`std::vector<ASTNode*>`,
`ptr_storage`) are used deliberately. Note that for Chemical `vector<T>` with destructible `T`
(strings/structs), `get(i)` returns a **bitwise copy** and can double-free; use `get_ptr(i)`
(see `AGENTS.md`).

## 6. Compiler Pipeline Timing

### Where time goes (per pipeline phase)

Indicative shares for a large project; exact numbers depend on generic density, backend, and
cache state. Per-module timings are collected in CI (see [Benchmark Dashboard](../benchmark_dashboard/SKILL.md)).

| Phase | Time % | Parallel? | Notes / site |
|-------|--------|-----------|--------------|
| Lexing | 5-10% | Per file | Single-pass, reads mmap |
| Parsing | 10-15% | Per file | Recursive descent; `ASTProcessor.cpp:737` |
| Top-level declare | 5-10% | Serial | Symbol table mutation; `ASTProcessor.cpp:475-488` |
| Link signatures | 15-20% | Per file | Type-heavy; `ASTProcessor.cpp:499-511` |
| Generic instantiation | 10-20% | Per file + condvar | Expensive for nested generics; `:533-543` |
| Link bodies | 15-20% | Per file | Most complex pass; `:602-612` |
| Type verification | 5-10% | Per file | `:775-802` |
| Codegen (C/LLVM) | 20-35% | **Serial** | Translator/Codegen is shared mutable state |
| Linking | 5-10% | Serial | OS/linker dependent |

Caching removes most of the above for unchanged modules (section 4), so incremental builds are
dominated by linking and the changed module's symres/codegen.

### Profiling the compiler

1. **Built-in phase benchmarks** (CLI flags registered in `core/main/CompilerMain.cpp`, wired via
   `LabBuildCompilerOptions`):
   - `-bm` / `--benchmark` — overall job timing.
   - `-bm-files` / `--benchmark-files` — per-file lexer/parser and per-file symres passes
     (`ASTProcessor.cpp:75-86`, `:219-226`, `:236-244`, `:259-267`, `:341-349`, `:357-365`).
   - `-bm-modules` / `--benchmark-modules` — per-module compilation timing
     (`LabBuildCompiler.cpp:745-785`, printing `bm:module`).

   ```bash
   ./chemical build.lab -v -bm            # per-phase/job benchmark
   ./chemical build.lab -v -bm-modules    # per-module breakdown
   ./scripts/test.sh --tcc --bm           # benchmark test build
   ```

2. **`process_module_tcc_bm`** wraps `process_module_tcc`, starting a `BenchmarkResults` before
   and printing after (`LabBuildCompiler.cpp:745-785`). `BenchmarkResults` uses a monotonic
   start/end and a `representation()` helper (`utils/Benchmark.h:8-24`).

3. **Manual timing** around a suspected region:

   ```cpp
   auto start = std::chrono::high_resolution_clock::now();
   // ... phase ...
   auto end = std::chrono::high_resolution_clock::now();
   auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
   std::cerr << "Phase took " << ms << "ms" << std::endl;
   ```

4. **Debug file shuffling** to expose order-dependent bugs (not a perf tool, but relevant when a
   timing regression coincides with ordering): DEBUG builds shuffle files and print
   `File order seed: N`; set `FILE_ORDER_SEED=N` to reproduce
   (`ASTProcessor.cpp:88-131`, `:180-185`).

5. **Historical/module-level data**: the benchmark dashboard ingests per-module compile times
   across TCCCompiler/LLVM/Interpreter. See [Benchmark Dashboard](../benchmark_dashboard/SKILL.md)
   for the `bench-*.sh` collection scripts and the module view.

6. **Sanitizers** for correctness/perf investigations: `--sanitize=address|thread|undefined|...`
   and `--tsan` (registered in `CompilerMain.cpp`, see [Build System](../build_system/SKILL.md)).
   ThreadSanitizer is the fastest way to catch the races described in section 9.

## 7. Common Performance Anti-Patterns

| Anti-pattern | Why it's slow | Better approach |
|--------------|---------------|-----------------|
| `std::string` concatenation in hot paths | Heap allocation + copy | Use `chem::string` + `append_view()` |
| `std::unordered_map` with `std::string` keys | String copy for hashing | Use `chem::string_view` key with transparent hash |
| Per-node `new`/`delete` | Heap fragmentation | Use `ASTAllocator` arena |
| Deep copies of AST subtrees | Exponential memory | Use shared pointers or immovable nodes |
| Recursive visitor in deep AST | Stack overflow, cache misses | Use iterative visitor for large trees |
| Virtual dispatch in inner loops | vtable indirection | Switch on `ASTNodeKind`/`ValueKind` enum |
| Thread contention on a shared mutex | Wait time | Per-thread state + merge; scope locks tightly |
| `dynamic_cast` | RTTI overhead | Use kind enum + `as_xxx_unsafe()` |
| Copying diagnostics between threads | Memory bandwidth | `std::move` diagnostics, don't copy |
| Sharing one `ASTAllocator` across threads | Serializes on `allocator_mutex` | Per-file allocators |
| Holding `print_mutex` across work | Serializes workers | Lock only around the actual print |
| Touching `ptr_storage` with raw memcpy | Skips/duplicates virtual destructors | Let `clear()` destruct, or use `allocate_released` |
| Recomputing primitives per file | Extra allocation | `TypeBuilder` flyweights |
| Copying mmap token text unnecessarily | Copies source bytes | Keep views; arena-copy only when outliving the map |

## 8. Gotchas: Thread Safety, False Sharing, Arenas

1. **Sharing one allocator serializes threads.** `allocate_size` locks `allocator_mutex`
   (`ASTAllocator.cpp:153-158`). Per-file work should use the per-file allocator; the shared
   `global_allocator` is for immortal flyweights only.
2. **`file_allocator.clear()` invalidates anything allocated on it.** A `Value*`/`ASTNode*`
   created during a pass must not survive into the next pass unless moved to `mod_allocator`
   or `job_allocator` (see the arena lifetime tiers). This is a correctness *and* a perf bug
   source: stale pointers cause crashes, over-provisioning allocators wastes memory.
3. **`ptr_storage` grows unbounded within a pass.** Each tracked allocation appends a pointer;
   a pass that allocates hundreds of thousands of small nodes grows the tracking vector
   (reallocation copies pointers). `clear()` resets it. Batch-allocate where possible.
4. **Virtual destructors mean arena clear is not free.** Do not assume `clear()` is O(1); it is
   O(tracked nodes + blocks). For a huge file allocator, expect destructor time at pass end.
5. **Condition-variable waits are global.** `InstantiationsContainer` has a single
   `instantiation_cv` and `notify_all` wakes every waiter (`GenericInstantiator.cpp:731`) — a
   potential thundering-herd/contention point under heavy generic parallelism. Profile with
   `-bm-files` before optimizing.
6. **Recursive mutexes hide lock ordering.** `generic_inst_reg_mutex` is recursive
   (`SymbolResolver.h:150`), so re-entrant registration does not deadlock, but it also means the
   critical section can be entered transitively. Keep registration work inside the lock minimal.
7. **False sharing.** Per-thread counters should be padded or private; `ConcurrentParsingState`
   uses `std::atomic<int> outstanding` and `std::atomic_bool has_errors`
   (`ASTProcessor.h:56-75`) which are cheap but still shared cache lines. Prefer per-task locals
   merged at join time.
8. **Global allocators never shrink.** `global_allocator` grows across the whole session
   (`LabBuildCompiler.cpp:183`); large one-off parses should use a dedicated allocator that is
   cleared (e.g. the 10 KB parser allocator at `:2401`).
9. **`std::vector<char*>` batches are never coalesced.** `reserve_heap_storage` appends
   (`ASTAllocator.cpp:115-123`); `clear()` deletes all but block 0. A pathological large object
   can bump `heap_batch_size` up (via `object_heap_pointer` `:140-143`) so subsequent batches are
   unnecessarily large until `clear()` restores the initial size (`:81`).
10. **Imports inside pool tasks must not be pushed.** Pushing nested tasks that wait on subtasks
    can exhaust the pool; the recursive importer processes them inline when `in_task` is true
    (`ASTProcessor.cpp:846-860`).

## 9. Concrete Optimization Patterns (Before → After)

### 9.1 Per-file state instead of shared allocator

**Before:** a pass allocates on the shared `global_allocator` from every worker → every
allocation serializes on `allocator_mutex`.
**After:** allocate per-file nodes on `file_allocator`, clear it after the pass
(`ASTProcessor.cpp:491`, `:522`), and move only the surviving declarations to `mod_allocator`.

### 9.2 String keys without copies

**Before:** `std::unordered_map<std::string, Symbol> table; table.find(std::string(name));`
constructs a temporary `std::string` per lookup.
**After:** hash with `std::hash<chem::string>` (`is_transparent`, FNV) and `chem::string_view`
keys so `find(view)` allocates nothing (`std/chem_string.h:551-583`).

### 9.3 Kind switch instead of `dynamic_cast`

**Before:** a visitor doing `if(auto* f = dynamic_cast<FunctionCall*>(v))` per value.
**After:** `switch(v->kind()) { case ValueKind::FunctionCall: ... }` plus
`as_function_call_unsafe()`. The CRTP `NonRecursiveVisitor` dispatcher already does this
(`preprocess/visitors/NonRecursiveVisitor.h:737`; see [AST Framework](../ast_framework/SKILL.md)).

### 9.4 Tightening the print critical section

**Before:** taking `print_mutex` before computing diagnostics.
**After:** compute into a local `ASTDiagnoser`, then take `print_mutex` only around
`Diagnoser::print_diagnostics` (`ASTProcessor.cpp:503-506`, `:793-796`) — matching the
`type_verify_file_task` pattern (`:759-772`).

### 9.5 Avoiding deep copy in generics

**Before:** deep-copying a generic body per instantiation.
**After:** keep a "master implementation" and a per-instantiation rewrite via
`GenericInstantiator` / `active_type_map`, deduplicating by key in `InstantiationsContainer`
(`InstantiationsContainer.h:97`). See [Generics](../generics/SKILL.md).

### 9.6 Cache-key discipline

**Before:** invalidating every module on any edit (full rebuild).
**After:** per-module timestamp files keyed by sorted `(path,size,mtime)` plus recursive
dependency `has_changed` propagation (`Timestamp.cpp:14-39`, `LabBuildCompiler.cpp:441-511`).

## 10. Key Optimization Files

| File | Optimization |
|------|-------------|
| `ast/base/ASTAllocator.h` / `.cpp` | Tracked arena: bump allocation + virtual-destructor tracking |
| `ast/base/BatchAllocator.h` | Pointer-bump batches, shared mutex, `allocate_released`/`allocate_str` |
| `std/chem_string.h` | SSO `chem::string`, move-only ownership, FNV transparent hash |
| `std/chem_string_view.h` | Zero-copy `chem::string_view` |
| `stream/FileInputSource.h` | Memory-mapped source input |
| `compiler/symres/SymbolTable.h` | Flat scope vector, `string_view` keys |
| `compiler/generics/InstantiationsContainer.h` | Instantiation dedup + status mutex/condvar |
| `compiler/generics/GenericInstantiator.cpp` | Monomorphization wait/notify (`:704`, `:724`) |
| `compiler/ASTProcessor.cpp` | Per-file parallel pass orchestration + benchmarks |
| `compiler/ASTProcessor.h` | `ConcurrentParsingState`, file cache, `print_mutex` |
| `compiler/lab/LabBuildCompiler.cpp` | Thread pool, module cache, partial-C/obj caching |
| `compiler/lab/timestamp/Timestamp.cpp` | Timestamp cache keys |
| `compiler/lab/ModuleStorage.h` | Fast module lookup index |
| `lib/ctpl/ctpl.h` | Thread pool implementation |
| `utils/Benchmark.h` | Phase/module timing |
| `preprocess/visitors/NonRecursiveVisitor.h` | Kind-switch dispatch (no RTTI) |
| `preprocess/2c/BufferedWriter.h` | Buffered C output |
| `ast/base/TypeBuilder.h` | Primitive-type flyweights |
| `ast/base/ScopeValueMap.h` | Flat interpreter scope storage |
| `server/utils/LRUCache.h` | LRU cache for LSP operations |

## Related Skills

- [Build System (Lab)](../build_system/SKILL.md) — pass pipeline, job types, CLI flags, allocator strategy
- [AST Framework](../ast_framework/SKILL.md) — `ASTAllocator`/`BatchAllocator` internals, node layout, dispatch
- [Generics](../generics/SKILL.md) — monomorphization, registration mutex, signature finalization
- [Benchmark Dashboard](../benchmark_dashboard/SKILL.md) — per-module/per-release timing collection and views
- [Symbol Resolution](../symres/SKILL.md) — the parallel phases being optimized
- [Diagnostics](../diagnostics/SKILL.md) — parallel-safe diagnostic collection and merging
