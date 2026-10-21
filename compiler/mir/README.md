# MIR implementation status

Branch: `mir`. Contract: `lang/docs/mir-implementation-plan.md` (§0 mandate,
§11 baselines) and `lang/docs/mir-design.md` (§19 generated-C analysis).

> **MIR → LLVM migration: COMPLETE.** The LLVM backend lowers from MIR (the
> AST-based `llvm_*` lowering is deleted), and DWARF debug info flows through MIR
> — compile unit, subprograms, locations, types, lexical blocks and per-local
> `dbg.declare` records (checkpoints 8.9/8.11/8.10, sessions 13–15). The full
> handoff, plan, hard requirements and bug/test policy live in
> **`lang/docs/mir-llvm-migration.md`**; the per-checkpoint log is in §2 of that
> doc and the sessions below. Future work is the *evaluation* action plan
> (`lang/docs/mir-migration-evaluation.md`): compile time, peak memory,
> generated-code and binary size.

MIR is the compiler's C lowering path. Read the plan before changing anything
here. Binding rules: **no feature flags, no legacy fallback, no capability
analysis;** generated C must stay functionally equivalent (not textually);
the release compiler must stay under 4 MB.

> **⚠️ Binding rule — no backend may read the AST; every backend works from MIR.**
> The AST is a *front-end* artifact (parser, symres, type-check, `MIRLowerer`);
> **no code generator may touch it.** Every backend consumes MIR only: types and
> layout from `MIRTypeTable`/`MIRLayout`, the calling convention from `MIRABI`,
> per-function facts from `MIRFacts`, names/mangling/linkage from the MIR symbol
> table, constants from `MIRConstantPool`, declarations from MIR declaration
> tables (`MIRDeclPrinter` today; `MIRTypeDecl`/`MIRPrototype`/`MIRGlobal` as they
> land). No `ast/` include, no `ASTNode*`/`BaseType*`/`StructDefinition*`/
> `FunctionDeclaration*` parameter, no source-node walk, no re-derivation of
> layout/ABI/declarations/destructor rules/lifetime from source form. The only
> AST-aware MIR components are the lowering boundary (`MIRTypeBuilder`,
> `MIRLowerer`) and the front-end declaration collection pass — they *feed* MIR
> and are never a backend's source of truth. A fact a backend needs that MIR does
> not publish yet is **added to MIR**, never fetched from the AST. Rationale and
> the review checklist live in `lang/docs/mir-implementation-plan.md` §0.9 and
> `lang/docs/mir-design.md` §16.

## ⚠️ Post-migration evaluation (2026-10-12) — read before optimising

The migration is functionally complete and green, but it **regresses every
non-functional axis** the project measures, and it has **not yet achieved the
"one small MIR core + thin backends"** shape that makes JVM/JS cheap. Full
evidence, root causes, and the actionable plan are in
**`lang/docs/mir-migration-evaluation.md`**. Headline measured facts (L1
machine, `Debug`, `debug_quick`, identical source, vs the pre-MIR `main`
compiler):

| Axis | Legacy (`main`) | MIR | Delta |
|---|---:|---:|---:|
| Main-suite compile, cold | 2.80 s | 6.49 s | **+132% (2.3×)** |
| Main-suite peak RSS | 43.4 MB | 60.4 MB | **+40%** |
| Probe generated C | 8,285 lines | 14,115 lines | **+70%** |
| Probe executable | 341 KB | 543 KB | **+59%** |
| `tests-tcc.exe` | 3.24 MB | 5.32 MB | **+64%** |
| `TCCCompiler` debug binary | 65.1 MB | 70.0 MB | **+7.5%** |

Root causes (all with file/line evidence in the evaluation doc):

1. **Declarations are still AST-based — partially addressed (session 18).**
   Aggregate *bodies* now print from MIR (`MIRDeclPrinter`, action A3's first
   bite); what still walks the AST is the collection walk and the remaining decl
   kinds — prototypes, globals + constant initializers, typedefs, vtables and
   static stubs. There is no single decl source for C, LLVM, JVM, JS yet.
   **Aggregate layout is no longer on this list** (session 10: `MIRLayout.h`
   computes every aggregate `size`/`alignment` and every field offset, action
   A1), and neither is **C type spelling** (action A2, verified session 15:
   `MIREmitter`'s `c_type_of`/`c_declarator`/`c_decl_type`/`c_function_proto` is
   the only speller — `MIRDeclEmitter::type_spelling`/`decl_spelling` are the two
   `BaseType*` → `TypeId` adapters that call it). The remaining A item is A3: a
   MIR declaration table so the declaration path stops walking the AST.
2. **Every backend is a full independent dispatcher** (`MIREmitter` 1.6k lines,
   `CodegenMir` 2.4k lines), but the **ABI classification is now shared**
   (`compiler/mir/MIRABI.h`, action B step 1, session 15) instead of written out
   nine times across three translation units, and so are the **per-function
   facts** — block offsets/labels, place write counts, parameter aliasing
   (`compiler/mir/MIRFacts.h`, action B step 2, session 18), which
   `emit_function_c` used to re-derive inline — the interface **vtable layout**
   (`compiler/mir/MIRVtable.h`, action B steps 3–4): slot order, self-erased slot
   types and the resolved callees are computed once and read by both emitters —
   and the **aggregate access layout** a backend that builds the type itself
   indexes with GEP (`mir::compute_aggregate_plan`, session 53). **Shared
   traversal, operand decode and naming now exist too** (`compiler/mir/MIRTraversal.h`
   — `operand_at`, `block_range`, `for_each_block(_inst)`, `symbol_name_view`,
   `constant_string_view`; action B step 6, session 54), along with the
   per-value compaction facts (`MIRValueFacts`). What remains in B is the AST
   still visible to the C declaration collection pass (above).
3. **Hot-path violations of the plan**: 18 `unordered_map`/sets in `MIRLowerer`,
   and `MIRLowerer.cpp` is one 8,224-line TU. The O(n) linear-scan
   `MIRTypeTable::intern` / `MIRConstantPool::add` were **fixed in session 7**
   (hash index, measured); the `std::string` per `place_for_name` lookup was
   *measured* at 2,181 calls / 0.002 s per suite compile, so it is a style issue
   rather than a hotspot.
4. **Generated-C waste**: an unconditional byte-wise zero loop on *every*
   aggregate `Alloca` (584 in one small probe; legacy: 0), aggregate
   load-then-assign double copies, redundant scalar param spills, and dead
   epilogue blocks. Session 16 removed the last branch-emission waste (every
   straight-line `Br`/`CondBr` arm used to print a jump) and **session 17
   removed the constructor-result temp and the `apply_struct_defaults` zeroes
   wherever the construction provably writes the whole value** (the two
   predicates are named below); what remains of this bullet is the zero loops
   themselves — an array literal's tail, `zeroed<T>()`, a temporary filled
   through a pointer, session 11's variant rule, and default-initializations
   that are *not* provably complete.

Do not add new functionality to the AST-based declaration path; move it into MIR
(action A/B in the evaluation doc). Any commit that grows generated C, compile
time, memory, or the compiler binary must pay for it.

### Session 60 (2026-10-21) — interning stops re-scanning the whole table, read-mostly interning takes the shared lock, the byte pool deduplicates

Session 59 replaced the interning caches' *containers*; sampling the Release
`TCCCompiler` (`SIGPROF` at 500 µs over 8 plain main-suite compiles) showed the
remaining cost was the interning **body**, not the container: `MIRTypeTable::intern`
was **8.1 %** of all in-executable samples and `MIRConstantPool::add_unlocked`
**6.7 %** — the top two symbols in the whole compiler.

**The verified fallback scan was pure cost.** Both functions, on a miss of the
hash hint, ran a linear scan of the *entire* table with `equal`/`matches` so a
record mutated after interning could still be found. Counting it (a temporary
`MIR_INTERN_STATS=1` build, reverted) gave the exact shape on the main suite:

| | calls | hint hits | misses | scan steps | scan **hits** | new records |
|---|---:|---:|---:|---:|---:|---:|
| types | 116,157 | 106,337 | 9,820 | 3,898,451 | **0** | 9,820 |
| constants | 55,723 | 50,021 | 5,702 | 3,538,383 | **0** | 5,702 |

Not one scan step ever found a duplicate, so 7.4 M `equal`/`matches` calls per
suite compile were thrown away. The scan is now skipped when the hash is
*absent*, which is provably safe: `hash_of` is a pure function of exactly the
fields `equal`/`matches` compare (so an equal record always hashes the same), and
**every** record's current hash is in the index — `intern`/`add` set it, and
`MIRTypeTable::push` (records pushed rather than interned, e.g. variant case
payloads) and `update` (the only mutator of a hashed field: `data_count`,
`field_index`, `explicit_alignment`) now set it too. A hash that is *present* but
names a different record — an exact 64-bit collision, or an entry left stale by
a mutation — still takes the verified scan, so dedup behaviour is unchanged.

**Interning is read-mostly, so it takes the shared lock.** With parallel lowering
on, every `intern`/`add` took the exclusive table lock, though 92 % of the calls
are hits. Both now answer a hit under `MIRReadLock` and only fall into the writer
lock on a miss (re-checked under it, since another worker may have interned the
record in between).

**`MIRBytePool` deduplicates by content and moves instead of copying.**
`append` is the most frequent allocation site in lowering (~193 k per plugin-suite
compile) and it paid two heap allocations per entry: a temporary `std::string`
plus the copy-assign inside `MIRStableArray::push_back`. `MIRStableArray` gained
`emplace_back`, which constructs once and move-assigns into the empty slot (one
allocation), and the pool now returns the **existing entry for identical bytes**
(a `MIRKeyMap` hint verified by a byte compare). The entry index is an opaque
handle that callers only read back, so sharing it is invisible; symbol names,
field names and `__chx_*` spellings repeat constantly.

**Measured** (`Release` `TCCCompiler`, `debug_quick`, `--no-cache`, interleaved
A/B, medians, same machine):

| Workload / metric | HEAD | Session 60 | Δ |
|---|---:|---:|---:|
| Main suite wall | 0.760 s | 0.740 s | **−2.6%** |
| Main suite user CPU | 1.000 s | 0.980 s | **−2.0%** |
| Plugin suite wall | 1.885 s | 1.855 s | **−1.6%** |
| Plugin suite user CPU | 2.595 s | 2.500 s | **−3.7%** |
| Peak RSS (plugin suite) | 142.5 MB | 140.9 MB | **−1.1%** |
| `TCCCompiler` binary | 5,200,816 B | 5,196,920 B | **−3,896 B** |

Sampling share of the in-executable samples: `intern` **8.1 % → 0.7 %**,
`add_unlocked` **6.7 % → 0.4 %**. The whole-compile numbers are smaller because
half the process CPU is TinyCC compiling the emitted C, libc, and the lowering
thread pool, not MIR.

**Output-invariant.** The emitted C of the main suite is identical modulo the
pre-existing run-to-run jitter: normalized (generic `__cgs__N`/`__cfg__N` and
lambda `__chemda_N` names folded) line-multiset difference **48** between base
and this tree, against a same-binary floor of **44–76** on four interleaved runs
(the `__chemda_N` maximum is 4713 on both). The compiled test executables print
identical output.

| Suite | Result |
|---|---|
| `MIRTests` | **4524 checks** |
| `--tcc --all` | **7287/7287** (11 suites) |
| `--llvm --all` | **7285/7285** (11 suites) |

**Rejected on measurement (kept here so they are not re-tried):**

- *`noinline` on `MIRDeclPrinter`'s `append_member_declarator`* (it costs three
  IPA-CP clones, 28.7 KB, in the binary). `noinline` does not stop cloning — it
  only stops inlining into the clones' call sites, so the binary **grew** by
  4 KB and ended up with five copies. Removing the clones needs
  `-fno-ipa-cp-clone`, i.e. a build-level change.
- *Deleting dead MIR API to shrink the binary.* A
  `-ffunction-sections`/`--gc-sections` probe shows 284 KB reclaimed, but it is
  almost entirely **COMDAT duplicates** (the same header-inline
  `intern`/`~MIRModule`/… emitted in every TU, of which the linker keeps one
  anyway), plus non-MIR Lab helpers that are live in the LLVM build. The MIR
  functions that are genuinely unreachable are ~500 B.

### Session 59 (2026-10-20) — the interning caches' containers, lock elision on the byte pools, an O(1) inline-struct set

Action D turned out not to be dominated by any *algorithm* left in MIR (the
quadratic scans are gone) but by the **containers the interning caches use** and
by **locking the shipping path does not need**. Four changes, all
output-invariant (verified byte-for-byte on the emitted C, below):

**`compiler/mir/MIRKeyMap.h` (new).** Every module-level interning cache is the
same shape — a 64-bit key (a hash, a pointer, a packed `(declaration, kind)`) to
a small dense id — and every one of them was a `std::unordered_map` (one
allocation per node, a pointer chase per probe, a modulo by a prime bucket count
per lookup) or, in the type builder, a `std::map` (a red-black tree compared key
by key). `MIRKeyMap` is a flat power-of-two open-addressing map with a splitmix64 probe,
linear probing and doubling growth; `find`/`set`/`clear` only, and the ids are
assigned by the caller, so the map's layout can never affect the result.
It replaces five containers:

- `MIRTypeTable::intern_index_` (was `unordered_map<uint64_t, TypeId>`), hit once
  per interned type,
- `MIRConstantPool::index_` (was `unordered_map<uint64_t, ConstantId>`), hit once
  per interned constant,
- `MIRTypeBuilder::memo_` and its per-thread twin (were
  `unordered_map<const BaseType*, TypeId>`), hit once per mapped AST type —
  900k times on the plugin suite,
- `MIRTypeBuilder::decl_ids_` + `decl_types_` (two `std::map`s keyed by
  `(declaration, aggregate kind)`), hit once per mapped aggregate — 80k times.
  The two maps also merged into one: the sequential `decl` id the first existed
  to hand out is already the record's `MIRTypeRecord::decl`, so `next_decl_id_`
  plus one map per aggregate kind (the kind used to be half the key) reproduces
  exactly the same ids in exactly the same creation order.

**Byte pools honour the single-writer lock policy.** `MIRBytePool` was the one
table that still took a `std::shared_mutex` on every `append` — and it is on the
path of every field name, type name, symbol name and string payload (193k
appends per plugin-suite compile), while its readers were already lock-free (the
entries never move). It now owns a `MIRTableLockPolicy` like the rest of the
module tables and is wired from `MIRModule::set_single_threaded`, so the shipping
single-writer pipeline appends without taking a lock and the parallel phase
(`kEnableParallelLowering`) still serialises.

**`record_inline_struct` is a flag per type id, not a scan.** `map()` calls it
for every struct type it resolves and the dedup was a linear scan of
`inline_structs`, so the cost grew quadratically with a module's aggregate count.
Type ids are dense, so a `std::vector<uint8_t>` indexed by id answers it in O(1)
(the mutex is also elided in single-writer mode). `MIRModule::clear` now clears
both tables (it previously kept `inline_structs`).

**`get()` returns a reference.** `MIRTypeTable`/`MIRConstantPool`/`MIRSymbolTable`
`get()` returned their record **by value** — a 44-byte copy of every `TypeId`
lookup, which is the most frequent read in the compiler (every operand's type, on
every lowered expression and every emitted instruction). Session 45 made them
by-value because the record vectors could realloc under a reader; session 55's
stable array removed that reason (elements never move), so the copy is pure cost.
300 call sites, no behaviour change (the record is append-only and immutable
after the build phase).

**Dead preamble deleted.** `MIRDeclEmitter::emit_preamble` (95 lines, 4.5 KB of
text plus its string literals) was never called — `CTranslationContext::prepare_translate`
emits the real preamble — so it was a second, silently drifting copy of the same
`#define`/typedef block. Removed (confirmed dead with a `--gc-sections` probe
build, which is also how the rest of the list was produced).

**Measured** (`Release` `TCCCompiler`, `debug_quick`, `--no-cache`, interleaved
A/B, same machine):

| Workload / metric | HEAD | Session 59 | Δ |
|---|---:|---:|---:|
| Plugin suite (`-arg-test-plugins`) user, median of 6 | 2.765 s | 2.705 s | **−2.2%** |
| Plugin suite wall, median of 6 | 2.08 s | 2.005 s | **−3.6%** |
| Main suite (`-arg-minimal`) user, median of 9 | 0.61 s | 0.60 s | **−1.6%** |
| Main suite wall, median of 9 | 0.48 s | 0.47 s | **−2.1%** |
| Peak RSS (plugin suite) | 145.4–148.1 MB | 144.3–147.0 MB | flat |
| `TCCCompiler` binary | 5,216,064 B | 5,200,816 B | **−15,248 B (−0.29%)** |

Sampling profile (`-pg`) puts the moved terms where the change is: `map`'s self
time dropped 6.2% → 2.7% of the run, and the `_Hashtable`/`_Rb_tree`
instantiations the interning caches needed are gone from the binary.

**Output-invariant, by diff.** The emitted C of every module of two suites (main
suite 26 files, plugin suite 26 files) is **byte-identical** between the two
binaries (`diff -r` → 0 lines).

**Rejected on measurement (kept here so they are not re-tried):**

- *Canonical-id caches for pointer/primitive types* (a dense per-thread
  `pointee -> TypeId` table, removing ~360k of the 400k `intern` calls). Measured
  **0%** wall-clock change and **+33 KB** of binary — `intern`'s hash hint already
  answers these. It also has to be per-thread, not a builder member: lowering
  runs on a `ctpl::thread_pool` worker and a growing member vector there was a
  real heap corruption. Not landed.
- *Opcode metadata tables* (`opcode_flags`/`opcode_name`/`opcode_portability` as
  `constexpr` arrays instead of switches, ~2–4 KB and one indexed load per
  instruction). GCC rejects the natural spelling of it — non-trivial designated
  array initializers (`[static_cast<uint32_t>(MIROpcode::X)] = …`) are
  unimplemented — and the template/`index_sequence` workaround reintroduces the
  same three parallel lists it was meant to remove. Not landed.
- *Dropping `intern`'s fallback linear scan* (it exists so a record mutated after
  interning can still be found). Skipping it entirely measured **no** change, so
  the safety net stays.

| Suite | Result |
|---|---|
| `MIRTests` | **4524 checks** (new `test_key_map`) |
| `--tcc` | **2307/2307** |
| `--llvm` | **2305/2305** |

### Session 58 (2026-10-20) — short-circuit `&&`/`||` reconstruction in the C emitter (action C)

`a && b` / `a || b` lower to a CFG shape the legacy 2c translator never
emitted: the LHS is stored to a `_Bool` temporary, the branch tests it, the RHS
runs in its own block and stores to the same temporary, and the merge block
loads it (`p = a; if (!p) goto end; p = b; end: …p…`). C has no optimizer here,
so the generated code carried the branch, the RHS block, its store and the merge
load for every logical operator.

The emitter now recognizes that exact shape and folds it back to an expression
when the RHS region is one straight-line block whose non-terminator
instructions are all spellable as C expressions (the generic `expr_of_inst`
gained `Load`, `Call` and `CallIndirect` cases for this; they are not reachable
from the single-use inline pass, which handles `Load` on its own and never
inlines a `Call`). The LHS store is kept — `a` must still evaluate exactly where
it did — the `CondBr`, the RHS block and its store are skipped, and the merge
load becomes:

- `<place> && <rhs>` / `<place> || <rhs>` folded into the value's single,
  *immediately adjacent* use (no temporary), or
- a `_Bool v = <place> && <rhs>;` declaration at the merge point when the value
  has several uses or a later one, so the RHS is evaluated once, exactly where
  the merge used to be.

The adjacency rule is a correctness requirement, not a size heuristic: inlining
at an arbitrary later use moves the RHS *past* anything between the merge and
that use — e.g. `return v.size() == 2 && v.get(1) == 20` read `v` after its
destructor. Reconstruction is single-pass (left-associative chains
`a && b && c` reconstruct level by level; a right-associative RHS has a nested
block start and is left as CFG), requires a single jump into the RHS block and
no other block start inside the region, and refuses a `CondBr` whose condition
is not the stored LHS value.

Two emitter-wide consequences:

- **The inline map is rebuilt after reconstruction.** The generic single-use
  pass ran first, so consumers had already been inlined with the merge spelled
  by its old name (`!p`); overriding the merge afterwards left them reading only
  the LHS and silently dropping the RHS. The pass is now a re-runnable lambda;
  after any reconstruction the map is cleared, the reconstructed merge
  expressions are frozen, and the pass runs again so every consumer sees them.
- **Dead labels are dropped.** A block nothing jumps to after the rewrite no
  longer prints its `__chx_bbN:;` (the RHS block disappears and the merge is
  now the fall-through).

Measured (`Release`, `debug_quick`, same source): probe C **9,198 → 8,927**
lines (−2.9%) and probe executable **398,265 → 392,153 B** (−1.5%); like-for-like
main-suite `-arg-minimal` executable **3,858,914 → 3,811,879 B** (−1.2%; link
order moves this by a few KB, the C line count is exact).
`MIRTests` **4506**; `--tcc` **2307/2307**.

### Session 57 (2026-10-19) — the declaration/definition path is MIR records end to end (action A3, endgame)

The last AST re-reads on the C declaration path are gone. Three moves:

**The declaration list is module data.** `MIRDeclList` used to be a local on
`MIRDeclEmitter`; it is now `MIRModule::declarations`, so the ordered list of
forward tags, aggregate bodies, typedefs, prototypes, global declarations and
vtable type/externs is a first-class MIR module table the front end fills and
the AST-free `MIRDeclPrinter::decl_list_c` renders. (An include cycle had to be
broken first: `MIRDeclList.h` → `MIRPrototype.h` → `MIRModule.h`; `MIRPrototype.h`
now forward-declares `struct MIRModule` instead of including it.)

**A global's *definition* is the `MIRGlobal::decl` record.** `MIRGlobal` gains a
`MIRGlobalDecl decl` filled by `MIRLowerer::build_global_initializer` (the
name-agnostic spelling split out of `build_global_declaration` as
`build_global_declaration_type`, with the name taken from the symbol table so it
cannot drift from the symbol the module declares). `MIRDeclEmitter::implement_global`
now takes that record — no `VarInitStatement` re-read, no re-mangle at implement
time.

**A vtable's *definition* is a `MIRVtableRecord`.** The hand-rolled callee
resolution in `MIRDeclEmitter::create_vtable`/`create_vtable_primitive` is gone;
both now call the shared `mir::build_vtable_def_record` (the same function the
LLVM driver reaches through `build_module_vtables`) and print it with the new
AST-free `MIRDeclPrinter::vtable_def_c` (container impl: `(type) callee,`;
primitive impl: `callee|0,` with no cast). The record is produced by the shared
builder rather than read from a prebuilt `module.vtables` because the impl set is
discovered by the declaration walk — including `#json`/macro *embedded* impls that
only the walk resolves — so a prebuilt node set would be fragile while adding no
shared logic. **A latent LLVM-path bug was fixed in passing:** for an inherited
interface method the slot's active user must be set on the *method's declaring
interface* (`func->parent()`), not on the interface being implemented, or the slot
names the base method (`…Interfacegive`) instead of the container's instantiation
(`…Interface_Impl_give`). The C pass had used the declaring interface; the shared
builder now does, for both backends.

`MIRTests` gains `test_vtable_def_printer` (pins both spellings and the null/
`__chx_unresolved` fallbacks). The `MIRTests` target also had to be un-broken: it
referenced `chem::comptime_stack_error_message` (`MIRInterpreter`) without linking
`utils/StackSpace.cpp` — pre-existing since Session 56; the source is now in the
target.

| Suite | Result |
|---|---|
| `--tcc` | **2307/2307** |
| `--llvm` | **2305/2305** |
| `--tcc --interpret` | **1896/1896** |
| `--tcc --negative` | **318/318** |
| `--tcc --plugins` | **1296/1296** |
| `MIRTests` | **4495 checks** |

### Session 56 (2026-10-19) — negative suite green: comptime stack containment + front-end duplicate diagnostics

The last failing suite is now clean: `--tcc --negative` **318/318**. Two unrelated
classes of bug were behind it.

**Unbounded comptime recursion no longer crashes the compiler.** A `comptime func`
that calls itself (`return f()`, or `f(n-1)` with `n = 1000000`) made the AST folder
(`FunctionDeclaration::call` → `fold_expression_body`), the MIR comptime driver
(`MIRComptime::evaluate*`) and the interpreter (`MIRInterpreter::call_function`) all
recurse until the host stack overflows. **`utils/StackSpace.{h,cpp}`** now owns a
*sticky, per-thread* state: `comptime_stack_exhausted()` (the low-stack detector,
512 KB reserve) plus `comptime_stack_failed()`/`mark_comptime_stack_failed()` and
`comptime_stack_reported()`/`mark_comptime_stack_reported()`. The first detector marks
*failed*; the first reporter emits the diagnostic once and marks *reported*; every later
evaluator bails silently. `MIRInterpreter::Impl` gains a `stack_exhausted` flag so its
top-level `call`/`call_sret`/`run` report and every nested `call_function` returns
immediately. `ASTProcessor`'s comptime-call and comptime-block lambdas report once
through the shared state. (A local flag that reset on healthy-stack entries produced
652 duplicate reports; a merely-sticky local still flooded the "statement body"
fallback past the negative runner's 16 KB capture — one message per thread is what
stays inside it.)

