# MIR Implementation Plan (v2)

Status: implementation plan (derived from mir-design.md)
Constraint: compilation speed is the primary non-functional requirement — MIR must not be a bottleneck
Revision: v2 — addresses contradictions, gaps, and missing details from v1 review

---

## Revision Notes: What Changed From v1

| Issue | v1 Problem | v2 Fix |
|-------|-----------|--------|
| Assignment ordering | MIR showed wrong order (load old → store new → destroy old) | Corrected to: eval RHS → destroy old → store new (matches `2cASTVisitor.cpp:1916-1939`) |
| Stage 6/7 coupling | Listed as independent PRs but deeply coupled | Merged into single stage; control flow without cleanup is unusable |
| `MIRArray<T>` undefined | Referenced but never defined | Defined as `{T* ptr; uint32_t len; uint32_t cap;}` with arena-backed growth |
| No address-of in Stage 3 | Stage 3 excluded pointer ops but function calls need `&raw` for reference params | Added `address_of`, `gep`, `field_addr` to Stage 3 |
| Cleanup scope details | Vague "push/pop" with no data structure | Full data structure: `CleanupScope { PlaceId[] owned; BlockId unwind; uint32_t parent; }` |
| Lambda handling missing | Lambdas not mentioned anywhere | Added Stage 3.5: lambda lowering with capture struct representation |
| Module-level declarations | Only function bodies addressed | Added Stage 2.5: `MIRModuleBuilder` for types, prototypes, globals |
| `comptime` integration | Not addressed | Explicit: comptime remains AST-interpreted, zero MIR output |
| `LegacyCFragment` bridge | Design doc requires it, plan ignored it | Added §3.5: fragment escalation rules |
| `MIRModuleContext` contents | Vague "immutable context" | Enumerated: 12 specific fields with types |
| Error handling | Not addressed | Added §3.6: lowerer error protocol |
| PR dependency graph | Claimed "independently shippable" (false) | Corrected: explicit dependency DAG |
| Type table construction | Not explained when/how | Added §2.6: type builder integration |
| Struct-return nesting | Not addressed | Added §6.8: nested sret handling |
| C module emitter | Only function emitter defined | Added §8.2: `CModuleEmitter` design |
| Interpreter + comptime | Not addressed | Explicit: comptime stays on AST interpreter |
| Drop flag representation | Mentioned but undefined | Defined as `PlaceId` pointing to a `bool` alloca |
| Move-path trees | Design doc requires them, plan ignores | Added §2.8: move-path tree representation |
| `assign_statement` correctness | Plan's MIR ordering would cause use-after-free | Corrected with detailed step-by-step |

---

## 0. Guiding Principles

1. **Speed first.** Every data structure and algorithm choice is evaluated against
   `debug_quick` compilation time. If a pass does not pay for itself in correctness
   or required semantics, it does not exist in the quick path.

2. **Incremental adoption.** Each stage is independently testable, reversible, and
   shippable. The legacy 2c and LLVM paths remain the default until MIR proves
   faster or equal on the full test suite.

3. **No shared mutable state in hot paths.** Every parallel worker owns its arena,
   builder, emitter, and diagnostics. Module-level tables are immutable after
   sealing.

4. **Typed construction, not post-hoc verification.** The builder API makes invalid
   MIR difficult to construct. The verifier is a safety net, not the primary
   correctness mechanism.

5. **One lowering pass.** Semantic decisions (overload resolution, implicit
   constructors, receiver placement, destructor scheduling) are made once during
   AST-to-MIR lowering. Backends consume MIR, not AST.

6. **Preserve existing evaluation order.** The current 2c visitor already computes
   correct evaluation order for assignments, calls, and constructors. MIR must
   produce identical order. The lowerer is validated by differential testing against
   the legacy 2c output.

---

## 1. Performance Budget

The MIR layer must fit within the existing compilation time envelope. The target
is: MIR construction + C emission ≤ direct 2c translation time for `debug_quick`.

### 1.1 Why this is achievable

The current 2c path already does most of the work MIR needs:
- `nested_value`, `current_assignable`, `local_allocated`, `destructible_refs`
  are implicit MIR-like state managed ad-hoc in `ToCAstVisitor`.
- The destructor scheduling in `CDestructionVisitor` is already a mini-CFG.
- Expression compaction in the emitter already does local use-count analysis.

MIR formalizes this state into a typed representation. The overhead is:
- Arena allocation for MIR instructions (cheaper than per-node heap allocation).
- ID assignment (incrementing counters, no hash maps).
- Block construction (appending to a vector, same as current statement emission).

The savings are:
- No `std::unordered_map<Value*, string>` per function (replaced by ID-indexed
  vectors).
- No re-traversal of AST for destructor scheduling (MIR carries explicit drop
  instructions).
- No `BufferedWriter` abort-on-failure (MIR uses fallible emission).
- Parallel function lowering without shared `ToCAstVisitor` state.

### 1.2 Allocation budget

| Component | Strategy | Size |
|-----------|----------|------|
| MIR instructions | Per-function bump arena | 32-64 KiB inline, geometric growth |
| MIR operands | Contiguous array in function arena | Packed with instructions |
| Value/Place tables | Dense ID-indexed vectors | Sized by AST node estimate |
| C emission buffer | Per-function, inline + chunks | 16-64 KiB inline |
| Module tables | Sealed immutable, allocated once | Small, long-lived |
| Diagnostics | Per-worker scratch, moved to coordinator | Small |

No `std::unordered_map` in the instruction hot path. No `std::string` per
instruction. No heap allocation per instruction.

### 1.3 Measurement targets

Track per-module:
```
parse_time
symres_time
typeverify_time
mir_lower_time
mir_verify_time (when enabled)
mir_emit_time
module_merge_time
backend_time (TinyCC/LLVM)
total_time
```

And per-function (in benchmark mode):
```
ast_nodes_lowered
mir_instructions_created
mir_operands_created
mir_arena_bytes_used
c_bytes_emitted
```

---

## 2. Data Structures

### 2.1 MIR Bump Arena

A dedicated per-function arena, NOT `ASTAllocator`. The key differences:

| Property | `ASTAllocator` | MIR Arena |
|----------|---------------|-----------|
| Mutex | Yes (every alloc) | **No** (thread-owned) |
| Destructor tracking | Yes (`ptr_storage`, `cleanup_fns`) | **No** (trivial types only) |
| Clear strategy | Destruct tracked objects, keep first block | Reset offset, keep all blocks |
| Growth | `heap_batch_size` (100 KB) | 32-64 KiB inline, then 2x geometric |

Implementation:
```cpp
class MIRArena {
    struct Chunk {
        Chunk* next;
        size_t capacity;
        size_t used;
        // data follows, no flexible array member on MSVC
    };
    Chunk* first_chunk;       // points to inline_buffer when small
    Chunk* current;           // current allocation chunk
    char inline_buffer[32 * 1024];  // 32 KiB inline — avoids first heap alloc
    size_t inline_used;

    void* allocate(size_t bytes, size_t alignment = 8) {
        // bump-pointer within current chunk
        // if insufficient space, allocate new chunk (2x geometric)
        // no mutex, no destructor tracking
    }

    void reset() {
        // keep all chunks, reset used counts to 0
        // reuse memory for next function
    }
};
```

The inline buffer is the first chunk. For functions that fit in 32 KiB of MIR
data (the majority), no heap allocation occurs. Chunks are never freed during
lowering — they are freed all at once when the function artifact is complete.

### 2.2 MIRArray<T>

A non-owning view into arena-allocated contiguous memory. Used for instruction
and operand arrays within a function.

```cpp
template<typename T>
struct MIRArray {
    T* ptr;
    uint32_t len;
    uint32_t cap;  // capacity (for appending during construction)

    T& operator[](uint32_t i) { return ptr[i]; }
    T* begin() { return ptr; }
    T* end() { return ptr + len; }
};
```

During construction, the builder appends to `MIRArray` by checking `len < cap`
(inline fast path) or growing via the arena (slow path, rare after initial
sizing). After construction, `cap` is unused and the array is read-only.

