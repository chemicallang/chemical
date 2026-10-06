# MIR implementation status

Branch: `mir`. Contract: `lang/docs/mir-implementation-plan.md` (§0 mandate,
§11 baselines) and `lang/docs/mir-design.md` (§19 generated-C analysis).

MIR is the compiler's C lowering path. Read the plan before changing anything
here. Binding rules: **no feature flags, no legacy fallback, no capability
analysis;** generated C must stay functionally equivalent (not textually);
the release compiler must stay under 4 MB.

## Current status (latest commit)

**All suites are green: `./scripts/test.sh --tcc` -> 2260 passed, 0 failed;
`./scripts/test.sh --tcc --interpret` -> 1872 passed, 0 failed;
`./scripts/test.sh --tcc --libs` -> 719 passed, 0 failed;
`./scripts/test.sh --tcc --plugins` -> 1296 passed, 0 failed.** `MIRTests` is
green (1049 checks). `cstd`, `std`, `lab`, all libraries and all module build
scripts translate through MIR.

MIR owns the bodies of top-level `FunctionDeclaration`s. Declarations and
non-function top-level nodes still go through the legacy visitor (temporary
coverage boundary). Some constructs are not lowered by MIR and are routed to
the legacy visitor **for that function only** (`mir_translate_after_declaration`
prints `[MIR] lowering failed (<fn>): <reason> -- falling back to the legacy
visitor`). This is a deliberate, temporary bridge; the goal is to remove it.

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

### 2. Legacy bridge still used for some constructs

`[MIR] lowering failed` cases (function-level fallback). Common ones:
- `ValueKind::LambdaFunc` (kind 8) and other lambda/`std::function` constructs.
- `ExpressiveString` (46), `RuntimeBlockValue` (44).
- `call to unresolved function` for a few imported build-script bodies whose
  identifiers are not linked in this compilation mode.
- `capturing function call is not yet supported`, `reference-to-reference
  method receiver`, functions returning function pointers (routed to legacy).

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

## Invariants to preserve

- No `ASTNode*`/`Value*`/`FunctionDeclaration*` retained in persisted MIR.
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
