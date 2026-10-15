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
   `emit_function_c` used to re-derive inline. Still open: shared traversal,
   naming, and the interface **vtable layout** (slot order) that
   `MIRDeclEmitter` and `CodegenMir::mir_vtable_*` still derive independently.
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
- **`memset` conflicts with cstd's `extern void* memset(...)`** — do not emit a
  call to `memset`; use an inline zero loop.
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