The builder estimates capacity from AST node counts and pre-allocates once:
```cpp
func.instructions.ptr = arena.allocate<MIRInstruction>(estimated_inst_count);
func.instructions.cap = estimated_inst_count;
func.instructions.len = 0;
```

### 2.3 Compact Instruction Representation

```cpp
// 16 bytes per instruction — fixed size, trivially movable
struct MIRInstruction {
    uint32_t opcode_and_flags;   // opcode (16 bits) + flags (16 bits)
    uint32_t result_or_place;    // ValueId or PlaceId or 0 (no result)
    uint32_t operand_offset;     // index into function's operand array
    uint16_t operand_count;      // number of operands (0-65535)
    uint16_t source_index;       // index into source location table, 0xFFFF = none
};

// 8 bytes per operand
struct MIROperand {
    uint32_t id;                 // ValueId, PlaceId, SymbolId, or constant index
    uint32_t kind_and_type;      // operand kind (4 bits) + TypeId (28 bits)
};
```

Operand kinds (4-bit enum):
```
Value = 0     // SSA value (result of an instruction)
Place = 1     // addressable storage (local, global, field, element)
Symbol = 2    // function, type, intrinsic, external
Constant = 3  // integer/float/string constant from pool
Block = 4     // block ID (for terminators)
Type = 5      // type operand (for sizeof, cast, alloca)
```

Opcode flags (16-bit bitmask):
```
FLAG_PURE       = 0x0001  // no side effects
FLAG_VOLATILE   = 0x0002  // memory barrier
FLAG_ATOMIC     = 0x0004  // atomic operation
FLAG_MAY_THROW  = 0x0008  // can transfer control to exception handler
FLAG_MAY_TRAP   = 0x0010  // can trap (division by zero, bounds check)
FLAG_READ       = 0x0020  // reads memory
FLAG_WRITE      = 0x0040  // writes memory
FLAG_CALL       = 0x0080  // calls unknown code
FLAG_TERMINATOR = 0x0100  // this is a terminator instruction
```

Flags are properties of the opcode, not stored per-instruction. The
`opcode_and_flags` field is `(opcode << 16) | fixed_flags_for_opcode`. This
avoids storing flags redundantly.

### 2.4 Function Container

```cpp
struct MIRFunction {
    SymbolId symbol;                // module-level symbol for this function
    TypeId function_type;           // function signature type

    // Instruction storage — all arena-allocated, contiguous
    MIRArray<MIRInstruction> instructions;
    MIRArray<MIROperand> operands;

    // Block metadata — each block knows its instruction range
    MIRArray<MIRBlock> blocks;

    // Dense ID-indexed tables — sized at function start, never resized
    MIRValueTable values;           // ValueId -> MIRValueDef
    MIRPlaceTable places;           // PlaceId -> MIRPlaceDef
    MIRCleanupScopeTable cleanups;  // cleanup scope stack

    // Side tables — sparse, only populated when needed
    MIRSourceTable sources;         // source locations (only when -g or diagnostics)
    MIRABIEntryTable abi_entries;   // call ABI records (one per call instruction)
    MIRDebugTable debug;            // debug identities (only when -g)

    // Move-path tree — tracks init/move/destroy state per aggregate field
    MIRMovePathTable move_paths;
};

struct MIRBlock {
    uint32_t inst_start;       // index into instructions array
    uint32_t inst_count;       // number of non-terminator instructions
    uint16_t arg_count;        // number of block arguments
    uint16_t arg_type_offset;  // index into block arg type array
    BlockId id;                // stable block identifier
    // terminator is instructions[inst_start + inst_count]
};

struct MIRValueDef {
    TypeId type;
    uint32_t def_inst;         // instruction index that defines this value
    uint16_t use_count;        // saturating use count (for inline decisions)
    uint8_t flags;             // value flags (constant, pure, addressable, etc.)
};

struct MIRPlaceDef {
    TypeId type;
    uint8_t storage_class;     // local, temporary, parameter, global
    uint8_t init_state;        // uninitialized, initialized, moved, destroyed
    uint32_t move_path_id;     // index into move-path tree (0 = no ownership)
};
```

### 2.5 Module Container

```cpp
struct MIRModule {
    // Sealed tables — populated before parallel lowering, immutable after
    MIRTypeTable types;
    MIRConstantPool constants;
    MIRSymbolTable symbols;
    MIRModuleLayout layout;        // target-dependent size/alignment for types

    // Function artifacts — populated after parallel lowering
    std::vector<MIRFunction> functions;

    // Module-level declarations (for C module emitter)
    std::vector<MIRTypeDecl> type_declarations;
    std::vector<MIRGlobal> globals;
    std::vector<MIRPrototype> prototypes;
};

// Read-only context passed to every worker
struct MIRModuleContext {
    const MIRTypeTable& types;
    const MIRConstantPool& constants;
    const MIRSymbolTable& symbols;
    const MIRModuleLayout& layout;
    const NameMangler& mangler;
    const LocationManager& loc_man;
    span<const MIRTypeDecl> type_declarations;
    span<const MIRPrototype> prototypes;
    span<const MIRGlobal> globals;
    span<const FunctionDeclaration*> function_bodies;  // concrete functions to lower
    OutputMode mode;
    bool is64Bit;
};
```

### 2.6 Type Table Construction

The MIR type table is populated **during the serial module prologue**, before
parallel lowering. The `MIRLowerer` reads from the AST's `TypeBuilder` to
construct `MIRTypeRecord` entries:

```cpp
// Serial: called once per module before workers start
void build_type_table(MIRModule& module, TypeBuilder& type_builder) {
    for(auto& type : type_builder.all_types()) {
        MIRTypeRecord rec;
        rec.kind = classify_type(type);           // int, float, ptr, ref, struct, ...
        rec.flags = extract_flags(type);          // signedness, mutability, ownership
        rec.size = layout.size_of(type);          // target-dependent
        rec.alignment = layout.align_of(type);
        rec.data_offset = type_data_pool.size();
        rec.data_count = emit_type_data(type, type_data_pool);  // field types, etc.
        module.types.intern(rec);
    }
}
```

This is serial because `TypeBuilder` may not be thread-safe, and because the
type table must be complete before workers reference it. The cost is small:
type count is proportional to source lines, and each type record is ~24 bytes.

### 2.7 MIRArray Growth During Construction

During instruction building, the builder appends to `MIRArray`. The growth
strategy:

1. **Initial sizing:** Before lowering a function, estimate instruction count
   from AST node count (a simple multiplier, e.g., 2x for expressions + 1x for
   statements). Pre-allocate from the arena.

2. **Appending:** Check `len < cap`. If yes, placement-new into `ptr[len++]`.
   If no, allocate a new block at 2x capacity from the arena, memcpy old data,
   update `ptr` and `cap`. The old data is not freed (arena owns it).

3. **No shrink:** After construction, `cap` is informational only. The array
   is read-only.

This is the same pattern as `std::vector` but without per-element heap
allocation and without calling `realloc` (the arena handles chunk management).

### 2.8 Move-Path Tree

Each aggregate type has a move-path tree that tracks ownership state per field.
The tree is stored in `MIRMovePathTable`:

```cpp
struct MIRMovePathNode {
    PlaceId root_place;         // the root place this path belongs to
    uint32_t parent_path;       // index of parent path (0 = root)
    uint32_t first_child;       // index of first child (0 = no children)
    uint32_t next_sibling;      // index of next sibling (0 = no more siblings)
    uint8_t init_state;         // uninitialized, initialized, moved, destroyed
    TypeId field_type;          // type of this field/element
};
```

Example for `struct Point { x: i32, y: i32 }`:
```
path[0] = { root=p0, parent=0, first_child=1, next=0, state=init, type=Point }
path[1] = { root=p0, parent=0, first_child=0, next=2, state=init, type=i32 }  // x
path[2] = { root=p0, parent=0, first_child=0, next=0, state=init, type=i32 }  // y
```

When `x` is moved: `path[1].state = moved`, `path[0].state = partially_moved`.

The move-path tree is built during lowering when an aggregate place is created
and tracked through `move_init`, `copy_init`, `destroy`, and `drop` instructions.
The verifier checks that destroy only walks initialized paths.