**`MIRComptime::evaluate_block` re-entrancy.** Lowering a comptime block did not mark
`impl_->lowering`, so a comptime call reached while the synthetic block function was
being lowered re-entered the same lowerer and cleared its builder
(`neg_comptime_non_const_expr`). It now sets a `LoweringGuard` for the block and
forwards nested blocks to a fresh driver, mirroring `evaluate`.

**Front-end duplicate diagnostics.** `func foo(a : int, a : int)` and two same-named
interface methods were accepted (the second silently shadowing the first, leaking a
duplicate C member). `MembersContainer::take_members_from_parsed_nodes` now rejects a
duplicate function name **except a constructor overload** (the std `string`/`span`
`make`/`make2` overloads rely on it — resolved by argument type from
`non_gen_range()`). `SymResLinkBody::VisitFunctionParam` uses the new
`SymbolTable::current_scope_start()` to reject two parameters with the same name in the
*same* list while still allowing a lambda parameter to shadow an enclosing one. A bare
`_` is now rejected as a struct/union field name (`var _ = expr` remains a valid local
discard). The redundant `getMembers`/`getFunctions` duplicate in `compiler`'s `UnionDef`
is removed.

**memset worked around, not fixed — now fixed.** `CTranslationContext::prepare_translate`
declares `memset`/`memcpy`/`memmove` with the target `size_t` spelling before any module
is translated, so MIR's bare `MemSet`/`MemCpy` see a prototype and cstd's later
declaration stays compatible (supersedes the inline-zero-loop workaround).

| Suite | Result |
|---|---|
| `--tcc` | **2307/2307** |
| `--llvm` | **2305/2305** |
| `--tcc --interpret` | **1896/1896** |
| `--tcc --negative` | **318/318** |
| `--tcc --plugins` | **1296/1296** |
| `--tcc --async` / `--regexp` / `--process` | **53/53**, **143/143**, **127/127** |
| `--tcc --libs` | **716/719** (the known `test_fs_directory_iteration` + a load-induced bcrypt timeout; bcrypt passes in isolation) |
| `MIRTests` | **4489 checks** |

### Session 51 (2026-10-17) — interpreter setup stops dominating (single-writer tables, incremental side tables, lazy field slots)

Constructing a `mir::MIRInterpreter` and taking the shared table locks were the
largest fixed costs left on a comptime-heavy path: a comptime driver lowers a
whole callee graph but executes only a handful of it, so most of the work done
at construction was thrown away. Four changes, all in the shared core:

- **Lock elision for a single writer** (`compiler/mir/MIRTableLock.h`, new). The
  module tables took a `std::shared_mutex` on every access — required for
  parallel lowering (action E1), pure overhead while one thread is lowering.
  Each table now owns a `MIRTableLockPolicy`, and its accesses go through
  `MIRReadLock`/`MIRWriteLock`, which take no lock at all once the owner calls
  `set_single_threaded(true)`. `ASTProcessor` marks every `MIRModule` it builds
  that way (`!kEnableParallelLowering`, the flag session 47 gated off, now at
  namespace scope), so the shipping pipeline reads its tables unlocked while
  readers may still run concurrently — the flag only rules out a concurrent
  *writer*. Turning the parallel driver back on flips the same flag and the
  old locking behaviour comes back.
- **Incremental side tables + `MIRInterpreter::update()`**. Every MIR table is
  append-only with stable ids, so each `build_*` now consumes only the records
  added since its last pass (one `built_*` cursor per table; a table that
  shrinks resets its cursor). `update()` runs them and is called from the
  constructor, `call`, `call_sret` and `run` — not from `has_function`, which
  stays `const`.
- **Field slots are built on first call** (`ensure_field_slots`). The old
  constructor resolved every field/index operand of *every* lowered function
  into a 16-byte slot; that was the single biggest construction cost. Slots are
  now built per function the first time it runs, with `operand_count` detecting
  an in-place body replacement.
- **One comptime engine** (`MIRComptime::Impl::engine()`). A comptime call used
  to rebuild an interpreter; it now creates one and `update()`s it when the
  module has grown.

**The `const_str` bug this introduced, and the fix.** String constants moved
from a `malloc` per constant into one `std::deque<char>` blob. That is wrong in
a way that looks harmless: a deque is contiguous only *within* an internal
chunk (512 bytes for `char`), while `const_str` is handed to interpreted code
as a raw `*char` that is read linearly — so a string straddling a chunk
boundary walks into the unrelated heap after the first chunk. On
`FILE_ORDER_SEED=42 ./scripts/test.sh --tcc --interpret` **177** constants read
foreign memory and `Test 1793 [string literal exact-fit char array]` failed
(HEAD passes that seed, and the compiled `--tcc` suite passed it too, so the
fault was interpreter-only). Canary bytes after every constant proved nothing
was *written* out of bounds — only reads diverged. Storage is now a 4 KiB block
arena (`intern_string_constant`): an entry that does not fit in the current
block starts a new one, so no constant ever straddles, the pointers stay valid
as the append-only module grows, and it still costs one allocation per 4 KiB
instead of one `malloc` per constant.

Measured (cold `lang/tests/build.lab`, `--no-cache`, same command on both
binaries, `MIR_OPS_COUNTERS=OFF`, runs uninterleaved):

| Workload | HEAD | Session 51 | Δ |
|---|---:|---:|---:|
| Main-suite compile, `debug_quick` (5 runs) | 1.772 s | 1.678 s | **−5.3%** |
| Interpretation job, `debug_complete` (3 runs) | 1.070 s | 0.967 s | **−9.7%** |
| Main-suite compile, `debug_complete` (3 runs) | 1.867 s | 1.867 s | noise |
| Peak RSS | ~49 MB | ~49 MB | flat |

Green: `MIRTests` 4417 · `--tcc` 2304/2304 · `--llvm` 2302/2302 ·
`--tcc --interpret` 1893/1893 at the default seed **and** at
`FILE_ORDER_SEED=42` and `=7` · `--async` 53/53. `--negative` fails at HEAD too
(a `memset` redefinition in the translated C), so it is not this session's.

Housekeeping: the local CMake cache had `MIR_OPS_COUNTERS=ON` left over from
profiling; it is back to the documented default (`OFF`). With it on,
`MIRTests` does not link at all — the counter object it references lives in
`MIRLowerer.cpp`, which is not part of that target.

### Session 52 (2026-10-18) — throwaway `std::string`s on the print/emit/lookup paths

Sessions 37–44 removed the per-node *allocations* in lowering; what was left is
the other half of the same class — a temporary `std::string` built only to be
copied into a buffer, or a name copied out of MIR's own pools just to be
`==`-compared. Each one is small, but they sit on paths that run once per field,
once per constant-tree child, once per emitted instruction, and once per field
access the interpreter executes. Nothing here changes a byte of output.

- **The declaration printer appends.** `MIRDeclPrinter`'s member and constant
expressions returned a fresh `std::string` per element (`member_declarator_at`
built `c_declarator(...) + ";"`, `const_expr_c` recursed through `std::string`
concatenation, `decl_list_c` concatenated each item's whole body). All of them
now write into the caller's buffer: `append_member_declarator`,
`append_const_expr_c` (with local `append_int64`/`append_float`/
`append_double`), `append_global_decl_spelling`, `append_aggregate_body_c` and
`append_inline_aggregate_c`. The public string-returning functions remain as thin
wrappers (`MIRTests` asserts their exact text and it is unchanged), and
`agg`/`inline_agg` bodies keep their byte-for-byte spelling — including the
variant case's deliberately-short two-tab indent.
- **MIR's own names are readable without a copy.** `MIRTypeTable::name_view` /
`field_name_view`, `MIRConstExprTable::child_name_view` and
`MIRSymbolTable::mangled_view` return `std::string_view` over the
append-only pools (the contract `MIRBytePool` already documents), and the sites
that only compare or append a name use them: the interpreter's field-path
resolver (per field access), `CodegenMir`'s constant-tree `find_child` (per
child, and its `field_name_of(*f) == name` comparison), and
`mir_named_field_type` in the async lowering. `has_name` replaces
`!name_of(id).empty()` in the type builder.
- **One `std::string` per struct field is gone from the type builder.**
`fill_aggregate_fields`' `push_field` took `const std::string&` and every caller
built `std::string(nv.data(), nv.size())` from the AST's view; it takes
`chem::string_view` now. The variant case name is built in one reused
`thread_local` buffer instead of a three-way concatenation.
- **The C emitter's spellers take views and have public append forms.**
`append_c_type_of` / `append_c_declarator` moved into `MIREmitter.h` (with
`append_uint`) so the declaration printer can use them; `c_decl_type`,
`c_declarator` and `c_function_proto` accept `std::string_view`; and the ~20
`append_declarator(..., vname(v), out)` sites in the emit loop became
`append_vname_declarator(module, type, v, out)`, which writes `__chx_v<id>`
itself for the scalar/pointer shape every instruction actually has. A function
header is now printed straight into `out` (no parameter-list string), and
parameter names come from a stack buffer (`param_arg_name`) rather than
`"__chx_a" + std::to_string(i)`.
- **Vtables and comptime bindings.** `build_module_vtables` appends the mangled
name into each record's own string instead of building a `std::string` per slot
and copying it, and uses the shared `mangle_writer()` instead of a fresh
`BufferedWriter(256)` per vtable definition. `comptime_bindings_` gained the
transparent `MIRStringHash`/`MIRStringEq`, so `lower_expr`'s per-identifier
lookup (`comptime_bindings_.find(...)`) no longer constructs a `std::string`.
- **The interpreter's field lookup takes a view.**
`MIRInterpreter::resolve_field` takes `std::string_view` and slices the
designator with `substr` instead of allocating a `std::string` per dotted
segment; `constant_text_view` feeds it from the constant pool (the old path
allocated a copy of the designator on every `FieldAddr`/`IndexAddr`) and
`child_name_view` feeds it from the constant tree.

**Measured** on this checkout (`Debug`, `debug_quick`, `--no-cache`, `--emit-c`,
the same command on both binaries). Allocations are counted with an
`LD_PRELOAD`ed `malloc`/`calloc`/`realloc`/`free` counter, which is the metric
this class of change moves (wall time is in the noise):

| Metric | HEAD | Session 52 | Δ |
|---|---:|---:|---:|
| Main-suite `malloc` calls | 696,700 | 660,550 | **−36,150 (−5.2%)** |
| Main-suite bytes requested | 325.1 MB | 322.1 MB | −3.0 MB (−0.9%) |
| Main-suite `realloc` calls | 186,306 | 186,306 | flat |
| Main-suite compile, `debug_quick` (3 runs) | 1.71/1.72/1.72 s | 1.73/1.82/1.76 s | noise |
| Main-suite emitted C lines | 99,686 | 99,686 | identical |

**Output-invariant.** Main-suite C is 99,686 lines either side; the normalized
line multiset (generic `__cgs__N` and lambda `__chemda_N` numbers folded) differs
by **224–638** lines depending on the run, against a **788**-line same-binary
two-run floor, and every residual line is such a renumbering — no structural
difference.

| Suite | Result |
|---|---|
| `MIRTests` | **4417 checks** |
| `--tcc` | **2304/2304** |
| `--llvm` | **2302/2302** |
| `--tcc --interpret` | **1893/1893** |
| `--tcc --async` | **53/53** |
| `--tcc --libs` | **718/719** (the known environment-sensitive `test_fs_directory_iteration`) |

**Still string-based** (left for a later pass, all cold): `MIRDeclEmitter`'s own
prototype/vtable spellers, `MIRLowerAsync`'s per-async-function declaration
concatenations, the `InlineAsm` operand list, and `MIRComptime`'s name maps.

### Session 55 (2026-10-18) — E3d landed: lock-free stable tables + parallel lowering is a net win

The last blocker on E3d was the reader lock itself. E1/E3a/E3c made every table
*stable* under an append, but a reader still took a `std::shared_mutex` shared
lock so it would not race a realloc; under parallel lowering that hot lock's
refcount (one cache line) serialized the workers and made the phase a net loss.

**`compiler/mir/MIRStableArray.h`** — an append-only array whose elements never
move and whose reads take **no lock**. Storage is block-chunked (`new T[1024]`
blocks, never freed until destruction), so `&arr[i]`, a `MIRFieldInfo*` and a
`const char*` into a pooled string stay valid across any later append — the
identity guarantee the old `std::deque` gave. The block pointers live in a small
map published through an atomic pointer; growing the map copies the pointers and
*retains* the old map, so a reader that loaded an older map still finds every
block it could hold an id for. A reader is two atomic loads; a writer-input scan
(`find_writer`, the intern fallback) pays the indirection once per block.

- `MIRTypeTable::types`/`data`/`fields`, `MIRConstantPool::constants`,
  `MIRSymbolTable::symbols` and `MIRBytePool::entries` became `MIRStableArray`.
  Every read path (`get`, `size`, `name_of`/`name_view`, `data_at`, `field_of`,
  `name_data`, `data_at`/`text`) is now lock-free; only the mutators take the
  exclusive write lock, and only when the table is not single-threaded.
- The parallel phase already scopes `module.set_single_threaded(false)` around
  the worker run and back to `true` after the join, so a module that never enters
  it pays no lock at all. `MIRReadLock` is now unused.
- **A copy-on-grow design was tried first and rejected:** publishing a fresh flat
  buffer on growth breaks element *identity* — a caller holding a `MIRFieldInfo&`
  across a growth reads the old buffer — and failed comptime tests 714/715
  deterministically. Chunked storage (elements fixed in place) is required.

**The storage change exposed a latent bridge lifetime bug (fixed here).** The
comptime driver's `MIRModule` is a local in the lowering pass, but the AST values
it bridges back (`StructValue::values` keys, a bridged `*char` string) were
`chem::string_view`s pointing *into* the driver's `field_names` pool (or a
caller-local `std::string`). `codegen` reads those values after the driver is
gone, so the keys dangled; `std::deque`'s freed chunks kept the bytes alive by
luck, while the stable array's blocks are reused in place by the next driver and
clobbered them deterministically. `MIRComptime`'s `read_struct_value`/
`read_union_value`/`read_scalar_value` now copy the bytes into the AST allocator
(`stable_name` → `BatchAllocator::allocate_str`, lived until the AST dies), which
is the correct lifetime.

**Pre-lowering also had to restore declaration-before-body ordering.** A body's
`#html`/macro lowering reads signatures the top-level embedded declarations
publish — a `#styled` wrap's generated class name is the concrete case (the
`styled_wrap_owns_css_linked_to_element` plugin test). The serial pass resolves
each declaration at its source slot, so a later body sees the signature
populated; pre-lowering a body ahead of that slot ran its `#html` conversion
against a half-initialized signature (the declaration-only replacement had run,
the full one had not) and dropped the wrapper class. `mir_presolve_embedded_decls`
now resolves the top-level embedded declarations first, and `process_nodes`
reuses those replacements so the side-effecting macro still runs exactly once.

**Measurements** (main suite, `--mode debug_complete --no-cache`, loaded 12-core
box, min of alternating runs): serial min wall 3.21 s / user ~4.03 s / RSS
~57 MB; parallel min wall **2.71 s** / user ~4.05 s / RSS ~61 MB → **wall −15 %,
user unchanged, RSS +~7 %**.

**Output-invariant.** Emitted C for a 40-function module is byte-identical
(normalizing GEP temps) across `CHEM_MIR_LOWER_WORKERS=1` and `=8`, and against
the pre-change serial binary.

**Verification.** `--tcc` **2307/2307** · `--llvm` **2305/2305** ·
`--tcc --interpret` **1896/1896** · `--tcc --async` **53/53** ·
`--tcc --plugins` **1296/1296** · `MIRTests` **4489 checks**.

### Session 54 (2026-10-18) — shared traversal, operand decode and naming (action B)

The last "still open in B" architectural item was the walk and the lookups
themselves. Three translation units carried their own copy of the one-line
operand accessor (`MIREmitter.cpp`, `MIRFacts.cpp`, and a lambda inside
`CodegenMir::mir_emit_function`), the block's instruction-range arithmetic
(`inst_start + inst_count + 1`, clamped) was open-coded in `CodegenMir`, and the
LLVM emitter spelled the same symbol-name and string-constant reads three and
three more times by hand. None of it is target-specific.

New AST-free, target-free core header **`compiler/mir/MIRTraversal.h`**:

- `operand_at(fn, inst, i)` — the one operand-decode accessor.
- `MIRBlockRange` / `block_range(fn, block)` — a block's `[begin, end)` in the
  instruction stream (terminator included, clamped to the stream).
- `for_each_block` / `for_each_block_inst` — the traversal idiom a new backend
  reads instead of re-walking; `MIRDump` now uses them.
- `symbol_name_view(module, sym)` — the linker name (mangled, plain-name
  fallback) as a view into the stable name pool.
- `constant_string_view(module, cid)` — a String constant's bytes as a view into
  the stable data pool.

`MIREmitter` and `MIRFacts` drop their `operand_at` copies; `MIREmitter`'s
`symbol_name`/`constant_string` (and their append twins) and `MIRInterpreter`'s
`constant_text`/`constant_text_view` now go through the shared views;
`CodegenMir` drops its `arg` lambda body, its block-range arithmetic and its six
hand-rolled symbol/constant reads.

**`MIRValueFacts` (the other half of the item).** `MIRFacts.h` gains
`compute_value_facts`: per value, `def_opcode`, `single_use` and
`single_use_pure` (`single_use` and the defining opcode is `OF_Pure`) — the
classification a compacting backend reads before it recomputes a value at its
use site. The C emitter's inline pass now reads `single_use` instead of poking
`MIRValueDef::use_count` directly; the load-fusion / integer-promotion *policy*
stays backend-side.

**Output-invariant.** Every substitution is provably identical: the lowerer
always interns a symbol's `name` and `mangled` spelling to the same bytes, so
the mangled view equals the plain name `MIREmitter` read; `data.text(offset)`
is exactly the `(data_at, data_count)` payload; `block_range` is the same
clamp; `single_use == (use_count == 1)`. `MIRDump`'s walk is the same
non-terminators-then-terminator sequence.

Green: `--llvm` **2305/2305** · `--tcc` **2307/2307** ·
`--tcc --interpret` **1896/1896** · `MIRTests` **4462 → 4489 checks** (new
`test_value_facts`, `test_traversal_helpers`). No lowering or output change, so
compile time, peak memory and binary size are untouched.

### Session 53 (2026-10-18) — the aggregate access plan is MIR's, not the LLVM backend's

Session 10 put every aggregate's `size`/`alignment`/field-`offset` in MIR
(`MIRLayout.h`, action A1). A backend that lets a C compiler lay the type out
(the C target) only needs those numbers. A backend that *builds* the aggregate
itself needs one more thing: an explicit ordered element list — one element per
padding run, and one **shared** element per union-member / variant-case overlap
group (a struct type cannot have two fields at the same offset). The LLVM
backend had derived that list inline in `mir_type` (~70 lines of overlap-group
selection and padding walk), keyed by two ad-hoc maps in `Codegen`
(`mir_agg_elems_`, `mir_field_elem_`).

That list is a pure function of the type table — identical for every such
backend — so it now lives in the core as `mir::compute_aggregate_plan` /
`MIRAggregatePlan` / `MIRAggElement` (`compiler/mir/MIRLayout.h`, next to
`compute_aggregate_layout`). It reports, per aggregate, the ordered elements
(representative type + field range, or a padding byte count) plus the
field → element map; the backend only spells each element (`mir_type(rep_type)`,
or `[pad x i8]`) and keeps the plan for GEP and constant building. `Codegen`
stores the plan in `mir_agg_plans_`; `mir_field_elem`, the constant builder's
`plan_for` and the GEP path are behaviourally unchanged (they now read the core
plan). `CodegenMir::mir_type`'s aggregate case shrank to a loop over the plan.

**Output-invariant, on purpose.** The plan is the same sequence the inline code
produced: an aggregate with no explicit alignment and no overlap maps field *i*
to element *i* exactly as before; union/variant groups pick the same
largest-aligned member; padding runs are the same byte counts. Nothing here
touches lowering, the C backend or the interpreter.

**Coverage.** New `MIRTests::test_aggregate_plan` pins the plan for
interior/trailing padding, a union group, a variant (tag, pad, shared case
group), an over-aligned `@maxalign` type and an explicitly aligned member — the
same shapes `test_aggregate_layout` pins against the C compiler, but at the
element level. `MIRTests` 4417 → **4462 checks**.

Green: `--llvm` **2305/2305** · `--tcc` **2307/2307** ·
`--tcc --interpret` **1896/1896** · `MIRTests` **4462 checks**. No lowering or
emission change, so compile time, peak memory and binary size are untouched.

### Session 50 (2026-10-16) — the interpretation job runs on MIR

`--arg-interpret` no longer uses the AST interpreter. `do_interpretation_job`
now lowers every module to MIR and runs that MIR: the job builds one shared
`mir::MIRInterpProgram` (module + arena + task), each module is lowered in
collect mode via the new `ASTProcessor::interpret_module`, and `main` is run by
`mir::MIRInterpreter`. `interpretation_mode` is off, so tests take the compiled
(`printf`) branches — interpretation is the same semantics as C/LLVM, which is
the whole point. See `lang/docs/mir-interpretation-plan.md` (Phases 0–4 done).

The engine (`compiler/mir/MIRInterpreter.{h,cpp}`) is AST-free and
single-threaded. Load time builds flat side tables (type facts, field offsets,
callee targets, constant bits, global storage, vtable slots); the loop reads
only those. It implements the scalar/control core, aggregates as addresses with
`Load` copies, C integer promotion/conversion, C pointer arithmetic
(including addressable values), the sret/aggregate-by-pointer ABI, drops and
destructors, and interface `DynamicCall` through the shared `MIRVtableTable`.
Host bindings cover the libc the suite uses (`printf` with a small formatter,
`malloc`, `memcpy`, `memset`, …).

Two MIR bugs surfaced and were fixed on the way:

- `MIRTypeBuilder::fat_pointer_type` never computed its field offsets (both 0);
  it now calls `compute_aggregate_layout`. The C emitter spells fields by name
  so it is unaffected — but the interpreter and the LLVM GEP path need real
  offsets. This is the one change with any reach beyond the interpreter; both
  `--tcc` and `--llvm` stay green.
- `build_module_vtables` cleared `module.vtables` on every call; it now takes a
  `clear` flag so the interpretation job (one shared module across source
  modules) accumulates vtable records instead of keeping only the last
  module's.

Green: `MIRTests` 4400 · `--tcc` 2307/2307 · `--llvm` 2305/2305 ·
`--tcc --interpret` **1896/1896 (through the engine)** · `--libs` 718/719
(known env failure) · `--async` 53/53.

Phase 5 (comptime on MIR) is **done**: the AST statement interpreter
(`compiler/Interpreter/Core.cpp`) is deleted. A lowering-time comptime call is
lowered and executed by `compiler/mir/MIRComptime.cpp` (a statement-level
`comptime { }` block too, via `evaluate_block`); the AST evaluator that survives
is the expression layer only — symres-time folding of self-contained expressions
and the compiler-intrinsic `call` overrides (see
`lang/docs/mir-interpretation-plan.md` §5.3.1/§5.4).

### Session 49 (2026-10-16) — the E3d blocker is cross-function AST/comptime contamination

Instrumented the parallel path with a diagnostic that fires when `intern_global`
is reached for a **non-top-level** `VarInitStatement` (a local/parameter
mis-resolved as a global). On the main suite it fires, e.g. for `world` — the
parameter of `make_greeting_from_expr`, whose `${world}` reference should resolve
to the parameter's place. The corrupted C corroborates it: the parallel output for
`make_greeting_from_expr` referenced `greeting`, a local of a *different*
function.

So the failure is **not a missing field lock**: a worker's lowering resolves a
reference through a `linked`/comptime value owned by — or mutated by — another
function. The candidates are the shared AST (`Identifier::linked`, mutated by a
concurrent comptime evaluation) and the module-level captured-comptime ref pool.
Making this safe needs one of:

- per-worker AST views for the nodes comptime evaluation can rewrite, or
- serialising comptime evaluation **and** every AST read that can observe its
  result (a large surface), or
- the plan's "seal" design (lower serially, freeze the tables, emit in parallel).

The driver stays off (`kEnableParallelLowering = false`); the diagnostic was
removed. `MIRTests` 4400; `--tcc` **2307/2307**; main-suite C unchanged.

### Session 48 (2026-10-16) — hunting the E3d race with ThreadSanitizer

