# MIR migration — evaluation & consolidation/performance plan

Status: **evaluation + actionable plan** (evidence-backed, reproducible).
Branch: `mir`. Supersedes nothing; complements `compiler/mir/README.md`,
`lang/docs/mir-implementation-plan.md`, `lang/docs/mir-llvm-migration.md`.

> **Verdict (short).** The migration is *functionally* a large achievement — MIR
> is the only C path, the legacy 2c visitor is deleted, the LLVM target emits
> from MIR, and the suites are green. It is **not yet a great job on the
> non-functional axes we care about**: it is ~2.3× slower to compile, uses ~40%
> more peak memory, emits ~70% more C and ~60–70% larger executables, grows the
> compiler binary instead of shrinking it, and does **not** reduce per-backend
> code (it adds ~17.5k lines to delete ~11k, with the declaration/type layer
> still AST-based). All numbers below are measured on the L1 machine against the
> pre-MIR `main` compiler, same source, same mode.

The four goals this document optimises for (per project direction): **compiler
throughput, compiler peak memory & binary size, generated-code quality (runtime
& binary size), and one shared MIR core with four thin backends** (C, LLVM, JVM,
JS; `Compiler` links all four, `TCCCompiler` links C only and must stay tiny).

---

## 0. How the numbers were produced

Environment: Linux, `wakaztahir`, AMD Ryzen 5 7430U (6C/12T), 14 GiB RAM,
`Debug` build (`cmake-build-debug`), mode `debug_quick`. The pre-MIR reference is
`main` (`26be371c0`) built in a git worktree at `/tmp/opencode/chemical-main`
with `scripts/configure.sh --no-llvm && scripts/build.sh --tcc -j 6`.

```bash
# suite compile (cold = per-module C cache cleared, as the README requires)
rm -rf lang/tests/build/chemical-tests.dir lang/tests/build/lab
/usr/bin/time -v ./scripts/test.sh --tcc --no-build --no-run --mode debug_quick

# identical standalone probe compiled by both compilers
./cmake-build-debug/TCCCompiler lang/compiled/perfprobe/chemical.mod \
    -o /tmp/probe_mir.exe --mode debug_quick --no-cache --emit-c
# legacy: same command from the main worktree
```

The probe (`lang/compiled/perfprobe/`, gitignored) exercises struct methods,
`&mut self` mutation, `std::vector`, `std::string`, recursion, function pointers,
loops, and struct construction. Both compilers produce **identical program
output**, so the comparison is purely about codegen quality/throughput.

---

## 1. Measured evidence

### 1.1 Compiler throughput & memory

| Workload | Legacy (`main`) | MIR (`mir`) | Delta |
|---|---:|---:|---:|
| Main suite compile, cold cache (median of 3) | **2.80 s** | **6.49 s** | **+132% (2.3×)** |
| Main suite compile, warm cache | 1.31 s | 3.98 s | +204% (3.0×) |
| Main suite peak RSS, cold | **43.4–45.6 MB** | **60.4–63.7 MB** | **+40%** |
| Probe compile | 0.20 s | 0.50 s | **+150% (2.5×)** |
| Probe peak RSS | 14.9 MB | 17.2 MB | +15% |

This is well outside the plan's own NFR (`NFR-1 ≤ baseline × 1.05`, `NFR-2`). The
plan explicitly forbids accepting this: *"A functional fix that doubles compile
time or grows the binary is not a fix until the regression is resolved"*
(`mir-implementation-plan.md` §0 rule 6).

### 1.2 Generated code & generated binary size

Probe (identical source and output):

| Metric | Legacy | MIR | Delta |
|---|---:|---:|---:|
| Translated C | 8,285 lines / 310,696 B | 14,115 lines / 615,461 B | **+70% lines / +98% bytes** |
| Aggregate zeroing loops | **0** | **584** | new |
| Executable | 341,203 B (text 285,299) | 542,895 B (text 486,964) | **+59% / +71% text** |
| Runtime (4×10⁷-iteration struct-method loop) | ~0.45 s | ~0.50 s | ~+10% |
| Runtime (documented bcrypt cost-12, README) | 0.6 s | 1.0 s | **+67%** |