---

## 3. Compilation Pipeline Integration

### 3.1 Current pipeline (for reference)

```
parse → symres (6 passes) → typeverify → 2c/LLVM → link
```

### 3.2 MIR pipeline

```
parse → symres (6 passes) → typeverify → [MIR phase] → link
                                         ┌─────────────────────┐
                                         │ serial:              │
                                         │   build type table   │
                                         │   build symbol table │
                                         │   enumerate functions│
                                         │   seal context       │
                                         │                      │
                                         │ parallel (per file): │
                                         │   lower functions    │
                                         │   emit C / LLVM      │
                                         │                      │
                                         │ serial:              │
                                         │   merge artifacts    │
                                         │   emit module prologue│
                                         └─────────────────────┘
```

### 3.3 Insertion point in ASTProcessor

The MIR layer integrates at the same point as `declare_module()`/`implement_module()`:

```cpp
// In process_module_tcc() or process_module_gen():
processor.import_module_files_direct(job, module);
processor.sym_res_module(job, module);
processor.type_verify_module_parallel(job, module);

if(options.use_mir) {
    MIRLowerContext ctx(processor, module, options);

    // Serial prologue — builds tables, enumerates functions
    ctx.prepare_module();

    // Parallel lowering + emission — one task per file
    ctx.lower_and_emit_module();

    // Serial merge — concatenates function artifacts into module output
    ctx.merge_module();

    // Pass the merged C output to the existing TinyCC/LLVM pipeline
} else {
    processor.declare_module(c_visitor, module);
    processor.implement_module(c_visitor, module);
}
```

### 3.4 `prepare_module()` — Serial Prologue

This runs once per module, serially, before workers start:

```cpp
void MIRLowerContext::prepare_module() {
    // 1. Build MIR type table from TypeBuilder
    build_type_table(module, processor.type_builder);

    // 2. Build MIR symbol table — enumerate all concrete functions
    //    including generic instantiations and lambda bodies
    build_symbol_table(module, processor.container);

    // 3. Build module layout (target-dependent size/alignment)
    build_layout(module, options);

    // 4. Enumerate concrete function bodies to lower
    //    Includes: module functions, generic instantiations, lambda bodies
    auto functions = enumerate_concrete_functions(module, processor.container);

    // 5. Build and seal MIRModuleContext (immutable from this point)
    module.context = MIRModuleContext {
        .types = module.types,
        .constants = module.constants,
        .symbols = module.symbols,
        .layout = module.layout,
        .mangler = processor.mangler,
        .loc_man = processor.loc_man,
        .type_declarations = ...,
        .prototypes = ...,
        .globals = ...,
        .function_bodies = functions,
        .mode = options.out_mode,
        .is64Bit = options.is64Bit,
    };
}
```

### 3.5 Feature Gate and Legacy Fallback

```cpp
enum class LoweringPath { Legacy, MIR };

LoweringPath select_lowering_path(
    FunctionDeclaration* fn,
    const MIRModuleContext& ctx) {
    if(!ctx.options.use_mir) return LoweringPath::Legacy;

    // Check for unsupported features
    if(fn->is_comptime()) return LoweringPath::Legacy;        // comptime stays AST
    if(fn->has_cbi_annotation()) return LoweringPath::Legacy;  // CBI needs TinyCC
    if(fn->has_exception_throw()) return LoweringPath::Legacy; // throw not yet supported
    // ... other checks

    return LoweringPath::MIR;
}
```

**Critical rule:** A function is either fully MIR or fully legacy. Never mix
within one function. When a function falls back to legacy:
- The entire function body is translated by `ToCAstVisitor` (legacy path).
- The function's C output is treated as a `LegacyCFragment` for module merge.
- The fragment must have no ownership/cleanup interaction with surrounding MIR
  code (this is guaranteed by the whole-function fallback).

### 3.6 `LegacyCFragment` Bridge

The design doc §1 explicitly requires a `LegacyCFragment` for incremental
adoption. In v2, this is handled at the function level (not statement level):

```cpp
struct MIRFunctionArtifact {
    enum Kind { MIRArtifact, LegacyArtifact };

    Kind kind;
    SymbolId symbol;
    uint32_t source_order;

    // MIR path
    MIRArena* arena;                    // null after emission in debug_quick
    std::vector<char> c_bytes;

    // Legacy path
    // c_bytes contains the legacy 2c output directly
    // No MIR arena needed
};
```

When a function falls back to legacy, `ToCAstVisitor` translates it normally
and the output is wrapped in a `LegacyArtifact`. The merge step concatenates
MIR and legacy artifacts in source order. This is safe because:
- Each function is a separate C function body.
- Module-level declarations (types, prototypes) are shared by both paths.
- No MIR cleanup state interacts with legacy cleanup state.

### 3.7 Error Handling During Lowering

When the lowerer encounters an AST node it cannot handle:

```cpp
MIRExprResult MIRLowerer::lower_expr(Value* val) {
    switch(val->val_kind()) {
        case ValueKind::IntN: return lower_int(val->as_int_n());
        // ... known cases
        default:
            // Report diagnostic: "unsupported MIR lowering for <node kind>"
            // Return error result — caller escalates to legacy fallback
            return MIRExprResult::error();
    }
}
```

The lowerer returns an error result. The caller (`lower_function`) catches
this and returns a `LoweringResult::Fallback` status. The orchestrator then
runs legacy 2c on this function and wraps the output as a `LegacyArtifact`.

Error diagnostics are collected in the worker's scratch diagnostic buffer.
They are NOT printed from the worker (that would race on `print_mutex`).
They are moved to the coordinator and printed during merge.

### 3.8 comptime Blocks

`comptime` blocks and functions are **fully evaluated by the AST interpreter**
during compilation. They produce zero MIR instructions and zero C output.

The MIR lowerer skips comptime constructs entirely:
- `comptime { ... }` blocks → no MIR emitted
- `comptime func ...` → no MIR prototype or body emitted
- `comptime var/const` → no MIR alloca emitted
- Runtime calls to comptime functions → the call is replaced by the
  pre-computed result value (already stored in the AST by the interpreter)

This matches the current 2c behavior (`2cASTVisitor.cpp:6245-6247`).

### 3.9 Lambda Handling in MIR

Lambdas are separate functions in MIR, matching the 2c pattern where each
lambda becomes a static C function (`2cASTVisitor.cpp:2344-2424`).

**Discovery:** Lambda declarations are discovered during symbol resolution.
The `InstantiationsContainer` tracks lambda functions alongside generic
instantiations. During `prepare_module()`, lambda bodies are enumerated as
concrete functions with their own `SymbolId`.

**Capture environment:** Each capturing lambda has an implicit capture struct:

```text
; For lambda captures {x: i32, s: string}
%struct.__chemda_0_cap = struct {
    x_field: i32,
    s_field: string
}
```

The capture struct is a regular MIR struct type with a destructor if any
captured variable has a destructor. The capture struct is allocated and
initialized at the lambda call site.

**Lowering:** A lambda body is lowered like any other function, with an
additional implicit `env_ptr` parameter (the capture struct pointer). Field
accesses to captured variables are lowered as `field_addr` on the env_ptr.

**C emission:** The lambda becomes a `static` C function with a `void* env`
parameter, plus a capture struct definition at file scope. The call site
creates a compound literal `{fn_ptr, &capture_struct}` (the fat pointer).

### 3.10 Module-Level Declarations

The C module emitter (`CModuleEmitter`) handles the module prologue, which
remains serial and is shared by both MIR and legacy functions:

```cpp
class CModuleEmitter {
    BufferedWriter& output;    // module-level output buffer

    void emit_prologue();      // #include, runtime support
    void emit_type_decls();    // struct/enum/union forward declarations and definitions
    void emit_prototypes();    // function prototypes (from MIRSymbolTable)
    void emit_globals();       // global variable declarations
    void emit_epilogue();      // module-level initializers, if any
};
```

The `CModuleEmitter` reads from the sealed `MIRModuleContext` — it does not
depend on any worker output. It runs serially before workers start (for
forward declarations) and after workers finish (for any deferred stubs).

