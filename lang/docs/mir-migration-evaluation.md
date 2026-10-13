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

> **Status update (2026-10-13) — action D started.** The quadratic type/constant
> interning was removed (verified hash index; see §1.5). Measured on the same
> machine with clean binaries and interleaved A/B runs: suite-compile user CPU
> **3.92 s → 3.61 s (−8%)**, `MIRTypeTable::intern` **0.498 s → 0.208 s (−58%)**,
> `MIRConstantPool::add` 0.186 s → 0.165 s, peak RSS flat. That is ~20% of the
> gap to the legacy baseline; the remaining MIR time per suite compile is
> `MIRLowerer` **1.98 s** (unattributed — no profiler available here), C emission
> **0.63 s**, `declare_module` **0.18 s**.
>
> **Status update (2026-10-13, session 8) — the MIR phase is now attributed per
> function** (temporary RAII timers, since removed) and there is **no hot
> function**: 1,318.6 ms of lowering over 6,776 calls / 1,530 functions /
> 200,651 instructions (≈ 6.6 µs each), ≈ 665 ms of C emission (≈ 3.3 µs each),
> top-20 functions only 36% of the phase (see §1.5). Action C3 (scalar parameter
> spills) landed in the same session: probe C 13,713 → **12,393** lines, probe
> spills 673 → **25**, main-suite exe text **−4.3%**, suite-compile user CPU
> 3.85 s → **3.81 s** (5 interleaved pairs, ≤ in every pair).

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
>
> **Status update (2026-10-13, session 8) — action C3 landed (scalar parameter
> spills).** `emit_function_c` aliases a scalar parameter place to its C argument
> (`__chx_aN`) when the body never reassigns or addresses it, so neither the
> stack slot nor the spill store is emitted. Same probe, same mode
> (`debug_quick`), before = the post-C1 binary:
>
> | Metric | Legacy | MIR (post-C1) | MIR (now) |
> |---|---:|---:|---:|
> | Probe generated C lines | 8,285 | 13,713 | **12,393** (−9.6%) |
> | Probe generated C bytes | 310,696 | 565,650 | **523,052** (−7.5%) |
> | Probe `__chx_pN = (__chx_aM);` spills | 0 | 673 | **25** (−96%) |
> | Probe executable text | 285,299 | — | **409,048** |
> | Full-suite spill statements | — | 3,364 | **143** (−96%) |
> | Main-suite executable (text) | — | 4,165,453 | **3,985,243** (−4.3%) |
>
> **Status update (2026-10-13, session 8, continued) — actions C2 and C4 landed
> too.** C4 emits a block label only for a block a terminator can jump to (every
> emitted `goto` comes from a `Block` operand, so that set is exact — the entry
> block's `__chx_bb0:;` was pure noise). C2 collapses `T v = *src; dest = v;`
> into `dest = *src;` when the load's single use is the immediately following
> store and the destination is one of the frame's own slots (a `Parameter` copy
> or a `Temporary`), so the read cannot move past anything and the source cannot
> overlap the destination.
>
> | Metric | Legacy | post-C1 | after C3 | after C2+C4 |
> |---|---:|---:|---:|---:|
> | Probe generated C lines | 8,285 | 13,713 | 12,393 | **11,868** |
> | Probe generated C bytes | 310,696 | 565,650 | 523,052 | **515,872** |
> | Probe block labels | — | 1,833 | 1,833 | **1,331** (−27%) |
> | Probe deref double hops (`v = *src; dest = v`) | 0 | 23 | 23 | **0** |
> | Probe executable text | 285,299 | — | 409,048 | **407,577** |
> | Suite generated C lines | — | 120,782 | 114,280 | **108,475** |
> | Suite block labels / `goto`s | — | — | 17,216 / 10,334 | **11,660** / 10,334 |
> | Main-suite executable text | — | 4,165,453 | 3,985,124 | **3,971,116** |
>
> (Line/label/`goto`/spill counts are deterministic; C bytes and executable size
> vary by a few bytes run to run because the emitted opaque forward declarations
> are ordered non-deterministically — a budget gate (M2) must compare counts.)
>
> Cumulatively C1+C3+C2+C4: suite C **−10.2% lines**, main-suite text **−4.7%**,
> probe C **−13.4% lines**. The probe is still well above the legacy targets
> (11,868 vs 8,285 C lines; 407 KB vs 285 KB text), so action C's acceptance is
> still not met: what is left is largely the aggregate-zeroing class and
> field-wise initialization. The aggregate half of C3 stays deliberately
> un-aliased (a by-value aggregate parameter is a real copy). `MIRTests`'
> `test_control_flow_emitter` had asserted the entry label was emitted; that
> expectation was *updated* (not dropped) to require the referenced labels and
> the absence of the dead one — 1049 → 1052 checks.