> **Status update (2026-10-13) — action C1 landed (partial).** Aggregate zeroing
> is now conditional: the emitters no longer zero *every* aggregate `Alloca`,
> and the one lowerer site that relied on that blanket zero
> (`var s : [N]char = "<literal>"`) now emits its own explicit `MemSet`
> (`MIRLowerer::lower_stmt`). Same probe, same machine, same mode:
>
> | Metric | Legacy | MIR (before) | MIR (now) |
> |---|---:|---:|---:|
> | Probe zero loops | 0 | 584 | **182** (−69%) |
> | Probe generated C lines | 8,285 | 14,115 | 13,713 |
> | Probe executable | 341 KB | 543 KB | **501 KB** (−7.7% vs before) |
>
> This is a real step toward the C2/KB targets but nowhere near them yet; C2
> (aggregate double copies), C3 (single-use param spills) and D (hot path) are
> still open. Note the zeroing change removed 402 of the 584 loops; the
> remaining 182 are explicit `zero_init`s whose destination is then fully
> initialized field-wise (the `110 call` + `38 assign` classes below), which is
> the next C1 bite.

The main test executable corroborates it: `tests-tcc.exe` is **3.24 MB (legacy)
vs 5.32 MB (MIR)** (+64%). The plan's explicit intent — *"we will deliberately
emit less C, better C, and more performant C than legacy 2c"* (§6.14) — is not
met; the inverse is currently true.

Representative cause (MIR output for a trivial `&mut self` method):

```c
void perfprobe_Counterbump(struct perfprobe_Counter* __chx_a0, int __chx_a1) {
    struct perfprobe_Counter* __chx_p0;
    __chx_p0 = (__chx_a0);          /* param spill */
    int __chx_p1;
    __chx_p1 = (__chx_a1);          /* param spill */
    int __chx_v4 = (__chx_p0)->total;
    (__chx_p0)->total = ((int)(__chx_v4 + (__chx_p1)));
    int __chx_v8 = (__chx_p0)->total;
    return __chx_v8;
}
```

```c
/* every aggregate alloca gets a byte-wise loop */
struct perfprobe_Vec2 __chx_p0;
for (unsigned long __chx_za2 = 0; __chx_za2 < sizeof(__chx_p0); ++__chx_za2)
    ((unsigned char*)&__chx_p0)[__chx_za2] = 0;
struct perfprobe_Vec2 __chx_v2 = *(__chx_a1);   /* load-then-assign double copy */
__chx_p0 = __chx_v2;
```

Concrete emit-level waste to remove:
1. **Unconditional byte-wise zero loop on every aggregate `Alloca`** (584 in one
   small probe; legacy emits none). It is both a size and a runtime cost, and it
   runs per loop iteration for loop-local aggregates.
2. **Aggregate double copies** (`v = *src; dest = v`) that can be one `memcpy` /
   assignment.
3. **Redundant scalar parameter spills** (`__chx_pN = __chx_aN`) where the param
   has a single use; the existing single-use inlining covers `Value` params but
   not these `Place`-typed spills.
4. **Redundant blocks/terminators** (`__chx_bbN:; return;` epilogues, gotos for
   trivial if/else).

### 1.3 Compiler binary & code volume

| Metric | Legacy | MIR | Delta |
|---|---:|---:|---:|
| `TCCCompiler` debug binary | 65,110,688 B | 69,988,848 B | **+7.5%** |
| MIR core (cpp+h, excl. tests/README) | — | **15,245 lines** | new |
| `CodegenMir.cpp` (MIR→LLVM) | — | 2,249 lines | new |
| Deleted legacy (`2cASTVisitor.*`, destruction/top-level visitors) | ~**11,024 lines** | — | removed |
| New `TranslationContext` | — | ~751 lines | new |