**Key insight:** Type declarations and function prototypes are already available
after `prepare_module()`. The module prologue can be emitted before workers
start. Only function bodies need parallel lowering.

### 3.11 `MIRModuleContext` — Complete Field List

```cpp
struct MIRModuleContext {
    // Type information
    const MIRTypeTable& types;           // canonical type records
    const MIRModuleLayout& layout;       // target-dependent size/alignment

    // Symbol information
    const MIRSymbolTable& symbols;       // function/type/global symbols
    const MIRConstantPool& constants;    // interned constants (string literals, etc.)
    const NameMangler& mangler;          // name mangling (read-only)

    // Source information
    const LocationManager& loc_man;      // source location lookups (read-only)

    // Declaration snapshots (for C module prologue)
    span<const MIRTypeDecl> type_declarations;
    span<const MIRPrototype> prototypes;
    span<const MIRGlobal> globals;

    // Function enumeration
    span<FunctionDeclaration*> function_bodies;  // concrete functions to lower
    span<GenericInstantiation*> generic_bodies;  // generic instantiations to lower
    span<LambdaFunction*> lambda_bodies;         // lambda functions to lower

    // Configuration
    OutputMode mode;                     // debug_quick, debug_complete, release, etc.
    bool is64Bit;
    bool emit_debug_info;
};
```

All fields are const references or spans into data owned by `MIRModule` or
the compiler infrastructure. Workers receive a `const MIRModuleContext&` and
never mutate it.

---

## 4. Stage-by-Stage Implementation

### Dependency Graph

```
PR 1 (types + arena)
  └─→ PR 2 (builder)
       ├─→ PR 3 (lowerer straight-line + lambda)
       │    └─→ PR 4 (C emitter straight-line)
       │         └─→ PR 5 (MIR interpreter)
       │              └─→ PR 6 (CFG + aggregates + cleanup)  ← merged stage
       │                   ├─→ PR 7 (MIR default + legacy removal)
       │                   ├─→ PR 8 (LLVM lowering)
       │                   └─→ PR 9 (parallel lowering)
       └─→ PR 3.5 (module-level declarations)
```

**Not all PRs are independent.** PRs 6-9 depend on the full instruction set
from PRs 1-5. PR 9 depends on PR 7 (MIR must be default before parallelizing).
The dependency chain is: 1 → 2 → 3+3.5 → 4 → 5 → 6 → 7 → 8 → 9.

However, each PR is independently **testable** and **reversible** — you can
revert any PR without breaking the others (the feature gate falls back to
legacy).

### Stage 1: Core MIR Types + Arena (PR 1)

**Goal:** Establish the data structures and allocation infrastructure.

**Files to create:**
```
compiler/mir/MIRTypes.h          // TypeId, ValueId, PlaceId, BlockId, InstId, SymbolId
compiler/mir/MIRArena.h/.cpp     // Per-function bump arena
compiler/mir/MIRInstruction.h    // Instruction, Operand, Opcode enum, EffectSummary
compiler/mir/MIRFunction.h       // Function container
compiler/mir/MIRModule.h         // Module container + ModuleContext
compiler/mir/MIRTypeTable.h      // Type interning
compiler/mir/MIRConstantPool.h   // Constant interning
compiler/mir/MIRSymbolTable.h    // Symbol records
compiler/mir/MIRMovePath.h       // Move-path tree
compiler/mir/MIRDump.h/.cpp      // Textual MIR dump
```

**Key decisions:**
- `MIRArena` is a simple bump allocator with 32 KiB inline buffer. No mutex.
- `MIRArray<T>` is a `{T*, len, cap}` view into arena memory.
- Opcode enum is a `uint16_t` with explicit values.
- All function-local IDs are 32-bit. Module-level IDs (SymbolId, TypeId) are
  also 32-bit but index into module tables.

**Tests:**
- Unit tests that create MIR manually and dump it.
- Arena allocation benchmarks vs `ASTAllocator`.
- MIRArray growth benchmarks.

**Performance gate:** Arena allocation must be ≥10x faster than `ASTAllocator`
for sequential 100-byte allocations (no mutex overhead).

---

### Stage 2: MIR Builder (PR 2)

**Goal:** Typed construction API that makes invalid MIR hard to build.

**Files to create:**
```
compiler/mir/MIRBuilder.h/.cpp           // High-level builder
compiler/mir/MIRValueTable.h            // Dense value definitions
compiler/mir/MIRPlaceTable.h            // Dense place definitions
compiler/mir/MIRCleanupScope.h          // Cleanup scope tracking
compiler/mir/MIRModuleBuilder.h/.cpp    // Module-level table construction
```

**Builder API:**
```cpp
class MIRBuilder {
    MIRArena& arena;
    MIRFunction& func;

    // Current state
    BlockId current_block;
    uint32_t current_inst_index;

    // Block management
    BlockId new_block(uint16_t arg_count = 0);
    void set_terminator(MIRTerminator term);
    void switch_to_block(BlockId block);

    // Value/Place creation
    ValueId new_value(TypeId type);
    PlaceId new_place(TypeId type, StorageClass sc);
    void set_drop_flag(PlaceId place, ValueId flag);

    // Instruction emission (appends to current block)
    ValueId emit_add(ValueId lhs, ValueId rhs, TypeId result_type);
    ValueId emit_sub(ValueId lhs, ValueId rhs, TypeId result_type);
    ValueId emit_mul(ValueId lhs, ValueId rhs, TypeId result_type);
    ValueId emit_div(ValueId lhs, ValueId rhs, TypeId result_type);
    ValueId emit_rem(ValueId lhs, ValueId rhs, TypeId result_type);
    ValueId emit_eq(ValueId lhs, ValueId rhs, TypeId result_type);
    ValueId emit_lt(ValueId lhs, ValueId rhs, TypeId result_type);
    // ... all arithmetic/comparison ops

    PlaceId emit_alloca(TypeId type, StorageClass sc);
    void emit_store(PlaceId dest, ValueId src);
    ValueId emit_load(PlaceId src, TypeId type);
    ValueId emit_address_of(PlaceId place, TypeId ptr_type);
    ValueId emit_gep(ValueId base, span<ValueId> indices, TypeId result_type);
    ValueId emit_field_addr(PlaceId base, uint32_t field_index, TypeId field_type);
    ValueId emit_index_addr(PlaceId base, ValueId index, TypeId elem_type);

    MIREmitResult emit_call(SymbolId callee, span<MIRArgument> args, TypeId ret_type);
    MIREmitResult emit_call_indirect(ValueId fn_ptr, span<MIRArgument> args, TypeId ret_type);

    void emit_return(ValueId val);
    void emit_return_void();
    void emit_br(BlockId target, span<ValueId> args = {});
    void emit_cond_br(ValueId cond, BlockId true_block, BlockId false_block,
                       span<ValueId> true_args = {}, span<ValueId> false_args = {});
    void emit_unreachable();

    void emit_init(PlaceId dest, SymbolId constructor, span<MIRArgument> args);
    void emit_copy_init(PlaceId dest, ValueId src, TypeId type);
    void emit_move_init(PlaceId dest, ValueId src, TypeId type);
    void emit_assign(PlaceId dest, ValueId src, TypeId type);
    void emit_drop(PlaceId place, SymbolId destructor);
    void emit_destroy(PlaceId place, SymbolId destructor);
    void emit_set_drop_flag(PlaceId place, ValueId flag, bool active);

    // Cleanup scope management
    void push_cleanup_scope();
    BlockId pop_cleanup_scope();          // returns unwind target block
    void register_owned(PlaceId place);   // add place to current cleanup scope
    void emit_cleanup_chain(span<BlockId> unwind_targets);  // for nested returns

    // Block arguments
    ValueId emit_block_arg(BlockId block, uint32_t arg_index, TypeId type);
};
```

**Cleanup scope data structure:**
```cpp
struct CleanupScope {
    uint32_t parent;              // index of enclosing scope (0 = function entry)
    uint32_t owned_start;         // index into owned_places array
    uint32_t owned_count;         // number of owned places
    BlockId unwind_target;        // block to branch to for cleanup on abnormal exit
};

struct MIRCleanupScopeTable {
    MIRArray<CleanupScope> scopes;
    MIRArray<PlaceId> owned_places;     // all owned places, grouped by scope
    MIRArray<uint8_t> drop_flags;       // drop flag values (runtime booleans)
};
```

