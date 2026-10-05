# MIR implementation status

Branch: `mir`. Contract: `lang/docs/mir-implementation-plan.md` (§0 mandate,
§11 baselines) and `lang/docs/mir-design.md` (§19 current generated-C analysis).

This directory holds the MIR (mid-level IR) used by the compiler's lowering
pipeline. Read the plan before changing anything here. The binding rules are:
**no feature flags, no legacy fallback, no capability analysis;** generated C
must stay **functionally equivalent** (not textually) and is deliberately
optimized; the release compiler must stay under 4 MB.

## Done

### PR 1 — core types + arena + module tables + dump + verifier (commit `9a572bae8`)

| File | Contents |
|------|----------|
| `MIRTypes.h` | `TypeId`/`ValueId`/`PlaceId`/`BlockId`/`InstId`/`SymbolId`/`ConstantId`, `MIROperandKind`, `MIRStorageClass`, `MIRInitState`, `MIRValueFlags`, `MIRPortability` |
| `MIRArena.h/.cpp` | thread-owned bump arena, inline first chunk, `reset()` reuse; no mutex, no destructor tracking |
| `MIRArray.h` | arena-backed append-only array `{T* ptr; uint32 len, cap;}` |
| `MIRInstruction.h/.cpp` | 16-byte `MIRInstruction`, `MIROperand`, `MIROpcode`; **opcode-static** flags/name/portability tables (no per-instruction metadata) |
| `MIRFunction.h` | `MIRBlock`, `MIRValueDef`, `MIRPlaceDef`, `MIRCleanupScope`, `MIRSourceLoc`, `MIRFunction` |
| `MIRTypeTable.h` | canonical `MIRTypeRecord` interning, shared data pool |
| `MIRConstantPool.h` | interned int/float/double/bool/null/string constants |
| `MIRSymbolTable.h` | symbol records + interned name pool |
| `MIRMovePath.h` | move-path tree node |
| `MIRModule.h` | `MIRModule`, sealed `MIRModuleContext` |
| `MIRDump.h/.cpp` | stable textual dump |
| `MIRVerifier.h/.cpp` | structural checks (block ranges, terminator placement, operand bounds, positioned blocks) |

### PR 2 — typed builder (commit `c45ec3ca0`)

`MIRBuilder.h/.cpp`: block reservation/positioning (blocks emitted in creation
order, contiguous ranges), dense value/place creation, move-path roots,
constants, `alloca/load/store/address_of/field_addr/index_addr/gep`,
`unary/binary/compare/cast/select`, `call_scalar/call_sret/call_indirect`,
`init/copy_init/move_init/assign/destroy/drop/set_drop/memcpy/memset`,
`ret/br/cond_br/unreachable`, and cleanup-scope `push/pop/register_owned`.
Tracks saturating use counts and value def-inst indices for later inline
compaction. Has `ok()`/`error()` for OOM/misuse.

### Tests

`compiler/mir/tests/mir_tests.cpp` builds a CFG + storage + verified dump. It is
the standalone `MIRTests` target (core only; never links LLVM):

```bash
bash -lc "source scripts/msvc_env.sh && cmake --build cmake-build-debug --target MIRTests -j 8"
./cmake-build-debug/MIRTests.exe     # -> mir_tests: OK (N checks)
```

### PR 3a — AST→MIR type mapper (commit `f4189f3fa`)

`MIRTypeBuilder.h/.cpp` maps resolved `BaseType` to canonical `TypeId`
(void/bool/intN/float/double/pointer/reference/array/function; structs/
variants/unions/enums get a per-declaration `TypeId` until the aggregate
milestone). This is the only MIR component that includes AST headers. MIR core
`.cpp` were added to `COMMON_SOURCES` so the real compiler build compiles them.

### PR 3b — straight-line lowerer (commit `965a70a25`)

`MIRLowerer.h/.cpp` with `MIRExprResult`: primitives, identifiers/loads, unary,
binary/compare, casts, direct scalar calls, var init, assignment, return, and
bare-expression statements. Params are spilled to places. Unsupported constructs
return a structured error (no fallback).

### PR 4a — straight-line C emitter (commit `52a86b9eb`)

`MIREmitter.h/.cpp` emits C from MIR for the straight-line subset, instruction
per line, no GNU statement expressions; unsupported opcodes fail
transactionally. `MIRBinaryOp`/`MIRUnaryOp` keep the emitter AST-free. Tested in
`MIRTests` (emits a hand-built `add(a,b)`).

## Remaining

The pipeline is **not** switched yet; MIR is isolated infrastructure. The switch
happens at PR 4b and must not be gated by a flag.

### PR 3c / 4b — module builder + pipeline switch (next)

1. `MIRModuleBuilder`: walk a module's `Scope::nodes`, enumerate concrete
   functions (free functions, then generic instantiations and lambda bodies),
   and build `MIRModule::{types,symbols}`. Mangled names must be produced with
   `NameMangler` (into a temporary `BufferedWriter`) and copied into
   `MIRSymbolTable`; MIR must not retain `FunctionDeclaration*`.
2. In `ASTProcessor::implement_module`, replace the per-node
   `c_visitor.translate_after_declaration(nodes)` body loop: lower each
   top-level `FunctionDeclaration` through MIR and emit its C into
   `c_visitor.writer`; keep declaration emission (`declare_module`) and
   non-function top-level nodes on the legacy visitor for now. Then extend to
   methods/impls/generics/lambdas.