MIR added materially more code than it deleted, in direct tension with the
"delete as you add / stay under 4 MB" budget (`mir-implementation-plan.md`
§1.4). `MIRLowerer.cpp` is a single **8,224-line** translation unit (the plan
specified `MIRLowerExpr/Stmt/Call/Lambda`); `lower_expr` alone is 1,745 lines and
`lower_stmt` 1,072. This slows the compiler's own build and makes review hard.

> The binary numbers are `Debug`; the release `< 4 MB` budget must be confirmed
> with a release build (tracked as **M1** below). The *direction* is unambiguous.

### 1.4 Architecture: not one core + thin backends

The intended shape is "one MIR, small per-backend emitters". Today:

- **The declaration / type / layout layer is still AST-based.** `MIRDeclEmitter`
  includes ~47 AST headers and walks `ASTNode*` / `BaseType*` /
  `StructDefinition*` / `FunctionDeclaration*` to emit C declarations,
  prototypes, globals and vtables (`compiler/mir/MIRDeclEmitter.{h,cpp}`,
  1,236 lines). It does **not** read `MIRTypeRecord`. So MIR is not the single
  source of truth for the program's types.
- **Aggregate layout does not exist in MIR.** `MIRTypeBuilder::aggregate_type`
  never sets `size`/`alignment` for structs/unions/variants (`MIRTypeBuilder.cpp`
  L134–174); `MIRTypeRecord.size == 0` for every aggregate. Layout is therefore
  computed **three times**: by the AST decl emitter (C), by LLVM `DataLayout`
  (`CodegenMir::mir_type`), and by TinyCC. A JVM/JS backend would be a fourth.
- **C type spelling is implemented twice.** `MIREmitter.cpp` (`c_type_of`,
  `c_decl_type`, `declarator`, `c_function_proto`) and `MIRDeclEmitter.cpp`
  (`type_spelling`, `decl_spelling`) independently spell the same types and must
  be kept in sync by hand (see README gotcha "type spelling must match").
- **Each backend is a full independent instruction dispatcher.** `MIREmitter`
  (1,601 lines, ~80 opcode cases) and `CodegenMir` (2,249 lines, ~49 cases) share
  no MIR traversal, ABI, naming, or layout helper. Adding JVM and JS as two more
  full dispatchers is exactly the per-backend code explosion to avoid.

**What is genuinely good and must be preserved:** MIR is AST-free at its core
(`CodegenMir.cpp` has zero AST type references; only `MIRTypeBuilder`,
`MIRLowerer`, `MIRDeclEmitter` touch AST); `MIRInstruction` is 16 bytes with
static opcode metadata; there is a portability classification
(`MIRPortability` + `opcode_portability`, `MIRInstruction.cpp` L199) ready for
JVM/JS; drop/lifetime decisions are explicit in MIR; and the legacy deletion is
real (no fallback, no capability analysis).

### 1.5 Performance of the lowering hot path (root causes in code)

The plan's hot-path rules ("no `std::unordered_map` in the hot path", "no
per-instruction strings", §6.1/§6.9/§1.2) are violated:

- **Every identifier lookup allocates a `std::string`.**
  `MIRLowerer::place_for_name` does `name_places_.find(std::string(name.data(),
  name.size()))` (`MIRLowerer.h` L442–446) and is called on every identifier
  (`MIRLowerer.cpp` L2581, L3711, L4320, …). `bind_name` allocates on every local
  declaration.
- **18 `unordered_map`/`unordered_set` members in `MIRLowerer`**
  (`MIRLowerer.h` L452–552): `var_places_`, `name_places_`, `func_symbols_`,
  `named_method_symbols_`, `global_symbols_`, `pattern_bases_`,
  `pattern_results_`, `lambda_materializations_`, `async_site_index_`, … The plan
  specified dense ID-indexed tables instead.
- **Type interning is an O(n) linear scan** (`MIRTypeTable::intern`,
  `MIRTypeTable.h` L156–162) called from `pointer_type`/`sized_array_type`/
  `function_signature`/`int_type`, i.e. on the lowering path.
- **Constant interning is an O(n) linear scan with a byte loop**
  (`MIRConstantPool::add`, `MIRConstantPool.h` L42–57), called for every literal.