**How cleanup works on return:**
1. The builder checks all active cleanup scopes from innermost to outermost.
2. For each scope with owned places, emit `drop` instructions in reverse
   initialization order.
3. If a scope's unwind target is set (from a conditional branch), emit
   `cond_br` on the drop flag to skip cleanup for moved values.
4. After all cleanup, emit the `return` instruction.

**Tests:**
- Build MIR for simple functions (no branches, no calls).
- Verify use counts, block structure, terminator placement.
- Build MIR with cleanup scopes and verify drop instruction ordering.

---

### Stage 2.5: Module-Level Declarations (PR 3.5, parallel with PR 3)

**Goal:** Build MIR type table, symbol table, and module prologue.

**Files to create:**
```
compiler/mir/MIRModuleBuilder.h/.cpp    // builds MIRModule from resolved AST
compiler/mir/MIRTypeBuilder.h/.cpp      // converts TypeBuilder → MIRTypeTable
compiler/mir/MIRSymbolBuilder.h/.cpp    // converts resolved symbols → MIRSymbolTable
```

**Key responsibility:** After type verification, the AST has fully resolved
types and symbols. The module builder converts these into MIR's internal
representation. This is serial (happens once per module) and must complete
before parallel lowering starts.

**Type conversion:** Each AST type → `MIRTypeRecord`:
- Primitive types: straightforward mapping.
- Pointer types: `MIRTypeKind::Pointer` + pointee TypeId + address space.
- Reference types: `MIRTypeKind::Reference` + referent TypeId + mutability.
- Struct types: `MIRTypeKind::Struct` + field TypeIds + offsets + destructor
  flag + constructor flags + copy/move contract.
- Variant types: `MIRTypeKind::Variant` + case types + discriminator.
- Array types: `MIRTypeKind::Array` + element TypeId + length.
- Function types: `MIRTypeKind::Function` + parameter TypeIds + return TypeId
  + calling convention + varargs flag.

**Symbol conversion:** Each resolved function/variable → `MIRSymbolRecord`:
- Function: name, mangled name, function type, linkage, visibility, ABI info.
- Global: name, mangled name, type, linkage, initializer kind.
- External: name, library, ABI.

---

### Stage 3: AST-to-MIR Lowerer — Straight-Line Functions (PR 3)

**Goal:** Lower simple functions from resolved AST to MIR.

**Scope:** Functions containing:
- Primitive values and arithmetic
- Local variables (alloca + store + load)
- Address-of operations (`&raw`, `&mut`)
- Function calls with scalar and reference arguments
- Scalar and struct-return calls (via explicit result place)
- Return statements
- Simple assignments (no destructors)
- Lambda definitions (calls to lambdas treated as opaque function pointers)

**NOT yet supported (falls back to legacy):**
- Control flow (if/else, while, for, switch) → Stage 6
- Destructors, moves, copies, cleanup → Stage 6
- Aggregates (struct/array/variant construction) → Stage 6
- Variant pattern matching → Stage 6
- Atomics, TLS → later stages
- Comptime → stays on AST interpreter

**Files to create:**
```
compiler/mir/MIRLowerer.h/.cpp          // AST → MIR lowering
compiler/mir/MIRLowerExpr.h/.cpp        // Expression lowering
compiler/mir/MIRLowerStmt.h/.cpp        // Statement lowering
compiler/mir/MIRLowerCall.h/.cpp        // Call lowering (complex ABI logic)
compiler/mir/MIRLowerLambda.h/.cpp      // Lambda lowering
```

**Lowering strategy:** A direct switch over AST node/value kinds. Returns
`MIRExprResult` — 9 bytes, no heap allocation:

```cpp
struct MIRExprResult {
    uint32_t id;        // ValueId or PlaceId
    uint32_t type;      // TypeId
    uint8_t kind;       // 0=Value, 1=Place, 2=Address, 3=Void
};

enum class MIRExprKind : uint8_t {
    Value = 0,    // result is an SSA value (e.g., add result)
    Place = 1,    // result is an addressable place (e.g., alloca)
    Address = 2,  // result is a pointer/reference to a place
    Void = 3,     // no result (e.g., statement with no value)
};
```

**Function body lowering:**
```cpp
MIRLowerResult MIRLowerer::lower_function(FunctionDeclaration* fn) {
    // 1. Create function in MIR module
    auto func = builder.create_function(fn->symbol_id, fn->type_id);

    // 2. Lower parameters → param instructions
    for(auto& param : fn->parameters) {
        auto result = builder.emit_param(param.type_id);
        // Store to alloca if parameter is addressable
        if(param.is_addressable()) {
            auto place = builder.new_place(param.type_id, StorageClass::Local);
            builder.emit_store(place, result);
        }
    }

    // 3. Lower body statements
    for(auto& stmt : fn->body->statements) {
        lower_stmt(stmt);
    }

    // 4. Verify all paths have terminators
    if(!builder.current_block_has_terminator()) {
        builder.emit_return_void();
    }

    return MIRLowerResult::success();
}
```

**Call lowering (critical detail):**

The lowerer must handle the ABI-specific details that `ToCAstVisitor` currently
rediscovery during C emission. For each call:

1. Resolve the callee's ABI from `MIRSymbolTable`.
2. Map arguments to physical parameters (hidden self, sret, environment).
3. For struct-return calls: allocate result place, add hidden sret parameter.
4. For reference parameters: emit `address_of` on the argument.
5. For move parameters: emit `move_init` from argument to parameter place.

Example:
```chemical
// Chemical source
check(some_runtime_func(), create_str())
```

MIR:
```text
%p0 = call some_runtime_func() : i32          ; scalar arg, evaluated first
%s = alloca string, temporary                  ; temp for second arg
init %s, create_str                            ; constructor, evaluated second
call check, value_of %p0, address_of %s        ; call with all args materialized
drop %s                                        ; cleanup after call
```

**Tests:**
- Lower arithmetic expressions and verify MIR dump.
- Lower function calls with scalar args and verify argument ordering.
- Lower struct-return calls and verify sret convention.
- Lower address-of operations and verify place/address distinction.
- Compare C output with legacy 2c on the same functions (differential testing).

---

### Stage 4: C Emitter from MIR (PR 4)

**Goal:** Emit C code from MIR for the straight-line subset.

**Files to create:**
```
compiler/mir/cbackend/MIREmitter.h/.cpp        // MIR → C emission
compiler/mir/cbackend/MIRExprEmitter.h/.cpp    // Expression compaction
compiler/mir/cbackend/MIRStmtEmitter.h/.cpp    // Statement emission
```

**Emitter design:**
```cpp
class MIREmitter {
    BufferedWriter& output;         // function-local buffer (16-64 KiB inline)
    const MIRFunction& func;
    const MIRModuleContext& ctx;

    // Name assignment (MIR IDs → C identifiers)
    // Dense vectors, indexed by ValueId/PlaceId
    // Names are generated deterministically: __chx_v_N, __chx_p_N
    std::vector<uint32_t> value_name_ids;  // maps ValueId → name index
    std::vector<uint32_t> place_name_ids;  // maps PlaceId → name index
    unsigned next_name_counter;

    chem::string get_value_name(ValueId id);
    chem::string get_place_name(PlaceId id);

    // Expression compaction state
    // Updated during emission, not a separate pass
    std::vector<bool> value_emitted_inline;  // tracks which values were inlined

    void emit_function();
    void emit_block(BlockId block);
    void emit_instruction(const MIRInstruction& inst);
    void emit_terminator(const MIRTerminator& term);
};
```

**Expression compaction (debug_quick):**

The emitter compacts pure single-use instructions inline during emission. No
separate analysis pass. The algorithm:

1. When emitting a `call` instruction's arguments, check each argument value.
2. If the value's defining instruction is:
   - In the same block (or dominates it — trivially true for straight-line)
   - Has `FLAG_PURE` set
   - Has `use_count == 1`
   - Is not a load from a potentially-aliased location
