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
>
> **Status update (2026-10-18, session 54) — B's last architectural item:
> shared traversal, operand decode and naming.** The walk and the lookups were
> still copied per backend: `operand_at` existed in `MIREmitter.cpp`,
> `MIRFacts.cpp` and as a lambda in `CodegenMir`, the block instruction-range
> arithmetic was open-coded in `CodegenMir`, and the LLVM emitter hand-rolled
> the same symbol-name and string-constant reads six more times. New AST-free,
> target-free core header `compiler/mir/MIRTraversal.h` provides `operand_at`,
> `MIRBlockRange`/`block_range`, `for_each_block(_inst)`, `symbol_name_view` and
> `constant_string_view`; both emitters, `MIRFacts`, `MIRInterpreter` and
> `MIRDump` consume them. `MIRFacts.h` also gains `MIRValueFacts`
> (`compute_value_facts`: `def_opcode`/`single_use`/`single_use_pure`), the
> compaction classification the C emitter now reads instead of poking
> `MIRValueDef::use_count`. **Output-invariant** (name == mangled in the
> lowerer; `data.text` == the `(data_at, data_count)` payload; same block clamp;
> `single_use == (use_count == 1)`). `MIRTests` **4462 → 4489**;
> `--llvm` **2305/2305**, `--tcc` **2307/2307**, `--tcc --interpret`
> **1896/1896**. No headline gate moves. Details: `compiler/mir/README.md`,
> session 54.
>
> **Status update (2026-10-18, session 53) — the aggregate access layout is
> shared.** A backend that builds an aggregate type itself (LLVM) needs an
> ordered element list — one element per padding run and one shared element per
> union/variant overlap group — not just the size/offset numbers. That list is a
> pure function of the type table, so it moved out of `CodegenMir::mir_type` into
> the core: `mir::compute_aggregate_plan` / `MIRAggregatePlan` / `MIRAggElement`
> (`compiler/mir/MIRLayout.h`). `Codegen` now stores the plan (`mir_agg_plans_`)
> and its GEP/constant paths read it; `CodegenMir::mir_type` shrank to a loop
> over the plan. **Output-invariant by construction** (same element sequence:
> field *i* → element *i* with no overlap/alignment; union/variant pick the same
> largest-aligned member). `MIRTests` **4417 → 4462** (new
> `test_aggregate_plan`); `--llvm` **2305/2305**, `--tcc` **2307/2307**,
> `--tcc --interpret` **1896/1896**. No headline gate moves. Details:
> `compiler/mir/README.md`, session 53.
>
> **Status update (2026-10-14, session 15) — what this session closed in the
> plan.** **A2 is done** (there is one C type speller — see §1.4 and §2A.2; the
> "implemented twice" claim was stale), and **B has its first deliverable**
> (`compiler/mir/MIRABI.h`: the parameter/return classification that was written
> out four times is now one shared, AST-free, target-free definition — §2B step
> 1). Also fixed a hot-path rule violation: `CodegenMir` ran a `getenv` per
> emitted MIR instruction (§2D6). Nothing in this session touched the two
> headline gates — **compile time (D/E)** and **generated code size (C)** — whose
> remaining work is unchanged: the `Type(args)` constructor temp and the
> `apply_struct_defaults` zero-inits, splitting the 8,537-line `MIRLowerer.cpp`
> (D4), the 18 `unordered_map`s (D3) and parallel file lowering (E), which is the
> only lever sized to the 2.3× compile-time gap.
>
> **Status update (2026-10-15, session 27) — D4 first step: `MIRLowerer.cpp`
> split.** The async group is now `MIRLowerAsync.cpp`, lambda/capturing
> `MIRLowerLambda.cpp`, and the const-expression builder `MIRLowerConst.cpp`
> (shared `MIRLowererInternal.h`); `MIRLowerer.cpp` is **9,026 → 7,014 lines**.
> Pure move — identical emitted C (96,915 lines, 194 `memset` sites); `--tcc`
> **2305/2305**, `--llvm` **2303/2303**. Enabler work for the compile-time gate
> (the gate still needs D3 / per-instruction / E), and **the gate is not
> measurable on this Windows checkout** (identical cold runs: 15.6/24.4/23.5 s).
> Details: `compiler/mir/README.md`, session 27.
>
> **Status update (2026-10-16, session 33) — D4 done: `MIRLowerer.cpp` split.**
> The remaining 7,090-line TU is now the core `MIRLowerer.cpp` (867) plus
> `MIRLowerExpr.cpp` (1,858), `MIRLowerStmt.cpp` (2,176), `MIRLowerCall.cpp`
> (1,523) and `MIRLowerAggregate.cpp` (473), with the shared AST-aware free
> helpers in a new `MIRLowererHelpers.h` (`inline`). Pure move, output-invariant
> (emitted C 96,803 lines either side; normalized diff **52** against a **62-line**
> same-binary floor); `MIRTests` **4400**, `--tcc` **2305/2305**, `--llvm`
> **2303/2303**, `--tcc --libs` **718/719**. No gate moves — reviewability and the
> compiler's own incremental build. Details: `compiler/mir/README.md`, session 33.
>
> **Status update (2026-10-15, session 26) — multi-statement constructors no
> longer zero the result temp.** `constructor_writes_whole_value` gained the
> `... <setup>; return T{...}` and `... var m = T{...}; ...; return m` shapes
> (ordered_map / unordered_map / mutex / CondVar). Main-suite `memset` sites
> **424 → 194 (−54%)** across sessions 25–26, generated C **97,144 → 96,915**
> lines, `tests-tcc.exe` **3,665,077 → 3,654,837 B**; output differs only by the
> removed `memset`s; probes print identical correct output on both backends;
> `--tcc` **2305/2305**, `--llvm` **2303/2303**, `--tcc/--llvm --libs`
> **718/719**. Details: `compiler/mir/README.md`, session 26.
>
> **Status update (2026-10-15, session 25) — generated code: two dead
> `zero_init`s removed.** A full array literal and a `dyn<T>` fat pointer no
> longer zero a temporary they fully write. Main-suite `memset` sites
> **424 → 233 (−45%)**, generated C **97,144 → 96,953** lines, `tests-tcc.exe`
> **−8.7 KB**; the C output differs only by the removed `memset`s; `--tcc`
> **2305/2305**, `--llvm` **2303/2303**, `--tcc/--llvm --libs` **718/719**.
> Details: `compiler/mir/README.md`, session 25.
>
> **Status update (2026-10-15, session 24) — the C declare pass prints from a
> declaration list.** `compiler/mir/MIRDeclList.h` (AST-free item kinds) +
> `MIRDeclPrinter::decl_list_c` (MIR core, `MIRTests`-covered) replace the
> declare pass's inline printing; `MIRDeclEmitter` records, `declare_module`
> prints once. **Output-invariant** by the normalized line multiset (46/60 vs a
> 56-line same-binary floor — the C reorders generics run-to-run, so the multiset
> is the invariant); `--tcc` **2305/2305**, `--llvm` **2303/2303`,
> `--tcc/--llvm --libs` **718/719**, `MIRTests` **1170 → 1180**. Details:
> `compiler/mir/README.md`, session 24.
>
> **Status update (2026-10-15, session 23) — one global-declaration spelling.**
> `compiler/mir/MIRGlobalDecl.{h,cpp}` (`MIRGlobalDecl` +
> `build_global_declaration`) is the single spelling of a global's
> name/declared-type (incl. unsized `T[]`)/flags, consumed by `declare_var` and
> `implement_global`; `MIRGlobal::flags` shares the enum. **Output-invariant**:
> C 97,144 lines either side, normalized diff 50 against a 46-line noise floor;
> `--tcc` **2305/2305**, `--llvm` **2303/2303**, `--tcc/--llvm --libs`
> **718/719**, `MIRTests` **1170 checks**. Details: `compiler/mir/README.md`,
> session 23.
>
> **Status update (2026-10-15, session 22) — one function-prototype ABI.**
> `compiler/mir/MIRPrototype.{h,cpp}` (`MIRPrototype` + `mir_function_signature`
> + `build_function_prototype`) is now the single implementation of the prototype
> ABI, consumed by the C emitter and by `MIRLowerer::intern_function`
> (`MIRDeclEmitter::function_type_of` deleted). **Output-invariant**: C 97,144
> lines either side, previous-step → this-step normalized diff 50 against a
> 46-line noise floor; `--tcc` **2305/2305**, `--llvm` **2303/2303**,
> `--tcc/--llvm --libs` **718/719**, `MIRTests` **1170 checks**. Details:
> `compiler/mir/README.md`, session 22.
>
> **Status update (2026-10-15, session 21) — vtable declarations are MIR
> records; the LLVM driver no longer reads the AST.** `MIRModule::vtables`
> (`MIRVtableTable`) now holds a type record per interface and a definition
> record per impl (ordered slots, self-erased types, resolved callee names),
> built by `mir::build_module_vtables` in `implement_module`.
> `mir_emit_llvm_module` consumes them and its interface/impl AST walk is
> deleted — the last place a backend read the AST. **Output-invariant**: C is
> 97,144 lines either side, previous-step → this-step normalized diff 66 against
> a 46-line noise floor (no vtable line differs); `--tcc` **2305/2305**,
> `--llvm` **2303/2303**, `--tcc/--llvm --libs` **718/719**, `MIRTests`
> **1170 checks**. Details: `compiler/mir/README.md`, session 21.
>
> **Status update (2026-10-15, session 20) — B's third step: the vtable layout is
> shared.** The interface vtable slot order and self-erased slot types were the
> last algorithm both backends derived independently; they now come from
> `compiler/mir/MIRVtable.{h,cpp}` (`mir_vtable_slots`), consumed by
> `MIRDeclEmitter` (C) and `mir_emit_llvm_module` (LLVM). **Output-invariant**:
> main-suite C 97,144 lines either side, A3→A3+B normalized diff 60 against a
> 46-line same-binary noise floor (no vtable line differs); `--tcc` **2305/2305**,
> `--llvm` **2303/2303**, `--tcc/--llvm --libs` **718/719**, `MIRTests`
> **1170 checks**. No headline gate moves; still open in B: shared traversal/
> naming and the vtable *declaration* records. Details: `compiler/mir/README.md`,
> session 20.
>
> **Status update (2026-10-15, session 19) — A3's second bite: global
> initializers are MIR.** The initializer walk that lived in *both* backends
> (`MIRDeclEmitter::const_value_expr` for C, `GlobalConstantLowerer` for LLVM,
> ~370 lines) is replaced by one AST-free constant-expression tree
> (`compiler/mir/MIRConstExpr.h`) built once by `MIRLowerer::build_global_initializer`
> and recorded in `MIRModule::globals`; C prints it (`MIRDeclPrinter::const_expr_c`)
> and LLVM builds its `Constant` from it (`Codegen::mir_apply_global_initializers`).
> **Output-invariant**: main-suite C is 97,144 lines either side, normalized diff
> 42/52 against a **46-line** same-binary noise floor; `--tcc` **2305/2305**,
> `--llvm` **2303/2303**, `--tcc/--llvm --libs` **718/719**, `MIRTests`
> **1154 → 1170**. No headline gate moves (this is architecture, not C/D/E); the
> remaining A3 work is the collection walk and the `MIRTypeDecl`/`MIRPrototype`
> records. Details: `compiler/mir/README.md`, session 19.
>
> **Status update (2026-10-15, session 18) — architecture, not a gate: A3's
> first bite and B's second step landed.** The C declaration path no longer walks
> the AST to spell an aggregate body: `compiler/mir/MIRDeclPrinter` prints
> structs/unions/variants (inherited bases, `__chx_align_struct(N)`,
> `__chx_align(N)`/`volatile` members, the variant tag + inline case payloads,
> and the inline anonymous-member form) from MIR alone; `MIRFieldInfo` grew the
> spelling facts it needs (`flags`) and `MIRTypeRecord` the explicit alignment
> (a layout *input*), so there is nothing left for the emitter to ask the AST
> about a field; `MIRDeclEmitter::emit_member`/`emit_field` are deleted and the
> driver prints synthesized inline structs from MIR too. Separately,
> `compiler/mir/MIRFacts` (`compute_function_facts`) now computes the
> per-function analysis `emit_function_c` used to run inline — block
> offsets/labels/fall-throughs, place write counts, `place_addressed`,
> `param_alias` — ≈150 lines of analysis replaced by ≈10 lines of reads.
> **Output-invariant by measurement**: main-suite C is 97,144 lines before and
> after; the normalized C diff is 96 (A3) / 111 (A3+B) lines against a 102-line
> *same-binary two-run* noise floor (all residual lines are generic-instantiation
> jitter); `--tcc` **2305/2305**; `MIRTests` **1115 → 1154 checks**. A shape
> probe (inheritance, `@align`, `@volatile`, anonymous unions, variant payloads;
> `lang/compiled/mir_a3_probe`, gitignored) builds and runs with the expected
> output on both backends. Suite results with the rebuilt compilers: `--tcc`
> **2305/2305**, `--llvm` **2303/2303**, `--tcc --libs` **718/719** (the known
> environment-sensitive `test_fs_directory_iteration`), `MIRTests` **1154
> checks**. **No
> headline gate moves** — the two below are still the remaining zeroing classes
> (C) and D/E (compile time / RSS), and the line gate is still missed. One
> environment caveat: on this Windows machine the `--llvm` test-exe build
> aborted twice with a JIT `RUNTIME ERROR: invalid memory access` (exit 255) —
> once with the binary captured **before** this change, once with the A3-only
> one — before a later run completed at 2303/2303. The LLVM job skips C emission
> entirely, so that flake is not caused by this work. Details:
> `compiler/mir/README.md`, session 18.
>
> **Status update (2026-10-15, session 17) — the first headline-gate bite since
> this document was written.** Action C1's named remainder is done where it is
> provable: a `T(args)` result temporary (and the `apply_struct_defaults`
> base/member temporaries) is no longer zeroed when the construction provably
> writes the whole value — a constructor whose body is `{ return T { ... } }`,
> or a generated/recursive default initialization covering every field. Measured
> on this checkout: main-suite C zero loops **539 → 440 (−18%)**, probe zero
> loops **37 → 25**, probe exe **444,223 → 442,993 B**, `tests-tcc.exe`
> **4,248,688 → 4,238,513 B**; suites green (`--tcc` **2305/2305**,
> `--llvm` **2303/2303**, `--tcc --async` **53/53**, `--tcc --interpret`
> **1895/1895**, `MIRTests` **1115 checks**). Details and the residual (a
> multi-statement constructor body, a temporary filled through a call) are in the
> MIR README, session 17. The line gate is still missed and the compile-time/RSS
> gates are untouched (D/E remain the levers).

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
>
> **Status update (2026-10-13, session 11) — the dominant remaining zeroing class
> is gone.** Variant construction (`Variant.Member(args)`) unconditionally
> `zero_init`'d its result temporary. The only bytes ever read for a variant case
> are the discriminator and that case's payload, so the zero is dead once every
> payload field is written; it is now emitted only for an omitted-without-default
> payload parameter, a partial (array) payload, or an inherited base. Measured,
> same source, same mode, clean cache:
>
> | Metric | Session-9/10 | Now |
> |---|---:|---:|
> | Probe (`perfprobe`) zero loops | 93 | **37** (−60%) |
> | Probe generated C lines | 11,779 | **11,723** |
> | Probe executable | 454,718 B | **448,640 B** |
> | Main-suite C zero loops | 1,231 | **539** (−56%) |
> | Main-suite generated C lines | 107,191 | **106,373** |
>
> Suites: `--tcc` **2300/2300**, `--llvm` **2298/2298**, `--tcc --async` **53/53**,
> `--tcc --libs` **718/719**, `MIRTests` **1111 checks** (a Linux-only
> `MIR_TEST_ALIGNED` placement bug that made one layout check fail on GCC was also
> fixed). This is C1's largest remaining bite; what is left of C1 is the
> constructor-result temporary (`Type(args)`) and the `apply_struct_defaults`
> `zero_init`s, which need a "constructor covers every field" proof. An attempt to
> unify `apply_struct_defaults` onto the in-place `apply_struct_defaults_ptr` was
> reverted — it grew generated C without removing any targeted zero. **That proof
> landed in session 17** (see §2C.1 and the MIR README, session 17): where the
> constructor's body is `{ return T { ... } }`, or the default initialization
> provably covers every field, the zero is gone — main-suite zero loops
> **539 → 440**, probe zero loops **37 → 25**.

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
   trivial if/else). **RESOLVED — action C4 (sessions 8 and 16):** an
   unreferenced label is no longer emitted (session 8), and a branch to the
   block physically next to it is a fall-through and prints nothing — a `Br`
   emits no statement, a `CondBr` with one falling-through arm becomes a
   single-arm `if`. Measured interleaved A/B on the main suite: standalone
   `goto`s 5,723 → **1,738**, fall-through jumps 3,985 → **0**, `if/else goto`
   4,501 → **0**, C lines 106,373 → **96,909**, `tests-tcc.exe` 4,292,216 →
   **4,248,581 B**; probe 11,723 → **10,743 lines** / 448,625 → **444,239 B**.

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
§1.4). `MIRLowerer.cpp` was a single **8,224-line** translation unit (the plan
specified `MIRLowerExpr/Stmt/Call/Lambda`); `lower_expr` alone is 1,745 lines and
`lower_stmt` 1,072. This slowed the compiler's own build and made review hard.
**Split (session 33):** `MIRLowerer.cpp` is now **867 lines**, with
`MIRLowerExpr.cpp` (1,858), `MIRLowerStmt.cpp` (2,176), `MIRLowerCall.cpp`
(1,523) and `MIRLowerAggregate.cpp` (473) beside the already-split
async/lambda/const TUs.

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
- **C type spelling is implemented twice.** ~~`MIREmitter.cpp` (`c_type_of`,
  `c_decl_type`, `declarator`, `c_function_proto`) and `MIRDeclEmitter.cpp`
  (`type_spelling`, `decl_spelling`) independently spell the same types and must
  be kept in sync by hand (see README gotcha "type spelling must match").~~
  **RESOLVED — action A2.** There is now exactly one C type speller:
  `MIREmitter`'s `c_type_of`/`c_declarator`/`c_decl_type`/`c_function_proto`.
  `MIRDeclEmitter::type_spelling`/`decl_spelling` are thin wrappers that map an
  AST `BaseType*` to a `TypeId` and delegate (the only AST input left on that
  path). The remaining A item is A3 (the declaration table), not the printer.
- **Each backend is a full independent instruction dispatcher.** `MIREmitter`
  (1,601 lines, ~80 opcode cases) and `CodegenMir` (2,249 lines, ~49 cases) share
  no MIR traversal, ABI, naming, or layout helper. Adding JVM and JS as two more
  full dispatchers is exactly the per-backend code explosion to avoid.
  **PARTLY ADDRESSED — action B, first step (session 15):** the ABI
  classification is no longer re-derived per backend. `compiler/mir/MIRABI.h`
  defines it once (`abi::is_aggregate`, `abi::needs_sret`,
  `abi::is_struct_like`, `abi::is_pointer_like`, `abi::is_void`) and
  `MIRLowerer`, `MIRDeclEmitter` and `CodegenMir` all call it — four hand-synced
  copies became one definition, and the "must match *exactly*" comments that
  guarded them are gone. Shared **traversal/operand-decode/naming services** are
  now in place too (`compiler/mir/MIRTraversal.h`, action B step 6, session 54),
  along with the per-function (`MIRFacts`) and per-value (`MIRValueFacts`) facts;
  the residual is the AST the C declaration collection pass still sees.

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
- **`MIRLowerer.cpp` was one 8,224-line TU** (build-time and review cost).
  **Split (session 33):** core is 867 lines; the per-construct TUs are listed in
  §1.3. Remaining per-instruction / low-value work in D is unchanged.
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
2. ~~Add a single `mir::TypePrinter` (or `MIRTypeFormat`) in MIR core that
   renders a `TypeId` + `declarator` for the **C** backend from MIR only; delete
   the duplicate `MIRDeclEmitter::type_spelling`/`decl_spelling` in favour of
   it.~~ **DONE (verified session 15).** The single speller is
   `MIREmitter`'s (`c_type_of`/`c_declarator`/`c_decl_type`/`c_function_proto`);
   `MIRDeclEmitter` keeps `type_spelling`/`decl_spelling` only as the two
   `BaseType*` → `TypeId` adapters that call it. No second spelling exists. (The
   speller lives in `MIREmitter.cpp` rather than a separate `MIRTypeFormat` TU;
   moving it is cosmetic and is left to whenever that file is next touched.)
3. Replace `MIRDeclEmitter`'s AST walk with a MIR declaration table
   (`MIRTypeDecl`/`MIRPrototype`/`MIRGlobal`, already sketched in the plan §2.5)
   populated by the lowerer, so declarations are emitted from MIR for every
   backend. **PARTLY DONE (session 18).** Aggregate *bodies* no longer walk the
   AST: `compiler/mir/MIRDeclPrinter.cpp` (`aggregate_body_c`,
   `inline_aggregate_c`, `member_declarator_c`) spells a struct/union/variant
   from `MIRTypeRecord`/`MIRFieldInfo` alone, `MIRDeclEmitter`'s three
   `declare_*_def_only` call it, `emit_member`/`emit_field` are deleted, and the
   field facts it needed are now recorded by `MIRTypeBuilder`
   (`MIRFieldInfo::flags`, `MIRTypeRecord::explicit_alignment`). Still on the
   AST: the collection walk, prototypes, globals + constant initializers
   (`const_value_expr`), typedefs, interface vtables/static stubs. **Second bite
   DONE (session 19): `MIRGlobal` + constant-expression tree.** A top-level
   global's initializer is now a target-neutral `MIRConstExpr` tree
   (`compiler/mir/MIRConstExpr.h`) built once by the AST-aware lowering boundary
   (`MIRLowerer::build_global_initializer`) and recorded in `MIRModule::globals`;
   the C backend prints it with `MIRDeclPrinter::const_expr_c` and the LLVM
   backend builds its `Constant` with `Codegen::mir_apply_global_initializers`.
   Both AST walkers — `MIRDeclEmitter::const_value_expr`/`struct_default_expr`
   and `GlobalConstantLowerer` (~370 lines) — are **deleted**. Measured
   output-invariant (main-suite C 97,144 lines either side; normalized diff
   42/52 against a 46-line same-binary noise floor); `--tcc` **2305/2305**,
   `--llvm` **2303/2303**, `--tcc/--llvm --libs` **718/719**, `MIRTests`
   **1154 → 1170**. **Third bite DONE (session 21): interface vtables are MIR
   records.** `MIRModule::vtables` (`MIRVtableTable`) holds a type record per
   interface and a definition record per impl, with resolved callee names, built
   by `mir::build_module_vtables` in `implement_module` — so
   `mir_emit_llvm_module` no longer walks the AST at all (see item B step 4).
   The C declaration pass still uses `mir_vtable_slots`; the C-side vtable
   *declarations*, prototypes, typedefs, the globals' declaration flags, the
   collection walk and the general `MIRTypeDecl`/`MIRPrototype` records are still
   the next step. **Fourth bite DONE (session 22): one function-prototype ABI.**
   `compiler/mir/MIRPrototype.{h,cpp}` (`MIRPrototype`,
   `mir_function_signature`, `build_function_prototype`) is now the single
   implementation of the prototype ABI, consumed by the C emitter
   (`declare_contained_func`/`function_proto`) and by
   `MIRLowerer::intern_function` (both used to carry a "must match" copy;
   `MIRDeclEmitter::function_type_of` is deleted). Measured output-invariant
   (97,144 lines either side; 50-line normalized diff against a 46-line noise
   floor). Remaining on the AST: typedefs, the globals' declaration flags and the
   declaration *ordering* (the `MIRTypeDecl` records and a declaration-list walk).
   **Fifth bite DONE (session 23): one global-declaration spelling.**
   `compiler/mir/MIRGlobalDecl.{h,cpp}` (`MIRGlobalDecl`,
   `build_global_declaration`) is now the single spelling of a global's
   name/declared-type (incl. unsized `T[]`)/flags, consumed by both `declare_var`
   and `implement_global`; `MIRGlobal::flags` shares the same enum. Output
   invariant (97,144 lines either side, 50-line diff against a 46-line floor).
   Remaining on the AST: typedefs (already MIR-spelled via `decl_spelling`) and
   the declaration *ordering* (the `MIRTypeDecl` records and a declaration-list
   walk). **Sixth bite DONE (session 24): the C declare pass prints from a
   declaration list.** `compiler/mir/MIRDeclList.h` (AST-free item kinds) +
   `MIRDeclPrinter::decl_list_c` (MIR core, `MIRTests`-covered) replace the
   declare-pass AST walk's inline printing; `MIRDeclEmitter` records and
   `ASTProcessor::declare_module` prints once. Output-invariant by the normalized
   line multiset (46/60 vs a 56-line same-binary floor — the C reorders generics
   run-to-run); `MIRTests` **1170 → 1180**. Remaining: the collection walk itself
   (the allowed front-end pass) and the implement pass's interleaved definitions.
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

**Step 2 DONE (session 18) — `compiler/mir/MIRFacts.h`.** The per-function
facts a backend needs before it can spill a place, alias a parameter or place a
label are now MIR's: `compute_function_facts(module, fn, out)` fills
`block_start`/`block_starts_inst`/`block_referenced`/`label_at_inst`
(+ `is_fallthrough_inst`), `place_writes`, `place_addressed`,
`param_index_at_inst` and `param_alias`. `emit_function_c` reads them instead of
running the ≈150-line analysis inline (the analysis was transcribed exactly, not
re-derived, so the C output is unchanged — 97,144 lines either side, normalized
diff within the 102-line same-binary noise floor). `MIRTests` gained two
functions' worth of assertions (1154 checks). **Still open in B:** shared
traversal and place/symbol/constant naming.

**Step 3 DONE (session 20) — `compiler/mir/MIRVtable.{h,cpp}`.** The interface
vtable slot **order** and each slot's self-erased function **type** — the last
algorithm the two backends derived independently (`MIRDeclEmitter`'s
`vtable_slot_type` + recursive walk, `mir_emit_llvm_module`'s
`slots_of`/`slot_ftype`) — are now computed once by
`mir::mir_vtable_slots(types, module, iface, out, out_funcs)` /
`mir::mir_vtable_slot_type(...)` and consumed by both. `MIRVtableSlot` is the
shared record. The slot *spelling* (C `c_decl_type` vs LLVM `mir_fn_type`) and
the callee resolution stay backend-side. Measured output-invariant for C
(main-suite C 97,144 lines either side, A3→A3+B normalized diff 60 against a
46-line same-binary noise floor; no vtable line differs); `--tcc` **2305/2305**,
`--llvm` **2303/2303**, `--tcc/--llvm --libs` **718/719**, `MIRTests`
**1170 checks**. It also aligns a latent LLVM/C divergence (the old LLVM
`slot_ftype` used a local `is_agg` that excluded typedef structs and arrays from
sret). Still open in B: shared traversal/naming and the vtable *declaration*
records (so the LLVM driver stops walking the AST for interfaces/impls), plus
the primitive-impl slot alignment noted in the MIR README. **The primitive-impl
alignment is now FIXED (session 36):** `create_vtable_primitive` iterates the
shared `mir_vtable_slots` (inherited-then-own, every method) and emits a null
entry for a non-self/`no-overrider` slot, so the primitive vtable definition
matches its type; regression tests in `primitive_impl.ch` (`--tcc` **2307/2307**,
`--interpret` **1897/1897**, `--llvm` **2305/2305**). Still open in B: shared
traversal/naming.

**Step 4 DONE (session 21) — `MIRVtableTable.h` + `build_module_vtables`.** The
interface/impl vtable *declarations* are now MIR records: `MIRModule::vtables`
(`MIRVtableTable`) holds a type record per interface and a definition record per
(interface, impl), each with its ordered slots, self-erased types and resolved
callee mangled names. They are built by
`mir::build_module_vtables(mangler, types, module, nodes)` in
`ASTProcessor::implement_module`, so **`mir_emit_llvm_module` no longer walks the
AST at all** — the last place a backend read it. The C declaration pass keeps
using `mir_vtable_slots` (it is the allowed collection pass). Measured
output-invariant for C (97,144 lines either side; previous-step → this-step
normalized diff 66 against a 46-line noise floor, no vtable line differs);
`--tcc` **2305/2305**, `--llvm` **2303/2303**, `--tcc/--llvm --libs`
**718/719**, `MIRTests` **1170 checks**. Still open in B: shared traversal/
naming.

**Step 1 DONE (session 15) — `compiler/mir/MIRABI.h`.** The parameter/return
classification is now shared rather than re-derived: the same rules had been
written out **nine times across three translation units** (three in
`MIRLowerer.cpp` — `needs_aggregate_path`, `needs_sret_return`, `is_struct_like`;
two in `MIRDeclEmitter.cpp` — `needs_aggregate`, `needs_sret`; four in
`CodegenMir.cpp` — `mir_is_aggregate`, `mir_needs_sret`, `mir_is_ptr_like`,
`mir_is_void`), several carrying a comment saying they had to match the others
*exactly*. They are now `mir::abi::{is_valid,
is_void, is_struct_like, needs_sret, is_aggregate, is_pointer_like}` — pure
functions of the interned type table, AST-free and target-free, which is what a
JVM/JS backend needs. Measured output-invariant: probe C unchanged at 11,723
lines / 37 zero loops / 1,331 labels / 25 spills, probe exe 448,622 B (±2 B
forward-declaration jitter), identical program output; `MIRTests` 1111,
`--tcc` 2300/2300, `--llvm` 2298/2298. **Still open in B:** shared traversal,
naming, block iteration and `MIRValueFacts`.
**Step 5 DONE (session 53) — `mir::compute_aggregate_plan`.** The ordered
element list a type-building backend indexes with GEP — one element per padding
run, one shared element per union/variant overlap group, plus the field → element
map — was derived inline in `CodegenMir::mir_type` and kept in two ad-hoc
`Codegen` maps. It is a pure function of the type table (identical for every
such backend), so it now lives in the AST-free core next to
`compute_aggregate_layout` (`MIRLayout.h`): `MIRAggregatePlan` / `MIRAggElement`
/ `compute_aggregate_plan(types, agg, out)`. `Codegen` keeps the plan per
aggregate (`mir_agg_plans_`) for `mir_field_elem` and the constant builder; the
LLVM `mir_type` aggregate case is now a loop over `plan.elements`.
Output-invariant by construction; `MIRTests` gained `test_aggregate_plan`
(4417 → **4462**); `--llvm` **2305/2305**, `--tcc` **2307/2307**,
`--tcc --interpret` **1896/1896**. Still open in B: shared traversal and
place/symbol/constant naming.
**Step 6 DONE (session 54) — `compiler/mir/MIRTraversal.h` + `MIRValueFacts`.**
The walk and the lookups are now core. `MIRTraversal.h` (AST-free, target-free,
inline) provides `operand_at` (the copy in `MIREmitter.cpp`, the copy in
`MIRFacts.cpp` and `CodegenMir`'s `arg` lambda collapse into it),
`MIRBlockRange`/`block_range` (the block instruction-range arithmetic), the
`for_each_block`/`for_each_block_inst` idiom (`MIRDump` consumes it),
`symbol_name_view` and `constant_string_view` (the symbol/constant reads
`MIREmitter`, `CodegenMir` and `MIRInterpreter` each spelled by hand).
`MIRFacts.h` gains `MIRValueFacts` — `def_opcode`/`single_use`/`single_use_pure`
for the compaction classification — computed by `compute_value_facts` and read
by the C emitter's inline pass instead of `MIRValueDef::use_count` directly.
Output-invariant by construction; `MIRTests` **4462 → 4489** (`test_value_facts`,
`test_traversal_helpers`); `--llvm` **2305/2305**, `--tcc` **2307/2307**,
`--tcc --interpret` **1896/1896**. **B's remaining item is the AST the C
declaration collection pass still sees** (deliberately allowed).
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
   `MIRTests` **1052 checks OK**. **Third bite DONE (session 11):** variant
   construction no longer zeroes a temporary whose active case writes its whole
   payload (only an omitted-without-default param, a partial array payload, or an
   inherited base keeps the zero). Main-suite C zero loops **1,231 → 539 (−56%)**,
   main-suite C lines **107,191 → 106,373**, probe zero loops **93 → 37**, probe
   exe **454,718 → 448,640 B**; `--tcc` **2300/2300**, `--llvm` **2298/2298**,
   `--tcc --async` **53/53**, `MIRTests` **1111 checks OK**. **Fourth bite DONE
   (session 17):** the constructor-result temp (`Type(args)`) and the
   `apply_struct_defaults` base/member temporaries are no longer zeroed when the
   construction provably writes the whole value — a constructor whose body is
   `{ return T { ... } }` (its result is a whole-value store of a fully lowered
   literal temporary) or a generated/recursive default initialization that
   covers every field (`MIRLowerer::constructor_writes_whole_value` /
   `default_init_writes_all_fields`). Main-suite C zero loops **539 → 440
   (−18%)**, main-suite C lines **96,909 → 96,810**, probe zero loops **37 → 25**,
   probe exe **444,223 → 442,993 B**, `tests-tcc.exe` **4,248,688 → 4,238,513 B**;
   `--tcc` **2305/2305** (5 new tests in
   `lang/tests/common/src/values/ctor_coverage.ch`), `--llvm` **2303/2303**,
   `--tcc --async` **53/53**, `--tcc --interpret` **1895/1895**, `MIRTests`
   **1115 checks OK**. Still open in this class: a constructor whose body is not
   a single `return T { ... }` (e.g. `{ var m = T { ... }; <call>; return m }`,
   as in `std::mutex.constructor` / `std::CondVar.constructor`), a temporary
   whose address is filled by a call, and default-initializations that are not
   provably complete; the per-constructor details are in the MIR README,
   session 17. **Fifth bite DONE (session 25): a full array literal and a
   `dyn<T>` fat pointer no longer zero a temporary they fully write.** Main-suite
   `memset` sites **424 → 233 (−45%)**, generated C **97,144 → 96,953** lines,
   `tests-tcc.exe` **−8.7 KB**; output differs only by the removed `memset`s (plus
   the usual generic-ordering jitter); `--tcc` **2305/2305**, `--llvm`
   **2303/2303**, `--tcc/--llvm --libs` **718/719**. **Sixth bite DONE (session
   26): multi-statement constructors no longer zero the result temp.**
   `constructor_writes_whole_value` now also proves `... <setup>; return T{...}`
   (only a `return` writes the hidden sret) and `... var m = T{...}; ...; return m`
   — the `std::ordered_map`/`unordered_map`/`mutex`/`CondVar` shapes. Main-suite
   `memset` sites **424 → 194 (−54%)**, C **97,144 → 96,915** lines,
   `tests-tcc.exe` **3,665,077 → 3,654,837 B**; a probe (multi-statement
   `@make`, `var m = T{...}; return m`, `std::mutex` lock/unlock) prints
   identical correct output on both backends. Remaining in C1 is the deliberately
   conservative residue (another ctor shape, call-filled temporaries, partial
   array literals, `zeroed<T>()`, the string-into-`char[]` and union tails).
   **Seventh bite DONE (session 34): an empty array literal for a fixed-size
   array field no longer double-writes the field.** The struct-literal lowering
   skips the field store when the initializer is an empty array literal and the
   enclosing temp is `zero_init`'d (`buffer : []` in `std::default::function_instance`).
   Main-suite `memset` sites **212 → 200**, C **96,803 → 96,767** lines; the
   normalized line multiset differs only by the removed sites (12 `memset` +
   12 `memcpy` + scratch allocas) plus jitter, against a 62-line floor.
   `MIRTests` 4400, `--tcc` 2305/2305, `--llvm` 2303/2303, `--libs` 718/719. The
   *non-empty* partial array literal still builds a full-width scratch (next bite).
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
4. ~~Emit no `goto`/block label for straight-line statements~~ **DONE (sessions 8
   and 16)**. Session 8: a label is emitted only for a block some `Block`
   operand jumps to — suite labels 17,216 → **11,660** (−32%), C lines
   114,280 → **108,475**. Session 16 (the *branch* half): a branch whose target
   starts at the very next instruction is a fall-through and prints nothing — a
   `Br` emits no statement at all, and a `CondBr` with one falling-through arm
   becomes a *single-arm* `if` (negating the condition when the true arm falls
   through). The referenced-label set is computed from the non-fall-through
   operands, so the two halves compound. Measured interleaved A/B on the same
   machine and mode, `TCCCompiler` built from this tree both ways:
   main-suite standalone `goto`s **5,723 → 1,738**, pure fall-through jumps
   **3,985 → 0**, `if/else goto` with a falling-through arm **4,501 → 0** (of an
   unchanged 4,611 `CondBr`s), `goto` tokens **14,945 → 6,459**, C lines
   **106,373 → 96,909** (−8.9%), `tests-tcc.exe` **4,292,216 → 4,248,581 B**,
   probe C **11,723 → 10,743** lines and probe exe **448,625 → 444,239 B**,
   suite compile 3.03 s → **2.98 s** (median of 3, ranges 3.01–3.03 and
   2.96–2.98). This is a *runtime* item too: the C backend
   has no optimizer, so the jump was compiled into the object file and executed
   every iteration in a loop body. Runtime ≥ parity — `benchprobe` 0.60 s →
   0.59–0.60 s, `bcryptbench` cost-12 median **9.72 s → 9.42 s**, both with
   identical output; `--tcc` **2300/2300**, `--llvm` **2298/2298**, `MIRTests`
   **1115 checks**. Invariance: with label/`goto` lines filtered out and the
   pre-existing `__cgs__N`/declaration-order jitter normalised, before/after C
   differs by **4 lines** over the whole suite (a same-compiler two-run baseline
   differs by 6).
**Acceptance.** Probe C lines and executable text ≤ legacy on `perfprobe`; no
zero-loop on an alloca that is fully initialized field-wise; runtime ≥ parity on
the `benchprobe` loop and the bcrypt cost-12 benchmark.
**Status (session 8).** Probe C lines 14,115 → **11,868** and text 486,964 →
**407,577** B since the evaluation, against legacy 8,285 lines / 285,299 B: the
C1+C3+C2+C4 fixes closed ~40% of the line gap and ~45% of the text gap without
touching runtime-relevant zeroing; the acceptance is not met yet.

**Status (session 16).** Probe C **10,743 lines / 444,239 B** and suite C
**96,909 lines** with `tests-tcc.exe` **4,248,581 B** — against legacy 8,285
lines / 285,299 B the probe line gap is now ~57% closed (was ~40% after session
8). Runtime half of the acceptance is **met**: `benchprobe` 0.60 → 0.59–0.60 s
and `bcryptbench` cost-12 median 9.72 → **9.42 s**, both identical output, with
no zero-loop change (session 16 only touched branch and label emission). The
size half is still missed: what is left in C is the two zeroing classes named
against item 1 above.

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
   **Measured and rejected (session 29):** with the new `-DMIR_OPS_COUNTERS`
   tool, all eighteen maps together are ~0.4 M operations on a 296,017-instruction
   main-suite lowering (~1% of the 2.9–3.2 s phase, ~0.3% of the cold build).
   Dense tables cannot move the gate; the cost is per-function/per-instruction C++
   overhead in the `Debug` compiler. See `compiler/mir/README.md`, session 29.
4. Split `MIRLowerer.cpp` into `MIRLowerExpr/Stmt/Call/Aggregate/Lambda/Async`.
   **DONE (session 33).** The async/lambda/const groups moved in session 27; the
   remaining bulk is now `MIRLowerExpr.cpp` (1,858), `MIRLowerStmt.cpp` (2,176),
   `MIRLowerCall.cpp` (1,523), `MIRLowerAggregate.cpp` (473), leaving the core
   `MIRLowerer.cpp` at **867 lines** (place resolution, `lower_address_of`,
   moves/drops, the static-interface stub, `lower_function`). The AST-aware free
   helpers became an `inline` `compiler/mir/MIRLowererHelpers.h`. Pure move,
   output-invariant (emitted C 96,803 lines either side; normalized diff 52
   against a 62-line same-binary floor); `MIRTests` 4400, `--tcc` 2305/2305,
   `--llvm` 2303/2303, `--libs` 718/719. Reviewability + incremental-build win;
   the compile-time gate still needs D3/per-instruction/E. See
   `compiler/mir/README.md`, session 33.
5. Add phase timers (behind an env var, off by default) so the remaining
   `MIRLowerer` time can be attributed without `perf`, which is unavailable on
   this machine (`perf_event_paranoid=4`).
   **DONE (session 29, partial):** `-DMIR_OPS_COUNTERS` (CMake option, off by
   default → zero cost) + `MIR_OPS=1` counts lowering calls, emitted instructions,
   time inside `lower_function`, and the hot-table operations, printed at exit
   (`compiler/mir/MIRHotOps.h`). It is what D3's rejection above is based on.
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
**Update (session 27).** D4's first step landed: the async group moved to
`MIRLowerAsync.cpp`, lambda/capturing to `MIRLowerLambda.cpp`, and the
const-expression builder to `MIRLowerConst.cpp` (shared `MIRLowererInternal.h`),
so `MIRLowerer.cpp` is **9,026 → 7,014 lines** (a pure move — identical emitted
C). It is **enabler work**: it speeds the compiler's own incremental builds and
review but does not move the compile-time gate, which still needs D3,
per-instruction cost, and E. **The gate is not measurable on this Windows
checkout** (three identical cold runs: 15.6 / 24.4 / 23.5 s), so D3/per-instruction
work here can only be verified for correctness, not for a compile-time delta.

### E. Parallelise lowering (P2, after D)

**Symptom.** Serial per-module lowering.
**Evidence.** §1.5; session 29 measured **≈2.9–3.2 s of `lower_function` inside a
~9.5–10.8 s cold suite build** for 296,017 emitted MIR instructions on 6,726
functions (no redundancy: 6,721/6,726 distinct).
**Action.** Stage 9: file/function-parallel lowering with reusable per-worker
arena/buffers and slot-direct deterministic merge.
**Acceptance.** ≥1.5× on 4+ cores for large modules; no RSS regression on small
modules.

**Code-grounded plan (session 30 — read before starting).** The emission seam is
already good: `lower_and_emit` (`ASTProcessor.cpp`) lowers one function into a
per-function `MIRArena` and emits its C into a **local `std::string`** before
appending to `visitor.writer`, and the async path already writes
`pre + poll + post + ramp` in order. So a "function artifact = bytes + source
slot" model is a small step, not a rewrite. The blockers are all in the **shared,
mutable module state** the single `MIRLowerer`/`MIRModule`/`MIRTypeBuilder` hold:

1. **Shadowing symbol cache (the hard one).** `MIRLowerer::intern_function` dedupes
   through the *per-lowerer* `func_symbols_` and then unconditionally
   `module_.symbols.add(...)`. Two workers with their own lowerer would both miss
   their cache and add **two symbols for one mangled name**. E needs either a
   shared, thread-safe `mangled-name → SymbolId` index in `MIRSymbolTable` (so
   `add` dedupes) or a shared `FunctionDeclaration* → SymbolId` table. The
   `MIRSymbolTable` dedup-by-name route is the durable one (it also retires the
   transient AST-pointer cache).
2. **Type/constant interning.** `MIRTypeBuilder::map` hits a per-file
   `unordered_map` `memo_`, then interns into the shared `MIRTypeTable` /
   `MIRConstantPool` (already deduplicating by value). Both the memo and the
   `intern` paths become concurrent; the pools need a lock (or sharding) and the
   memo must become worker-local (or `clear_memo` per worker).
3. **C `visitor` state.** `lower_and_emit`/`lower_impl` mutate shared
   `CTranslationContext` (`visitor.error(...)`, `iface->active_user`,
   `remove_static_interface_for_stub_impl`, the `mangler`). These must move into
   the worker or be serialised. `mangler_` (`std::function` → `std::string`) is
   the one shared read that also allocates; its thread-safety is unverified.
4. **Order nondeterminism.** `module_.symbols`/`globals`/`functions` are appended
   during lowering; concurrent appends make ids and the collected-function order
   nondeterministic. For the LLVM path (`mir_llvm_collect`) `module.functions` is
   iterative; for the C path the merge must be **slot-direct in source order**
   (the plan's §6.11) so the C stays functionally identical.

**Staged steps (each output-invariant + measured):**

- **E1 — shared interning.** Add a `name → id` index (mutex-guarded) to
  `MIRSymbolTable` so `add` dedupes; make `MIRTypeTable`/`MIRConstantPool`
  intern thread-safe. Verify: serial behaviour unchanged (`--tcc/--llvm`
  suites + `MIRTests`), interning cost ≤ noise (the locks are uncontended on the
  serial path).
  **DONE (session 31), with one deviation.** The three tables' mutators are
  mutex-guarded and grew `clear()`; `MIRTests::test_table_thread_safety` hammers
  them from 8 threads (1180 → 4400 checks). But **symbol ids are deliberately not
  deduplicated**: the backends already match symbols by mangled name, so a shared
  name index would only risk cross-kind collisions and entangle the weak-stub
  pass — a proven-unnecessary optimisation. Interleaved A/B of the pre/post
  `TCCCompiler` shows no measurable cost. Suites: `--tcc` 2305/2305, `--llvm`
  2303/2303, `--libs` 718/719. See `compiler/mir/README.md`, session 31.
- **E2 — worker context, still serial.** Introduce the plan's `MIRTaskContext`
  (per-worker `MIRLowerer` + `MIRArena` + output buffer) and route
  `lower_and_emit` through it, with exactly one worker. Verify: identical C.
  **DONE (session 32).** One context; the C is invariant (normalized diff 68 <
  the 136-line same-binary jitter floor) and the suites are green.
- **E3a — reference-stable intern pools.** E1 guarded the tables' mutators but
  their byte pools were still `std::vector<char>`, so a worker's append could
  realloc and dangle a `const char*` another worker's `name_data`/`data_at` had
  returned. Every text/payload pool is now a `MIRBytePool` (individually-allocated
  entries: stable *and* contiguous). **DONE (session 32), with one measured
  deviation.** The *record* containers were **left `std::vector`** — making them
  `std::deque` too cost **+27 % user time** (4.00 s → 5.08 s, RSS unchanged)
  because the hot `equal`/`hash_of` scan indexes them constantly. So the fast
  vector stays where only opaque ids escape, and the stable byte pool is where a
  raw pointer escapes. Post-E3a cost is within noise (user 3.83–4.08 s vs the E2
  baseline 3.90–4.01 s; RSS 56.0–56.8 MB vs 56.4–57.3 MB).
- **E3c — storage class per read pattern.** **DONE (session 32).** Rather than
  requiring a seal, the containers are classified by how they are read: the hot
  record vectors (`types`/`constants`/`symbols`/`exprs`) hand out only opaque
  ids; the pointer-handing / concurrently-indexed but *cold* pools
  (`MIRTypeTable::data`/`fields`, `MIRConstExprTable::children`) are
  reference-stable `std::deque`; the text/payload pools are `MIRBytePool`. After
  E1+E3a+E3c a reader never holds a pointer/reference into a container another
  worker can realloc. Cost within noise (user 4.07–4.15 s vs the 3.90–4.01 s
  baseline; RSS 56.1–57.7 MB), emitted C invariant (94 < 136-line floor), suites
  green. A hard seal is therefore unnecessary for reader safety.
- **E3d — parallel file/function tasks.** Run the module's functions through a
  thread pool above a measured size threshold (the plan's "≥10 sufficiently large
  functions"); merge slot-direct. Verify: `--tcc/--llvm` suites green, C
  functionally identical, peak RSS not regressed.
- **E4 — measure the gate.** ≥1.5× on 4+ cores for a large module; raise/disable
  the threshold if a shape regresses time or RSS.

**Honest payoff bound.** Lowering is ~30% of the cold build, so 8-core
parallelism caps the win at roughly **−20–25% cold build time** (~9.5 s → ~7.3 s),
which **still misses the ≤2.94 s gate**. E is the largest single lever but not
sufficient on its own; it must land *after* (or beside) a **release-build
re-measure (M1)** — the gate has only ever been measured with a `Debug` compiler
(§0) — and probably alongside fewer-instruction lowering. Parallelism also risks
peak RSS (NFR-2), so the threshold is load-bearing, not cosmetic.


### D6. Hot-path rule violations (P2, opportunistically)

**DONE (session 15).** `CodegenMir` called `std::getenv("MIR_LLVM_TRACE")` (and
`MIR_LLVM_DUMP`) *once per emitted MIR instruction* — a linear scan of `environ`
per instruction, hundreds of thousands of times per suite compile, for a tracing
facility that is off in every normal run. Both are read once into a file-static
`const bool`. (Not separately benchmarked: the call count, not a timing, is the
evidence. The `MIR_LLVM_TRACE` output itself is unchanged.)

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
> **Session 16 (C.4, branch half) moves the C-size gates.** Straight-line
> branches no longer print a jump: main-suite C **106,373 → 96,909 lines** and
> `goto`s **14,945 → 6,459**, `tests-tcc.exe` **4,292,216 → 4,248,581 B**
> (−1.0%), probe **11,723 → 10,743 lines** / **448,625 → 444,239 B**, suite
> compile 3.03 s → 2.98 s; runtime ≥ parity (`bcryptbench` cost-12 median
> 9.72 s → 9.42 s, `benchprobe` unchanged, identical output), `--tcc`
> 2300/2300, `--llvm` 2298/2298. The probe line gate (≤ 8,285) and the
> compile-time/RSS gates are still missed: what is left of C is the
> remaining zeroing classes (session 17 removed the constructor-result temp and
> the `apply_struct_defaults` zeroes wherever the construction covers the value),
> and the compile-time gap is D (per-instruction cost) plus E (parallel lowering).
>
> **Session 17 (C.1, fourth bite) moves the zero-loop count, not the line gate.**
> Main-suite zero loops **539 → 440**, probe zero loops **37 → 25**, probe C
> **10,743 → 10,731 lines** / **474,614 → 473,067 B**, probe exe **444,223 →
> 442,993 B**, `tests-tcc.exe` **4,248,688 → 4,238,513 B**, `--tcc` **2305/2305**,
> `--llvm` **2303/2303**, `--tcc --async` **53/53**, `--tcc --interpret`
> **1895/1895**, `MIRTests` **1115 checks**. The probe line gate (≤ 8,285),
> the compile-time and RSS gates are still missed.
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
