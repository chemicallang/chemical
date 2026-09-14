---
name: Performance Optimization
description: Comprehensive guide to performance optimization patterns in the Chemical compiler — parallelization, allocation strategies, caching, and profiling.
---

# Performance Optimization

The Chemical compiler is designed for speed. This document describes the key performance optimization patterns used throughout the codebase and strategies for further improvements.

## Design Philosophy

1. **Compile-time performance first** — the compiler should be as fast as possible
2. **Parallelize everything** — exploit multi-core CPUs at every pipeline stage
3. **Zero-cost abstractions** — AST nodes are plain C++ structs with virtual dispatch
4. **Arena allocation** — no per-node new/delete, batch deallocation
5. **Minimal copying** — use `chem::string_view`, move semantics, in-place mutation

## 1. Arena Allocation (ASTAllocator)

The most impactful optimization. All AST nodes are allocated from arena allocators:

```cpp
// ASTAllocator derives from BatchAllocator, a pointer-bump arena:
class BatchAllocator {
    std::vector<char*> heap_memory;   // heap batches (configurable size, e.g. 100 KB)
    std::size_t heap_batch_size;      // bytes per batch
    std::size_t heap_offset;          // current position within current batch

    char* reserve_heap_storage();                     // grab a new batch
    char* object_heap_pointer(size_t size, size_t align);
};

class ASTAllocator final : public BatchAllocator {
    template<typename T> T* allocate();                 // typed allocation
    char* allocate_size(size_t obj_size, size_t align); // raw allocation
    void clear();   // frees everything, allocator becomes reusable
};
```

**Benefits**:
- **Fast allocation**: Simple pointer bump (3-10x faster than `malloc`)
- **Cache-friendly**: Contiguous allocation → better cache locality
- **No per-node deallocation**: Entire arena is freed at once
- **No destructor calls**: AST nodes are POD-like (no virtual destructors needed)

**Multiple allocators** serve different lifetimes:

| Allocator | Lifetime | Purpose |
|-----------|----------|---------|
| Global ASTAllocator | Entire compilation session | Long-lived AST modifications |
| Module allocator | Per-module | Module-level declarations |
| File allocator | Per-file | File-level temporary nodes |
| Per-pass allocator | Per-pass | Temporary nodes during a pass |

## 2. Custom String Types

The compiler avoids `std::string` for performance:

```cpp
// chem::string — SSO-optimized string (replaces std::string)
// chem::string_view — lightweight view (replaces std::string_view)
```

**Key properties**:
- **Small String Optimization (SSO)**: Strings up to `STR_BUFF_SIZE` (16) bytes stored inline in the object, no heap allocation
- **No COW**: Copy-on-write is unpredictable; Chemical strings use eager copying
- **Integral hash**: `chem::string_view` can be used as a hash map key without copying

## 3. Parallelization Patterns

### Module-Level Parallelism

Modules are independent and can be processed in parallel:

```
thread 1: Module A (parse → symres → typecheck → codegen)
thread 2: Module B (parse → symres → typecheck → codegen)
thread 3: Module C (parse → symres → typecheck → codegen)
```

### File-Level Parallelism (Current)

Files within a module are symbol-resolved in parallel:

```
For each file F in module M:
    thread 1: F1 symres (Signatures + Bodies)
    thread 2: F2 symres (Signatures + Bodies)
    thread 3: F3 symres (Signatures + Bodies)
```

Each thread gets its own:
- `SymResLinkBody` with per-file `SymbolTable`
- `GenericInstantiatorAPI` with thread-local `active_type_map`
- `ASTDiagnoser` for diagnostic collection

### Generic Instantiation Parallelism (Current)

Multiple instantiations of the same generic are processed with fine-grained locking:

```cpp
// Thread-safe registration (guarded by resolver.generic_inst_reg_mutex):
{
    std::lock_guard lock(resolver.generic_inst_reg_mutex);
    auto existing = container.getInstantiationTypesFor(key);
    if(!existing.empty()) return existing;
    container.registerInstantiation(key, types, implData, current_file_id);
}

// Signature finalization with notify/wait:
thread 1: FinalizeSignature(genDecl, inst1, 0) → notifySignatureFinalized(genDecl, 0)
thread 2: waitSignatureFinalized(genDecl, 0) → FinalizeBody(genDecl, inst1, 0)
```

### Future Parallelization Opportunities

| Stage | Current | Future |
|-------|---------|--------|
| Lexing | Parallel per file (current) | — |
| Parsing | Parallel per file (current) | — |
| Top-level decl | Serial across files | Parallel with scoped locking |
| Link signatures | Parallel per file | Parallel per file (current) |
| Generic instantiation | Parallel per instantiation | More granular parallel |
| Link bodies | Parallel per file | Parallel per function |
| Type verification | Parallel per file (current) | — |
| C codegen | Serial per function | Parallel per function |
| LLVM codegen | Serial per function | Parallel per function (thread-safe contexts) |
| Linking | Serial | Linker can be parallel |

## 4. Caching Strategies

### Object File Cache