3. Then emit the defining instruction inline as a C expression instead of
   declaring a temporary.

For `take(i * 8, i * 2, i * 4)`:
```text
%v0 = load i_place           ; load is NOT pure (may alias) → materialize
%v1 = mul %v0, 8             ; pure, single-use → inline
%v2 = mul %v0, 2             ; pure, single-use → inline
%v3 = mul %v0, 4             ; pure, single-use → inline
call take, %v1, %v2, %v3     ; all args are inlineable
```

C output: `take(i * 8, i * 2, i * 4);` — no temporaries.

**Materialization rules (when NOT to inline):**
- Load from a non-local or potentially-mutated place
- Call or intrinsic (has side effects)
- Alloca, store, init, drop, destroy (lifetime operations)
- Value used in multiple blocks
- Value used more than once (use_count > 1)
- Value crosses a block boundary
- Value is a block argument

**Key difference from legacy 2c:**
- No `nested_value` boolean — MIR knows whether a result is needed as a value
  or a place (via `MIRExprKind`).
- No `current_assignable` string — break/loop values use block arguments.
- No `local_allocated` or `destructible_refs` maps — places are in the dense
  place table with explicit cleanup instructions.
- No `CDestructionVisitor` — drop instructions are already in MIR.
- No compound expressions `(*({ ... }))` — struct-return calls use explicit
  result places.

**Struct-return C emission:**
```text
; MIR:
%s = alloca Point, local
init %s, make_point, arg0, arg1
call use_point, address_of %s

; C output:
Point __chx_p0;
make_point(&__chx_p0, arg0, arg1);
use_point(&__chx_p0);
```

No compound expression needed. The result place is a declared local.

**Nested struct-return calls:**
```text
; Chemical: use_point(make_point(x, y))
; MIR:
%p = alloca Point, temporary
init %p, make_point, x, y
call use_point, address_of %p
drop %p

; C output:
Point __chx_p0;
make_point(&__chx_p0, x, y);
use_point(&__chx_p0);
```

Each nested call gets its own result place. No compound expressions.

**Tests:**
- Golden tests: MIR C output must match legacy 2c output for straight-line
  functions.
- Evaluation order tests: effectful calls must remain in source order.
- Expression compaction tests: `take(i * 8, i * 2, i * 4)` produces no temp
  variables.
- Struct-return tests: no compound expressions, explicit result places.

---

### Stage 5: MIR Interpreter (PR 5)

**Goal:** Execute MIR directly, providing a correctness oracle for C and LLVM.

**Files to create:**
```
compiler/mir/interpreter/MIRInterpreter.h/.cpp
compiler/mir/interpreter/MIRFrame.h
compiler/mir/interpreter/MIRMemory.h
```

**Runtime model:**
```cpp
class MIRInterpreter {
    MIRFrame frame;            // PlaceId → storage slot, ValueId → immutable value
    MIRMemory memory;          // pointer-addressable storage with bounds metadata
    MIRControlState control;   // current block, instruction index
    MIRCleanupState cleanup;   // active initialized/moved places

    // Execution
    MIRExecResult execute(const MIRFunction& func);

    // Per-instruction dispatch
    void exec_add(const MIRInstruction& inst);
    void exec_load(const MIRInstruction& inst);
    void exec_store(const MIRInstruction& inst);
    void exec_call(const MIRInstruction& inst);
    void exec_drop(const MIRInstruction& inst);
    // ... one method per opcode
};
```

**Key requirement:** The interpreter must execute the same MIR that the C emitter
produces. It must NOT fall back to AST interpretation.

**comptime interaction:** The MIR interpreter does NOT handle comptime evaluation.
Comptime is handled by the existing AST interpreter before MIR lowering. The MIR
interpreter only sees runtime code.

**Tests:**
- Run MIR interpreter on straight-line functions and compare results with
  compiled executables.
- Run same functions through MIR C → TinyCC and compare output.

---

### Stage 6: CFG + Aggregates + Cleanup (PR 6, single merged stage)

**Goal:** Lower control flow, aggregate types, constructors, destructors,
moves, copies, drop flags, and cleanup scopes. This is the largest and most
complex stage.

**Why merged:** Control flow without cleanup is unusable in Chemical. An `if`
statement with a string temporary in one branch requires cleanup edges. The
design doc §7.1 explicitly ties cleanup scopes to control flow. Separating
CFG from cleanup would produce an intermediate state that cannot pass the
test suite.

**Scope expansion (adds to Stage 3):**
- `if/else` → `cond_br` + two successor blocks + join block with optional
  result value
- `while` → condition block + body block + back edge
- `for` → init + condition + body + step + exit
- `switch` → `switch` terminator + case blocks
- `break`/`continue` → branch to target block (with optional value via block arg)
- Short-circuit `&&`/`||` → branches + join value
- Struct construction → `init` instructions
- Array construction → element-by-element initialization
- Variant construction → discriminator + payload init
- Constructor/destructor calls → `init` + `drop` + `destroy`
- Move/copy semantics → `move_init` + `copy_init` + move-path tracking
- Assignment with destructor → eval RHS → destroy old → store new
- Return cleanup → cleanup chain across scopes
- Temporary lifetime regions → alloca + init + drop at region end

**Cleanup scope implementation:**

Cleanup scopes are a stack of `CleanupScope` records. Each scope tracks:
- Which places it owns (reverse initialization order)
- Its unwind target (for abnormal exits)

On **normal scope exit** (end of block, break, continue):
1. Emit `drop` for each owned place in reverse initialization order.
2. Pop the scope.

On **return**:
1. Walk the scope stack from innermost to outermost.
2. For each scope, emit `drop` for owned places that are still initialized.
3. Move the return value to a temporary that survives cleanup (if the return
   type has a destructor).
4. Emit the return.

On **abnormal exit** (exception throw — future):
1. Branch to the unwind target of the current scope.
2. The unwind target's cleanup block handles destruction.

**Drop flags:**

A drop flag is a `PlaceId` pointing to a `bool` alloca. It records whether a
conditionally-initialized place has been initialized.

```text
; if/else with string temporary
br cond, then_block, else_block

then_block:
  %s_then = alloca string, local
  init %s_then, create_str
  set_drop_flag %df, true
  br join_block

else_block:
  set_drop_flag %df, false
  br join_block

join_block:
  ; ... use %s ...
  drop_cond %s, string::dtor, %df   ; only destroys if %df is true
```

The `drop_cond` instruction checks the flag before invoking the destructor.
If the flag is false (the place was never initialized), no destruction occurs.

**Assignment with destructor (corrected from v1):**

The current 2c visitor (`2cASTVisitor.cpp:1916-1939`) evaluates the RHS first,
then destroys the old LHS, then stores the new value. MIR must follow the same
order:

```text
; x = expr (where x has a destructor)
; Step 1: Evaluate RHS while old x is still alive
%rhs = call expr
; Step 2: Destroy old x (old value may be referenced by RHS via pointer)
destroy %old_x, string::dtor
; Step 3: Store new value
store x_place, %rhs
```

Wait — this is wrong for the case where the RHS takes a pointer to the LHS.
Consider `x = f(x.to_view())`:
1. `x.to_view()` takes a pointer to `x`'s internal data.
2. `f()` reads through that pointer.
3. After `f()` returns, the pointer is no longer needed.
4. Now we can destroy old `x` and store the new value.

The correct MIR:
```text
; Step 1: Evaluate all RHS subexpressions (including the pointer-taking one)
%view = call string::to_view, address_of x_place   ; pointer to x's data
%rhs = call f, %view                                ; f reads through pointer
; Step 2: Destroy old LHS
destroy x_place, string::dtor
; Step 3: Store new value
store x_place, %rhs
```

This matches the 2c visitor's ordering. The key insight: the RHS is fully
evaluated (including any pointer-taking) before the old LHS is destroyed.
The `destroy` happens between RHS evaluation and new value storage.

**However**, there's a subtlety: if the RHS result IS the new value for the
same place, we need to be careful about the destroy. The 2c visitor handles
this by storing the RHS in a temp first (line 1916-1922), then destroying old
x, then assigning temp to x. MIR does the same: the `%rhs` is an SSA value
or a separate place, not the same place as `x`.