Added `-DENABLE_TSAN=ON` (a `TCCCompiler`-only ThreadSanitizer build, mirroring
`ENABLE_ASAN`) and ran the enabled parallel-lowering path under it. TSan found
real races; three were in E3d's path and are fixed:

- **Unlocked `data`-pool reads.** `emit_function_c` (and the async emitter) read
  `module.types.data[i]` directly to get a function type's parameter types,
  racing `MIRTypeTable::append_data`'s `push_back`. Added locked
  `MIRTypeTable::data_at`/`data_size` and used them in `MIREmitter.cpp` /
  `MIRLowerAsync.cpp`.
- **Comptime captured-value reads.** `lower_expr` read
  `CapturedComptimeVariable` / `%runtime_value` captured refs' `.evaluated` while
  the comptime interpreter (`eval_comptime_call`, already lock-guarded) lazily
  filled them. Added `MIRLowerer::captured_comptime_value_locked` /
  `captured_ref_value_locked` and used them.
- **Constructor-fact cache.** `prime_constructor_fact` /
  `construction_writes_whole_value` read and wrote the shared `FunctionDeclaration`
  `ctor_whole_value_known` flag; now under the visitor lock.
- Removed the visitor lock from the **type-name resolver** (it reads no shared
  mutable mangler state, and holding it inverted the builder→visitor lock order
  `map` uses).

TSan also flagged many **pre-existing** races in the parallel *symres*
(`FunctionDeclaration::set_has_usage`, `TypeLoc`/`BaseType` reads) — not E3d.

**Still gated off.** With the fixes the crash is gone, but the emitted C is still
corrupted nondeterministically — e.g. a forward declaration's name gains a path
fragment (`main_Deletable` → `test-cbor_main_Deletable`) and the file contains NUL
bytes. TSan did not catch this one as a plain field race, so it is either a
different class of bug (a shared `BufferedWriter`/name buffer reused across
workers) or a race TSan's schedule missed. `kEnableParallelLowering` stays false;
the race fixes and the TSan option are kept (they are correct either way).
`MIRTests` 4400; `--tcc` **2307/2307**; `--llvm` **2305/2305**; `--tcc --async`
**53/53**; `--tcc --interpret` **1897/1897**; main-suite C unchanged.

### Session 47 (2026-10-16) — E3d stage 3: parallel-lowering machinery + two shared-state fixes (driver gated off)

Built the E3d stage-3 machinery and closed two more shared-state gaps, then found
a remaining race and **gated the parallel driver off** rather than land corrupted
output.

Landed:

- **`MIRTypeBuilder` is lock-guarded.** It is shared by all workers and its
  `memo_`/`decl_ids_`/`decl_types_`/`fat_pointer_id_`/`inline_counter_` caches
  were unsynchronized. `map` takes a recursive `builder_mtx_` (it recurses);
  `named_struct` and `aggregate_type`/`fill_aggregate_fields` use the locked
  `MIRTypeTable::push`/`update`/`size` instead of touching `types[]` directly.
- **`MIRBytePool` reads are lock-guarded.** The entries are stable, but
  `std::deque`'s chunk map can reallocate on a concurrent `push_back`, so a bare
  `data_at` read raced the append. Appends take an exclusive lock, reads a shared
  one; the returned bytes stay valid after the lock (entries are immutable).
- **The machinery**: `mir_lower_and_emit_into` (the C-path body of
  `lower_and_emit`, extracted so a worker can run it against its own
  `MIRTaskContext`), `mir_collect_parallel_functions` (the source-order collector
  mirroring `process_nodes`), `MIRLowerer::copy_callbacks_from` (a worker's
  lowerer reuses the driver's callback wiring), and the `ctpl::thread_pool`
  driver that pre-lowers the plain functions into per-function buffers that
  `lower_and_emit` appends at their source-order slots. `MIRModule::record_inline_struct`
  is now locked (workers append to it).

**Blocker — the driver is gated off (`kEnableParallelLowering = false`).** With
one worker the output is byte-identical to serial, but with more than one worker
the **mangled name written into the type/symbol pools is corrupted**
(nondeterministically, e.g. `main_Deletable` → `main\0github._Deletable`). Locking
the pools/fields and the builder did not fix it, so a shared *name-mangling*
buffer still races despite `mir::mangle_writer()` being `thread_local`; the next
step is to find that buffer (candidates: a `NameMangler` scratch, the `mangle_buf_`
path, or a `decl_list_`/`MIRDeclItem` interaction) before re-enabling. Until then
the compiler is exactly as before: `MIRTests` 4400; `--tcc` **2307/2307**;
`--llvm` **2305/2305**; `--tcc --async` **53/53**; `--tcc --interpret` **1897/1897**;
`--tcc --libs` **718/719**; main-suite C unchanged (96,825 lines, normalized diff 50).

### Session 46 (2026-10-16) — E3d stage 2: the shared `CTranslationContext` access is serialised

Stage 1 made the module tables safe to read concurrently; the other half of the
E3d blocker is the shared `CTranslationContext` the lowering callbacks touch. A
single **`std::recursive_mutex`** now guards every such access, so a worker's pure
MIR computation stays lock-free and only the shared operations serialize:

- every callback wired in `implement_module` / `emit_static_interface_stubs` — the
  name resolver, the source-location provider, the manglers, the comptime
  if-resolver / eval / value-eval / ctor-eval / block-eval, `set_current_func`,
  the embedded node/value resolution, `async_normalize` and `symbol_lookup` —
  takes the lock for its body.
- `MIRLowerer` holds the same lock (`set_visitor_mutex`) around
  `intern_method_function`, which sets `iface->active_user` and mangles (the
  mangler callback re-locks recursively — hence a recursive mutex).
- `mir_translate_after_declaration` takes the lock and holds it across the
  `iface->active_user` set / lower / restore in `lower_impl`, and around every
  `visitor.error` diagnostic, so `active_user` (read by the mangler) and the
  `Diagnoser` are never touched by two workers at once.

Recursive, so a locked operation that calls another locked one (mangling inside
`intern_method_function`) does not deadlock. Output-invariant: main-suite C is
96,825 lines either side, normalized diff 48 (floor 62). `MIRTests` 4400;
`--tcc` **2307/2307**; `--llvm` **2305/2305**; `--tcc --async` **53/53**;
`--tcc --interpret` **1897/1897**; `--tcc --libs` **718/719**.

**Next for E3d** (stage 3): the thread pool and the ordered merge — dispatch the
function tasks to a `ctpl::thread_pool` with one `MIRTaskContext` per worker, and
merge each worker's buffer into `visitor.writer` in source order, interleaving the
serial structural units (globals, vtables, the inline-struct `move_range`).

### Session 45 (2026-10-16) — E3d stage 1: the module tables are lock-safe for concurrent access

Parallel lowering and emission need the module intern tables to be safe to read
while another worker appends. E3a/E3c made the byte/element pools reference-stable,
but the *record* vectors (`MIRTypeTable::types`, `MIRConstantPool::constants`,
`MIRSymbolTable::symbols`) still returned references from `get()` and reallocated
on append — so a concurrent reader's reference dangled. This stage closes that and
makes every record mutation locked:

- `get()` on all three tables now returns the record **by value** under a
  **`std::shared_lock`** (`size()` too); the E1 `std::mutex` became a
  `std::shared_mutex`, so any number of readers run concurrently and a writer
  excludes them. `MIRTypeTable::field_of` locks its record read and returns a
  pointer into the stable `fields` deque.
- Direct record writes are gone: `MIRTypeTable` gained locked `push` (variant
  case payloads / synthesized structs) and `update` (the
  `data_count`/`field_index`/`explicit_alignment` fill); `MIRSymbolTable` gained a
  locked `set_type` (the function-signature back-patch). `fill_aggregate_fields`,
  the async-frame and lambda-closure fills and every `symbols[i].type = …` use
  them, so no `types[i]`/`symbols[i]`/`constants[i]` is touched outside the lock.
- `MIRConstExprTable` stays lock-free on purpose: global initializers are built and
  emitted as serial units (globals are not parallelized), so it is never touched by
  two workers at once.

Output-invariant: main-suite C is 96,825 lines either side, normalized diff 46
(floor 62). `MIRTests` 4400; `--tcc` **2307/2307**; `--llvm` **2305/2305**;
`--tcc --interpret` **1897/1897**.

**Next for E3d** (not yet done): the shared `CTranslationContext` state — the
comptime callbacks, `iface->active_user`, `visitor.error`/`Diagnoser`,
`current_func_type`, the AST allocator and the embedded-replacement path — still
needs per-worker ownership or a shared lock, and the C output merge must be
slot-direct in source order. The table lock alone does not make the
walk-and-emit loop parallel-safe.

### Session 44 (2026-10-16) — the mangler appends into a reused buffer

`MIRLowerer`'s `mangler_` callback returned a fresh `std::string` for every
mangled declaration (function, global, interface default method). It now has an
append-style counterpart (`set_mangler_into`) and a reusable `mangle(ASTNode*)`
that writes into one member buffer; `intern_function`, `intern_global`,
`intern_method_function`, the dynamic-dispatch vtable name and the async mangled
name all consume it. Each caller uses the buffer immediately and never holds two,
so a single buffer is safe (documented on `mangle`).

Output-invariant: main-suite C is 96,825 lines either side and the normalized
diff is **58** (floor 62). `MIRTests` 4400; `--tcc` **2307/2307**; `--llvm`
**2305/2305**; `--tcc --async` **53/53**; `--tcc --interpret` **1897/1897**;
`--tcc --libs` **718/719**.

### Session 43 (2026-10-16) — one fewer heap vector per unqualified field access

`lower_member_chain` took a `std::vector<Value*>&`, and the unqualified-field
path in `lower_expr` built a two-element `std::vector` for every bare `field`
(an implicit `self.field` inside a method) — a heap allocation per access. It now
takes `Value* const*` + count; the field path uses a stack array and the other two
callers pass `vec.data()`.

Output-invariant: main-suite C is 96,825 lines either side and the normalized
diff is **52** (floor 62). `MIRTests` 4400; `--tcc` **2307/2307**; `--llvm`
**2305/2305**; `--tcc --interpret` **1897/1897**; `--tcc --libs` **718/719**.

### Session 42 (2026-10-16) — reuse the per-function emitter scratch tables; drop a field-path copy

- `emit_function_c` no longer allocates its per-function scratch on every call:
  `MIRFunctionFacts` (seven vectors), the inline-expression table
  (`std::vector<std::string>`) and the parameter-type list are now single reused
  `thread_local` objects (`assign`/`clear` keep their capacity), and `MIRFacts`'
  internal `candidate` table is the same. One emitter runs per thread, and
  emitting two functions is never nested, so a thread_local is the right home.
- `member_field_path` writes the (possibly inherited) field path straight into
  its output instead of building an intermediate string and copying it, once per
  member access.

Output-invariant: main-suite C is 96,825 lines either side and the normalized
diff is **50** (the same-binary floor is 62). `MIRTests` 4400; `--tcc`
**2307/2307**; `--llvm` **2305/2305**; `--tcc --async` **53/53**; `--tcc --interpret`
**1897/1897**; `--tcc --libs` **718/719**.

### Session 41 (2026-10-16) — the C emitter appends straight into the buffer

The emitter built a heap `std::string` per operand / type / field access:
`place_name`/`vname`, `operand_expr`, `c_type_of`, `declarator` and the
`member_access`/`const_literal`/`symbol_name`/`constant_string` helpers all
returned a fresh `std::string`, and the emit loop concatenated them
(`out += declarator(...) + " = " + operand_expr(...) + ";\n"`). For a Debug
compiler that is a construction — and, for the longer aggregate / mangled names,
a heap allocation — per operand of every one of ~300k instructions.

Every speller now has an **append** form writing into `out` directly
(`append_place_name`, `append_vname`, `append_operand_expr`, `append_c_type_of`,
`append_declarator`, `append_member_access`, `append_const_literal`,
`append_symbol_name`, `append_constant_string`, `append_escape_c_string`,
`append_fn_ptr_type`, plus `append_uint` for ids/counts); the string-returning
functions are thin wrappers over them (the declaration path and the single-use
inline map still want strings). The emit loop's cases — Alloca, Param, Const*,
Load, Store, FieldAddr, IndexAddr, AddressOf, Unary, Binary/Compare, Cast, Call,
Return, CopyInit/MoveInit, SetDrop, Drop, Destroy, MemCpy, MemSet, FunctionAddr,
GlobalAddr, Init, Br, CondBr, SizeOf/AlignOf/OffsetOf), the indirect/dynamic
calls (`CallIndirect`, `DynamicCall`) and the async opcodes (`AsyncAwait`,
`AsyncFinish`, `AsyncFrameAlloc`, `AsyncRampFinish`) all append directly. Only a
short field name or a twice-used access expression is still materialized into a
small (SSO) local.

Output-invariant: main-suite C is 96,825 lines either side and the normalized
diff is **62** (the same-binary floor). `MIRTests` 4400; `--tcc` **2307/2307**;
`--llvm` **2305/2305**; `--tcc --async` **53/53**; `--tcc --interpret`
**1897/1897**; `--tcc --libs` **718/719**.

Still string-based (rarer): `InlineAsm`'s operand list and the declaration
emitter's own spellers.

### Session 40 (2026-10-16) — reuse the emit body buffer

The non-async path emitted each function into a fresh local `std::string c` and
then copied it into the per-worker `task.out`. `MIRTaskContext` now owns a reused
`body` scratch; the emitter writes there (its capacity persists across functions)
and the result is appended, so emitting a function allocates only when the buffer
must grow.

Output-invariant: main-suite C is 96,825 lines either side and the normalized
diff is **58** against the **62-line** same-binary floor. `MIRTests` 4400;
`--tcc` **2307/2307**; `--llvm` **2305/2305**; `--tcc --interpret` **1897/1897**.

### Session 39 (2026-10-16) — one reused mangling buffer (no malloc per mangled name)

`mir_mangle_name`, `MIRDeclEmitter::mangle_name` and `MIRVtable::mangle_node` each
constructed a `BufferedWriter(256)` — a `malloc(256)` — for every mangled name
(per function, per global, per prototype, per vtable slot) and then copied the
bytes out into a `std::string`. The legacy 2c visitor kept one writer alive for
the whole translation; MIR allocated one per name. New
`compiler/mir/MIRMangleBuffer.h` hands out one reset `thread_local BufferedWriter`
(`mir::mangle_writer()`), and the three call sites reuse it. Mangling is a leaf
(it writes into the buffer and never re-enters the helper), so one buffer per
thread is safe.

Output-invariant: main-suite C is 96,825 lines either side and the normalized
diff is **46** against the **62-line** same-binary floor. `MIRTests` 4400;
`--tcc` **2307/2307**; `--llvm` **2305/2305**; `--tcc --interpret` **1897/1897**.

### Session 38 (2026-10-16) — pooled operand buffers: no per-call argument vector

`lower_call_args` and the construction sites each declared a fresh
`std::vector<MIROperand>` for the call's arguments, so every call/construction in
the AST heap-allocated (libstdc++ vectors have no inline storage). `MIRLowerer`
now owns a pool of operand buffers and `MIRArgScope` hands one out (cleared, but
keeping its capacity) and returns it on scope exit:

```cpp
MIRArgScope _args(*this);
std::vector<MIROperand>& args = _args.v;
```

Thirteen sites converted. The pool is a `std::deque` so acquiring a nested buffer
cannot invalidate an outer buffer's address, and it is indexed by a depth
counter, so recursive lowering (a call inside an argument) takes a deeper slot;
after warm-up the common path allocates nothing.

Output-invariant: main-suite C is 96,825 lines either side and the normalized
line-multiset diff is **40** against the **62-line** same-binary floor.
`MIRTests` 4400; `--tcc` **2307/2307**; `--llvm` **2305/2305**;
`--tcc --interpret` **1897/1897**.

### Session 37 (2026-10-16) — lowering allocation sweep: no per-identifier / per-literal strings

The lowering hot path constructed strings and containers it did not need. This
session removes the per-node ones (intended output-neutral; the suites are green):

- **`MIRFieldNameSet` replaces the `std::set<std::string>`** built for every
  struct literal's explicit-field set. It is a `std::vector<chem::string_view>`
  over the (AST-owned) field names with a linear `contains`/`erase` — a struct
  has a handful of fields, so the scan beats a red-black tree that
  heap-allocates a node and copies every name. `apply_struct_defaults` and
  `struct_literal_defines_all_fields` take it; the per-field `std::string nm` in
  both (and in `apply_struct_defaults_ptr`) is gone — the field's
  `chem::string_view` is used directly for the `add_string` ConstantId.
- **Transparent name-map lookup.** `name_places_`, `named_method_symbols_`,
  `named_globals_` and `extern_symbols_` stay keyed by owned `std::string` but
  now use a transparent `MIRStringHash`/`MIRStringEq`, so a lookup by
  `chem::string_view` (an identifier, a parameter/method name) no longer
  constructs a temporary `std::string`. `place_for_name` — the per-lookup
  allocation the evaluation doc named — is fixed.
- **`emit_function_c` reserves its output.** The generated C for one function
  runs to many KB; `out.reserve(instructions * 24 + 128)` stops `out`
  reallocating as it grows.

These apply the plan's "no per-node strings / no hot-path `unordered_map`" rule
where it is cheap. Still open for a later pass: the `std::function` mangler
returning a fresh `std::string` per function/global (the legacy visitor mangled
straight into a `BufferedWriter`), the per-call `std::vector<MIROperand>` argument
buffers, and the `place_name`/`operand_expr` string temporaries in `MIREmitter`.

### Session 36 (2026-10-16) — the primitive-impl vtable aligns with the shared layout (action B, correctness)

`MIRDeclEmitter::create_vtable_primitive` built the vtable **definition** by
iterating the interface's own `instantiated_functions()` (self methods with an
overrider only), while the vtable **type** (`create_vtable_type`) uses the shared
`mir_vtable_slots` — inherited interface slots first, then every own method. For
a primitive impl of an interface that inherits (or declares a non-self method)
the two disagreed: the own method landed in the *inherited* slot and the own slot
was left null, so a dynamic call through the derived interface jumped through a
null pointer.