- **`MIRLowerer.cpp` is one 8,224-line TU** (build-time and review cost).
- **Lowering is serial per module.** Stage 9 (parallel file lowering) is not
  implemented; `implement_module` walks files one at a time and clears a
  per-file memo (`ASTProcessor.cpp` L2354–2376).

---

## 2. Action plan

Each item: symptom → evidence → action → acceptance. Priority P0 (correctness of
the architecture/size story), P1 (perf), P2 (polish). "Done" means the numbers in
§3 move and the C + LLVM suites stay green (with `MIRTests` green and the
`--tcc` main suite at parity).

### A. Make MIR the single source of truth for types & layout (P0)

**Symptom.** Declarations/layout are AST-derived and recomputed per backend;
adding JVM/JS multiplies the work.
**Evidence.** §1.4: `MIRDeclEmitter.cpp` AST-coupled; `MIRTypeRecord.size == 0`
for aggregates; two independent C type spellers.
**Action.**
1. Fill `MIRTypeRecord` for every aggregate in `MIRTypeBuilder`:
   `size`, `alignment`, and each `MIRFieldInfo.offset` (target layout from
   `MIRModuleLayout`, not host). Add `MIRTests` comparing against `sizeof`/
   `offsetof` in C.
2. Add a single `mir::TypePrinter` (or `MIRTypeFormat`) in MIR core that renders
   a `TypeId` + `declarator` for the **C** backend from MIR only; delete the
   duplicate `MIRDeclEmitter::type_spelling`/`decl_spelling` in favour of it.
3. Replace `MIRDeclEmitter`'s AST walk with a MIR declaration table
   (`MIRTypeDecl`/`MIRPrototype`/`MIRGlobal`, already sketched in the plan §2.5)
   populated by the lowerer, so declarations are emitted from MIR for every
   backend.
**Acceptance.** No AST headers in the declaration path; one C type speller;
`MIRTypeRecord.size/alignment` non-zero for aggregates and verified by
`MIRTests`; C + LLVM output unchanged or smaller.

### B. Make each backend a thin opcode dispatcher over shared MIR services (P0)

**Symptom.** Each backend re-implements traversal/ABI/naming; JVM/JS would add
two more full emitters.
**Evidence.** §1.4. `MIREmitter` and `CodegenMir` share no helper.
**Action.** Extract backend-neutral services used by all emitters:
`MIRABI` (sret, aggregate-by-pointer, fat pointer, variadic, capturing ABI),
`MIRValueFacts` (use counts, single-use/pure classification for compaction),
place/symbol/constant naming, and block iteration. Backends implement only
`opcode → target construct`. Use `opcode_portability()` to keep `NativeOnly`
opcodes (raw pointers, `InlineAsm`, atomics) out of JVM/JS and report a
diagnostic for them.
**Acceptance.** Each new backend adds only a dispatcher + printer, no layout/ABI
logic; measured by (backend emitter LOC / total MIR LOC) trend.

### C. Kill the generated-code waste (P1)

**Symptom.** +70% C lines, +70% text, runtime worse.
**Evidence.** §1.2.
**Action.**
1. Zero an aggregate only when a partial initializer needs it; never both the
   destination and its temp. Prefer field-wise init. (Respect the `memset`/cstd
   conflict: inline loop only where required.)
2. Collapse `temp = *src; dest = temp` into one copy/`memcpy`; extend the
   single-use collapse to aggregate places and loaded values.
3. Do not spill single-use scalar parameters (`__chx_pN = __chx_aN`); the
   `Param` inliner must cover `Place`-typed spills too.
4. Emit no `goto`/block label for straight-line statements; drop dead epilogue
   blocks.
**Acceptance.** Probe C lines and executable text ≤ legacy on `perfprobe`; no
zero-loop on an alloca that is fully initialized field-wise; runtime ≥ parity on
the `benchprobe` loop and the bcrypt cost-12 benchmark.

### D. Fix the lowering hot path (P1)