**Tests (comprehensive):**
- `check(some_runtime_func(), create_str())` — constructor after first arg,
  destructor after call.
- Self-referencing assignment: `x = f(x.get_ptr(), N)`.
- Move from one owner to another, verify no double destruction.
- Partial field moves (when type contract permits).
- Conditional initialization with drop flags in if/else.
- Cleanup on return with nested scopes.
- Cleanup on break/continue with values.
- Switch with destructible temporaries in case bodies.
- Array of destructible values — element destruction order (last to first).
- Variant destruction — only active payload destroyed.
- Nested function calls with struct returns — multiple result places.

---

### Stage 7: MIR C Default + Legacy Removal (PR 7)

**Goal:** Make MIR C the default for all functions that can be lowered.

**Approach:**
1. Enable MIR by default for all functions.
2. Functions with unsupported features fall back to legacy automatically.
3. Track fallback count in verbose output.
4. Benchmark MIR vs legacy on full test suite.
5. Fix remaining fallbacks one by one.
6. When fallback count = 0, remove legacy path.

**Feature gates:**
```
--use-mir          Force MIR for all functions (error on unsupported)
--no-mir           Force legacy for all functions
--mir-only <fn>    MIR for specific function (debugging)
--dump-mir         Print MIR for all functions
--verify-mir       Run full MIR verification
```

**Legacy removal checklist:**
- [ ] All functions in test suite lower through MIR
- [ ] All library code (std, net, http, tls, etc.) lowers through MIR
- [ ] All CBI plugin code lowers through MIR (or is explicitly excluded)
- [ ] Fallback count = 0 in verbose output for full test suite
- [ ] Performance: MIR ≤ legacy on all benchmarks

---

### Stage 8: LLVM Lowering (PR 8)

**Goal:** Lower MIR to LLVM IR, replacing the current AST-based LLVM codegen.

**Mapping rules:**
- MIR SSA values → LLVM SSA values (1:1 mapping).
- MIR places → LLVM `alloca` in entry block (or `LLVMBuildAlloca`).
- `load`/`store` → LLVM `LoadInst`/`StoreInst` with correct alignment.
- `field_addr`/`index_addr`/`gep` → `LLVMBuildGEP2` with typed indices.
- `call` → `LLVMBuildCall2` with ABI metadata from `MIRABIEntryTable`.
- `cond_br` → `LLVMBuildCondBr`.
- `br` → `LLVMBuildBr`.
- `switch` → `LLVMBuildSwitch`.
- `drop`/`init`/`move_init` → explicit `call` to destructor/constructor.
- Block arguments → LLVM PHI nodes.
- Debug locations → `llvm::DebugLoc` from MIR source table.

**Integration:** The LLVM emitter consumes `MIRFunction` and produces
`llvm::Function`. It does not touch AST nodes.

**Key difference from legacy LLVM:**
- The current `Codegen` class walks AST nodes directly. The new emitter walks
  MIR instructions.
- The current `Codegen` has its own destructor scheduling. The new emitter
  uses MIR's explicit `drop`/`destroy` instructions.
- The current `Codegen` uses `LLVMBackendContext` for shared state. The new
  emitter uses `MIRModuleContext` for type/symbol information.

**Tests:**
- All existing LLVM tests must pass.
- MIR LLVM output must be functionally equivalent to legacy LLVM output.

---

### Stage 9: Parallel Function Lowering (PR 9)

**Goal:** Lower functions in parallel across files.

**Topology:**
```
module semantic barrier
  → immutable MIRModuleContext
  → serial module prologue (type declarations, prototypes, globals)
  → thread-pool file/function tasks
       each owns MIRTaskContext + MIRArena + private C buffer
  → deterministic serial artifact merge
  → module finalization
```

**Worker context:**
```cpp
struct MIRTaskContext {
    MIRArena arena;              // 32 KiB inline buffer, bump-pointer
    MIRFunctionBuilder builder;  // wraps arena + function being built
    MIRLowerer lowerer;          // AST → MIR (reads from MIRModuleContext)
    MIREmitter emitter;          // MIR → C (writes to private buffer)
    MIRDiagnosticSink diag;      // scratch diagnostics
    unsigned name_counter;       // local C name counter
};
```

**Worker lifecycle:**
1. Worker receives a `FunctionDeclaration*` from the task queue.
2. Creates a fresh `MIRTaskContext` (or resets a reusable one).
3. Lowers the function body: `lowerer.lower_function(fn, builder)`.
4. Emits C code: `emitter.emit_function(func, output_buffer)`.
5. Returns `MIRFunctionArtifact` containing the C bytes and metadata.
6. Arena is reset (or the whole context is discarded).

**Artifact merge:**
```cpp
void MIRLowerContext::merge_module() {
    // Sort artifacts by source_order (deterministic)
    std::sort(artifacts.begin(), artifacts.end(),
              [](const auto& a, const auto& b) {
                  return a.source_order < b.source_order;
              });

    // Concatenate into final module buffer
    for(auto& artifact : artifacts) {
        module_writer.write(artifact.c_bytes);
    }
}
```

**Performance gate:** Parallel MIR must be ≥1.5x faster than serial MIR on
4+ core machines for modules with ≥10 functions.

---

## 5. File Organization

```
compiler/mir/
├── MIRTypes.h                 // ID types, enums, forward declarations
├── MIRArena.h / .cpp          // Per-function bump allocator
├── MIRArray.h                 // Non-owning arena view
├── MIRInstruction.h           // Instruction, Operand, Opcode, EffectSummary
├── MIRTerminator.h            // Terminator instructions (br, cond_br, switch, return)
├── MIRFunction.h              // Function container
├── MIRModule.h                // Module container + sealed context
├── MIRTypeTable.h / .cpp      // Type interning
├── MIRConstantPool.h / .cpp   // Constant interning
├── MIRSymbolTable.h / .cpp    // Symbol records
├── MIRMovePath.h / .cpp       // Move-path tree
├── MIRBuilder.h / .cpp        // Typed construction API
├── MIRValueTable.h            // Dense value definitions
├── MIRPlaceTable.h            // Dense place definitions
├── MIRCleanupScope.h          // Cleanup scope tracking
├── MIRVerifier.h / .cpp       // Structural verification
├── MIRDump.h / .cpp           // Textual MIR dump
├── MIRLowerer.h / .cpp        // AST → MIR lowering (top-level)
├── MIRLowerExpr.h / .cpp      // Expression lowering
├── MIRLowerStmt.h / .cpp      // Statement lowering
├── MIRLowerCall.h / .cpp      // Call lowering (ABI details)
├── MIRLowerLambda.h / .cpp    // Lambda lowering
├── MIRModuleBuilder.h / .cpp  // Module table construction
├── MIRTypeBuilder.h / .cpp    // Type table from TypeBuilder
├── MIRSymbolBuilder.h / .cpp  // Symbol table from resolver
├── cbackend/
│   ├── MIREmitter.h / .cpp        // MIR → C emission (function-level)
│   ├── MIRExprEmitter.h / .cpp    // Expression compaction
│   ├── MIRStmtEmitter.h / .cpp    // Statement emission
│   └── MIRModuleEmitter.h / .cpp  // C module prologue (types, prototypes, globals)
├── interpreter/
│   ├── MIRInterpreter.h / .cpp    // MIR execution engine
│   ├── MIRFrame.h                 // Value/Place storage
│   └── MIRMemory.h                // Pointer-addressable storage
└── llvmbackend/
    └── MIRLLVMEmitter.h / .cpp    // MIR → LLVM IR
```

---

## 6. Critical Design Decisions (detailed)

### 6.1 No `std::unordered_map` in hot path

The current 2c visitor uses 5 `unordered_map` members per function. MIR replaces
these with:

| 2c State | MIR Replacement | Why faster |
|----------|----------------|------------|
| `local_allocated` (`unordered_map<Value*, string>`) | `MIRPlaceTable` (dense vector indexed by PlaceId) | O(1) indexed access vs O(1) amortized hash |
| `destructible_refs` (`unordered_map<Value*, string>`) | `MIRCleanupScopeTable` (stack of owned places) | Push/pop vs insert/erase |
| `aliases` (`unordered_map<void*, string>`) | `MIRValueTable` (dense vector indexed by ValueId) | O(1) indexed access |
| `local_temp_var_i` (counter) | `name_counter` per emitter | Same — O(1) increment |
| `current_assignable` (`std::string`) | Block arguments | No string allocation |

### 6.2 No `BufferedWriter` 4 MiB default for per-function emission

Each function emitter gets a 16-64 KiB inline buffer. For small functions (the
majority), this avoids any heap allocation. The buffer grows geometrically only
when needed. The final module merge uses a contiguous writer, but function
artifacts own their segments until merge.

The artifact merge concatenates C bytes in source order. No intermediate copies.
The final module buffer receives each artifact's bytes directly.

### 6.3 No shared `ToCAstVisitor` state

The `ToCAstVisitor` has ~20 mutable members that prevent parallel use. MIR
eliminates this by:
- Each worker owns its `MIRBuilder`, `MIRArena`, `MIREmitter`.
- Module-level state (types, symbols, mangling) is in the sealed immutable
  `MIRModuleContext`.
- C names are assigned per-emitter using a local counter, not a shared map.

### 6.4 No AST node pointers in persisted MIR

MIR must not retain `ASTNode*`, `Value*`, `FunctionDeclaration*`, or
`MembersContainer*` as semantic state. These pointers are invalidated when the
file allocator is cleared.

During lowering, the lowerer holds a `FunctionDeclaration*` parameter — this
is transient and valid for the duration of `lower_function()`. The lowerer
copies all needed information (source locations, declaration IDs, type
information) into MIR records. After `lower_function()` returns, no AST
pointers remain in the MIR.

### 6.5 Block arguments, not phi instructions

MIR uses block arguments as the canonical join mechanism. This is simpler to
construct, easier to verify, and maps directly to:
- LLVM PHI nodes (one-to-one).
- C join locals (declare + assign on each incoming edge).
- Interpreter block entry binding.

Block arguments are declared on the target block:
```text
join_block(i32 %val_then, i32 %val_else):
  ; %val_then comes from the then-branch
  ; %val_else comes from the else-branch
```

And supplied on each edge:
```text
br join_block, %then_result, %else_result
```

### 6.6 Struct-return calls use explicit result places

A struct-returning function call in MIR:
```text
call string::make, result_place %s, args...
```

The result place is pre-allocated by the caller. No compound expression
`(*({ ... }))` pattern. The C emitter declares the local and passes its address.

For nested calls like `f(g())`, each call gets its own result place:
```text
%p0 = alloca GResult, temporary
call g, result_place %p0
call f, address_of %p0
drop %p0
```

### 6.7 Cleanup is explicit, not inferred

Every destructor call, drop, and cleanup edge is an explicit MIR instruction.
The C emitter does not infer cleanup from variable names or C scopes. This
eliminates an entire class of bugs where the C text looks correct but destroys
the wrong object.

### 6.8 Assignment ordering (corrected)

Chemical's assignment semantics (verified from `2cASTVisitor.cpp:1916-1939`):

1. **Evaluate RHS fully** — including any subexpressions that take pointers
   to the LHS. The old LHS value is still alive during RHS evaluation.
2. **Destroy old LHS** — only after RHS is materialized in a temp.
3. **Store new value** — from the temp to the LHS place.

MIR representation:
```text
; x = f(x.to_view()) where x : string
%p0 = call string::to_view, address_of x_place    ; step 1a: pointer to x
%rhs = call f, %p0                                 ; step 1b: RHS evaluates
destroy x_place, string::dtor                      ; step 2: destroy old x
store x_place, %rhs                                ; step 3: store new value
```

The `destroy` is safe because `%rhs` is already computed and `%p0` is no
longer used (the call returned). The old x's data is not needed after this
point.

---

## 7. Risk Mitigation

### 7.1 Performance regression risk

**Mitigation:** Benchmark at every stage. The performance gates in §1.3 are
mandatory. If MIR construction + C emission is slower than legacy 2c for any
benchmark, investigate before proceeding to the next stage.

### 7.2 Correctness risk (subtle behavior differences)

**Mitigation:**
- Differential testing: run legacy 2c and MIR 2c on the same input, compare
  C output and runtime behavior.
- The MIR interpreter provides an independent correctness oracle.
- Evaluation order tests with observable side effects (calls, constructors,
  destructors).
- The test suite in `lang/tests/` is the primary regression gate.

### 7.3 Incremental adoption risk (MIR/legacy boundary bugs)

**Mitigation:**
- A function is either fully MIR or fully legacy. Never mix within one function.
- The feature gate is conservative: any unsupported feature → legacy fallback.
- Fallback count is tracked and visible in verbose output.
- Legacy path is never removed until fallback count = 0.

### 7.4 Parallelism risk (race conditions, nondeterministic output)

**Mitigation:**
- Workers own all mutable state. Module tables are immutable after sealing.
- Output is sorted by source order before merge. Deterministic regardless of
  scheduling.
- Debug builds shuffle file order to expose ordering bugs.

### 7.5 Cleanup correctness risk

**Mitigation:**
- Cleanup order is verified by the existing destructor ordering tests.
- The MIR interpreter validates cleanup by executing MIR directly.
- Drop flag correctness is tested with conditional initialization in if/else,
  while, and switch.
- Return cleanup is tested with nested scopes and multiple returns.

---

## 8. What NOT to Build (Explicit Exclusions)

1. **No MLIR dependency.** Borrow concepts, not a framework.
2. **No serialized MIR format.** In-memory only for now.
3. **No whole-program optimization.** Per-function lowering is sufficient.
4. **No alias analysis.** The verifier checks structural validity, not pointer
   provenance.
5. **No SSA promotion (mem2reg).** Scalars are SSA values where naturally
   available; aggregates stay in memory.
6. **No expression DAG or global C optimization.** Local compaction only in
   `debug_quick`.
7. **No JVM, Wasm, or JIT emitters.** Focus on C and LLVM first.
8. **No shared mutable state in workers.** No mutex in the hot path, ever.
9. **No MIR for comptime.** Comptime stays on the AST interpreter.
10. **No statement-level LegacyCFragment.** Whole-function fallback only
    (until proof that statement-level fragments are safe).

---

## 9. Implementation Order Summary

| PR | Stage | Scope | Dependencies | Test Gate |
|----|-------|-------|--------------|-----------|
| 1 | Core types + arena | Data structures, arena, dump | None | Arena benchmark ≥10x vs ASTAllocator |
| 2 | Builder | Typed construction API | PR 1 | Build simple MIR manually |
| 3 | Lowerer (straight-line) | AST → MIR for primitives + calls + lambdas | PR 2 | Lower arithmetic, verify dump |
| 3.5 | Module declarations | Type/symbol table construction | PR 1 | Build module context from resolved AST |
| 4 | C emitter (straight-line) | MIR → C for simple functions | PR 2, PR 3 | Golden tests match legacy 2c |
| 5 | MIR interpreter | Execute straight-line MIR | PR 3 | Compare with compiled output |
| 6 | CFG + aggregates + cleanup | Full language subset | PR 3, PR 4, PR 5 | All destructor/control flow tests pass |
| 7 | MIR default | Enable for all, legacy fallback | PR 6 | Full test suite passes |
| 8 | LLVM lowering | MIR → LLVM IR | PR 6 | All LLVM tests pass |
| 9 | Parallel lowering | File-level parallelism | PR 7 | Parallel ≥1.5x on 4+ cores |

---

## 10. Success Criteria

1. **Functional:** All existing tests pass through MIR C output.
2. **Performance:** `debug_quick` MIR compilation time ≤ legacy 2c time.
3. **Correctness:** MIR interpreter results match compiled executable output.
4. **Adoption:** MIR is the default for all functions within 6 months.
5. **Parallelism:** File-level parallel MIR lowering is ≥1.5x faster than serial.
6. **Code quality:** MIR C output is at least as clean as legacy 2c output.