The compiler caches compiled object files using a timestamp file written to the build directory:

```
Cache record = { output mode, [(source path, file size, last_write_time), ...] }
Cache hit    = same file set, sizes, mtimes and output mode
Cache value  = object file (.o)
```

See `compiler/lab/timestamp/Timestamp.{h,cpp}` (`save_mod_timestamp` / `compare_mod_timestamp`).

### Generic Instantiation Cache

`InstantiationsContainer` ensures each unique combination of generic args is only processed once:

```cpp
// Key: generic declaration pointer
// Value: declared instantiations (type argument spans + impl data)
std::unordered_map<void*, DeclInstantiations> instantiations;
```

### Module Cache

Compiled modules are cached based on their source timestamps and their dependency DAG:

```
Cache key = module source timestamps + dependency has_changed flags
```

### Cache Invalidation

Invalidation triggers:
- Source file `last_write_time` or size change
- Added/removed source files (file count mismatch)
- Output mode change
- Dependency recompilation (`has_changed` propagation)
- Plugin recompilation (`--frecompile-plugins`)

## 5. Memory Optimization

### Batch Processing

Diagnostics, symbol entries, and other per-pass data are stored in `std::vector` and moved (not copied) between phases:

```cpp
// Move diagnostics — no copy:
main_diagnoser.diagnostics.insert(
    main_diagnoser.diagnostics.end(),
    std::make_move_iterator(phase.diagnostics.begin()),
    std::make_move_iterator(phase.diagnostics.end())
);
```

### Zero-Copy String Handling

Use `chem::string_view` for function arguments and hash map lookups:

```cpp
void declare(const chem::string_view& name, ASTNode* node);  // No string copy
```

### SymbolTable Optimization

The `SymbolTable` uses `chem::string_view` for lookups and stores symbols in a flat `std::vector` with scope nesting markers. No per-symbol heap allocation for names.

### Compact AST Nodes

AST nodes use minimal memory:
- `ASTNodeKind` enum (1 byte) for RTTI
- No virtual table if not needed
- Pointers to children rather than owning containers
- `std::vector` used only for variable-length children

## 6. Compiler Pipeline Timing

### Typical Timing Breakdown

| Phase | Time % | Notes |
|-------|--------|-------|
| Lexing | 5-10% | Fast, single-pass |
| Parsing | 10-15% | Recursive descent |
| Symbol Resolution | 30-40% | Most complex, currently parallel |
| Generic Instantiation | 10-15% | Can be expensive for deeply nested generics |
| Type Verification | 5-10% | Single pass |
| Codegen (C/LLVM) | 20-30% | Depending on backend |
| Linking | 5-10% | OS-dependent |

### Profiling Tips

1. **Benchmark support** in `test.sh` (benchmark flags are `--bm`, `--bm-files`, `--bm-modules`):
   ```bash
   ./scripts/test.sh --tcc --bm
   ```

2. **Timing flags**:
   ```bash
   ./chemical build.lab -v -bm            # per-phase benchmark
   ./chemical build.lab -v -bm-modules    # per-module breakdown
   ```

3. **Manual timing**:
   ```cpp
   auto start = std::chrono::high_resolution_clock::now();
   // ... phase ...
   auto end = std::chrono::high_resolution_clock::now();
   auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
   std::cerr << "Phase took " << ms << "ms" << std::endl;
   ```

## 7. Common Performance Anti-Patterns

| Anti-pattern | Why it's slow | Better approach |
|--------------|---------------|-----------------|
| `std::string` concatenation in hot paths | Heap allocation + copy | Use `chem::string` + `append_view()` |
| `std::unordered_map` with `std::string` keys | String copy for hashing | Use `chem::string_view` key |
| Per-node `new`/`delete` | Heap fragmentation | Use `ASTAllocator` arena |
| Deep copies of AST subtrees | Exponential memory | Use shared pointers or immovable nodes |
| Recursive visitor in deep AST | Stack overflow, cache misses | Use iterative visitor for large trees |
| Virtual function calls in inner loops | vtable dispatch, no inlining | Use CRTP, templates, or direct dispatch |
| Thread contention on shared mutex | Wait time | Use per-thread state and merge results |
| `dynamic_cast` | RTTI overhead | Use `ASTNodeKind` enum + `as_xxx_unsafe()` |
| Copying diagnostics between threads | Memory bandwidth | Move diagnostics, don't copy |

## 8. Key Optimization Files

| File | Optimization |
|------|-------------|
| `ast/base/ASTAllocator.h` | Arena allocation |
| `std/chem_string.h` | SSO-optimized `chem::string` |
| `std/chem_string_view.h` | Zero-copy `chem::string_view` |
| `compiler/symres/SymbolTable.h` | Flat scope vector, string_view keys |
| `compiler/generics/InstantiationsContainer.h` | Generic instantiation deduplication |
| `preprocess/visitors/NonRecursiveVisitor.h` | Iterative AST walking |
| `preprocess/2c/BufferedWriter.h` | Buffered C output |
| `server/utils/LRUCache.h` | LRU cache for LSP operations |