The main test executable corroborates it: `tests-tcc.exe` is **3.24 MB (legacy)
vs 5.32 MB (MIR)** (+64%). The plan's explicit intent — *"we will deliberately
emit less C, better C, and more performant C than legacy 2c"* (§6.14) — is not
met; the inverse is currently true.

Representative cause (MIR output for a trivial `&mut self` method; the two
annotated spills and their slots no longer appear after C3 in session 8):

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

After C3 the same method emits `__chx_a0`/`__chx_a1` directly:

```c
int perfprobe_Counterbump(struct perfprobe_Counter* __chx_a0, int __chx_a1) {
    int __chx_v4 = __chx_a0->total;
    __chx_a0->total = ((int)(__chx_v4 + __chx_a1));
    return __chx_a0->total;
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
- ~~**Aggregate layout does not exist in MIR.**~~ **DONE (session 10, action
  A1).** `MIRTypeBuilder::aggregate_type` used to never set `size`/`alignment`
  for structs/unions/variants, so `MIRTypeRecord.size == 0` for every aggregate
  and layout was computed **three times** (AST decl emitter / LLVM `DataLayout` /
  TinyCC). Now `compiler/mir/MIRLayout.h` (AST-free, `MIRTests`-covered) fills
  `size`, `alignment` and every `MIRFieldInfo.offset` from the type table alone:
  `MIRTypeBuilder::fill_aggregate_fields` lays out each struct/union/variant and
  each variant case payload, and `MIRLowerer` lays out the async frame and the
  capturing-lambda closure struct. Target dependence enters only through the
  field records (pointer/`long` facts built from `MIRModuleLayout`). It is a
  *data-filling* step — nothing reads the fields yet, so generated C, exe size,
  compile time and RSS are unchanged (measured: probe 102 zero loops / 11,550
  lines / 504,054 B and main-suite C 1,231 / 107,191 lines, identical both
  sides). What is left of A is the type printer and declaration table (A2/A3);
  LLVM `CodegenMir::mir_type` still builds its own layout from the same table.
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
per-instruction strings", §6.1/§6.9/§1.2) are violated. **Measured 2026-10-13**
per cold main-suite compile (`MIR_PROF` instrumentation, since removed):

- **Type interning was an O(n) linear scan** (`MIRTypeTable::intern`) called from
  `pointer_type`/`sized_array_type`/`function_signature`/`int_type`, i.e. on the
  lowering path: **186,289 calls scanning 101.4 M elements, 0.498 s**. *Fixed in
  session 7 by a verified hash index* — now 7.0 M scanned / 0.208 s (−93% / −58%).
- **Constant interning was an O(n) linear scan with a byte loop**
  (`MIRConstantPool::add`), called for every literal: **97,279 calls scanning
  56.3 M elements, 0.186 s** → now 6.6 M / 0.165 s (the residual is the payload
  hash, not the scan). Also fixed in session 7.
- **Every identifier lookup allocates a `std::string`.**
  `MIRLowerer::place_for_name` does `name_places_.find(std::string(name.data(),
  name.size()))` and `bind_name` allocates on every local declaration. **Measured:
  2,181 `place_for_name` calls / 0.002 s and 19,705 `bind_name` calls / 0.017 s
  per suite compile** — a violation of the plan's letter, but *not* a measurable
  cost, so it is not worth churning for performance (fix it with a transparent
  hash when the surrounding code is touched anyway).
- **18 `unordered_map`/`unordered_set` members in `MIRLowerer`**
  (`MIRLowerer.h`): `var_places_`, `name_places_`, `func_symbols_`,
  `named_method_symbols_`, `global_symbols_`, `pattern_bases_`,
  `pattern_results_`, `lambda_materializations_`, `async_site_index_`, … The plan
  specified dense ID-indexed tables instead. **Attributed per function in session
  8** (RAII timers in `MIRLowerer::lower_function` + around the lower+emit call,
  temporary): lowering **1,318.6 ms** over **6,776 calls / 1,530 distinct
  functions / 200,651 instructions** (≈ 6.6 µs per instruction), C emission
  **≈ 665 ms** (≈ 3.3 µs per instruction). **No function dominates**: the top 20
  by self time are ≈ 470 ms (36%), the largest being `build` 67.4 ms (98 calls,
  20,031 instructions), then `test_destructors` 30.9, `test_for_in_span` 28.2,
  `test_nodes` 27.2, `delete` 26.2 (388 calls), … — the remaining ~65% is the tail
  of ~1,510 small functions, mostly per-type generic instantiations (`delete`
  ×388, `make` ×331, `resize` ×74, `push` ×65). So the fix is the
  **per-instruction** cost (builder/arena/type-mapping overhead) or lowering
  fewer instantiated copies — not rewriting one hot function.
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
for aggregates (fixed by item 1 below); two independent C type spellers.
**Action.**
1. ~~Fill `MIRTypeRecord` for every aggregate in `MIRTypeBuilder`: `size`,
   `alignment`, and each `MIRFieldInfo.offset` (target layout from
   `MIRModuleLayout`, not host). Add `MIRTests` comparing against `sizeof`/
   `offsetof` in C.~~ **DONE (session 10).** `compiler/mir/MIRLayout.h` —
   `compute_aggregate_layout` follows the C target's rules (fields in order at
   their alignment; a union puts every field at 0 and is sized by its largest
   member; a variant is bases → `int` tag → *union* of the case payloads; the
   aggregate's explicit `__chx_align(N)` raises its alignment and pads its size).
   Callers: `MIRTypeBuilder::fill_aggregate_fields` (every struct/union/variant
   and every variant case payload) plus `MIRLowerer` for the async frame and the
   lambda closure struct. `MIRFieldInfo` gained `offset`/`align` in the field
   pool (never interned, so type identity is unaffected). `MIRTests` **1052 →
   1111 checks**: nine mirrored C aggregates checked against the C compiler's
   `sizeof`/`offsetof`; independently, a probe with inheritance, a nested struct,
   an array member, a variant and an anonymous union matched `gcc`'s numbers for
   the emitted C exactly. Output-invariant by construction and by measurement
   (see §1.4); `--tcc` **2300/2300**, `--llvm` **2298/2298**, `MIRTests` OK.
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
   conflict: inline loop only where required.) **Second bite DONE (session 9):**
   a struct literal's whole-temp `zero_init` is now emitted only when the literal
   actually leaves a member undefined (`MIRLowerer::struct_literal_defines_all_fields`
   + the partial-array guard). Measured, same machine/mode, interleaved A/B:
   main-suite C zero loops **2,655 → 1,231 (−54%)**, main-suite C lines 108,615 →
   107,191, struct-heavy probe zero loops 173 → **102**. `--tcc` **2300/2300**,
   `MIRTests` **1052 checks OK**. Still open: the constructor-result temp
   (`Type(args)`) and the four `zero_init`s inside `apply_struct_defaults`.
2. ~~Collapse `temp = *src; dest = temp` into one copy/`memcpy`; extend the
   single-use collapse to aggregate places and loaded values~~ **DONE
   (session 8)**: a deref load whose only use is the immediately following store
   collapses into it when the destination is a frame-local slot. Probe double
   hops 23 → **0**, probe C 12,393 → **11,868** lines.
3. ~~Do not spill single-use scalar parameters (`__chx_pN = __chx_aN`)~~ **DONE
   (session 8)**: `emit_function_c` aliases a `Parameter` place to its C argument
   when it is non-aggregate, written once by the `Param` store, and never
   addressed or otherwise named; the slot and the spill store disappear. Probe
   spills 673 → **25**, suite spills 3,364 → **143**, probe C 13,713 → **12,393**
   lines, main-suite exe text 4,165,453 → **3,985,243 B** (−4.3%), compile time
   unchanged/slightly better (5 interleaved A/B pairs, median 3.85 s → 3.81 s).
   The `Param` inliner covers the `Place`-typed spill as the item asked; the
   aggregate case is intentionally left alone (see §1.2 status update).
4. ~~Emit no `goto`/block label for straight-line statements~~ **DONE (session
   8, the label half)**: a label is emitted only for a block some `Block`
   operand jumps to — suite labels 17,216 → **11,660** (−32%), `goto`s unchanged,
   C lines 114,280 → **108,475**. *Still open:* dropping dead epilogue blocks.
**Acceptance.** Probe C lines and executable text ≤ legacy on `perfprobe`; no
zero-loop on an alloca that is fully initialized field-wise; runtime ≥ parity on
the `benchprobe` loop and the bcrypt cost-12 benchmark.
**Status (session 8).** Probe C lines 14,115 → **11,868** and text 486,964 →
**407,577** B since the evaluation, against legacy 8,285 lines / 285,299 B: the
C1+C3+C2+C4 fixes closed ~40% of the line gap and ~45% of the text gap without
touching runtime-relevant zeroing; the acceptance is not met yet.

### D. Fix the lowering hot path (P1)

**Symptom.** 2.3× compile time, +40% RSS.
**Evidence.** §1.5.
**Action.**
1. Intern names in a module byte pool and key maps by a `(data,len)` id/hashed
   view; eliminate `std::string` construction per identifier/declaration.
   *(Downgraded after measurement: 2,181 + 19,705 calls, 0.019 s total.)*
2. ~~Replace linear-scan type/constant interning with a hash index~~ **DONE
   (session 7)**: `MIRTypeTable`/`MIRConstantPool` keep a `hash → id` index that
   hashes exactly the compared fields and is used as a *verified hint* (every hit
   re-checked with `equal`, every miss falls back to the scan), so mutated-after-
   intern records cannot be aliased. −8% suite-compile user CPU (3.92 s → 3.61 s
   median, ranges disjoint); −0.31 s of MIR time; RSS flat.
3. Replace the 18 `unordered_map`s in `MIRLowerer` with dense vectors keyed by
   AST-node id or a per-function symbol/lookup table (plan §6.1). Needs a
   profiler or finer phase timers to target (see D5).
4. Split `MIRLowerer.cpp` into `MIRLowerExpr/Stmt/Call/Aggregate/Lambda/Async`.
5. Add phase timers (behind an env var, off by default) so the remaining
   `MIRLowerer` time can be attributed without `perf`, which is unavailable on
   this machine (`perf_event_paranoid=4`).
**Acceptance.** Main-suite cold compile ≤ 1.05× legacy; peak RSS ≤ 1.05× legacy;
no `std::string` allocation on the identifier path (verifiable by review/instr).
**Status.** Action D is ~20% done by the CPU-time measure: the quadratic
interning term is gone (−0.31 s of 3.92 s), and the remaining ~1.98 s of the MIR
phase (1.32 s lowering + 0.67 s emission, 0.18 s of declaration emission) is
still open. Sessions 7 and 8 closed the *measurement* half of D5 — the phase is
attributed down to per-function self time with no `perf` — and disproved the
"one hot function" hypothesis: the cost is per-instruction overhead and the tail
of ~1,510 small generic instantiations, so D3 (dense tables) and lower per-
instruction cost are the levers, not a targeted rewrite.

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

> **Progress against the table (2026-10-13, sessions 7–8).** The "Current MIR"
> column is the evaluation's baseline snapshot; the numbers below are from this
> checkout, measured with `TCCCompiler lang/tests/build.lab --mode debug_quick
> --no-cache` (interleaved A/B, both binaries built from this tree):
>
> | Axis | Legacy ref | Eval snapshot | Now (session 8) |
> |---|---:|---:|---:|
> | Main-suite compile+link user CPU (`debug_quick`, incl. TinyCC) | — | — | **3.81 s** |
> | Probe C lines | 8,285 | 14,115 | **11,868** |
> | Probe executable (text) | 285,299 B | 486,964 B | **407,577 B** |
> | Main-suite C lines / labels | — | — | **108,475 / 11,660** |
> | Main-suite executable (text) | — | 4,165,453 B | **3,971,116 B** |
>
> The compile+link number is this repo's `debug_quick` suite build including the
>TinyCC step; it is *not* directly comparable to the 6.49 s row (different
> measurement), but the direction after sessions 7–8 is a ~8% CPU reduction and a
> 9.6% smaller probe C. Every gate is still missed; C.2/C.4, D.1/D.3/D.4 and
> A2/A3/B remain open.
>
> **Session 9–10 add no gate movement, by design.** Session 9 (conditional
> struct-literal zeroing) cut main-suite zero loops 2,655 → 1,231 and probe zero
> loops 173 → 102 at flat line count; session 10 (A1, layout in MIR) is
> output-invariant: probe C 102 zero loops / 11,550 lines / **504,054 B** (bytes
> jitter ±2 B between runs) and main-suite C **1,231 / 107,191 lines** either
> side of it, `--tcc` 2300/2300 and
> `--llvm` 2298/2298. A1 is the prerequisite for A2/A3 and B, not a metric win —
> the gates above still need C (the remaining zeroing class), D (per-instruction
> cost) and A2/A3+B (declaration/layout single source, thin backends).

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