**Symptom.** 2.3× compile time, +40% RSS.
**Evidence.** §1.5.
**Action.**
1. Intern names in a module byte pool and key maps by a `(data,len)` id/hashed
   view; eliminate `std::string` construction per identifier/declaration.
2. Replace linear-scan type/constant interning with a hash index
   (`unordered_map<hash, vector<id>>` or an open-addressed table keyed on the
   structural fields). Keep deterministic ordering.
3. Replace the 18 `unordered_map`s in `MIRLowerer` with dense vectors keyed by
   AST-node id or a per-function symbol/lookup table (plan §6.1).
4. Split `MIRLowerer.cpp` into `MIRLowerExpr/Stmt/Call/Aggregate/Lambda/Async`.
**Acceptance.** Main-suite cold compile ≤ 1.05× legacy; peak RSS ≤ 1.05× legacy;
no `std::string` allocation on the identifier path (verifiable by review/instr).

### E. Parallelise lowering (P2, after D)

**Symptom.** Serial per-module lowering.
**Evidence.** §1.5.
**Action.** Stage 9: file/function-parallel lowering with reusable per-worker
arena/buffers and slot-direct deterministic merge.
**Acceptance.** ≥1.5× on 4+ cores for large modules; no RSS regression on small
modules.

### M. Measurement & budget tasks

- **M1 (P0):** build the **release** `TCCCompiler` for `main` and `mir`; confirm
  the `< 4 MB` budget and the net binary delta. Add the two numbers to
  `mir-implementation-plan.md` §11.5.
- **M2 (P0):** add a CI/perf gate script that runs the probe + suite compile and
  fails on >5% regression vs the recorded legacy baseline (the plan already
  specifies `scripts/mir-baseline.sh`; extend it with C-lines, exec size, and the
  probe).
- **M3 (P1):** record the same suite/probe table on W1 (Windows) as the plan
  requires (both baselines must hold).

---

## 3. Targets (acceptance for "the migration is done well")

| Axis | Legacy reference | Current MIR | Target |
|---|---:|---:|---:|
| Main-suite compile (cold) | 2.80 s | 6.49 s | ≤ 2.94 s (×1.05) |
| Main-suite peak RSS | 43.4 MB | 60.4 MB | ≤ 45.6 MB (×1.05) |
| Probe executable | 341 KB | 543 KB | ≤ 341 KB |
| Probe C lines | 8,285 | 14,115 | ≤ 8,285 |
| Zero loops / probe | 0 | 584 | only where required |
| `tests-tcc.exe` | 3.24 MB | 5.32 MB | ≤ 3.24 MB |
| `TCCCompiler` binary | 65.1 MB (D) | 70.0 MB (D) | ≤ legacy; release < 4 MB |
| Per-backend code | n/a | 2 full dispatchers | thin dispatcher over shared MIR |

These are hard gates from `mir-implementation-plan.md` §0 rule 6 / §10, not
aspirations. They are also the prerequisites for the JVM/JS backends being
tractable: without a layout/ABI/decl core in MIR and a shared traversal, each new
backend is another 2k-line emitter with its own bugs.

---

## 4. What is already right (do not regress)

- MIR is the only path; no flag, no fallback, no capability analysis.
- Legacy 2c visitor deleted; function bodies, static-interface impls, CBI
  `EmbeddedNode`, and async all lower through MIR.
- `MIRInstruction` is a compact 16-byte record with opcode-static flags.
- `MIRPortability`/`opcode_portability` exists for non-native targets.
- The LLVM emitter is AST-free and the main `--llvm` suite is green.
- Lifetime/drop semantics are explicit MIR instructions (good basis for JVM/JS
  GC-aware lowering).

## 5. One-line summary

The MIR migration got the **semantics and single-path discipline** right, but at
the cost of **2.3× compile time, +40% memory, +70% generated code/binary size, a
compiler binary that grew instead of shrank, and a type/declaration layer that is
still AST-based** — so the "one small core, thin backends" architecture that
makes JVM/JS cheap does **not** exist yet. The action plan above makes it exist.