3. The branch is expected to go red here; fix forward, never fall back.

### PR 5–9 (unchanged)

MIR interpreter; CFG + aggregates + cleanup (merged); delete the legacy
translator; LLVM lowering; parallel lowering. See the plan §9.

## Invariants to preserve

Exact AST API map (verified):

- **Value kinds/accessors** — `ast/base/Value.h`, `ast/base/ValueKind.h`.
  Predicates `Value::isXxx(ValueKind)` and safe `as_*` (`as_int_num_value_unsafe`,
  `as_bool_value`, `as_float_value`, `as_double_value`, `as_string_value`,
  `as_identifier`, `as_func_call`, `as_access_chain`, `as_casted_value`,
  `as_expression`, `as_negative_value`, `as_not_value`, `as_bitwise_not`,
  `as_addr_of_value`, `as_reference_of_value_unsafe`, `as_deref_value`,
  `as_null_value`, `as_sizeof_value`, `as_index_op`, `as_variant_case`).
- **Types** — `ast/base/BaseType.h`, `ast/base/BaseTypeKind.h`. `kind()` then
  `as_*` (`as_intn_type` → `IntNType` with `IntNKind()`/`num_bits()`/
  `is_unsigned()`, `as_bool_type`, `as_float_type`, `as_double_type`,
  `as_pointer_type` (`PointerType::known_child_type()`), `as_reference_type`,
  `as_array_type` (`ArrayType::get_array_size()`), `as_struct_type` +
  `BaseType::get_direct_linked_struct()`), variants via
  `get_direct_linked_variant()`, enums via `get_direct_linked_enum()`, functions
  via `as_function_type()`/`as_capturing_func_type_unsafe()`. `BaseType::byte_size()`
  is available for size/alignment caching.
- **Literals** — `IntNumValue::get_num_value()`, `BoolValue::value`,
  `FloatValue::value`, `DoubleValue::value`, `StringValue::value`/`length`.
- **Identifiers** — `VariableIdentifier::value` (name) + `linked` (resolved decl);
  cast the linked node with `as_var_init()`/`as_function()`/`as_struct_member()`.
- **Calls** — `FunctionCall::parent_val`, `values` (args), `function_type()`,
  `linked_func()`. 2c reference: `ToCAstVisitor::VisitFunctionCall`
  `preprocess/2c/2cASTVisitor.cpp:7707`; names via
  `visitor.mangler.mangle_no_parent(writer, func_decl)`.
- **Functions** — `FunctionDeclaration` (`ast/structures/FunctionDeclaration.h`):
  `body` (`std::optional<Scope>`, `Scope::nodes`), inherited `params`/`returnType`,
  `is_comptime()/is_extern()/is_delete_fn()/is_constructor_fn()`,
  `returnType->requires_destructor()`.
- **Params** — `FunctionParam::type`, `attrs.has_address_taken/is_implicit/get_has_assignment`.
- **Statements** — `ReturnStatement::value`, `VarInitStatement`
  (`located_id`, `type`, `value`, `known_type()`, `is_const()`),
  `AssignStatement` (`lhs`, `value`, `assOp`, `is_first_init`),
  `ValueWrapperNode::value`.
- **Mangling** — `compiler/mangler/NameMangler.h`, `mangle_no_parent`.
- **Types to seed** — `TypeBuilder` (`ast/base/TypeBuilder.h`) canonical
  primitives (`getI32Type()`, `getU32Type()`, `getBoolType()`, `getVoidType()`, ...).

PR 3 deliverables:
1. `MIRTypeBuilder` — `BaseType*` → `TypeId` (canonical interning in
   `MIRModule::types`), seeding primitives once.
2. `MIRSymbolBuilder` — enumerate concrete functions (module fns, generic
   instantiations, lambda bodies) into `MIRModule::symbols`; mangled names via
   `NameMangler`.
3. `MIRLowerer` — a direct `switch(val->val_kind())` producing `MIRExprResult`
   (`{uint32 id; uint32 type; uint8 kind;}`). Return `error()` for anything not
   yet implemented (a structured diagnostic, never a fallback).
4. Straight-line subset first: constants, identifiers/loads, address-of,
   scalar arithmetic/compare/cast, scalar and struct-return calls, `return`.
   Then blocks/if/loops, then aggregates/lifetime (the merged PR 6 scope).

### PR 4 — C emitter + pipeline switch

`compiler/mir/cbackend/MIREEmitter` emits C from MIR (explicit result places, no
`({ ... })`, lazy names, expression compaction for pure single-use scalars). At
the end of PR 4 the C backend lowers **every** function through MIR; unsupported
constructs are compile errors (never routed to legacy). This is where
`compiler/mir/*.cpp` joins `COMMON_SOURCES` (currently only `MIRTests` compiles
them) and where the branch is expected to go red until coverage catches up.

### PR 5–9

MIR interpreter (correctness oracle); CFG + aggregates + cleanup (merged);
delete the legacy translator; LLVM lowering; parallel lowering. See the plan §9.

## Invariants to preserve

- No `ASTNode*`/`Value*`/`FunctionDeclaration*` retained in persisted MIR.
- No `std::unordered_map`/`chem::string` per instruction; dense ID tables only.
- Effects/portability/compaction safety are opcode-static.
- Every block ends in exactly one terminator; use-counts updated on append.
- `MIRTests` must stay green before every MIR commit.