The primitive definition now iterates the shared `mir_vtable_slots` too, emitting
one entry per slot — a self method with a `direct_child_function` overrider
resolves to that callee, and a non-self slot or a self slot with no overrider is a
null entry (matching `build_module_vtables`' kind-2 records for LLVM). The slot
*layout* now comes from one place for both backends and both vtable kinds.

Probe (`lang/compiled/prim_probe`, gitignored): `PrimDerived : PrimBase` implemented
for `int` now emits `{ 0, prim_probe_int_derived_val }` (was `{ prim_probe_int_derived_val }`,
which put the own method in the base slot). Regression tests in
`lang/tests/common/src/decls/interfaces/primitive_impl.ch`: a dynamic call on the
derived interface resolves its own slot (`== 12`; previously a null-pointer jump)
and the base interface resolves the inherited slot (`== 11`).

| Suite | Result |
|---|---|
| `--tcc` | **2305 → 2307/2307** |
| `--tcc --interpret` | **1895 → 1897/1897** |
| `--llvm` | **2303 → 2305/2305** |
| `--tcc --libs` | **718/719** (the known `test_fs_directory_iteration`) |

### Session 35 (2026-10-16) — a pointer-target zero for anonymous-aggregate array fields (action C1, eighth bite)

Session 34 skipped the field store for an *empty* array literal in the struct
literal's **main** field loop. The same shape inside an **anonymous aggregate**
(`buffer : []` in `std::string`'s `storage.sso`, `lang/libs/std/src/string.ch`)
went through `store_anon_aggregate`, which still built a zeroed scratch array and
copied it — and there, once through `lower_arg_converted` → `field_store_ptr` →
`load`, it emitted **three copies of the zeroes**:

```c
char __chx_p4[16];
memset(&__chx_p4, 0, sizeof(__chx_p4));
char __chx_v4[16];
memcpy(__chx_v4, __chx_p4, sizeof(__chx_v4));
memcpy(&__chx_v3->buffer, __chx_v4, sizeof(__chx_v3->buffer));
```

`MemSet` already carried a `(dest, byte, size)` shape (used by the LLVM emitter)
but nothing lowered it with an address destination and the C emitter assumed a
place. Now:

- `MIRBuilder::memset_ptr(ValueId dest, ValueId byte, ValueId size)` emits the
  general shape with the destination an **address value**.
- The C emitter's `MemSet` case handles it (`memset(ptr, byte, size)`); the LLVM
  emitter already did.
- `store_anon_aggregate` gained `enclosing_zeroed`: an empty array literal for a
  fixed-size array field is **skipped** when the enclosing temp is already
  `zero_init`'d, and otherwise **zeroed through its address** — one `memset`
  replaces the scratch + double copy. The flag is threaded through its recursion
  from the struct literal's `all_defined`.

`make_with_char` is now `memset((&__chx_v3->buffer), 0, 16);` (was four lines /
three copies). The transformation fires at one site in the main suite (only one
`buffer : []` literal exists), so the size delta is small — main-suite C
**96,767 → 96,763** lines — but it removes a triple array copy from a real
`std::string` constructor, and the pointer-zero primitive is now available to any
anonymous-aggregate lowering.

| Suite | Result |
|---|---|
| `MIRTests` | **4400 checks** |
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` | **718/719** (the known environment-sensitive `test_fs_directory_iteration`) |
| normalized C diff | only the changed site (a `memset` replaced by a `memset`, scratch + 2 `memcpy` removed) plus jitter |

### Session 34 (2026-10-16) — an empty array literal no longer double-writes a struct field (action C1, seventh bite)

A struct literal with an **empty** array initializer for a fixed-size array field
— `default_function_instance { …, buffer : [] }`, the shape `std::function` uses
everywhere (`lang/libs/std/src/function.ch`) — wrote the field's bytes **three
times**: the whole-temp `zero_init` (forced by the partial-array guard), a zeroed
scratch `char[N]` temporary, and a whole-width `memcpy` of that scratch into the
field. The scratch and the copy are dead once the enclosing temp is zeroed: the
field's bytes are already defined.

`MIRLowerer`'s struct-literal lowering now skips the field store when the
initializer is an empty array literal for a fixed-size array field and the
enclosing temp is `zero_init`'d (`!all_defined`), leaving the field defined by
the temp zero. This is exact: an empty literal into a fixed array means "all
zeroes", which is what the temp zero wrote.

Measured (main suite, `Debug`, `debug_quick`, `--no-cache`, `--emit-c`): `memset`
sites **212 → 200 (−6%)**, generated C **96,803 → 96,767** lines. The before →
after normalized line multiset differs by only the removed sites (**12 `memset` +
12 `memcpy` + scratch allocas**) plus the usual generic-instantiation ordering
jitter, against a **62-line** same-binary two-run floor.

| Suite | Result |
|---|---|
| `MIRTests` | **4400 checks** |
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` | **718/719** (the known environment-sensitive `test_fs_directory_iteration`) |

**Residual of this class.** A *non-empty* partial array literal (`buffer : [1,2,3]`
into `[32]char`) still materializes a full-width scratch and copies the whole
array; only the provided elements are live. Writing those elements in place into
the field address (and letting the temp zero cover the tail) would remove another
scratch `memset`+`memcpy` per site, but needs an in-place element store into a
field address. Left for the next bite.

### Session 33 (2026-10-16) — D4: `MIRLowerer.cpp` split into per-construct TUs

`MIRLowerer.cpp` was a single 7,090-line translation unit — the plan asked for
`MIRLowerExpr/Stmt/Call/Aggregate` (evaluation §1.3, §2D.4). The async/lambda/const
groups were already split (session 27); the remaining bulk is now split too:

- **`MIRLowerer.cpp` (867 lines)** — the core: place resolution
  (`place_for_linked`/`resolve_place*`, `resolve_method_fallback`,
  `intern_method_function`), `lower_address_of`, the move/drop machinery
  (`mark_moved`/`clear_moved_ref`/`emit_drops*`), the static-interface stub and
  `lower_function`.
- **`MIRLowerExpr.cpp` (1,858)** — `lower_expr`.
- **`MIRLowerStmt.cpp` (2,176)** — `lower_stmt`, `lower_incdec_value`, and the
  control-flow lowering (`lower_scope*`, `lower_if*`, `lower_switch*`,
  `lower_loop*`, `lower_while`/`lower_do_while`/`lower_for`/`lower_for_in`,
  `lower_asm_operand`, `note_source`).
- **`MIRLowerCall.cpp` (1,523)** — calls, receiver/method dispatch, member chains,
  patterns, operator overloads, symbol interning (`intern_function`/`intern_global`),
  and the atomic intrinsics.
- **`MIRLowerAggregate.cpp` (473)** — `apply_struct_defaults*`, the
  constructor/whole-value proofs, `store_anon_aggregate`, the call-temp registry.

New **`compiler/mir/MIRLowererHelpers.h`** holds the AST-aware free helpers that
were an anonymous namespace in the old file (`struct_derives_from`,
`resolve_sizeof_operand`, `captured_comptime_value`, `inherited_field_path`,
`is_implicit_field_node`, `map_binary`, `operator_method_name`/`compound_method_name`,
`compound_binary`, and `AtomicIntrinsic`/`atomic_intrinsic_of`) as `inline` in
`namespace mir`, so every TU sees one definition. It is the lowering boundary
only — no backend includes it.

**Pure move, output-invariant by measurement.** Main-suite emitted C is 96,803
lines either side; the before → after normalized line-multiset diff is **52**
against a **62-line same-binary two-run floor** (the generic-instantiation
ordering jitter). `MIRTests` **4400 checks**; `--tcc` **2305/2305**; `--llvm`
**2303/2303**; `--tcc --libs` **718/719** (the known environment-sensitive
`test_fs_directory_iteration`). **No headline gate moves** — this is
reviewability + the compiler's own incremental build (touching an expression
case no longer recompiles the statement and call TUs), the enabler the plan names
before D3 / the per-instruction cost / E.

### Session 32 (2026-10-16) — E2 + E3a: the worker seam, then reference-stable intern pools

**E2 — one worker, output-invariant.** `compiler/mir/MIRTaskContext.h` holds the
per-worker state the plan calls for (a private `MIRLowerer` + `MIRArena` + private
output buffer). `ASTProcessor::lower_and_emit` lowers a function (and its lambda
bodies) into the context's buffer and the coordinator appends it to the module
writer in source order — one context today, but the seam a worker thread will run
is exactly the one the serial path runs, so serial and parallel cannot drift.
Verified output-invariant: normalized emitted-C diff **68** lines against a
**136**-line same-binary noise floor (the generic-instantiation ordering jitter),
`--tcc` 2305/2305, `--llvm` 2303/2303.

**E3a — the interning pools are reference-stable.** E1 guarded the tables'
mutators, but their byte pools were still `std::vector<char>`: `name_data`/
`data_at` hand a reader a `const char*` + length *into* the pool, so another
worker's append reallocating the pool dangles that pointer. Every text/payload
pool (`MIRTypeTable::names`/`field_names`, `MIRConstantPool::data`,
`MIRConstExprTable::data`/`names`, `MIRSymbolTable::names`) is now a `MIRBytePool`
(`compiler/mir/MIRBytePool.h`): a `std::deque<std::string>` whose entries are
individually allocated, so an entry's bytes are both **stable and contiguous**
for its whole lifetime and an "offset" is an entry index. `append_field_name` is
a guarded mutator so field names may be appended from several workers.

- **The *record* containers stay `std::vector` — measured, not assumed.** Making
  `types`/`fields`/`constants`/`symbols`/`exprs`/`children` `std::deque` too (for
  the same stability) cost **+27 % user time** on the main-suite cold compile
  (4.00 s → 5.08 s; RSS unchanged): the hot `equal`/`hash_of` scan indexes the
  record vectors constantly, and deque indexing is not vector indexing. So the
  containers that only hand out opaque ids keep the fast vector, and the pools
  that hand out raw pointers — the real dangling risk — are the stable ones.
  A `const MIRTypeRecord&`/`MIRFieldInfo*` from `get`/`field_of` is therefore
  valid only until the next `intern`/`append_fields`; **sealing** those tables
  before the parallel phase (E3c) is what makes them safe to read concurrently.
- **Cost.** Post-E3a `TCCCompiler` main-suite cold compile: user
  **3.83/4.04/4.08 s**, peak RSS **56.0–56.8 MB**; the E2 baseline measured the
  same session: user **3.90/3.98/4.01 s**, RSS **56.4–57.3 MB** → within noise,
  no time or memory regression.
- **Verification.** `MIRTests` 4400 checks OK; `--tcc` **2305/2305**; `--llvm`
  **2303/2303**; `--tcc --libs` **718/719** (`test_fs_directory_iteration`);
  emitted C invariant (68 < 136).

**E3c — storage classes by read pattern.** The remaining dangling class was the
containers that hand out *element pointers* or are indexed by a reader while
another worker may append. Those are split out by how each container is read:

- `types` (and `MIRConstantPool::constants`, `MIRSymbolTable::symbols`,
  `MIRConstExprTable::exprs`) stay `std::vector` — hot in the intern scan, and
  only opaque ids escape them.
- `MIRTypeTable::data`/`fields` and `MIRConstExprTable::children` became
  `std::deque` — **cold** (never in the intern scan) but they hand out element
  pointers (`field_of`, `child`) and `data` is indexed by both emitters, so they
  must not realloc under a reader. Because they are cold, this is free.
- `names`/`field_names`/payload pools are `MIRBytePool` (stable *and*
  contiguous).

So after E1 + E3a + E3c a reader never holds a pointer/reference into a
container another worker can realloc: the mutators are locked (E1), the byte
pools are stable (E3a), the pointer-handing element pools are stable deques
(E3c), and the remaining hot vectors hand out only ids.

- **Cost.** Post-E3c `TCCCompiler` main-suite cold compile: user
  **4.07/4.07/4.15 s**, peak RSS **56.1–57.7 MB**; E3a was 3.83–4.08 s /
  56.0–56.8 MB and the E2 baseline 3.90–4.01 s / 56.4–57.3 MB → within noise.
- **Verification.** `MIRTests` 4400 checks OK; `--tcc` **2305/2305**; `--llvm`
  **2303/2303**; `--tcc --libs` **718/719**; emitted C invariant (94 < 136).

Next per §2E: **E3d — landed (session 55).** The threshold-gated worker run with
the lock-free stable tables and the ordered merge is live; parallel lowering is
now a measured net win (wall −15 %, user unchanged). See session 55 for the
lifetime bug the storage change exposed and how it was fixed.

### Session 31 (2026-10-15) — E1: shared interning is thread-safe

Stage E1 of the parallel-lowering plan (`mir-migration-evaluation.md` §2E). The
shared module intern tables are now safe to populate from several workers:

- `MIRTypeTable` (`intern`, `append_data`, `append_fields`, `set_name`),
  `MIRConstantPool` (`add` + every `add_*`; a payload append and the intern that
  references it stay under one lock, so two workers can never interleave their
  `data` writes) and `MIRSymbolTable` (`add`) each guard their mutators with a
  `std::mutex` and grew a `clear()`; `MIRModule::clear()` uses them.
- **Symbol ids stay un-deduplicated on purpose.** The backends match symbols by
  *mangled name*, not id (`MIRSymbolTable::mangled_name`), so two workers may
  intern the same name and both records are valid — no cross-kind name-collision
  risk from a global name index, and no interaction with the weak-stub pass. The
  doc comment on `add` records the contract: a reader must consume a
  `get`/`mangled_name` result without another thread's `add` reallocating the
  pools in between (the planned worker model lowers and emits in one thread).
- **Verification.** New `MIRTests::test_table_thread_safety`: 8 threads × 200
  interns each against one `MIRTypeTable`/`MIRConstantPool` (content-based ids
  must agree with a serially-built reference, and the pool must not grow past the
  distinct count) plus 8 × 200 concurrent `MIRSymbolTable::add` (every record
  intact and its mangled name readable). `MIRTests` **1180 → 4400 checks**, OK.
- **Cost.** Interleaved A/B of the pre/post `TCCCompiler` on the main-suite cold
  compile: pre 12.25/13.07/13.40/16.80 s, post 12.17/13.02/15.05/15.79 s —
  ranges overlap, i.e. **below this machine's noise** (uncontended locks).
- **Suites.** `--tcc` **2305/2305**, `--llvm` **2303/2303**, `--tcc/--llvm --libs`
  **718/719** (the environment-sensitive `test_fs_directory_iteration`).

Next per §2E: **E2** — the one-worker `MIRTaskContext` seam (per-worker
`MIRLowerer` + `MIRArena` + output buffer), still serial and output-invariant.

### Session 30 (2026-10-15) — E (parallel lowering) scoped against the real code

Started action E. Read the lowering/emission seam and wrote the code-grounded
staged plan into `mir-migration-evaluation.md` §2E rather than landing a
half-parallel lowering blind. Findings that shape it:

- The emission seam is already artifact-shaped: `lower_and_emit` lowers one
  function into a per-function `MIRArena` and emits its C into a **local
  `std::string`** before appending to `visitor.writer` (the async path already
  concatenates `pre + poll + post + ramp` in order). So "function artifact =
  bytes + source slot" is a small step.
- The blockers are the **shared mutable module state**: (1) `intern_function`
  dedupes via the *per-lowerer* `func_symbols_` then unconditionally appends to
  `module_.symbols` — two workers would add two symbols for one mangled name, so
  `MIRSymbolTable` needs a shared, thread-safe `name → id` index; (2) the
  `MIRTypeBuilder` memo + the `MIRTypeTable`/`MIRConstantPool` interns become
  concurrent; (3) `CTranslationContext` (`visitor.error`, `iface->active_user`,
  the mangler) is mutated during lowering and must move to the worker; (4)
  concurrent interning makes ids/order nondeterministic, so the C merge must be
  slot-direct in source order.

Staged E1–E4 and an honest payoff bound (lowering ≈ 30 % of the cold build, so
8-core parallelism caps at ≈ **−20–25 %**, still short of the ≤2.94 s gate —
hence E alongside a release re-measure) are in §2E. No speculative locks or
worker machinery landed yet: they would be unmeasured churn until the parallel
consumer exists.

### Session 29 (2026-10-15) — D3 ("dense tables") is measured and rejected; the D5 counter tool lands

Action D3 (replace `MIRLowerer`'s `std::unordered_map`s with dense tables) was
started and **stopped on measurement**. The plan itself says D3 "needs a profiler
or finer phase timers to target" (`mir-migration-evaluation.md` §2D item 3), and
there is no `perf` on this machine, so the tool came first.

**New tool (D5).** `cmake -DMIR_OPS_COUNTERS=ON` + `MIR_OPS=1` prints a one-line
tally of lowering work at process exit (`compiler/mir/MIRHotOps.h`; the sites are
`MIR_OP(...)` macros that compile to nothing when the option is off — default
builds are byte-for-byte unaffected, verified by rebuild + green suite).

**Measurement** (main suite, `Debug` compiler, `debug_quick`, `--no-cache`,
two runs):

| Counter | Value |
|---|---:|
| `lower_function` calls | 6,726 (6,667–6,723 distinct → **no redundancy**) |
| MIR instructions emitted | **296,017** |
| time inside `lower_function` | **2,906–3,194 ms** (≈ 10 µs/instr, ≈ 440 µs/fn) |
| `MIRTypeBuilder::map` (memo) | 387k |
| `var_places_` finds / binds | 54k / 20k |
| `name_places_` finds / binds | 2k / 20k |
| `intern_function` | 28.5k |

**Verdict.** All eighteen maps together account for ~0.4 M operations — tens of
milliseconds, i.e. **≈ 1 % of the lowering phase and ≈ 0.3 % of a cold suite
build**. Dense tables cannot move the compile-time gate. The cost is *per
function / per instruction* in an unoptimised (`Debug`) compiler plus the front
end, exactly as session 8 found ("no hot function"). The sized levers remain
**E — parallel lowering** (≈ 3 s of serial lowering inside a ~9.5–10.8 s cold
build) and a **release-build re-measure** (the gate has only ever been measured
with a `Debug` compiler — `mir-migration-evaluation.md` §0). D3 is left undone on
purpose; the numbers above are the reason.

### Session 28 (2026-10-15) — cross-module constructor bodies are no longer read (crash fix)

The intermittent `--llvm --libs` compiler crash was a **heap-use-after-free**, caught
deterministically by address sanitizer on an ASan `TCCCompiler`
(`-DENABLE_ASAN=ON`; it needs `/STACK:67108864` because ASan frames trip the
comptime stack guard). ASan pinned it on `constructor_writes_whole_value` reading a
constructor's `body`, freed by the per-module `mod_allocator->clear()` — the C1
optimization was the only lowering code that inspected a *callee's* body.

Root cause is a module-lifetime rule: the parser puts a function's **declaration**
(and its type nodes) on the **job** allocator but its **body** on the **module**
allocator unless the body is retained (`parser/structures/Function.cpp`,
"comptime/generic/`@retained` bodies → job, every other function's body → module").
A module re-parses its own direct files fresh, but a **dependency** file is reused
from the import cache, so its job-allocated declarations outlive their
module-allocated bodies. Reading such a body from a dependent module is a
use-after-free.

Fix, in two parts:

- **Guard** — `MIRLowerer::function_body_is_readable`: a body may only be inspected
  if `is_body_retained()` or its `FileScope` is one of the module's own fresh files,
  passed by `ASTProcessor::implement_module` via `set_current_module_file_scopes`
  (direct files are always re-parsed into a new `ASTFileResult`).
- **Cache the fact while the body is alive** — `lower_function` calls
  `prime_constructor_fact`, which stores the C1 proof on the (job-lifetime)
  declaration (`FuncDeclAttributes::ctor_whole_value_known`/`_ctor_whole_value`);
  `construction_writes_whole_value` consumes the cached fact, so a dependent module
  never re-reads a freed body. This keeps — and slightly improves — the C1 win.

Verified: ASan `--arg-test-libs` **16/16 clean** (previously tripped within ~4–8);
`--tcc` **2305/2305**, `--llvm` **2303/2303**, `--libs` **718/719** on both backends
(the one failure is the environment-sensitive `test_fs_directory_iteration`),
`MIRTests` 1180 checks. Main-suite C translation: **184 `memset`** (was 194 at the
C1 baseline — the cached fact is more accurate than a possibly-clobbered freed
body), 96,888 lines. See `lang/docs/mir-llvm-migration.md` §2.3.

### Session 27 (2026-10-15) — split `MIRLowerer.cpp` (action D4)

`MIRLowerer.cpp` was a single 9,026-line translation unit — the plan asked for
per-construct TUs (evaluation §1.3). The two largest cohesive groups are now
their own files:

- **`compiler/mir/MIRLowerAsync.cpp`** (1,170 lines) — the async/coroutine
  lowering (`async_slot_field`, `lower_async_function`, `async_emit_frame_helpers`,
  `async_lower_await_var_init`/`_poll_body`/`_drop_body`/`_ramp`,
  `lower_async_main_trampoline`) and its three file-local statics
  (`mir_slot_is_destructible`, `mir_sym_name`, `mir_named_field_type`).
- **`compiler/mir/MIRLowerLambda.cpp`** (814 lines) — lambda/capturing lowering
  (`lower_captured_variable`, `lower_extraction_value`,
  `lower_capturing_function_call`, `materialize_capturing_lambda`, `lower_lambda`,
  `materialize_noncapturing_lambda`).
- **`compiler/mir/MIRLowerConst.cpp`** (377 lines) — the global-initializer
  constant-expression builder (`build_global_initializer`, `build_const_expr` /
  `_array` / `_struct_value` / `_struct_default` / `_variant` / `_variant_bare` /
  `_call`).
- **`compiler/mir/MIRLowererInternal.h`** — the single helper those TUs share
  (`is_void_type`), moved out of the anonymous namespace `MIRLowerer.cpp` still
  uses internally.

`MIRLowerer.cpp` is **9,026 → 7,014 lines**. This is a **pure move** — the emitted
C is identical (96,915 lines, 194 `memset` sites, unchanged) and the suites are
green: `--tcc` **2305/2305**, `--llvm` **2303/2303**. The win is the compiler's
own **incremental build** (touching an async/lambda/const method now recompiles
one small TU) and reviewability; it does **not**, by itself, move the compile-time
gate — that still needs the dense lowering tables (D3), the per-instruction cost
(D), and parallel lowering (E). `MIRLowerExpr`/`Stmt`/`Call`/`Aggregate` are the
remaining split.

> ⚠️ **The compile-time gate is not measurable on this Windows checkout.** Three
> identical cold `TCCCompiler build.lab --no-cache` runs took 15.6 s / 24.4 s /
> 23.5 s — run-to-run variance larger than any change — and Windows baselines are
> the untracked M3. So D3/per-instruction work here can be justified by the plan
> and verified for correctness (identical C), but not for a compile-time delta;
> that needs the Linux L1 or the M3 baselines.

### Session 26 (2026-10-15) — multi-statement constructors no longer zero the result temp (action C1)

Session 17 skipped the caller-side `zero_init` of a `T(args)` result temporary only
when the constructor's body was solely `return T { ... }`. The largest remaining
zeroing class was the **multi-statement** constructor — `std::ordered_map.make`,
`std::unordered_map.make`, `std::mutex`/`std::CondVar` and their `recursive`
forms — so `constructor_writes_whole_value` gained two more provable shapes:

- **`... <setup>; return T { ... }`.** The only statement that writes the hidden
  sret is a `return`, so a body whose *only* `return` is a final `return T { ... }`
  of the same container covers the result regardless of what precedes it (the
  setup statements write locals, not the sret). Restricted to `var`, bare
  expression and bare-call (`AccessChainNode`) setup statements so a nested
  `return` inside an `if`/loop cannot hide another exit path.
- **`... var m = T { ... }; <no-write stmts>; return m`.** `m` is fully defined by
  its literal (a literal that does not cover every field is itself zeroed, so
  either way the value is complete) and `return m` is a whole-value store. The
  returned local is found through a bare identifier, a one-element access chain,
  or — when lowering consumed the chain's `values` — the body's unique local
  built from a literal of the container (a constructor returns its own value).

Statement kinds the proof treats as harmless before/among the return: `var`, a
bare expression, a bare call (`AccessChainNode` — e.g. `memset(...)`,
`InitializeSRWLock(...)`), and all of the above for shape 2. Any other statement
kind (an assignment, an `if`, a loop, an extra `return`) keeps the zero. The
literal must construct the *same* container (`linked_extendable() == def`),
matching the existing proof.

Measured on this checkout (`Debug`, `debug_quick`, `--no-cache`, `--emit-c`):

| Metric | baseline | after arrays/dyn | now |
|---|---:|---:|---:|
| Main-suite `memset` sites | 424 | 233 | **194** (**−54%**) |
| Main-suite generated C lines | 97,144 | 96,953 | **96,915** |
| `tests-tcc.exe` (no `--emit-c`) | 3,665,077 B | 3,656,373 B | **3,654,837 B** |

**Output-invariant except the removals:** the baseline → this-step normalized
line multiset differs by exactly the removed `memset`s (191) plus the usual
generic-ordering jitter (24/24). Verified at runtime with a probe (a
multi-statement `@make`, a `var m = T { ... }; return m`, and a real
`std::mutex` lock/unlock): **identical correct output on both backends**.

| Suite | Result |
|---|---|
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` / `--llvm --libs` | **718/719** (the known `test_fs_directory_iteration`; the documented `--llvm` compiler flake showed on some runs, green on re-run) |

**What is left of C1 (the residue is deliberately not removed).** A constructor
whose body is another shape, a temporary whose *address* a call fills, a partial
array literal's tail, `zeroed<T>()`, the string-into-`char[]` tail, a union
literal's tail, a default-initialized array, and a struct literal with an
inherited base that a nested default pass fills — every one of these either has a
byte only the zero defines, or would need an interprocedural / path-sensitive
proof. The remaining ~194 `memset` sites in the main suite are in that set.

### Session 25 (2026-10-15) — two more dead `zero_init`s (action C1, generated code / runtime)

Two whole-temp `zero_init`s were provably dead and are now conditional:

- **A full array literal.** `lower_expr`'s `ValueKind::ArrayValue` case zeroed the
  array temporary before writing every element. When the literal fills the
  declared array (`values.size() >= data_count`, including the decayed unsized
  form, which is sized to the literal) every byte is written, so the whole-array
  zero was dead. A *partial* literal (`[1,2,3]` into `[5]int`) still zeroes the
  tail, as before.
- **A `dyn<T>` fat pointer.** The `DynamicValue` case built
  `{ void* first; void* second; }` and zeroed it, then wrote **both** fields
  (`first` = the object address, `second` = the vtable). No padding, so the zero
  was dead.

Measured on this checkout (`Debug`, `debug_quick`, `--no-cache`, `--emit-c`):

| Metric | before | after |
|---|---:|---:|
| Main-suite generated C lines | 97,144 | **96,953** (−191) |
| Main-suite `memset` sites | 424 | **233** (**−45%**) |
| `tests-tcc.exe` | 3,665,077 B | **3,656,373 B** (−8.7 KB) |

**Output-invariant except the removals.** The normalized line multiset differs by
exactly the removed `memset` lines (151 array + 40 fat-pointer) plus the usual
generic-instantiation ordering jitter (42/42) — no other line changed.

| Suite | Result |
|---|---|
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` / `--llvm --libs` | **718/719** (the known `test_fs_directory_iteration`) |

**What is left of C1.** The `Type(args)` result temporary is still zeroed when the
constructor's body is not provably covering — the multi-statement shape
`{ var m = T { ... }; <call>; return m }` (`std::ordered_map.make`,
`std::mutex.constructor`, `std::CondVar.constructor`) is the largest remaining
class (the docs' session 17 residual), plus a temporary whose address a call
fills, partial array literals, `zeroed<T>()` and the string-into-`char[]` tail.

### Session 24 (2026-10-15) — the C declaration pass prints from a declaration list (action A3, endgame)

The C declaration pass walked the AST *and* printed C in the same methods. It now
**records** an ordered declaration list and prints it with an AST-free printer:

- New `compiler/mir/MIRDeclList.h` (AST-free): `MIRDeclKind`
  (`ForwardStruct`/`ForwardUnion`/`AggregateBody`/`Typedef`/`Prototype`/`Global`/
  `VtableType`/`VtableExtern`) and `MIRDeclList`.
- `MIRDeclPrinter::decl_list_c` (MIR core, `MIRTests`-covered) renders the list;
  `global_decl_spelling` moved there and is exported (the definition emitter
  shares it).
- `MIRDeclEmitter`'s declare pass now records into `decl_list_`
  (`fwd_declare`/`declare_type_alias`/`declare` no longer take a writer;
  `create_vtable_type`/`declare_vtable_extern`/`declare_vtable_primitive_extern`
  push items). `print_decl_list` renders it. `ASTProcessor::declare_module`
  builds the list and prints once. The unused `type_spelling`/`decl_spelling`
  helpers are deleted.
- The **implement** pass (function bodies, global *definitions*, vtable
  *definitions*) is unchanged — it is interleaved with lowered bodies, so it is
  not a list.

**Output-invariance.** The generated C reorders generic instantiations *between
runs of the same binary* (~84k of 97k lines move; the documented jitter), so the
meaningful invariant is the normalized line multiset: 97,144 lines either side,
refactor-vs-previous multiset diff **46/60** against a **56**-line same-binary
floor. The declaration-list conversion preserves the emission call order by
construction, and the C still compiles and runs.

| Suite | Result |
|---|---|
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` / `--llvm --libs` | **718/719** (the known `test_fs_directory_iteration`; the LLVM run showed the documented one-off compiler `invalid memory access` flake once, green on re-run) |
| `MIRTests` | **1170 → 1180** (10 checks for `decl_list_c`) |

**Where A3 stands.** The *spelling* of every declaration — aggregate bodies,
prototypes, global declarations, typedefs and vtable types — is MIR now, and the
C declare pass prints from `MIRDeclList`. The remaining AST use is (a) the
collection walk itself (the declaration-collection pass, which the binding rule
explicitly allows to read the AST) and (b) the implement pass's definitions,
interleaved with lowered bodies. Both are structural, not spelling.

### Session 23 (2026-10-15) — one global-declaration spelling (action A3, globals)

`MIRDeclEmitter::declare_var` (the declaration) and `implement_global` (the
definition) each computed the same global spelling from the `VarInitStatement`:
the mangled name, the declared type — including the unsized-`T[]` declarator —
and the thread-local/static/volatile flags. It is now one record and one
builder:

- New `compiler/mir/MIRGlobalDecl.{h,cpp}`: `MIRGlobalDecl` (`name`, `type`,
  `elem` + `unsized_array`, `flags`) and
  `build_global_declaration(mangler, types, module, init, out)`, which maps the
  declared type (or the stored child type for an unsized array) and sets the
  `MIRGlobalDeclFlags` (`thread_local` / `volatile` / `static`).
- `declare_var` and `implement_global` both consume it; a small
  `global_decl_spelling` helper emits the `T name[]` form for an unsized array.
- `MIRGlobal::flags` (the definition-table entry, `MIRModule.h`) now uses the
  same `MIRGlobalDeclFlags` instead of a second enum.

Measured output-invariant for C: main-suite C is 97,144 lines either side, the
previous-step → this-step normalized diff is **50** against a **46**-line
same-binary noise floor, and no global-declaration line differs (every residual
is a generic-instantiation ordering/attribution reference).

| Suite | Result |
|---|---|
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` / `--llvm --libs` | **718/719** (the known `test_fs_directory_iteration`) |
| `MIRTests` | **1170 checks** |

**What is left in A3.** Typedefs are already MIR-spelled (`decl_spelling` →
`c_declarator`); the remaining AST use in the declaration path is the declaration
*ordering* (struct bodies interleaved with their method prototypes) plus the
`MIRTypeDecl` records and a declaration-list walk that would let `MIRDeclEmitter`
be a pure printer. Prototypes, aggregate bodies, global declarations/definitions
and interface vtables are all MIR now.

### Session 23 (2026-10-19) — flat-CFG residue: trampoline blocks elided, trailing `return` dropped (action C)

Two safe straight-line text wins against the flat `goto` CFG the C emitter prints,
measured on the generated-code probe (`lang/compiled/perfprobe`, `--emit-c`):

1. **Trailing bare `return;` dropped.** The lowering terminates void and
   hidden-sret functions with `Return` (no operand); the emitter now strips that
   `return;` when it is the function's last statement — a C function falls off its
   closing brace with the same effect. The legacy translator never printed a bare
   `return;` at all. `return;` sites **399 → 73**.
2. **Trampoline blocks elided.** A block with no instructions and one
   unconditional `Br` to the block on the very next slot is a no-op: its `Br` is a
   fall-through and its label is dead. `compute_function_facts` (`MIRFacts`) now
   marks its instruction (`inst_transparent_br`) and resolves every jump through it
   (`block_resolve` / `resolve_block`), `is_fallthrough_inst` steps over it, and
   the emitter drops its label/instruction and prints the resolved target id. This
   is the per-`if`-without-else `goto`+label pair that the legacy *structured*
   translator never emitted.
3. **Non-parameter place loads forwarded to their uses.** The legacy translator
   read a local directly by name; MIR round-trips every read through a stack
   temporary (`T v = p; ...use v...`) — the `__chx_vN = __chx_pN` residue. The
   emitter now forwards a load from a **non-parameter** scalar place across *all*
   of its uses when the place is a plain (non-aggregate) type, is never
   **addressed/escaped** (named anywhere but as operand 0 of a direct read/write —
   `place_escaped`, computed for *every* place, since a local's address can be
   taken too and a destructor can write through it), is not written **after** the
   load (`place_last_write <= load`), and every use lies after the load
   instruction (`first_use_inst > load`). The last-write guard is required because
   an inlined expression can be re-emitted at a use that follows a write (`v = p;
   v = v+1; store p, v; use v` must not become `use p` after the store).
   Parameter-place loads keep their original single-use spill-aliasing rule.

Probe: C **10,801 → 9,524** lines (−11.8% total, −7.0% from forwarding), label
lines **754 → 627**, `goto` **779 → 674**, `return;` **399 → 73**; probe exe
**446,317 → 409,240 B** (−8.3%); `tests-tcc.exe` **4,322,203 → 3,942,323 B**
(−8.8%); release suite compile user time ~**0.98 s** (was ~1.13) and RSS ~**59.6 MB**
(was ~59.2), i.e. no regression. `--tcc` **2307/2307**, `MIRTests` **4506 checks**
(covering `place_escaped` / `place_last_write` / `first_use_inst`).

**Release-vs-Release baseline (legacy `basemain`, same command, same machine).**
The pre-MIR 2c compiler (`basemain/cmake-build-release/TCCCompiler`, no MIR
symbols) compiles the main suite with **user 0.60–0.65 s / sys 0.24–0.25 s /
wall 0.48–0.54 s, RSS 44.0 MB**, `tests-tcc.exe 3,243,929 B`; the MIR compiler is
**user ~1.00 s / sys 0.49 / wall 0.80 s, RSS ~59.6 MB**, `tests-tcc.exe 3,942,322 B`.
Like-for-like the gates are **1.67× compile (user and wall)**, **1.35× RSS**,
**1.22× tests exe** — all above the 1.05× acceptance. This session's C work took
the tests exe from **1.33× → 1.22×** of legacy; the compile/RSS gap is action D.
**Correction (session 24):** these figures were taken with `MIR_OPS_COUNTERS=ON`
and are not like-for-like; the counters-off, interleaved re-measurement is
**1.42× compile / 1.14× RSS / 1.18× executable** (the RSS gate then passes). See
session 24 below.

**What remains (action C).** The dominant residue is structural, not the removed
trampolines or the forwarded loads: the legacy 2c emitted **structured** control
flow (`while`, `switch`, `if/else`, ~0 `goto`) while MIR emits a flat CFG (still
627 labels + 674 `goto` after this session). Reconstructing loops as `while`/`do`
and dense integer branch sets as `switch` is the next (larger) bite. `v = p`
loads are down to **524** (from 1,162); the remaining `__chx_vN` temporaries are
mostly aggregates, escaped locals and cross-block uses.

### Session 24 (2026-10-20) — place-store fusion + block-local load forwarding; corrected baseline (action C)

Two emitter-only C wins, then a re-measurement that corrects the recorded
Release-vs-Release baseline.

1. **`Alloca` + adjacent `Store` fused.** A place whose very next instruction is
   a plain store of itself now emits one initialized declaration (`T p = <rhs>;`)
   and the store is skipped, instead of `T p;` + `p = <rhs>;`. The store is
   adjacent and unconditional, so it is the same program with one fewer line.
   Probe C **9,524 → 9,411** lines, 109 places fused.
2. **Repeatedly-assigned places read by name inside their block.** Forwarding a
   load to its place name previously required that nothing write the place
   afterwards, which excluded loop-carried/reassigned locals (`v = i; i++`). The
   relaxation forwards when every site that can re-emit the name — the load's
   uses *and* the transitively inlined pure values built from them — stays in the
   load's own basic block and precedes the next write to the place. The cone is
   what keeps `++i` sound: the inlined `v + 1` is used after `i`'s write, so the
   load is correctly not forwarded. Probe C **9,411 → 9,198** lines; `trim`
   **89 → 71** lines.

3. **Short-circuit conditions read their place instead of re-evaluating.** The
   lowering stores a `&&`/`||` operand to a place and then branches on the same
   value, so the emitter printed `p = <cond>; if (!(<cond>))` — evaluating the
   condition twice, which the C backend cannot undo (it compiles without an
   optimizer). When that store is the immediately-preceding instruction in the
   same block, the branch now prints `if (!p)`. The win is runtime (no duplicated
   comparison/shift chain) and text, not lines. Applying it unconditionally grew
   the main-suite executable by 0.06% (reloading a `_Bool` place costs more than
   re-evaluating a bare two-operand comparison TinyCC already has in registers),
   so it is applied only when the recomputed expression is ≥ 40 characters:
   probe **398,376 → 398,265 B**, main-suite executable **3,859,166 →
   3,858,912 B**, C lines unchanged.

Measured on the probe (release, same binary, `--emit-c`): C **9,524 → 9,198**
(−326, −3.4%); probe exe **409,145 → 398,265 B** (−2.7%); main-suite executable
**3,940,014 → 3,858,912 B** (−2.1%). Compile time and RSS are unchanged (user
0.62–0.69 s, ~50.5 MB either side) — these are generated-code wins. `--tcc`
**2307/2307**, `MIRTests` **4506 checks**.

**Corrected Release-vs-Release baseline.** The session-23 figures (user ~1.00 s,
RSS ~59.6 MB, 1.67×/1.35×) were taken with `MIR_OPS_COUNTERS=ON` and are **not
like-for-like**; the counters build was reverted but the numbers were not
re-measured. Interleaved A/B, counters off, three runs each, same machine:

| Axis | Legacy (`basemain`) | MIR (this tree) | Ratio |
|---|---:|---:|---:|
| Compile user CPU | 0.42–0.47 s | 0.63–0.66 s | **1.42×** |
| Compile wall | 0.37–0.42 s | 0.51–0.53 s | **1.31×** |
| Peak RSS | 44.2–45.0 MB | 50.8–51.4 MB | **1.14×** |
| Main-suite executable | 3,262,185 B | 3,859,166 B | **1.18×** |
| Probe C lines | 7,901 | 9,198 | **1.16×** |
| Probe executable | 341,554 B | 398,376 B | **1.17×** |

The RSS gate (`≤1.35×`) now **passes**; compile (1.42× vs 1.05×) and binary size
still miss.

**What remains in C.** At 9,198 lines the *statement* count is at legacy parity:
9,198 − (627 labels + 674 `goto`) = 7,897 ≈ legacy 7,901. What is left is exactly
the flat CFG, and bracketing `if (!c) goto L; … L:;` as `if (c) { … }` replaces
two lines with two braces — **net 0**. A further line win needs fewer
*statements* (short-circuit `&&`/`||` / expression reconstruction), or a
structured emitter that shares join labels, not plain CFG bracketing. Remaining
materialization: 2,109 place declarations, 773 place stores, 1,461 value
declarations (mostly calls/non-pure), 243 unforwarded `v = p` loads (cross-block
or escaped places).

### Session 22 (2026-10-15) — one function-prototype ABI (action A3, prototypes)

The function *prototype* ABI — sret for struct-like returns, aggregates/arrays by
pointer, the static-interface erased self, the variadic tail marker — was built
in **two** places, each carrying a "must match the other" comment: the C
declaration emitter (`MIRDeclEmitter::function_type_of`) and the LLVM
symbol/call path (`MIRLowerer::intern_function`). It is now one function:

- New `compiler/mir/MIRPrototype.{h,cpp}`: `MIRPrototype` (`name`, signature
  `type`, extern/static/dllimport `flags`), `mir_function_signature(types,
  module, decl)` — the single ABI implementation, moved verbatim from
  `function_type_of` (including `static_iface_base_method`) — and
  `build_function_prototype(mangler, types, module, decl, out)`.
- `MIRDeclEmitter::function_type_of` and its `mir_static_iface_base_method`
  helper are **deleted**; `declare_contained_func`/`function_proto` build a
  `MIRPrototype` and print from it.
- `MIRLowerer::intern_function` calls `mir_function_signature` instead of its
  inline copy.

The C declaration pass still calls the builder with the AST declaration — it is
the declaration-collection pass, which the binding rule allows to see the AST;
what moved is the *spelling* (one implementation) and the record the printer
consumes, matching how `MIRVtable` handled the C vtable path.

Measured output-invariant for C: main-suite C is 97,144 lines either side, the
previous-step → this-step normalized diff is **50** against a **46**-line
same-binary noise floor, and no prototype line differs (every residual is a
generic-instantiation ordering/attribution reference).

| Suite | Result |
|---|---|
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` / `--llvm --libs` | **718/719** (the known `test_fs_directory_iteration`) |
| `MIRTests` | **1170 checks** |

**What is left in A3.** The C declaration pass still walks the AST for typedefs,
the globals' declaration flags, and the declaration *ordering* (struct bodies
interleaved with their method prototypes) — the `MIRTypeDecl` records and a
declaration-list walk are the end state. Prototypes, aggregate bodies, global
definitions and interface vtables are all MIR now; the LLVM side reads no AST.

### Session 21 (2026-10-15) — vtable declarations are MIR records (action A3 interface residual + B)

`mir_emit_llvm_module` still walked the AST: it iterated `module->direct_files`,
found `InterfaceDecl`/`ImplDecl` nodes and built the vtable struct types and
global definitions inline — the last place a backend read the AST. Those
declarations are now MIR records built by the front end:

- New AST-free `compiler/mir/MIRVtableTable.h`: `MIRVtableSlot` (`name`, `type`,
  and the resolved callee `target`), `MIRVtableRecord` (the `name`, the struct
  `type_name`, a `kind` — 0 type / 1 container impl / 2 primitive impl — and the
  slots) and `MIRVtableTable` (`types` + `defs`). `MIRModule` gained `vtables`.
- `mir::build_module_vtables(mangler, types, module, nodes)`
  (`compiler/mir/MIRVtable.cpp`) builds the records from a module's top-level
  interface/impl nodes. It resolves each slot's callee **once** — the mangling
  and the interface's `active_user` override — and stores the mangled name; the
  C and LLVM paths both used to resolve it themselves.
- Called from `ASTProcessor::implement_module` before the LLVM finish hook
  (only on the LLVM path), so `mir_emit_llvm_module` reads `m.vtables` and no
  longer touches the AST at all. Its `mangle`/`nodes` locals and the entire
  interface/impl walk are deleted.

The C declaration pass is unchanged: `MIRDeclEmitter::create_vtable_type` /
`create_vtable` keep using `mir_vtable_slots`, which is fine — that emitter is
the declaration-collection pass, which the binding rule explicitly allows to see
the AST. What the change removes is a *backend* reading it.

Measured output-invariant for C (the C path's emission is byte-for-byte the same
modulo the usual jitter): main-suite C is 97,144 lines either side, the
previous-B-step → this-step normalized diff is **66** against a **46**-line
same-binary noise floor, and **no vtable line differs** (every residual is a
generic-instantiation ordering/attribution reference).

| Suite | Result |
|---|---|
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` / `--llvm --libs` | **718/719** (the known `test_fs_directory_iteration`) |
| `MIRTests` | **1170 checks** |

The `dyn<I>` / interface-inheritance / primitive-impl tests
(`lang/tests/common/src/decls/interfaces/{basic,dynamic,primitive_impl}.ch`,
`lang/tests/src/basic/interface/external.ch`) exercise the records on both
backends.

**What is left in A3.** The C declaration pass still walks the AST for
prototypes, typedefs, the globals' declaration flags and the interface/vtable
*declarations* (the `MIRPrototype`/`MIRTypeDecl` records). That pass is allowed
to, but the records are the end state. **B** still has shared traversal/naming.

### Session 20 (2026-10-15) — the interface-vtable layout is shared (action B, third step)

The vtable slot **order** and each slot's **self-erased function type** were
derived independently by the two backends: the C declaration emitter
(`MIRDeclEmitter::vtable_slot_type` plus its recursive inherited-then-own walk)
and the LLVM emitter (`mir_emit_llvm_module`'s `slots_of`/`slot_ftype`). They are
now computed **once** in the new AST-aware `compiler/mir/MIRVtable.{h,cpp}`:

- `MIRVtableSlot` (a `std::string name` + `TypeId type`) and
  `mir_vtable_slot_type(types, module, func)` — the erased-self signature of one
  method.
- `mir_vtable_slots(types, module, iface, out, out_funcs = nullptr)` — the
  canonical order (inherited interfaces recursively, then
  `instantiated_functions()`), with an optional parallel `FunctionDeclaration*`
  list for callers that must resolve a definition's callee.

`MIRDeclEmitter::create_vtable_type`/`create_vtable` and
`mir_emit_llvm_module`'s interface/impl/primitive vtable blocks all consume it,
so the layout cannot drift. The slot *spelling* (C `c_decl_type` vs LLVM
`mir_fn_type`) and the *callee resolution* (mangling, `active_user`) stay with
each backend — those are genuinely backend-specific, not layout.

The shared `mir_vtable_slot_type` uses the canonical `abi::needs_sret`, which
also fixes a latent divergence: the old LLVM `slot_ftype` used a local `is_agg`
that excluded typedef structs and arrays, so a vtable method returning a `dyn`
fat pointer or an array would have been spelled with the sret pointer in a
different position than the C backend. No exercised vtable slot hits that case
(the suites are unchanged), so this is a correctness alignment, not an output
change.

Measured output-invariant for C: main-suite C is 97,144 lines either side, and
the A3→A3+B normalized diff is **60** lines against a **46**-line same-binary
noise floor — every residual line is a generic-instantiation ordering/attribution
reference; **no vtable line differs**.

| Suite | Result |
|---|---|
| `--tcc` | **2305/2305** |
| `--llvm` | **2303/2303** |
| `--tcc --libs` / `--llvm --libs` | **718/719** (the known `test_fs_directory_iteration`) |
| `MIRTests` | **1170 checks** |

**What is left in B.** Shared traversal / naming and the vtable *declaration*
records (so `mir_emit_llvm_module` no longer walks the AST for interfaces/impls)
remain. `MIRDeclEmitter::create_vtable_primitive` still emits only the
interface's own self-param methods in `instantiated_functions()` order, so a
primitive impl of an interface with inherited slots could still misalign — a
pre-existing, un-exercised divergence, left for when the primitive path moves
onto the shared slot list.

### Session 19 (2026-10-15) — global initializers are a MIR constant tree (action A3, second bite)

The initializer of a top-level global was walked from the AST **twice**, once per
backend: the C emitter (`MIRDeclEmitter::const_value_expr` / `struct_default_expr`)
spelled C text and the LLVM emitter (`GlobalConstantLowerer`, ~370 lines) built an
`llvm::Constant`. Both read the same `Value*` tree with the same rules. There is
now **one** AST walk, at the lowering boundary, producing a target-neutral tree
the two backends render:

- New AST-free `compiler/mir/MIRConstExpr.h`: a `MIRConstExprTable` of
  `MIRConstExpr` nodes (`Zero/Int/Float/Double/Bool/Null/String/Negate/
  Aggregate/Variant/FunctionRef`) plus a child pool. A designator is a **dotted
  field path** (`Case.param`, `Base.field`), the same convention the type table
  already uses for field names; array elements are positional.
- `MIRModule` gains `const_exprs` (the tree pool) and `globals`
  (`std::vector<MIRGlobal>`: symbol + initializer id + TLS/volatile flags).
- `MIRLowerer::build_global_initializer` interns the global's symbol and lowers
  its `Value*` into the tree — the **only** AST walk. It mirrors
  `const_value_expr` exactly (explicit members in source order, then inherited
  bases, then defaulted direct members; variant tag + active-case payload + base
  defaults; comptime calls folded through the interpreter).
- `MIRDeclPrinter::const_expr_c` prints the tree for C (AST-free); it reproduces
  the old spelling (designators, the `(T) { … }` cast on a default
  construction, the `(T) { .__chx__vt_621827 = N, … }` variant form) and
  `BufferedWriter`'s float formatting.
- `Codegen::mir_apply_global_initializers` (CodegenMir.cpp) builds the LLVM
  `Constant` from the same tree (AST-free), including the unsized-`char[]`
  re-size.

`MIRDeclEmitter::const_value_expr`, `struct_default_expr`, `write_escaped_string`
and ASTCompiler.cpp's `GlobalConstantLowerer` class are **deleted**; the AST value
headers they pulled into the declaration path are no longer needed by the
initializer path.

**Measurement and verification (this Windows checkout, `Debug`, `debug_quick`,
`--no-cache`, `--emit-c`).** The translated C is not byte-reproducible
(`__cgs__N`/`__cfg__N`/`__chemda_N` numbering and forward-declaration order), so
the comparison normalises those labels and compares the line multiset, against
the **same-binary two-run** noise floor:

| Metric | Value |
|---|---:|
| Main-suite C lines | 97,144 before and after (identical) |
| Main-suite C bytes | 4,195,236 (before) / 4,195,250 (after) — run jitter |
| Same-binary two-run noise floor (normalized) | 46 lines |
| Pre-change vs change (normalized) | 42 / 52 lines |
| `--tcc` suite | **2305/2305** |
| `--llvm` suite | **2303/2303** |
| `--tcc --libs` / `--llvm --libs` | **718/719** (the known environment-sensitive `test_fs_directory_iteration`) |
| `MIRTests` | **1154 → 1170 checks** |

The normalized diff is *inside* the noise floor and every residual line is a
generic-instantiation ordering/attribution reference — no global initializer
changed shape. A probe (`lang/compiled/a3probe`, gitignored) with scalar, string,
`char[]`-literal, array-literal, struct-literal, nested-struct and enum globals
builds and runs on both backends with **identical** output, and the LLVM
initializers are non-zero (struct defaults, nested members, string data all
present), confirming the tree is actually consumed and not a no-op.

> **Discovered, pre-existing, unchanged:** an enum-member initializer
> (`const c : Color = Color.Green`) prints `0` on *both* backends — neither the
> old `const_value_expr` nor `GlobalConstantLowerer` folded an `EnumMember` leaf.
> The two backends agree, so this is not a regression; folding it would change
> output and is left to a separate fix.

**What is left in A3.** The initializer walk is gone; the *collection* walk and
the decl kinds that still spell from AST (prototypes, typedefs, globals'
declaration flags, interface vtables/static stubs) remain — the
`MIRTypeDecl`/`MIRPrototype` records the plan names are still the next step. The
shared interface **vtable layout** (action B) is unchanged.

### Session 18 (2026-10-15) — declarations print from MIR (action A3, first bite) + shared per-function facts (action B, second step)

Two changes, both **output-invariant** by measurement (method and numbers below).

**A3 — the C declaration path stops walking the AST for aggregate bodies.**

- New AST-free `compiler/mir/MIRDeclPrinter.{h,cpp}` in MIR core.
  `aggregate_body_c(module, TypeId)` returns the whole C body of a
  struct/union/variant: the header (including `__chx_align_struct(N)`),
  inherited bases first, `__chx_align(N)`/`volatile` member prefixes, the
  variant tag `int __chx__vt_621827;` with an inline `union` of the case payload
  structs, and the inline `struct { … } name;` form for an anonymous member.
  `inline_aggregate_c` returns the one-line form the driver writes for a
  synthesized inline struct; `member_declarator_c` spells a single field.
- MIR learned the two facts the printer cannot derive from layout, both in the
  **field pool** (never interned, so type identity is untouched except where
  noted): `MIRFieldInfo::flags` (`MFF_VOLATILE`, `MFF_FAT_POINTER`,
  `MFF_ANONYMOUS_STRUCT`/`MFF_ANONYMOUS_UNION`) and `MIRTypeRecord::explicit_alignment`
  (an *input* to `compute_aggregate_layout`, so it belongs on the record —
  without the attribute the C compiler would lay the struct out with less
  alignment than MIR recorded; it is compared by `equal`/`hash_of` because a
  different explicit alignment is a different layout). `MIRTypeBuilder` fills
  both.
- `MIRDeclEmitter::declare_struct_def_only`/`declare_union_def_only`/
  `declare_variant_def_only` are now just their (AST-side) early-declare
  prologue plus a call to the printer. `emit_member`/`emit_field` — the AST
  member walk, including the `UnnamedStruct`/`UnnamedUnion`/`StructMember`
  spelling and the required-alignment/volatile/capturing-function branches — are
  **deleted**, and the driver's inline-struct loop prints via
  `inline_aggregate_c` instead of `emit_field`. The member *spelling* was already
  MIR's (`decl_spelling` → `c_declarator`); what moved into MIR is the
  enumeration, formatting and the field facts.
- `MIRTests` **1115 → 1126**: exact expected text for a struct, an aligned
  struct, a union, a variant (base + tag + two payload cases), volatile /
  fat-pointer / aligned members, the nested anonymous-member form, the inline
  form, and the empty result for unnamed/unsupported ids.

**B — `MIRFunctionFacts`: the shared per-function analysis.**

- New AST-free `compiler/mir/MIRFacts.{h,cpp}`.
  `compute_function_facts(module, fn, out)` fills a `MIRFunctionFacts`:
  `block_start`, `block_starts_inst`, `block_referenced`, `label_at_inst`
  (+ `is_fallthrough_inst`), `place_writes`, `place_addressed`,
  `param_index_at_inst`, `param_alias`.
- `emit_function_c` used to compute every one of those inline (~150 lines); it
  now reads the facts (~10 lines). The analysis was **transcribed, not
  re-derived**: the same opcode set counts a place write, the same "only operand
  0 of a plain load/store may name the place" rule sets `place_addressed`, the
  same one-write/not-addressed/non-aggregate rule sets `param_alias`, the same
  fall-through-excluded operand set produces the referenced labels, and the
  argument-index bound is the function type's `data_count` as before. The C
  emitter keeps what is C's: the inline-expression map and the emitted text.
- `MIRTests` **1126 → 1154**: a function whose two scalar parameters are aliased
  to their arguments (write counts, escape flags, argument indices, no label on
  the entry block), and a two-arm branch (the fall-through arm is not a reference
  and gets no label; the other arm and the join do; a twice-written local is
  never a parameter alias).

**What is left in A3/B.** A3's residual is the *collection* walk and the decl
kinds that still spell from AST: prototypes (`function_proto` already delegates
to MIR's `c_function_proto`), globals + constant initializers
(`const_value_expr`), typedefs/typealiases, interface vtables and static stubs,
plus the `inline_structs` trigger. B's residual is shared traversal/naming
(`place_name`, `__chx_aN`, `__chx_bbN` are C's own) — the vtable *layout* (slot
order) is still derived independently by `MIRDeclEmitter` (C) and
`CodegenMir::mir_vtable_*` (LLVM), which is the next shared service worth
building.

**Measurement and verification (this Windows checkout, `Debug`, `debug_quick`,
`--no-cache`).** The translated C is *not* byte-reproducible: the
`__cgs__N`/`__cfg__N`/`__chemda_cap_N` generic-instantiation numbering and the
forward-declaration order differ between runs of the *same* binary. So the
comparison below normalizes those labels (each `<label><digits>` → `<label>#`)
and compares the sorted line multiset, and it reports the **same-binary two-run
noise floor** beside each result:

| Metric | Value |
|---|---:|
| Main-suite C lines (`Translated.c`) | 97,144 before and after (identical) |
| Same-binary two-run noise floor (normalized) | 102 lines |
| Pre-change vs A3 (normalized) | 96 lines |
| Pre-change vs A3+B (normalized) | 111 lines |
| A3-only vs A3+B (normalized) | 108 lines |
| Main-suite C text differing *unnormalized* | 8,131 (noise floor) / 8,206 (pre-change vs A3+B) lines |
| `--tcc` suite | **2305/2305** |
| `--llvm` suite | **2303/2303** (after one pre-existing flaky abort — see below) |
| `--tcc --libs` | **718/719** (the known environment-sensitive `test_fs_directory_iteration`) |
| `MIRTests` | **1115 → 1154 checks** |

Every residual normalized line is a generic-instantiation reference
(`std_stdspan__cgs__#constructor` with a different type argument, etc.), i.e.
the same jitter the noise floor is made of: no declaration or body changed
shape. Declaration shapes the main suite does not cover were checked end to end
with a probe (`lang/compiled/mir_a3_probe`, gitignored): inheritance, an
`@align(16)` struct with an `@align(8)` member, a `@volatile` member, an
anonymous union of anonymous structs and a variant with payloads — it builds and
runs with identical expected values on both backends, and the emitted C shows
`__chx_align_struct(16) mir_a3_probe_Aligned16 {`, `__chx_align(8) char j;`,
`volatile int v;`, the inline `union { … } storage;` and the variant's tag +
case structs.

> ⚠️ **The `--llvm` suite is flaky on this machine (pre-existing, not this
> change).** Two consecutive runs aborted while *building the test executable*:
> `Compiler.exe` printed `RUNTIME ERROR: invalid memory access` and exited 255,
> so no `tests.exe` was produced (an older one stays on disk and `test.sh`'s
> `set -e` stops the run). The first of those used the binary captured **before**
> this session (i.e. the previous commit), the second the A3-only binary — and
> the LLVM job skips C emission entirely (`lower_and_emit` returns before
> `emit_function_c` when `llvm_collect`), so none of this can come from the
> C-emitter work here. A third run, with the A3+B binary, completed normally and
> reported **`--llvm` 2303/2303**. Treat the suite as flaky here and the abort as
> pre-existing; the standalone probe above also built and ran on both backends
> (exit 0, expected output).

### Session 17 (2026-10-15) — the result temporary is no longer zeroed when the construction covers it (action C1, fourth bite)

A `T(args)` result temporary carried a whole-value `zero_init` even when the
constructor writes every byte of it, and `apply_struct_defaults` zeroed each
base/member temporary it built. Both are dead bytes whenever the construction
defines the whole value, so `MIRLowerer` now emits them only when that cannot be
proved. Two new conservative predicates decide it (both AST-level, both mirrored
exactly against the lowering they guard):

- **`constructor_writes_whole_value(ctor)`** — the constructor's body consists
  solely of `return T { ... }` statements constructing its own container. Such a
  literal lowers to a temporary of its own and `return` in a struct-returning
  function lowers to a *whole-value store* of that temporary into the sret
  destination (`lower_stmt`, `ReturnStmt` with `sret_` set), so the callee
  overwrites every byte a caller-side zero would have written — no field-coverage
  reasoning is needed. This is the shape of essentially every constructor in the
  tree and in `lang/libs` (`vector.make`, `string_view.constructor`,
  `string.empty_str`, …). Any other statement in the body, a return value that is
  not a literal of the same container, or a body that can exit by another path
  makes it false.
- **`default_init_writes_all_fields(def)`** — for the *generated* parameterless
  constructor (whose body is `apply_struct_defaults_ptr`) and for the recursive
  default pass. It mirrors the write set of `apply_struct_defaults` exactly: a
  base or member built through a *user* parameterless constructor, a base/member
  that is not a `struct` container, or a member with no default value that is not
  a `struct` type all return false (the zero stays). It is depth-limited so an
  illegal by-value cycle cannot recurse forever.

Applied at four call sites: the two `T(args)` aggregate-construction paths, the
inherited-base temporary, and the member temporary inside
`apply_struct_defaults`. The root-cause bullet above is updated accordingly.

This is a *runtime* win as much as a size win: the C backend has no optimizer, so
each `zero_init` is a byte-wise loop that survives into the object file, and these
constructions are the ones library code performs everywhere.

Measured (this checkout, `Debug`, `debug_quick`, `--no-cache`, `--emit-c`;
`TCCCompiler` built from this tree with and without the change):

| Metric | before | after |
|---|---:|---:|
| Probe (`perfprobe`) zero loops | 37 | **25** (−32%) |
| Probe generated C lines | 10,743 | **10,731** |
| Probe generated C bytes | 474,614 | **473,067** |
| Probe executable | 444,223 B | **442,993 B** |
| Main-suite C zero loops | 539 | **440** (−18%) |
| Main-suite generated C lines | 96,909 | **96,810** |
| `tests-tcc.exe` | 4,248,688 B | **4,238,513 B** |

The probe prints the identical line. Invariance: with the zero-loop lines
filtered out and the pre-existing `__cgs__N`/`__cfg__N`/`__chemda_N`/`__chx_*N`
numbering and declaration-order jitter normalized, before/after C differs by **4
lines on the probe and 154 on the suite**, against a *same-compiler two-run*
noise floor of **4 and 168** — i.e. nothing but the removed zeros changed.

Regression tests: `lang/tests/common/src/values/ctor_coverage.ch` (5 tests — a
`return T { ... }` constructor, a nested member default construction, an
inherited base default construction, a generated default constructor, and the
conservative multi-statement case). They assert *values*, so a wrongly loosened
proof shows up as garbage in the result; the paths they cover are visible in the
emitted C (`CtorInner.make(&p)` with no zero before the whole-value store).

Suites: `--tcc` **2305/2305** (was 2300; the 5 new tests), `--llvm`
**2303/2303** (was 2298), `--tcc --async` **53/53**, `--tcc --libs` **718/719**
(the known environment-sensitive `fs` iteration test), `--tcc --interpret`
**1895/1895**, `MIRTests` **1115 checks**.

**Residual (why this class is not fully gone).** A constructor whose body is not
a single `return T { ... }` keeps its zero — e.g.
`{ var m = T { ... }; <call>; return m }` (`std::mutex.constructor`,
`std::CondVar.constructor`). Closing that needs a body-level proof that the
returned local is initialized by a covering literal and never reassigned.
A temporary whose *address* is handed to a call that fills it (the async
`Waker.clone` shape), an array literal's tail, `zeroed<T>()`, the session-11
variant rule and genuinely incomplete struct literals are all unchanged. A
separate, still-open size/runtime cost adjacent to this one: `zero_init` is
emitted as a byte-wise loop even for a scalar (an `int` temp costs a 4-iteration
loop instead of one store).

> ⚠️ **Measure from a fresh build.** This session began with a
> `cmake-build-debug/TCCCompiler` linked *before* a later `MIREmitter.cpp` edit
> whose mtime had been restored to precede the `.o`, so `make` reported the
> target up to date and the first measurements reproduced the *pre*-C4 numbers
> (106,373 lines / 14,945 `goto`s) although an `MIRTests` rebuilt later already
> contained that change. `cmake --build cmake-build-debug --target TCCCompiler`
> recompiled it and the documented session-16 numbers then reproduced exactly
> (96,909 lines / 6,459 `goto`s); all numbers above are from the rebuilt binary.

### Session 16 (2026-10-14) — straight-line branches no longer emit a jump (action C4, second half)

Session 8 stopped emitting a label for a block nothing jumps to, but the jumps
*themselves* were still emitted unconditionally, so most blocks still ended in a
`goto` to the block physically right after them. `emit_function_c` now computes
the instruction offset each block starts at (by block id) and treats a branch
whose target starts at the very next instruction as a **fall-through**:

- `Br` to the next block emits **nothing** (control falls into it).
- `CondBr` with one falling-through arm emits a **single-arm** `if`, negating the
  condition when it is the *true* arm that falls through (`if (!(c)) goto F;`
  instead of `if (c) goto T; else goto F;`).
- The referenced-label set is computed from the *non*-fall-through `Block`
  operands, so a block whose only reference was a fall-through loses its label
  too — the two fixes compound.

This is a runtime win, not cosmetics: the C backend has no optimizer, so the jump
survives into the object file, and in a loop body it is executed every iteration.
The legacy translator emitted no such jump.

Measured (interleaved A/B, same machine/mode, `TCCCompiler` built from this tree
with and without the change; `debug_quick`, `--no-cache`, `--emit-c`):

| Metric | before | after |
|---|---:|---:|
| Main-suite C lines | 106,373 | **96,909** (−8.9%) |
| Main-suite `goto` tokens | 14,945 | **6,459** (−57%) |
| …of which standalone `goto` statements | 5,723 | **1,738** |
| …standalone jumps that are pure fall-throughs | 3,985 | **0** |
| `if/else goto` whose arm falls through | 4,501 (of 4,611) | **0** (4,611 single-arm `if`s) |
| Suite compile+link (median of 3) | 3.03 s | **2.98 s** |
| Probe (`perfprobe`) C lines | 11,723 | **10,743** |
| Probe executable | 448,625 B | **444,239 B** |
| Suite executable (`tests-tcc.exe`) | 4,292,216 B | **4,248,581 B** |

Runtime ≥ parity: `benchprobe` 0.60 s → 0.59–0.60 s (identical output),
`bcryptbench` cost-12 median 9.72 s → **9.42 s** (identical output). Suites:
`--tcc` **2300/2300**, `--llvm` **2298/2298**, `MIRTests` **1115 checks** (the
emitter control-flow unit test now asserts the `if (!(` form, the absence of
`else goto`, and that the fall-through-only block gets no label).

Invariance: filtering label and `goto` lines out of before/after C and
normalising the pre-existing `__cgs__N` numbering and declaration-ordering
jitter leaves **4** differing lines across the whole main suite (a same-compiler
two-run baseline differs by 6), so nothing but the redundant branches changed.

### Session 15 (2026-10-14) — debug info from MIR is complete (checkpoint 8.10)

MIR→LLVM debug info had a compile unit, subprograms, locations and types but **no
locals and no scopes**, so `--mode debug_complete` produced a line table a
debugger could not name a variable from. MIR now carries what the emitter needs:

- `MIRPlaceDef::debug_name` (an interned String `ConstantId` — the source
  identifier of a named place) and `MIRPlaceDef::debug_scope` (the lexical scope
  it was declared in). Set by `MIRBuilder::set_place_debug_name`, called from
  `MIRLowerer::bind_name`.
- `MIRLexicalScope` + `MIRBuilder::push_lexical_scope`/`pop_lexical_scope`: the
  half-open instruction range and source location of every source scope. The
  lowerer wraps each statement body in one (`lower_scope`, the
  `Block`/`Scope`/`UnsafeBlock` statement cases, `lower_scope_value`).
  `MIRFunction::lexical_scope_of_inst` returns the innermost scope of an
  instruction (scopes are pushed LIFO, so the highest matching index is the
  innermost).
- `MIRLowerer::set_debug_info` gates all of it, wired from `options->debug_info`
  beside the source-location provider — release / `debug_quick` records nothing
  and pays nothing.

`CodegenMir` turns that into a `DILexicalBlock` per scope (each instruction's
`DILocation` is scoped to the innermost block covering it) and a
`DILocalVariable` + `dbg.declare` record per named place, owned by its declaring
scope. **No `dbg.value`**: every MIR local is memory-resident (an `alloca` plus
explicit load/store in both backends), so `dbg.declare` is the right intrinsic —
the same thing the deleted AST backend's `DebugInfoBuilder::declare` did.

Measured on `lang/compiled/perfprobe` (`debug_complete`): IR `DILexicalBlock`
0 → **491**, `DILocalVariable` and `#dbg_declare` records 0 → **1084**, DWARF
`DW_TAG_variable` 0 → **1084**, `DW_TAG_lexical_block` 0 → **61**;
`DILocation` 2734 → 3016. `debug_quick` still emits 0 `DI*` nodes, 0 `#dbg_*`
records and no `.debug_info` section. Suites: `MIRTests` **1111 checks**,
`--llvm` **2298/2298** (and **2298/2298** with `--mode debug_complete`, which is
what exercises the new code), `--tcc` **2300/2300**, `--tcc --async` **53/53**,
`--llvm --async` **53/53**. Smoke check:
`./scripts/mir-debug-info-check.sh` (asserts DI present in `debug_complete`
**and absent** in `debug_quick`).

### Session 14 (2026-10-14) — the dead LLVM shell is gone (checkpoint 8.11)

Session 13 deleted the AST lowering; its *API* survived as declarations with no
definitions in `Codegen`/`DebugInfoBuilder`, plus three orphan headers. All of it
is now deleted: `compiler/Codegen.h` **855 → 257** lines and
`compiler/Codegen.cpp` **1,229 → 886** (destructible queue, dyn-object helpers,
AST declare/compile API, `operate`/`implicit_cast`, loop/terminator wrappers,
per-task AST caches, coroutine state, write-only leftovers);
`DebugInfoBuilder` **152 → 95** / `.cpp` **240 → 82** (only the MIR-used compile
unit API is left — `instr`/`di_loc` had no callers); `LLVMCoroutine.h`,
`LLVMArrayDestructor.h` and `TraitImplFuncMapKey.h` are deleted;
`LLVMBackendContext` is stateless (`forget` is the same no-op the C backend has).

One behaviour was restored while deleting: the deleted `create_func` was the only
place applying the per-function unwind policy, so `-fno-unwind-tables` had become
a no-op and `debug_complete` stopped forcing unwind tables. `Codegen::mir_function`
now applies it (`uwtable` in `debug`/`debug_complete` unless `fno_unwind_tables`,
else `nounwind`; safe — MIR never emits `invoke`/`landingpad`).

Suites: `--llvm` **2298/2298**, `--tcc` **2300/2300**, `--llvm --async` **53/53**,
`--tcc --async` **53/53**, `MIRTests` **1111 checks**. `--llvm --libs` is 718/719:
the single failure is `test_fs_directory_iteration`, which fails identically in
both prebuilt lib-test executables with no compiler involved (a pre-existing
`fs::read_dir` `is_dir` issue on this filesystem). Full write-up:
`lang/docs/mir-llvm-migration.md` §2.5.

### Session 13 (2026-10-13) — the AST-based LLVM lowering is DELETED (checkpoint 8.9)

The AST LLVM lowering is gone. It was already unreachable
(`ASTProcessor::compile_module` returns `mir_compile_module` unconditionally);
the last reachable use — `LabBuildCompiler`'s C-header `node->code_gen_declare`
pass (LLVM-only, unused in the repo) — was removed first, then the code was
deleted:

- `compiler/backend/LLVM.cpp` (3,372 lines — every per-AST-node `llvm_*`),
  `compiler/backend/LLVMCoroutine.cpp` (1,084 — AST coroutines),
  `compiler/Operation.cpp` (`Codegen::operate`), `compiler/backend/LLVMGen.{h,cpp}`.
- ~8,200 lines of `llvm_*`/`code_gen_*` declarations and definitions inside
  `#ifdef COMPILER_BUILD` blocks across 167 `ast/` files.
- `Codegen.cpp`'s AST methods and `Codegen.h`'s AST declarations; the dead
  `compile_module` body and `code_gen_*` wrappers in `ASTCompiler.cpp`;
  `DebugInfoBuilder`'s AST methods (`create_struct_type`/`create`/scope/`declare`).

**Kept:** `Codegen`'s shared LLVM-module plumbing (`module_init`,
`setup_for_target`, `save_*`, the `Create*`/`SetInsertPoint` wrappers),
`DebugInfoBuilder`'s MIR-used subset (`createDiCompileUnit`,
`start/end_di_compile_unit`, `finalize`, `update_builder`, `isEnabled`), and
`LLVMBackendContext` — whose former out-of-line definitions (needed for the
vtable) now live in a new `compiler/backend/LLVMBackendContext.cpp`, with
`forget` kept faithful (`intrinsics::forget` reaches it) and the AST-only hooks
(`mem_copy`, `destruct_call_site`, the `atomic_*` intrinsics) as unreachable
stubs, since MIR lowers those as first-class ops.

**Net: −15,661 / +91 lines** over two commits. Verified from the committed
sources: `--llvm` **2298/2298**, `--llvm --async` **53/53**, `--tcc`
**2300/2300**, `MIRTests` **1111 checks**; `--mode debug_complete` still emits
`DICompileUnit`/`DISubprogram`/`DILocation`/`DICompositeType`. The only
`COMPILER_BUILD` blocks left in `ast/` are `GlobalFunctions.cpp`'s legitimate
intrinsic dual branches.

### Session 12 (2026-10-13) — debug info is emitted from MIR (MIR→LLVM)

The MIR→LLVM path emitted **no DWARF at all**: `MIRInstruction::source_index` was
never filled, the lowerer never recorded a function's source, and `CodegenMir`
never touched the `DIBuilder`. `--mode debug_complete` produced an executable with
only `.debug_frame`. This was the one hard-requirement gap (§5.1 of
`mir-llvm-migration.md`) blocking deletion of the AST LLVM lowering.

Source locations now flow through MIR and are emitted as DWARF:

- `MIRModule::source_files` interns file paths; `MIRSourceLoc.file` indexes it
  (the core stays AST-free). `MIRFunction` gained `decl_loc`/`has_decl_loc`.
- `MIRBuilder::set_source`/`add_source`/`set_function_source` attach a location
  to emitted instructions (`emit_impl` stamps `source_index`).
- `MIRLowerer` resolves each statement's and each function's encoded location via
  a new `set_source_loc_provider` callback (wired from `ASTProcessor` using the
  `LocationManager`), gated on `options->debug_info` so the common
  `debug_quick`/release path pays nothing.
- `CodegenMir` emits a `DICompileUnit` per module (`mir_emit_llvm_module`), a
  `DISubprogram` per function, a `DILocation` per instruction (statement
  granularity), and a `mir_di_type` that maps MIR types to `DIBasicType` /
  `DIPointerType` / `DICompositeType` (struct/union/variant members with real
  offsets from the MIR layout) / `DIArrayType`.

Measured on `lang/compiled/perfprobe` (`--mode debug_complete --assertions
--out-ll-all`): the IR carries **4 `DICompileUnit`, 485 `DISubprogram`, 2274
`DILocation`, 23 `DIBasicType`, 147 `DICompositeType`, 245 `DIDerivedType`**; the
executable has `.debug_info`/`.debug_line`/`.debug_str`/`.debug_abbrev`/
`.debug_pubnames`/`.debug_pubtypes`; `readelf --debug-dump=decodedline` shows real
per-statement lines (51 entries for `main.ch`); the program output is unchanged
and the IR passes `--assertions`. `--mode debug_quick` (the suites) emits no DI.

Suites: `--tcc` **2300/2300**, `--llvm` **2298/2298**, `MIRTests` **1111 checks
OK**.

Still open for full §5.1 parity: `llvm.dbg.declare`/`dbg.value` for locals (needs
local variable names in MIR) and `DILexicalBlock`s. The one *reachable* use of the
legacy AST codegen outside `compile_module` remains the C-header declaration path
(`LabBuildCompiler.cpp`, `node->code_gen_declare(gen)` for `mod->headers`, driven
by the `include_header` build API — unused in the repo and commented out on
Linux). See `lang/docs/mir-llvm-migration.md` §2.4 for the deletion readiness
assessment.

### Session 11 (2026-10-13) — variant construction no longer zeroes dead bytes (action C1)

Variant construction (`Variant.Member(args)`) allocated its result temporary and
unconditionally `zero_init`'d it before writing the discriminator and payload.
The whole-temp zero defines the bytes the active case does not write, but for a
variant the only bytes ever read are the discriminator and that case's own
payload (the union tail is never read), so the zero is dead as soon as every
payload field is written. `MIRLowerer` now emits it only when a payload
parameter is omitted **without** a default, a payload is a partially-written
(array) value, or the variant has an inherited base — the conservative cases
where a byte really would be left indeterminate. This is the dominant remaining
zeroing class (56 of 93 probe loops) and matches the legacy backend, which
emitted no zero at all.

Measured (`TCCCompiler`, `debug_quick`, `--no-cache`, same source; the suite
number is against the session-9/10 baseline, which is the only change):

| Metric | Before | After | Delta |
|---|---:|---:|---:|
| Probe (`perfprobe`) variant zero loops | 93 | **37** | **−60%** |
| Probe generated C lines | 11,779 | 11,723 | −56 |
| Probe generated C bytes | 504,551 | 497,265 | −1.4% |
| Probe executable | 454,718 B | **448,640 B** | −1.3% |
| Main-suite C zero loops | 1,231 | **539** | **−56%** |
| Main-suite generated C lines | 107,191 | **106,373** | −818 (−0.8%) |

Suites after the change (clean per-module cache): `--tcc` **2300/2300**,
`--llvm` **2298/2298**, `--tcc --async` **53/53**, `--tcc --libs` **718/719**
(the known environment-sensitive `fs` iteration test), `MIRTests`
**1111 checks OK**. The probe prints the identical line.

Also in this session: `MIRTests`' over-aligned mirror used a prefixed
`__attribute__((aligned(16))) struct X` form that GCC ignores ("attribute ignored
in declaration"), so the `MirrorOverAligned` size/offset checks failed on any
non-MSVC host; the attribute is now placed after the `struct` keyword
(`struct MIR_TEST_ALIGNED(16) X`) and the suite is green on Linux again
(1052 → **1111** checks, the session-10 count).

An attempt to unify the place-based `apply_struct_defaults` onto the in-place
`apply_struct_defaults_ptr` was **reverted**: it did not remove any of the
targeted zero loops and *grew* generated C (probe +151 lines, suite +1,200),
because `field_addr_ptr` per base/member costs more than the single
`field_store_place` it replaced. The remaining zeroing class is the constructor
result temporary (`Type(args)`) and the four `apply_struct_defaults`
`zero_init`s, which need a constructor-covers-all-fields proof before they can be
dropped.

The `--llvm --libs` compiler crash recorded in `mir-llvm-migration.md` §2.3 no
longer reproduces (the lib test exe builds cleanly); that doc is updated.

### Session 5 (2026-10-13) — generated-code waste + LLVM error propagation

Both backends now emit from MIR, so this pass worked the evaluation's action C
(generated-code quality) and a cross-backend correctness gap.

- **Aggregate zeroing is now conditional (action C1).** The emitters no longer
  zero *every* aggregate `Alloca`. The lowerer already emits an explicit
  `MemSet` (`zero_init`) at the sites that need well-defined tail bytes (struct
  temp with unset members, array literal, defaulted fields); the blanket
  `Alloca` zero duplicated all of them (two byte loops per temp). One site did
  rely on the blanket zero — `var s : [N]char = "<literal>"`, which copies only
  the literal bytes — and now has its own explicit `zero_init`
  (`MIRLowerer::lower_stmt`, `VarInitStmt`). That case passed on the C backend
  only by luck (TinyCC handed out zeroed stack); it was a latent bug on both
  backends. Measured on `lang/compiled/perfprobe` (`debug_quick`, `--no-cache`):
  byte-wise zero loops **584 → 182** (−69%), generated C **14,115 → 13,713**
  lines, executable **543 KB → 501 KB** (−7.7%), probe compile/RSS flat
  (0.49 s → 0.45 s, 17.7 → 17.1 MB).
- **The LLVM job no longer swallows translation-phase errors.** In
  `LabBuildCompiler`, the LLVM path called `processor.compile_module(gen, mod)`
  and discarded its return value; failure was decided only by
  `gen.has_errors()` (AST/codegen diagnostics), which is empty for the MIR path.
  A diagnostic the C backend turns into a failed build — `#universal` router
  validation, `@extern` with a body, `float % float` in the emitted C, … — was
  *printed* on LLVM but the compiler exited 0. `compile_module`'s non-zero
  status now fails the build like the C path does. `--llvm --negative` went
  **282 → 304 / 318**; TCC `--negative` unchanged at 311/318.

### Session 8 (2026-10-13) — per-function attribution + scalar parameter aliasing (action C3)

No profiler is available on this machine (`perf_event_paranoid=4`), so the
remaining MIR phase cost was attributed **per function** with a temporary
`MIR_PROF` hook (RAII timers in `MIRLowerer::lower_function`, nesting-aware, plus
one around the lower+emit call in `ASTProcessor`'s `lower_and_emit`; since
removed). Cold main-suite compile, `debug_quick`, one run:

| Metric | Value |
|---|---:|
| distinct functions lowered | 1,530 |
| `lower_function` calls (module × function) | 6,776 |
| MIR instructions produced | 200,651 |
| `MIRLowerer::lower_function` self time | **1,318.6 ms** (≈ 6.6 µs / instruction) |
| C emission (lower+emit − lower) | **≈ 665 ms** (≈ 3.3 µs / instruction) |
| top-20 functions by self time | ≈ 470 ms of 1,318 ms (**36%**) |

The headline finding is negative and worth recording: **there is no hot
function**. The largest single entry is `build` (67.4 ms over 98 calls,
20,031 instructions — the `std::vector`/`string` builder code that follows from
this file's own 8,224-line body being mostly `build()` calls), then
`test_destructors` 30.9, `test_for_in_span` 28.2, `test_nodes` 27.2,
`delete` 26.2 (388 calls), `test_capturing_lambda` 24.9, … — the top 20 add up to
about a third of the phase, so ~65% is spread over ~1,510 small functions,
mostly per-type generic instantiations (`delete` ×388, `make` ×331, `resize`
×74, `push` ×65, `remove` ×61). Any further win has to come from the
**per-instruction cost** (builder/arena/type-mapping overhead) or from lowering
fewer instantiated copies, not from rewriting one function.

**Fix this session — action C3: do not spill scalar parameters.** The C backend
has no optimiser, so `MIRLowerer`'s `Param → Alloca(Parameter) → Store` prologue
really does cost a stack slot, a store and a load per scalar parameter (LLVM does
not need this: mem2reg promotes the alloca). `emit_function_c` now *aliases*
such a place to its C argument instead: `place_name` spells it `__chx_aN`, and neither
its `Alloca` nor its spill `Store` is emitted. The alias is applied only when the
place is a `Parameter`-class place of a **non-aggregate** type (a scalar, pointer
or reference — aggregates are either a copy of the pointee or the hidden
argument pointer and keep their by-value semantics), is written **exactly once**
by a `Store` whose value *is* the `Param` (so the C argument), the derived
argument index is inside the function's parameter list, and the place is never
addressed or named anywhere else — any other use (operand 1+ of any instruction,
or operand 0 of anything but a plain load/store) disqualifies it. The analysis
fuses into the existing `place_writes` pass and sets `g_param_alias` before the
single-use inliner runs, so the inliner already emits `__chx_aN` (the first
attempt set it afterwards and produced `__chx_p0` uses with no declaration).

**Measured** (`TCCCompiler`, cold cache, before = session-7 binary, both binaries
interleaved; same probe source, same `debug_quick` mode):

| Metric | Before | After | Delta |
|---|---:|---:|---:|
| Probe (perfprobe) C lines | 13,713 | **12,393** | −9.6% |
| Probe C bytes | 565,650 | **523,052** | −7.5% |
| Probe `__chx_pN = (__chx_aM);` spills | 673 | **25** | **−96%** |
| Full suite C lines (all modules) | 120,782 | **114,280** | −5.4% |
| Full suite C bytes | 4,935,666 | **4,727,098** | −4.2% |
| Full suite spill statements | 3,364 | **143** | −96% |
| Main-suite executable | 4,701,541 B (text 4,165,453) | **4,521,332 B (text 3,985,243)** | −3.8% / −4.3% text |
| Suite compile+link user CPU (5 interleaved pairs) | 3.85 s [3.80–3.91] | **3.81 s [3.79–3.83]** | ≤ in 5/5 pairs |
| MIR phase (lower / lower+emit) | 1,318.6 / 1,983.8 ms | 1,315.8 / 1,974.2 ms | −10 ms |

The C-byte and executable figures above (and the C2/C4 ones below) are
single-run values: they move by a few bytes to ~0.01% between runs of the *same*
binary because the emitted forward declarations are ordered
non-deterministically (see the caveat below). The line and statement counts on
the same rows are exact. Where the C2/C4 table repeats a "before" figure it is
the same run that produced its "after" figure.

The 25 remaining probe spills are places that legitimately keep storage (an
aggregate param's copy, a reassigned or addressed parameter) — the guard is
deliberately conservative, and the executable itself shrank, which is the point:
TinyCC's output for the aliased form is smaller than for store-then-reload.
Suites after the change (final binary, fresh build): `--tcc` **2300/2300**,
`--llvm` **2298/2298**, `--tcc --plugins` **1296/1296**, `--tcc --async` **53/53**,
`--tcc --libs` **718/719** at that point (`test_fs_directory_iteration`; a second
pre-existing failure, the bcrypt 10 s budget, is characterised under the C2/C4
block below), `--tcc --negative` **313/318**,
`MIRTests` **1049 checks OK**; the probe prints exactly the same line as before
(`total=1999000 … name_len=8`).

**Same session, actions C2 and C4.** Two more generated-code items:

- **C4 — dead block labels.** The emitter labelled *every* block start. A label
  is only needed for a block a terminator can jump to, and every `goto` it can
  print comes from a `Block` operand (`Br`/`CondBr`; the emitter never reads
  `cleanups`/unwind targets), so the referenced set is exactly those operands.
  Labels are now emitted only for referenced blocks, so an unreferenced block
  can no longer overwrite (in creation order) the label of a referenced block
  sharing its `inst_start`. This is also the entry block's `__chx_bb0:;` — pure
  noise in every function.
- **C2 — deref-load double hops.** `T v = *src; dest = v;` (the second aggregate
  copy the legacy translator never made) now collapses into `dest = *src;`. The
  guard: the load is a plain deref of a *Value* base, its result has exactly one
  use which is the **immediately following** instruction (so the read cannot move
  past anything), that use is a two-operand `Store` of exactly that value into a
  place of the same type, and the destination is a `Parameter` copy or a
  `Temporary` — both are this frame's own slots, which the source cannot overlap.

C2 additionally requires the store to be in the same basic block as the load, so
a collapsed read can never turn into an unconditional read on a path that never
executed the load (measured cost of that guard: zero — the dominant sites are a
function's first two instructions).

| Metric | Before C2/C4 | After | Delta |
|---|---:|---:|---:|
| Probe C lines | 12,393 | **11,868** | −4.2% |
| Probe C bytes | 523,052 | **515,872** | −1.4% |
| Probe block labels | 1,833 | **1,331** | −27.4% |
| Probe deref double hops | 23 | **0** | −100% |
| Suite C lines | 114,280 | **108,475** | −5.1% |
| Suite C bytes | 4,727,103 | **4,646,344** | −1.7% |
| Suite block labels | 17,216 | **11,660** | −32.3% |
| Suite `goto`s | 10,334 | 10,334 | unchanged |
| Main-suite exe text | 3,985,124 B | **3,971,116 B** | −0.35% |
| Suite compile user CPU (5 interleaved pairs vs C3) | 3.83 s | **3.81 s** | noise |

**Measurement caveat (worth fixing with the M2 budget gate):** the translated C
is *not* byte-reproducible — the order of the emitted opaque forward declarations
varies between runs of the same binary (≈9.5k reordered lines on the probe). Line,
label, `goto` and spill **counts** are deterministic; C bytes and executable size
vary by a few bytes (±2 B on the probe, same binary, two runs). So a budget gate
must compare counts, not bytes.

`MIRTests`' `test_control_flow_emitter` asserted the entry label `__chx_bb0:;`
was present; that expectation was updated (not dropped) to assert the three
referenced labels are present **and** the unreferenced entry label is absent, so
the test now pins the C4 behaviour: 1049 → **1052 checks OK**. Suites: `--tcc`
**2300/2300**, `--tcc --plugins` **1296/1296**, `--tcc --async` **53/53** (label
removal is exercised hardest here — coroutine state machines), `--tcc --libs`
**717–718/719** (the two pre-existing failures below), `--tcc --negative`
**313/318**, and the probe prints the identical
line (`total=1999000 … name_len=8`).

Two `--libs` failure modes are pre-existing and *not* affected by this work:
`test_fs_directory_iteration` is environment-sensitive, and
`test_bcrypt_check_password_incorrect` sits exactly on its 10 s per-test budget
(1 × `hash_password` + 6 × `check_password` at cost 12). Measured directly with
`lang/compiled/bcryptbench/` (same 7 cost-12 operations): **10.39–10.46 s with the
pre-change compiler** vs 10.42–10.64 s with this one — over the budget either way,
which is why it fails in ~1 run in 3 and passed in 2 of the 3 runs here.

Cumulatively (C1 + C3 + C2 + C4 vs the post-C1 baseline): suite C **120,782 →
108,475 lines (−10.2%)**, suite exe text **4,165,453 → 3,971,116 B (−4.7%)**,
probe C **13,713 → 11,868 lines (−13.4%)**. The probe still exceeds the legacy
targets (8,285 lines / 285,299 B text), so action C's acceptance is still open —
what remains is mostly the aggregate zeroing class (§1.2) rather than spills,
copies or labels.

### Session 10 (2026-10-13) — aggregate layout is owned by MIR (action A1)

The evaluation (§1.4) found the last architectural hole in the "one MIR, thin
backends" shape: **layout did not exist in MIR**. `MIRTypeRecord.size` was 0 for
every aggregate and no field carried an offset, so the C declaration text (the
C compiler), the LLVM `DataLayout`, and TinyCC each derived the layout again — and
a JVM/JS backend would have been a fourth. This session fills it in.

**What changed.** New AST-free `compiler/mir/MIRLayout.h` (core, covered by
`MIRTests`):

- `MIRModuleLayout` moved there from `MIRModule.h` (same definition);
- `mir_align_up` / `mir_field_alignment` / `mir_field_size` helpers;
- `compute_aggregate_layout(types, id, explicit_alignment)` — fills `size`,
  `alignment` and each field's `offset`;
- `MIRFieldInfo::offset`, which emitters can read directly (no name lookup
  needed; `MIRTests` has a small local helper for the name → offset case).

`MIRFieldInfo` gained `offset` and `align` (the source's explicit
`__chx_align(N)`, 0 = natural). Both live in the *field pool*, not in the
interned record (interning compares `field_index` only), so layout data cannot
affect type identity.

Callers: `MIRTypeBuilder::fill_aggregate_fields` lays out every struct, union and
variant — and every variant case payload struct — right after it fills the field
list; `MIRLowerer` lays out the two aggregates it builds by hand, the async
coroutine frame and the capturing-lambda closure struct. All target dependence
enters through the field records (pointer size/alignment and `long` width came
from `MIRModuleLayout` in the builder), so nothing host-specific is read.

**The rules are the C target's**, because the C emitter spells the fields out and
lets the C compiler lay them out, so MIR has to agree with it: struct fields in
order, each at its own alignment; a union puts every field at offset 0 and is
sized by its largest member; a variant is a struct of its bases, then the `int`
discriminator, then a *union* of the case payloads (so `MIRFieldKind::VariantCase`
fields overlap while bases and tag do not); the aggregate's alignment is the
largest field (or the explicit one) and its size is rounded up to it.

**Verification.** `MIRTests` went **1052 → 1111 checks OK**: nine mirrored C
aggregates (padding after a small member, trailing padding, `double` alignment,
pointer, `int[3]`, nested struct by value, union, variant with two cases, and an
explicitly 16-byte-aligned struct) compare MIR's numbers against the C compiler's
`sizeof`/`offsetof`. Alignment is read via `mir_c_align<T>()`
(`offsetof(MirrorAlignProbe<T>, t)`) because MSVC's C mode has no `_Alignof`.

Independently, a real program was cross-checked end to end: a probe with
inheritance, a nested struct, an array member, a variant and an anonymous union
member was lowered with a temporary layout trace and the numbers compared with
`gcc`'s `sizeof`/`offsetof` for the C the emitter actually produced — every value
identical:

| Type | MIR | gcc on the emitted C |
|---|---|---|
| `LayoutBase {i64 a}` | 8/8, `a`+0 | 8/8, `a`+0 |
| `LayoutDerived : LayoutBase {u8 b; int c}` | 16/8, base+0, `b`+8, `c`+12 | same |
| `LayoutNested {Derived p; u8 q}` | 24/8, `p`+0, `q`+16 | same |
| `LayoutArr {int[3] arr; u8 t}` | 16/4, `arr`+0, `t`+12 | same |
| `LayoutVariant {int tag; union{Some(Derived); None}}` | 24/8, tag+0, `Some`+8 | same |
| `LayoutUnionHolder {union{…} storage; char state}` | 32/8, `storage`+0, `state`+24 | same |

**This step is output-invariant, and that is the point:** nothing reads the new
fields yet (`sizeof`/`alignof`/`offsetof` still emit the C operators, so codegen
is unchanged). Measured before/after on the same machine: struct-heavy probe
**102 zero loops / 11,550 lines** and main-suite C **1,231 zero loops / 107,191
lines** — identical both sides (bytes are 504,054/504,052, i.e. inside the ±2 B
run-to-run jitter noted above); the probe exits 39 either way. Suites (`--tcc` and
`--llvm`) are green: **2300/2300** and **2298/2298**.

What it buys is the prerequisite for the rest of action A: the single C type
speller (A2) and a MIR declaration table (A3) can now print and lay out types
from MIR alone, and backend B can share one ABI/layout service.

Known limits, deliberately not addressed here: an empty aggregate is 0 bytes /
alignment 1 (matching `named_struct` and the opaque frame types); `long double`
keeps the 16/16 the type mapping already recorded (a Windows target would need 8);
and an aggregate nested *by value* is laid out when it is mapped, so an illegal
by-value cycle would contribute 0 rather than loop.

### Session 9 (2026-10-13) — struct-literal aggregate zeroing is conditional (action C1)

The remaining zero loops after C1's first bite were the `zero_init` on a struct
literal's temporary whose members are then all written. `MIRLowerer` now emits
that `zero_init` only when the literal actually leaves a member undefined
(`struct_literal_defines_all_fields`): a member is defined if it is set
explicitly, takes a default value, or is default-constructed (a struct type) by
`apply_struct_defaults`; an inherited base is always written. Two cases stay
conservative and keep the zero:

- a member with no default that is not a struct type — only the zero defines it;
- an **explicit array-typed member initialized by a partial array literal** — the
  array literal lowers to a temporary of exactly the literal's length and
  `memcpy`s only those bytes into the field (the empty `[]` case resolves the
  declared size, a partial one decays), so the field's tail depends on the
  enclosing zero. This is why the first cut of the rule broke
  `test_partial_array_literal_zeroes_remaining`; the fix compares the literal's
  element count against the field's declared array size and, when short, treats
  the field as unset for the zero decision.

Measured on this checkout (`Debug`, `debug_quick`, `--no-cache`; same source,
only `MIRLowerer.{h,cpp}` differ, interleaved A/B):

| Metric | Before | After | Delta |
|---|---:|---:|---:|
| Struct-heavy probe zero loops | 173 | **102** | **−41%** |
| Struct-heavy probe C lines | 11,621 | 11,550 | −0.6% |
| Main-suite C zero loops | 2,655 | **1,231** | **−54%** |
| Main-suite C lines | 108,615 | **107,191** | −1.3% |

Suites: `--tcc` **2300/2300**; `MIRTests` **1052 checks OK**; the struct-heavy
probe's program output is unchanged. The win is mostly the removed per-value
byte loops (a size cost on every construction), not the line count. This is the
same class the evaluation (§1.2, action C1) named as the next bite; what is left
of C1 is the zero on a constructor result temp (`Type(args)`) whose constructor
covers every field, and the four `zero_init`s inside `apply_struct_defaults`
itself.

### Session 7 (2026-10-13) — O(1) type/constant interning (action D, partial)

Instrumented the MIR phases of a cold main-suite compile (`debug_quick`,
`MIR_PROF=1` counters and timers, since removed) and measured, per suite
compile:

| Hot spot | calls | elements `equal()`-scanned | time |
|---|---:|---:|---:|
| `MIRTypeTable::intern` | 186,289 | 101.4 M | 0.498 s |
| `MIRConstantPool::add` | 97,279 | 56.3 M | 0.186 s |
| `place_for_name` | 2,181 | — | 0.002 s |
| `bind_name` | 19,705 | — | 0.017 s |

The two interning scans were ~0.68 s of the compile, i.e. the textbook quadratic
term the evaluation flagged: the tables end up ~1,000 types / ~1,400 constants
but are *looked up* hundreds of thousands of times, so the cost is `lookups ×
table size`, not `distinct × distinct`.

**Fix.** Both tables now keep a `hash → id` index (`MIRTypeTable::hash_of`,
`MIRConstantPool::hash_of`), hashing exactly the fields their `equal`/`matches`
compare — never `name_offset`/`data_offset`, which differ between equal records,
and mixing function parameter ids in by value. The index is a *hint only*: every
hit is re-verified with `equal`, and every miss falls back to the linear scan and
indexes the result. That keeps the semantics **identical** to a pure scan even
where a record is mutated after interning (`fat_pointer_type` fills its fields;
aggregate/variant/lambda/frame records are pushed and filled outside `intern`),
so no equivalence proof was needed for those paths — a stale or mutated entry can
never be handed out.

**Measured** (same machine, interleaved A/B, 5 runs each, clean binaries —
`TCCCompiler` on `lang/tests/build.lab`, cold per-module cache):

| Metric | Before | After | Delta |
|---|---:|---:|---:|
| Suite compile, user CPU (median) | 3.92 s | **3.61 s** | **−8%** |
| Suite compile, user CPU (range) | 3.76–4.20 s | 3.55–3.72 s | no overlap |
| Suite compile, wall (median) | 5.36 s | 4.86 s | −9% |
| Peak RSS | 59.9–60.5 MB | 59.7–62.2 MB | flat |
| `intern` scanned elements | 101.4 M | **7.0 M** | −93% |
| `intern` time | 0.498 s | **0.208 s** | −58% |
| `MIRLowerer` total | 2.253 s | **1.977 s** | −0.28 s |
| Probe (small program) CPU | 0.45–0.51 s | 0.45–0.50 s | no measurable change |

The probe is flat on purpose: its tables are ~500 entries with 29k lookups, so
the quadratic never bit there. The win is on large compilations. Suites after the
change (clean cache): `--tcc` **2300/2300**, `--llvm` **2298/2298**, `--plugins`
**1296/1296** (both), `--async` **53/53** (both), `--negative` **312** and
**305** / 318, `--tcc --interpret` **1890/1890**, `--libs` **718/719** (both; the
remaining failure is the environment-sensitive `test_fs_directory_iteration`),
`MIRTests` **1049 checks OK**.

Still open in action D: no profiler is available here (`perf_event_paranoid=4`),
so the remaining ~2.0 s of `MIRLowerer` per suite compile is only broken down by
phase (`emit` 0.63 s, `declare_module` 0.18 s, interning 0.37 s), not by
rectangle. Constant interning is now ~0.165 s of which the hash dominates, so
further gains there need a different structural change (e.g. skipping the payload
hash for repeated identical string literals).

### Session 6 (2026-10-13) — `async func main` trampoline restored

The one real regression left by the legacy-2c deletion (see the "Known remaining
issues 2c" section, now closed): an application whose entry point is
`async func main` had no synchronous `int main`, so the coroutine *ramp* owned
the `main` symbol — the C binary exited 255 and the LLVM binary segfaulted.

- **The entry decl is renamed before any prototype is emitted**
  (`ASTProcessor::declare_module` → `rename_async_entry`), mirroring the legacy
  visitor's mutation, so the mangler produces `__chx_async_main` for the
  prototype, the definition, and every call.
- **The trampoline is built as MIR** (`MIRLowerer::lower_async_main_trampoline`)
  and emitted by both backends from the same code, so there is no C-text/LLVM
  divergence: ramp call through the hidden sret slot, zeroed `Context`, poll to
  `Ready`, drop the handle, return the cast result. Reuses the poll body's
  primitives — no new opcode, no new emitter path.

Suites after the change (clean per-module cache): `--tcc` **2300/2300**,
`--llvm` **2298/2298**, `--tcc --interpret` **1890/1890`, `--tcc/--llvm
--plugins` **1296/1296**, `--tcc/--llvm --async` **53/53**, `--tcc --negative`
**312/318** (was 311), `--llvm --negative` **305/318** (was 304; the async
regression test is no longer among the failures), `MIRTests` **1049 checks OK**.
`--tcc`/`--llvm --libs` are **717** and **718** / 719. The extra TCC failure is
`test_bcrypt_check_password_incorrect`, which is *not* a codegen regression: its
workload (1 `hash_password` + 6 `check_password`, all bcrypt cost 12) needs
~11.2 s of CPU here against the test runner's fixed 10 s budget. The pre-change
compiler, built from a stashed tree and measured on the same machine, produced
11.18–11.25 s for that workload versus 11.22–11.31 s now — the same work, at the
same speed — and `/tmp/libs_baseline.log` (2026-10-12, older compiler) already
shows this test timing out. It is a pre-existing boundary flake; the fix would be
a `@test(timeout: …)` (or a lower cost factor) in `bcrypt_test.ch`, deliberately
left alone so bcrypt performance stays a real signal.

Both backends were additionally checked by hand on four async entries: eager
(`return 7`), single await (`42`), two awaits with a `Pending` child (`201`), and
void (`async func main()` with no result) — all produce the expected exit code.

Suites re-run after the change (clean per-module cache): `./scripts/test.sh
--tcc` **2300/2300**, `--llvm` **2298/2298**, `--tcc --interpret` **1890/1890**,
`--tcc/--llvm --plugins` **1296/1296**, `--tcc/--llvm --async` **53/53**,
`--tcc/--llvm --process` **127/127**, `--tcc/--llvm --regexp` **143/143**,
`--llvm --libs` **718/719**, `--tcc --libs` **718/719**, `--tcc --server`
**9/9**, `MIRTests` **1049 checks OK**. The single `--libs` failure is the known
environment-sensitive `test_fs_directory_iteration`.

## Current status (verified with a clean test-exe cache)

**LLVM target (`Compiler`, MIR→LLVM): the main suite is green — 2298 / 2298.**
`--llvm --libs` also builds now (re-checked session 11; the session-4 compiler
abort no longer reproduces — see `mir-llvm-migration.md` §2.3). The remaining
non-main `--llvm` suites have not all been re-checked since.

Verified `2026-10-08` with the per-module C cache cleared (`--no-cache`):

`./scripts/test.sh --tcc` -> 2279 passed, 0 failed;
`--tcc --interpret` -> 1875 passed, 0 failed;
`--tcc --libs` -> 718/719 (one environment-sensitive `fs` test);
`--tcc --plugins` -> 1296 passed, 0 failed;
`--tcc --async` -> 53 passed, 0 failed;
`--tcc --process` -> 127 passed, 0 failed;
`--tcc --server` -> 8/9 (one double-free in the http parser, see below);
`--tcc --webview` -> 51 passed, 0 failed;
`--tcc --regexp` -> 143 passed, 0 failed;
`--tcc --negative` -> 311/318 (7 pre-existing diagnostic/exit-code gaps,
unrelated to codegen).
`MIRTests` is green (1049 checks). `cstd`, `std`, `lab`, all libraries and all
module build scripts translate through MIR.

> ⚠️ **Always clear `lang/tests/build/chemical-tests.dir` and
> `lang/tests/build/lab` before recording a suite result.** The per-module C is
> cached; several regressions (bcrypt, server, async) were hidden behind a
> stale cache. `--no-cache` alone does not invalidate the per-module C.

### Fixed in this session

- **13 GB OOM (TinyCC build script).** `IndexAddr` emitted `base[index]` for an
  array-typed value, which C spells as a pointer-to-array (`char (*v)[N]`), so
  the stride was the whole array. `&raw storage.sso.buffer[off]` wrote 176 bytes
  past the SSO buffer, corrupting the host heap; the corrupted `std::filesystem`
  path made `getFilesInDirectory` recurse until the kernel OOM-killed the
  compiler. `Load`/`Store` already deref array-typed bases; `IndexAddr` now
  does too. Peak RSS for the main build: 7 GB+ -> 76 MB.
- **Cast of a pointer place.** `src as *u8` on an array/pointer/reference place
  took the address of the pointer *variable* (`(uint8_t*)&p`) instead of the
  pointer value. bcrypt's `bf_encode` read the stack.
- **Multi-dimensional array field declarator.** `FieldAddr` built
  `uint (*v)[256][4]` instead of `uint (*v)[4][256]`.
- **Codegen performance / single-use inlining.** The emitter now collapses
  single-use, type-preserving values (address chains, casts, constants, param
  spills, parameter loads) into their use site. Arithmetic is wrapped in an
  explicit cast to its MIR type so C integer promotion cannot change
  overflow/negation results. bcrypt cost-12: 3.9 s -> 1.0 s (legacy 0.6 s).
- **Async array destruction.** A coroutine frame slot of an array type was
  destroyed with a single `dtor(&slot)`; the raw-C `__drop` helper and the
  `Destroy` opcode now loop over elements.
- **Constructor argument mapping.** Constructor calls used `self_included=true`,
  offsetting the parameter list by one; constructors have no self param (the
  result goes through the hidden sret pointer). This broke the server build.

### Fixed in this session (differential review vs the legacy 2c visitor)

Found by writing core-language differential probes against the design contract
(`mir-design.md`) and the LLVM backend. Regression tests live in
`lang/tests/common/src/destructors/order.ch` (all backends) and
`lang/tests/src/basic/{array_assignment,pointer_to_array}.ch` (compiled).

- **Destruction order was forward, not reverse.** `emit_drops()`,
  `lower_scope_nodes`, `lower_scope_value` and the block-value cleanup all
  iterated `destructibles_` in declaration order, so scope/function/array
  cleanup destroyed the *first* local first. `mir-design.md` §7.4 requires
  reverse initialization order (and the legacy 2c visitor and the LLVM backend
  both do it). All four loops now use `emit_drops_from(mark)` (innermost-first).
  The interpreter's `InterpretScope::destroy_values` and its array-element loop
  had the same forward-order bug and are now reverse too.
- **Whole-array assignment emitted invalid C.** `arr2 = arr1` (locals,
  multi-dimensional arrays, or an array-typed struct field) reached the generic
  `Store` path and printed `dest = src`; C rejects array assignment. The emitter
  now `memcpy`s the source bytes for an array-typed destination, and the
  assign-statement address path copies from the source address instead of
  loading (a load declared `T v[N] = *ptr`, also invalid).
- **Pointer-to-array was decayed to `T**`.** `MIRTypeBuilder::map` rewrote a
  pointer whose pointee is an array (`*mut [N]T`) to a pointer-to-pointer to
  match the old legacy declaration spelling. That made `&raw mut a[i]` on a
  multi-dimensional array a `T**`, so `p + 1` / `p[j]` read a pointer value out
  of the array data (crash). The decay is removed and `c_type_of` now spells
  pointer-to-array as `T (*)[N]...` (the named `declarator` already did).

**Every top-level function body (including extern-free, comptime-free bodies and
functions returning function pointers) is now emitted by MIR** — a build with a
`legacy-emitted body` counter reports **0** for the main suite. The only
remaining legacy use is *declarations* (struct/union/variant/enum C definitions,
`@extern` prototypes, type declarations) and the non-function top-level nodes;
MIR relies on those declarations for its C type spelling.

**Async/coroutine lowering is implemented in MIR** (`async func` is no longer
emitted by the legacy C visitor). `lower_async_function` emits the frame struct
+ `__poll` + `__drop` + vtable (helpers as C text; the poll body and ramp as
MIR), driven by the shared `AsyncLoweringPlan`. Frame-resident locals use
`MIRStorageClass::FrameField`/`FramePtr` (`__chx__af->__chx_slot_N`), the state
machine uses the `AsyncAwait`/`AsyncFinish`/`AsyncRampFinish`/`AsyncFrameAlloc`
opcodes, and `&raw obj.field` (address of a struct field) is lowered by
`lower_address_of`.

### CBI macro integration

`#html` / `#css` / `#js` / `#universal` / `#json` macro output is lowered through
the CBI replacement hooks: `EmbeddedNode` (`ReplacementNode`) and
`EmbeddedValue` (`ReplacementValue`) are resolved via callbacks wired from
`ASTProcessor` and lowered like ordinary AST. The capturing-function ABI is
`(void* this, sret?, params)` — identical to the legacy visitor — so MIR-built
closures interoperate with legacy-emitted declarations.

## Architecture / files

| File | Contents |
|------|----------|
| `MIRTypes.h` | id types, `MIROperandKind`, `MIRStorageClass`, flags |
| `MIRArena.h/.cpp` | thread-owned bump arena, `reset()` reuse |
| `MIRInstruction.h/.cpp` | `MIRInstruction`/`MIROperand`/`MIROpcode` + opcode-static flags/name tables |
| `MIRFunction.h` | `MIRBlock`, `MIRValueDef`, `MIRPlaceDef`, `MIRFunction` |
| `MIRTypeTable.h` | canonical `MIRTypeRecord` interning, name/data pools |
| `MIRTypeBuilder.h/.cpp` | **AST→MIR type mapping** (only core file that includes AST types) |
| `MIRBuilder.h/.cpp` | typed construction API (places, fields, indices, globals, lifetime, calls) |
| `MIRLowerer.h/.cpp` | **AST→MIR lowering** (includes AST headers) |
| `MIREmitter.h/.cpp` | **MIR→C emission** (AST-free) |
| `MIRDump.h/.cpp`, `MIRVerifier.h/.cpp` | dump + structural checks |
| `compiler/ASTProcessor.cpp` | integration: `mir_translate_after_declaration` + callbacks |

`MIRLowerer` gets the mangler and interpreter through callbacks
(`set_mangler`, `set_comptime_if_resolver`, `set_comptime_eval`,
`set_comptime_ctor_eval`, `set_symbol_lookup`) so core MIR stays AST-free.

## What MIR lowers today

- Scalars, arithmetic/compare/cast, unary, `&&`/`||`, `++`/`--`, compound
  assignment (locals **and** struct fields).
- Control flow: if/else-if/else, while, for, break/continue, switch.
- Aggregates: struct construction (with default init of unset members),
  member read/write (including **nested chains** with address-of intermediates),
  arrays (literals, indexing, decay), unions, variants, enums (auto ordinal),
  globals/externs, `&x`/`&mut x`, pointer deref.
- Functions: sret returns, aggregate params by pointer, default args,
  variadic tail, function pointers (`call_indirect`), function references,
  method/extension calls (receiver -> self).
- Comptime: `comptime if`, comptime function/constructor calls evaluated via
  the interpreter, `%runtime_value` captured refs, comptime module constants
  inlined, intrinsics evaluated as comptime calls.
- **Destructors / moves**: destructible locals + by-value params destroyed at
  scope exit via drop flags; moves (`var d = c`, `return x`, by-value args,
  conditional moves) clear the flag; destructor lookup for structs, variants,
  unions; by-reference temporaries destroyed by the caller after the call.

## Type spelling (must match the legacy prototypes)

`MIRTypeBuilder` preserves the source spelling so MIR-emitted definitions match
the prototypes emitted by `declare_module`:

- IntN keeps its exact kind: `char`/`short`/`int`/`long`/`long long`/
  `unsigned ...`, plus `int8_t`..`uint64_t` for `i8`..`u64`. Enum -> `int`.
- `bool` -> `_Bool`.
- Pointers: `*T` -> `const T*`, `*mut T` -> `T*`; references: `&T` -> `T*const`,
  `&mut T` -> `T*`.
- Aggregates named via `mir_mangle_name`; the name resolver runs in
  `aggregate_type` so cross-file aggregates get names.
- `BaseTypeKind::Dynamic` -> `__chemical_fat_pointer__` (typedef, no `struct`),
  passed **by value**.
- Typealias and generic-struct types resolved to their underlying struct.
- Function types: aggregate/array params wrapped as pointers, matching function
  definitions; `declarator()` handles function-pointer and pointer-to-array
  params/values.
- `%literal`/`MaybeRuntime`/`Runtime` -> their `underlying`.

## Known remaining issues

### 1. Intermittent JIT crash in the build script — FIXED

`TCCCompiler lang/tests/build.lab ...` used to fail non-deterministically
(~30-40% of runs, no test exe produced) with `0xcccccccc00000002: at ???:
RUNTIME ERROR: breakpoint/single-step exception:` (exit `3` / `0xC0000409` /
`0x80000003`). Root cause: **Array MIR types set `data_count` to the array
length but never `data_offset`, so `MIRTypeTable::equal` compared
`data[0..len]` against the function-parameter type pool — an out-of-bounds
`std::vector<unsigned int>` read** (MSVC "vector subscript out of range",
`_Pos` varied 10-16). Fixed by only comparing the shared `data` pool for
function types (commit `9c922ab00`). Verified 15/15 clean builds.

Diagnosis method that worked: run the compiler under lldb
(`D:\Software\Jetbrains\CLion 2025.3.3\bin\lldb\win\x64\bin\lldb.exe --batch
-o run -o "bt 30" -- ./cmake-build-debug/TCCCompiler.exe ...`) until the
`Exception 0x80000003` fired at `std::vector<unsigned int>::operator[]`.

### 2. The legacy 2c visitor is deleted

The C codegen is fully MIR. Function *bodies*, **static
`@compiler.interface` / `@static` impls**, CBI `EmbeddedNode` replacements
(e.g. a `#universal` component's SSR function), extern/comptime declarations,
generic-function declarations, non-top-level / comptime globals, inline-struct
fields, static-interface stub implementations, and runtime `comptime { }`
blocks are all lowered by MIR. An instrumented `MIR_DBG_LEGACY` build reports
**0** legacy-visitor invocations across every suite (main, plugins, libs,
async, process, regexp, webview, server, negative), and a MIR lowering failure
now reports a diagnostic instead of falling back.

`preprocess/2c/2cASTVisitor.{cpp,h}`, `CDestructionVisitor.h`,
`CTopLevelDeclVisitor.h` and `SubVisitor.h` are **deleted** (~11k lines). The
few utilities the MIR passes still need (output writer, name mangler,
comptime-interpreter bridge, CBI embedded resolution, static-stub bookkeeping)
now live in the small `CTranslationContext`
(`preprocess/2c/TranslationContext.{h,cpp}`).

The two previously-blocking construct classes were both fixed:

- **Static `@compiler.interface` / `@static` impls** now lower through MIR.
  `lower_impl` no longer returns early for `iface->is_static()`; it marks the
  interface (and inherited interfaces) as implemented via
  `remove_static_interface_for_stub_impl`, so the stub pass does not also emit
  the default methods. The earlier "`support` state corruption" was the
  type-table bug below, not the static-interface ABI.
- **`#universal` `EmbeddedNode` SSR functions** now lower through MIR. The
  element-type confusion was caused by the `MIRTypeBuilder` caches being keyed
  on bare AST pointers that the file allocator reuses across files. Fixed by
  keying `decl_types_`/`decl_ids_` on `(decl node, aggregate kind)` and clearing
  the `BaseType*`-keyed `memo_` after each file (`clear_memo`). The
  `universal_cbi` `.map()` SSR also had a shadowing bug: `find_ssr_local` scanned
  bottom-up, so a callback parameter (`tabs.map((tab, i) => ...)`) resolved to an
  outer local of the same name (`var tab = style { ... }`); it now scans
  innermost-first.

Static-interface stubs are generated by
`ASTProcessor::emit_static_interface_stubs`, called once at the end of a job
(after every module is implemented, so the implemented set is final), through a
fresh MIR context. Bodyless methods emit a trivial return-0 stub.

Note: when the compiler changes, the **CBI plugin object cache is not
invalidated** — stale plugin objects produce bogus generated code. Always clear
`lang/tests/build/*.dir` (not just `chemical-tests.dir`) before judging a
plugin/CBI change.

### 2b. Server: double-free in the http parser (one test)

`FRAM_truncated_chunked_body_terminates_at_body_timeout` aborts with
`free(): double free detected in tcache 2` in
`http_httpRequestdelete -> http_httpHeaderMapdelete -> vector<pair<string,string>>::delete`.
The header pair strings are shallow-copied through `HeaderMap.insert` ->
`push_back` -> `push` (each `memcpy`s the pair and clears its own local drop
flag), so a string's heap buffer is owned by more than one place. The legacy
visitor handled this via direct-by-pointer parameter mutation; MIR's
copy-then-move parameter ABI is the suspect. Needs a focused lifetime
investigation.

### 2c. `async func main` has no synchronous `int main` trampoline — FIXED

An application whose entry point is `async func main` must get a synchronous
`int main` that drives the coroutine to completion and returns its result (see
`lang/docs/async-await-design.md` §5.5). The legacy 2c visitor emitted that
wrapper (`emit_async_main_wrapper`, deleted with the visitor) and MIR initially
never reimplemented it, so the coroutine *ramp* owned the `main` symbol:

```c
static void main(struct core_coreasyncFutureHandle__cgs__1* __chx_a0) { … }  /* ramp */
/* no `int main(void)` */
```

Consequences (repro below): the C build links a program with no real entry point
(exit code is garbage) and the LLVM build SIGSEGVs, because crt calls the ramp
with the wrong ABI.

```bash
mkdir -p /tmp/negasync && cd /tmp/negasync
printf 'application neg_test\nsource "."\nimport cstd\nimport core\nimport async\n' > chemical.mod
printf 'struct Rd { var v : int }\nfunc rp(frame : *mut void, cx : *mut core::async::Context) : core::async::Poll<int> { var f = frame as *mut Rd; return core::async::Poll.Ready<int>(f.v) }\nfunc rd(frame : *mut void) { free(frame) }\nfunc mk(v : int) : core::async::FutureHandle<int> { var f = malloc(sizeof(Rd)) as *mut Rd; f.v = v; var t = malloc(sizeof(core::async::FutureTable<int>)) as *mut core::async::FutureTable<int>; t.poll = rp; t.drop = rd; return core::async::FutureHandle<int> { frame : f as *mut void, vtbl : t } }\nasync func main() : int { var n = await mk(40); return n + 2 }\n' > test.ch
../../cmake-build-debug/TCCCompiler chemical.mod --no-cache -o c.exe && ./c.exe; echo "C exit=$?"   # expect 42
../../cmake-build-debug/Compiler   chemical.mod --no-cache -o l.exe && ./l.exe; echo "LLVM exit=$?" # expect 42
```

In that state the C binary exited 255 and the LLVM binary segfaulted.
Regression test: `lang/tests/negative/src/async.ch::async_main_trampoline_runs`
(asserts exit 42); it passes on both backends now.

**How it was fixed.** (1) *Renaming the entry.* `ASTProcessor::declare_module`
runs `rename_async_entry` over each module's top-level nodes before the
declaration emitter writes a byte — the same mutation the legacy visitor
performed — so an application's `async func main` (marked `no_mangle` by symres)
becomes `__chx_async_main` in the prototype, the definition, and every call. The
name is `MIRLowerer::kAsyncMainInternalName`.
(2) *Building the trampoline.* `MIRLowerer::lower_async_main_trampoline` builds a
real `MIRFunction` named `main` (external linkage, `int` result via
`MIRTypeBuilder::int_c_type`), so **both backends share one implementation**:
call the ramp through its hidden sret slot into a `FutureHandle<T>`, `zero_init`
a `Context` (the "no waker" context `block_on` uses), poll
`h.vtbl->poll(&p, h.frame, &cx)` until the `Poll<T>` tag is 0, call
`h.vtbl->drop(h.frame)`, and return `(int) p.Ready.value` (0 for a void/`Unit`
result). It reuses the poll body's primitives — place load, field load,
`call_indirect`, `cond_br`, `cast` — so no new opcode was added. The entry
signature mirrors the ramp's function type minus its sret slot, so declared
parameters (`main(argc, argv)`) carry over. Both the C and the LLVM
`lower_and_emit` paths write it right after the ramp; failing to build it is a
build error, never a silent program without an entry point.
Verified: eager (`return 7`), multi-await suspending (201), and void
(`async func main()` with no result) entries all produce the expected exit code
on both backends; `async_main_trampoline_runs` passes, so `--tcc --negative` is
now **312**/318 and `--llvm --negative` **305**/318.

**Original fix sketch (superseded).** (1) Give the ramp a distinct symbol when it is the async main
(`mangler_(decl) + "__ramp"`), in both `MIRLowerer::intern_function` and
`mir_translate_after_declaration`'s pass 1, and own `main` in a new wrapper.
(2) Emit the wrapper. Its body is exactly the old C text (also mirrored by the
now-dead `emit_llvm_async_main_wrapper` in `compiler/backend/LLVMCoroutine.cpp`):
call the ramp into a `FutureHandle<T>`, then poll `h.vtbl->poll(&p, h.frame,
&cx)` until `p.__chx__vt_621827 == 0`, `h.vtbl->drop(h.frame)`, return the cast
`p.Ready.value` (0 for a void/`Unit` result). The "build it as MIR" variant reuses
`lower_async_await`'s primitives (place load, vtbl field load, `call_indirect`,
`cond_br`) so the LLVM emitter needs no new opcode; the C-text variant goes into
`async_post_decls_` for the C backend. Build the MIR variant so both backends
share one implementation.

### 3. Aggregate layout milestone

`MIRTypeRecord.size`/`field data` are not filled for aggregates; C type spelling
uses the name pool, not layout. Needed before LLVM lowering.

## Gotchas learned (hard-won)

- **Cache**: the test-exe build caches per-module C. After changing the compiler
  you MUST clear `lang/tests/build/chemical-tests.dir` (and `lang/tests/build/lab`)
  or you will inspect/run stale C. Use `--emit-c` to keep `Translated.c`.
- **`--emit-c`** on the build.lab keeps `lang/tests/build/chemical-tests.dir/Translated.c`.
- **value id 0 == `MIR_NULL`**: never treat `result_or_place == MIR_NULL` as
  "no result" for opcodes that can return value id 0 (fixed for Call).
- **Aggregate params are pointer-typed places**: `&param` yields the pointer for
  an aggregate param but the slot address for a genuine reference param (key on
  `needs_aggregate_path(ast_type)`, not on the place type). Drop/Destroy and
  field/indirect stores must deref pointer-typed places.
- **Enum members**: auto-assigned members must use `get_default_index()`, not 0
  (0 made `ModuleType.CFile` become `File`, parsing C files as Chemical).
- **Namespace-qualified function calls returning aggregates** must NOT be
  mistaken for constructors (resolve the callee first).
- **Comptime calls**: evaluate via the interpreter and call `evaluated_value`
  on a returned `%runtime_value` while the scope is alive; resolve
  `CapturedComptimeVariable` bridge nodes and captured call refs.
- **Array literals**: zero the temp; array fields are copied with `memcpy` using
  the **source** size; array params decay to element pointers; zero-length arrays
  cannot take `= {0}`.
- **`memset` conflicts with cstd's `extern void* memset(...)`** — `CTranslationContext::prepare_translate`
  declares `memset`/`memcpy`/`memmove` (with the target's `size_t` spelling) before
  any module is translated, so emitting a bare `memset`/`memcpy` call resolves and
  cstd's later declaration stays compatible. (An inline zero loop is no longer
  required.)
- **Compound assignment on struct fields** must load/apply/store. The same is true
  for **index and dereference targets** (`arr[i] += x`, `p[i] *= y`, `*p += z`):
  they must load the current element/pointee, apply the operator and store.
  (MIR used to store the RHS directly, so `p[0] += 4` emitted `p[0] = 4`.)
- **`&raw arr[i]` / `&mut p[i]`** is the *element address*, not a load of the
  element. Lower it through `index_addr` / `index_addr_ptr` (`&base[i]`), which the
  emitter spells with `__typeof__`. `lower_address_of` must intercept
  `IndexOperator`, not fall through to `lower_expr`.
- **Blocks are scopes**: `ASTNodeKind::Block` / `Scope` / `UnsafeBlock` must lower
  their statements as a scope and destroy the locals declared inside them
  (`lower_scope_nodes`), not as a plain statement list.
- **Static method references** (`Type::method`, e.g. `CSSParser::parseMargin`):
  when the base links to a type and the leaf links to a `FunctionDecl`, lower the
  leaf to a function pointer (`function_addr`), even for methods with a `self`
  param. Do not take the address of the type name.
- **Qualified module constants** (`std::NPOS`): when the chain leaf links to a
  `VarInitStmt`, resolve the leaf directly (comptime constants inline).
- **String literals are `char*`** (matching the legacy `VisitStringType`), so
  `hex[i]` indexing a literal works. Do not map `BaseTypeKind::String` to `void*`.
- **Anonymous structs/unions**: a field whose type has no spellable C name (e.g.
  `union { ... } value;`) must be addressed with `__typeof__(base.field)* v =
  &base.field;` in `FieldAddr`/`IndexAddr` (TinyCC supports `__typeof__`).
- **Chain walker intermediate pointer fields**: `a.b.c` where `b` is a pointer
  field must dereference (`a.b->c`), not take `&a.b` and treat it as a struct
  pointer. Load the field address when the field type is Pointer/Reference.
- **Default arguments** must go through `lower_arg_converted` with the parameter
  type, so implicit constructors run (`path : string_view = ""` must materialize a
  `string_view` temp and pass its address, not pass the raw `""`).
- **Scope-based destruction**: `lower_scope`/`lower_scope_nodes` drops the
  destructibles it created; `return` drops all outstanding; `lower_function` drops
  the rest.
- **Function-pointer types returning an aggregate** must use the legacy **sret**
  spelling: `void(*)(struct Ret*, <params>)`, not `struct Ret(*)(<params>)`. The
  forward declarations come from the legacy visitor, so a mismatch is a C
  redefinition error. `CallIndirect` must pass the sret place as the first
  argument (`call_indirect_sret`).
- **`&raw` / `&mut` on a reference** (parameter or local) yields the *referent*
  address (the reference value), not `&param` (the slot). A `Reference`-typed
  place must be loaded, not addressed, in `lower_address_of`.
- **Post-increment/decrement yields the OLD value**; pre yields the new one.
  `lower_incdec_value` must return `cur` for `post`, `cur ± 1` for pre. (Used as
  an array index, e.g. `buf[bi++] = hex[...]`, a wrong result corrupts encoders.)
- **Global arrays decay**: indexing a module-level `char[]`/`T[]` must go through
  a pointer to the first element (`arr[i]`), not a pointer-to-array
  (`(&arr)[i]`). `intern_global` records the symbol type and `GlobalAddr` emits
  the bare name (decay) for array globals.
- **Destruction order is reverse**: every scope/function/break/continue cleanup
  must drop `destructibles_` innermost-first. Use `emit_drops_from(mark)` (or
  `emit_drops()` which forwards to it); never iterate `destructibles_` forward
  when emitting drops. Struct *members* are the exception: declaration order.
- **Arrays are not assignable in C**: any whole-array destination (local,
  element, field, through a pointer) must `memcpy` from the source address; never
  `load` an array (which spells `T v[N] = *ptr`) or emit `dest = src`.
- **`*mut [N]T` is pointer-to-array** (`T (*)[N]`), not `T**`. Do not decay an
  array pointee in `MIRTypeBuilder::map`; `declarator`/`c_type_of` spell the
  pointer-to-array form.
- **Empty aggregates are zero-sized.** An aggregate with no fields lowers to an
  empty LLVM struct, matching C's `struct S { }` and the interpreter's
  `byte_size` (→ 0). Never pad it with an `i8`: that made `sizeof` disagree with
  comptime and shifted every field of a struct embedding an empty aggregate.
- **An aggregate `Alloca` must be zeroed.** The C emitter emits a zero loop for
  every struct/union/variant/array `Alloca` right after declaring it; the C
  lowerer *relies* on that (a partial initializer such as `var s : [10]char =
  "hello"` only `memcpy`s the literal bytes). The LLVM `Alloca` case must emit
  the same `memset` at the same program point — not only at function entry — so
  a loop-body alloca is re-zeroed each iteration.
- **A member field name is a *dotted path*, possibly several levels deep.** An
  inherited member lowers to `Base.Mid.Leaf.field`
  (`MIRLowerer::inherited_field_path`); a variant payload is `Case.param`. The C
  emitter splices the whole string into the access chain, so any consumer that
  resolves the path against the type table (LLVM `field_path`, DI, dumps) must
  walk **every** dot-separated segment, resolving each against the type reached
  so far.
- **A `dyn<T>` built from a by-value aggregate parameter must use the argument
  pointer**, not the parameter's by-value copy (`MIRLowerer` keeps the argument
  pointer in `aggregate_param_ptrs_` for exactly this). The copy is a local of
  the callee, so pointing a fat pointer at it dangles the moment the callee
  returns — and the very next call's frame (usually the vtable method itself)
  overwrites it. This matches the interpreter, which shares the argument value,
  and the documented "aggregate params are pointer-typed places" rule.

## Invariants to preserve

- No `ASTNode*`/`Value*`/`FunctionDeclaration*` retained in persisted MIR.
- **No backend reads the AST — backends are MIR-only.** No `ast/` include, no
  source-node parameter or walk, no re-derived layout/ABI/declaration/lifetime
  facts in an emitter; missing facts get added to MIR. Only `MIRTypeBuilder` and
  `MIRLowerer` touch the AST.
- No `std::unordered_map`/`chem::string` per instruction; dense ID tables only.
- **Never read a cross-module function body during lowering.** A body lives on the
  module allocator (freed after its module) unless `is_body_retained()`; only the
  job-allocated *declaration* survives. Cross-module facts must be cached on the
  declaration while the body is alive (`prime_constructor_fact`), and body reads
  gated by `function_body_is_readable` (Session 28).
- Effects/portability/compaction safety are opcode-static.
- Every block ends in exactly one terminator; use-counts updated on append.
- `MIRTests` must stay green before every MIR commit.
- **MIR core `.cpp` are in `COMMON_SOURCES`** so the real compiler build
  compiles them (not just `MIRTests`).

## Build / test commands

```bash
bash -lc "source scripts/msvc_env.sh && cmake --build cmake-build-debug --config Debug --target TCCCompiler MIRTests -j 8"
./cmake-build-debug/MIRTests.exe

# full main suite (green as of latest commit):
Remove-Item -Recurse -Force lang/tests/build/chemical-tests.dir, lang/tests/build/lab
./scripts/test.sh --tcc

# inspect the translated C for the test executable:
./cmake-build-debug/TCCCompiler.exe lang/tests/build.lab -o lang/tests/build/tests-tcc.exe --mode debug_quick --no-cache --emit-c
# -> lang/tests/build/chemical-tests.dir/Translated.c
```
