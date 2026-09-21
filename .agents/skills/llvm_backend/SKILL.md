---
name: LLVM Backend
description: Comprehensive guide to the LLVM codegen backend — how Chemical AST is lowered to LLVM IR, key patterns, gotchas, debugging, and parallelization strategies.
---

# LLVM Backend

The LLVM backend is the primary optimizing codegen path for the Chemical compiler. It translates the fully resolved and type-checked AST into LLVM IR, which LLVM then optimizes and lowers to machine code.

## Architecture Overview

### Pipeline

```
Type-checked AST → LLVM.cpp (expression/value lowering) → LLVMGen.cpp (IR builder helpers) → LLVM IR → LLVM optimization passes → Object code
```

### Key Files

| File | Purpose |
|------|---------|
| `compiler/backend/LLVM.cpp` | Main codegen — converts AST values, types, and expressions to LLVM IR |
| `compiler/backend/LLVMGen.cpp` | IR builder utilities — allocation, GEP, stores |
| `compiler/backend/LLVMGen.h` | LLVMGen class declaration |
| `compiler/backend/LLVMBackendContext.h` | `BackendContext` shim for the build system (mem_copy, atomics, forget) |
| `compiler/backend/DebugInfoBuilder.h/.cpp` | Debug info (DWARF) generation |
| `compiler/backend/include/LLVMArrayDestructor.h` | Array destructor helpers |
| `compiler/backend/CLANG.cpp` | Clang integration for C driver mode |
| `compiler/Codegen.h/.cpp` | Holds the LLVM state (`ctx`, `module`, `builder`, `TargetMachine`, `DebugInfoBuilder`, `LLVMGen`) and drives codegen |

Chemical → LLVM type mapping is implemented by `BaseType::llvm_type(Codegen&)` / `Value::llvm_type(Codegen&)` methods spread across `ast/` (e.g. `ast/structures/VariantDefinition.cpp`, `ast/values/StructValue.cpp`).

### Where Each Lowering Lives (Source Map)

The LLVM backend is virtual methods per AST node plus shared `Codegen` helpers.

| AST category | Type lowering | Value/body lowering | Notes |
|---|---|---|---|
| `Codegen` state / driver | `compiler/Codegen.h/.cpp` | `Codegen::declare_nodes`/`compile_nodes` | cache clearing, save passes |
| Primitives / pointers / arrays | `compiler/backend/LLVM.cpp:115-296` | `LLVM.cpp:300-934` | `*Type::llvm_type`, `*Value::llvm_value` |
| Structs / unions | `ast/structures/StructDefinition.cpp:187`, `ast/types/StructType.h` | `StructDefinition::code_gen` (`:45`), `StructValue.cpp` | named vs anonymous layout |
| Variants | `ast/structures/VariantDefinition.cpp:34-135` | `VariantDefinition::code_gen_once` (`:154`) | tag + payload union |
| Interfaces / vtables | `ast/structures/InterfaceDefinition.cpp:184-299` | `InterfaceDefinition::code_gen` (`:83`) | fat pointer + `GlobalVariable` vtable |
| Functions | `ast/types/FunctionType.cpp:22-70` | `ast/structures/FunctionDecl.cpp:260` | sret, attributes, ctor/dtor |
| Function calls | `ast/values/FunctionCall.cpp` | `llvm_chain_value` (`:697`), `llvm_dynamic_dispatch` (`:442`) | self, implicit args, dynamic |
| Arrays | `ast/types/ArrayType.h`, `ArrayValue.cpp:178` | `ArrayValue::initialize_allocated` (`:37`) | zero-fill + element stores |
| Strings | `LLVM.cpp:543` (StringValue) | `StringValue::llvm_value` (`:557`) | `GlobalStringPtr` / private global array |
| Globals / constants | `ast/statements/VarInit.cpp:284` | `VarInit.cpp:52`/`:95` | linkage, TLS, `dso_local` |
| Lambdas / closures | `ast/types/CapturingFunctionType.h` | `ast/values/LambdaFunction.cpp:24-179` | capture struct + fat pointer |
| Enums / pattern matching | `ast/structures/EnumMember.h` | `EnumMember::llvm_load` (`LLVM.cpp:2997`); `PatternMatchExpr.cpp:50` | `i32` constants; variant tag compare + PHI |
| Control flow | — | `If.cpp`, `WhileLoop.cpp`, `ForLoop.cpp`, `DoWhileLoop.cpp`, `SwitchStatement.cpp` | terminators + PHIs |
| Atomic / memory backend | — | `LLVM.cpp:3057-3267` (`LLVMBackendContext`) | atomics, fences, `mem_copy` |

## Codegen / LLVMBackendContext

The actual LLVM state is held by `Codegen` (`compiler/Codegen.h`), not by `LLVMBackendContext`:

```cpp
class Codegen : public ASTDiagnoser {
    std::unique_ptr<llvm::LLVMContext> ctx;     // LLVM context
    std::unique_ptr<llvm::Module> module;        // LLVM module
    llvm::IRBuilder<...>* builder;               // IR builder
    llvm::TargetMachine* TargetMachine;          // Target machine description
    DebugInfoBuilder di;                         // Debug info
    LLVMGen llvm;                                // IR builder helpers
    std::unordered_map<ASTNode*, llvm::Value*> mod_ptr_cache;  // node -> llvm value
    std::unordered_map<ASTNode*, llvm::Type*> ctx_ptr_cache;   // node -> llvm type
    // ... plus current function/blocks, destruct job stack, etc.
};
```

`LLVMBackendContext` (`compiler/backend/LLVMBackendContext.h`) is only a `BackendContext` implementation used by the build system and macros — it holds a `Codegen* gen_ptr` and implements `emit`, `mem_copy`, `supports`, `destruct_call_site`, and the `atomic_*` / `signal_fence` helpers (defined in `LLVM.cpp`).

### Key Responsibilities (Codegen)

1. **Creating LLVM functions** — mapping Chemical functions to LLVM functions
2. **Creating LLVM global variables** — `llvm::GlobalVariable` with proper linkage
3. **Managing IR builder** — insertion point, current function
4. **Debug info** — managing `DIBuilder` for source-level debugging
5. **Caches** — `mod_ptr_cache` (node → llvm value) and `ctx_ptr_cache` (node → llvm type)

### The Virtual `Codegen` Contract

There is no abstract `Codegen` interface. Backend dispatch is via virtual methods
that AST nodes/values/types override (LLVM-only virtuals are guarded by
`#ifdef COMPILER_BUILD`; the 2c backend uses visitors instead).

| Virtual | Declared | Purpose |
|---|---|---|
| `ASTNode::code_gen_declare(Codegen&)` | `ast/base/ASTNode.h:368` | emit a prototype/declaration only |
| `ASTNode::code_gen_external_declare(Codegen&)` | `ast/base/ASTNode.h:378` | declare an imported symbol (no `dso_local`) |
| `ASTNode::code_gen(Codegen&)` | `ast/base/ASTNode.h:385` | emit body / statement |
| `ASTNode::code_gen(Codegen&, Scope*, unsigned)` | `ast/base/ASTNode.h:394` | scope-indexed dispatch |
| `ASTNode::llvm_load(Codegen&, SourceLocation)` | `ast/base/ASTNode.h:416` | load a value from an lvalue node |
| `BaseType::llvm_param_type(Codegen&)` | `ast/base/BaseType.h:449` | ABI parameter type (arrays/structs decay to ptr) |
| `Value::llvm_allocate(...)` | `ast/base/Value.h:423` | materialize a value into an alloca |
| `Value::llvm_pointer(Codegen&)` | `ast/base/Value.h:485` | get the storage address of a value |
| `Value::llvm_value(Codegen&, BaseType*)` | `ast/base/Value.h:493` | get a loaded/constant value |
| `Value::llvm_arg_value(Codegen&, BaseType*)` | `ast/base/Value.h:506` | value as a call argument |
| `Value::llvm_ret_value(Codegen&, Value*)` | `ast/base/Value.h:512` | write into the sret slot |
| `Value::llvm_assign_value(...)` | `ast/base/Value.h:517` | initialize a destination slot |

`code_gen_declare` and `code_gen_external_declare` are separate on purpose: the first
marks symbols `dso_local` (defined here), the second does **not** (defined elsewhere).

## LLVM Codegen: Key Patterns

### Type Lowering

Chemical types are mapped to LLVM types via a visitor pattern:

| Chemical Type | LLVM Type | Notes |
|---------------|-----------|-------|
| `int` (i32) | `i32` | Direct mapping |
| `i8`, `i16`, `i64` | `i8`, `i16`, `i64` | Direct mapping |
| `float` | `float` | Direct mapping |
| `double` | `double` | Direct mapping |
| `bool` | `i1` | Zero-extended to `i8` for storage |
| `*T` | `T*` | LLVM pointer type |
| `&T` | `T*` | Lowered to pointer |
| `[N]T` | `[N x T]` | LLVM array type |
| `struct S` | `{ T1, T2, ... }` | LLVM struct type |
| `variant V` | `{ i32, { ... } }` | Tagged union (i32 discriminator + payload struct) |
| `func (P) → R` | `R(*)(P)` | Function pointer |
| `void` | `void` | Only for function returns |

**Correction (verified against source):** in the LLVM backend `bool` is `i1`
*everywhere* — `BoolType::llvm_type` returns `getInt1Ty()` unconditionally
(`compiler/backend/LLVM.cpp:132`). The `i8`/`int` promotion is a **2c / C backend**
concern (`c_codegen` skill), not an LLVM lowering rule.

The full set of `llvm_type` implementations (not just the common subset above):

| BaseTypeKind | LLVM type | Source |
|---|---|---|
| `IntN` (incl. `char`) | `iN`, N = `num_bits(target)` | `LLVM.cpp:144` |
| `Float` / `Double` / `Bool` | `float` / `double` / `i1` | `LLVM.cpp:136,140,132` |
| `LongDouble` / `Float128` | `x86_fp80` on x86/ppc, else `fp128`/`double` | `LLVM.cpp:154,170` |
| `Pointer` / `Reference` / `NullPtr` / `String` | opaque pointer | `LLVM.cpp:120,189,201,237` |
| `Array` | `[N x T]`; parameter type is `ptr` | `LLVM.cpp:124,128` |
| `Struct` / `Linked` / `Generic` / `Union` | named `%struct.…` or `{ <largest member> }` | `LLVM.cpp:213,225,241,253` |
| `Variant` | `{ [inherited…], i32, { <largest payload> } }` | `VariantDefinition.cpp:34` |
| `Function` / `CapturingFunction` | fn-pointer; capturing instances are struct types | `FunctionType.cpp:68`, `LLVM.cpp:290` |
| `Dynamic` (interface object) | `{ ptr, ptr }` fat pointer (object + vtable) | `LLVM.cpp:282` |
| `Complex` / `Any` / `Void` | `float` placeholder / error / `void` | `LLVM.cpp:184,115,278` |

#### Type Lowering Implementation Notes

`LinkedType`/`GenericType` forward to the linked declaration and memoize their
`llvm::StructType` in `gen.ctx_ptr_cache` (`StructDefinition::llvm_type`, `:187`).
Named structs use `llvm::StructType::create(..., mangled_name)`; anonymous
structs/unions use the literal form (`with_elements_type`, `:174`). Struct/array
**parameter** types decay to pointers via `llvm_param_type` (`BaseType.h:449`).
`DynamicType` is the only two-pointer aggregate; other interface uses are pointers.

### Struct Lowering

Structs are the most complex lowering target:

1. **Simple structs** → LLVM named struct type with packed/non-packed layout
2. **Struct with destructor** → generates an `@llvm.memcpy` pattern for assignment (see gotchas)
3. **Struct with constructor** → generates constructor function called at initialization
4. **Struct return values** → lowered to sret (struct return) pointer parameter

Additional source-verified rules:

- Layout order: inherited base structs first, then direct variables
  (`StructDefinition::llvm_type`, `:187`); `add_child_index` maps a named member to
  its GEP index.
- Default field initializers are emitted by the generated constructor
  (`initialize_def_struct_values`, `FunctionDecl.cpp:682`); a user `@constructor`
  suppresses the generated one via `is_generated_fn()`.
- The generated destructor calls member destructors in order, recursing into array
  members and capturing-function captured destructors (`FunctionDecl.cpp:736`).

#### Struct Assignment: Temp + Destruct + Memcpy (source-verified)

`AssignStatement::code_gen` (`LLVM.cpp:2718-2819`) uses the pattern below for any
struct/variant/union assignment:

```llvm
%tmp = alloca %struct.Type
call void @llvm.memcpy(%tmp, %rhs, size, align)   ; fill temp
call void @Type_destruct(%lhs)                    ; destruct old lhs (if not first init)
call void @llvm.memcpy(%lhs, %tmp, size, align)   ; move temp into place
store i1 true, i1* %drop_flag                     ; reinitialized
```

`copy_or_move_struct` (`LLVM.cpp:2694`) always memcpys a referenced struct and, if
the container has a destructor, clears the source drop flag via
`set_drop_flag_for_moved_ref`. `aggregate_store` chooses `store` vs `memcpy`
(`Codegen.cpp:1154`); `memcpy_struct` uses `getTypeAllocSize` (`:1137`).

#### Drop flags / conditional destruction

Every destructible local gets an `i1` drop flag (`createDropFlag`, `Codegen.cpp:493`).
Scope exit / `break` / `continue` / `return` call `dispatch_destruct_jobs`, which
walks `gen.destruct_nodes` in reverse and destructs conditionally on the flag
(`conditional_destruct`, `LLVM.cpp:2021`; `destruct_current_scope`, `:2060`).
Moved-from variables have their flag cleared (`set_drop_flag_for_ref`, `:2492`).

### Function Lowering

Chemical functions are lowered following C ABI conventions:

1. **Name mangling**: Scoped → `scope_name` prefix, generic containers → `__cgs__N`, generic functions → `__cfg_N` suffix
2. **Parameters**: Direct mapping, with sret for struct returns
3. **Main function**: Not mangled for `application` packages
4. **External functions**: `@extern` → no mangling, external linkage

#### Function signature construction (sret, self, capturing)

`FunctionDeclaration::create_llvm_func_type` (`FunctionDecl.cpp:123`) builds the
type from `llvm_func_return` + `llvm_func_param_types`. A struct-like return becomes
`void` plus a hidden sret pointer parameter (`FunctionType.cpp:22`, `:38`). Parameter
order is: **sret ptr** (if struct-like return) → **capture data ptr** (if
`isCapturing()`) → declared params (variadic sentinel `any...` excluded).
`getStructReturnArgIndex()` is `isCapturing() ? 1 : 0` (`FunctionType.h:122`);
`StructValue::llvm_ret_value` writes into that arg (`StructValue.cpp:284`) and
`writeReturnStmtFor` memcpys other struct returns into it (`LLVM.cpp:2096-2102`).

#### Parameters, `self`, default args, variadics, extern

- `FunctionParam::llvm_pointer` (`FunctionDecl.cpp:79`) uses
  `calculate_c_or_llvm_index` (`:866`) to account for sret / extension receiver /
  capturing self. Address-taken params are spilled to an alloca, otherwise the
  incoming `arg` is used.
- **`self`** is the first implicit parameter (constructor `self` at index `0`,
  `FunctionDecl.cpp:118`); methods use `get_self_param()` and
  `BaseDefMember::llvm_pointer` (`StructDefinition.cpp:105`). Receivers in chains
  are handled by `getSelfArgFromGrandpa`/`put_self_param`
  (`FunctionCall.cpp:89`, `:116`).
- **Default args**: the LLVM signature contains all params; the caller passes only
  supplied ones and `FunctionParam::defValue` fills the rest.
- **Variadics**: only `@extern` C functions; `isVariadic()` appends the vararg flag
  (`FunctionType.cpp:56`). **`@extern`/`@no_mangle`**: unmangled names,
  `external_declare_fn` bails for non-public (`FunctionDecl.cpp:361`).
- **Attributes** (`llvm_attributes`, `FunctionDecl.cpp:302`): `InlineStrategy` →
  `InlineHint`/`AlwaysInline`/`NoInline`/`OptimizeForSize`/`MinSize`; `@stdcall` →
  `X86_StdCall`; `@dllimport`/`@dllexport` → DLL storage. **Linkage** from
  `AccessSpecifier` (`to_linkage_type`, `Codegen.cpp:453`).

#### Declaration vs definition, weak stubs, ctor/dtor/nested

- `declare_function` inserts a prototype only if absent; `define_function`
  completes an existing declaration **or weak stub** into a strong definition
  (`Codegen.cpp:713`, `:724`). `declare_weak_function` emits a `WeakAnyLinkage`
  stub with a default return (static-interface methods, `:763`).
- Constructors: `code_gen_constructor` (`FunctionDecl.cpp:700`) initializes default
  fields + member constructors. Destructors: `code_gen_destructor` (`:736` struct,
  `:783` variant) uses a cleanup block + `redirect_return` (`:542`).
- Non-capturing lambdas / local fns go through `create_nested_function`
  (`Codegen.cpp:677`) with `PrivateLinkage`.

### Control Flow Lowering

| Construct | LLVM Pattern |
|-----------|--------------|
| `if/else` | `icmp` + `br cond` → blocks with `phi` for if-expression results |
| `while` | `br` → header block → `icmp` + `br` → body/exit |
| `for` | init block → `br` → header → body → increment → header |
| `switch` | `switch` instruction with cases |
| `break` | `br` to after-loop block |
| `continue` | `br` to loop header |

#### Terminator safety (`has_current_block_ended`)

LLVM forbids multiple terminators per block. `Codegen` tracks
`has_current_block_ended`; `CreateBr`/`CreateRet`/`CreateCondBr`/`CreateUnreachable`
no-op once set, and `SetInsertPoint` resets it (`Codegen.cpp:1171-1247`).
`CreateRet` emits `ret void` for a null value (`:1203`); `CreateCondBr` promotes
non-`i1` conditions with `icmp ne 0` (`:1239`); `DefaultRet`/`FunctionRet` respect
`redirect_return` (destructor cleanup) and otherwise emit the type's zero/undef
(`:1214`, `:1222`). `Scope::code_gen_no_scope` stops after a terminator
(`LLVM.cpp:2601-2614`) so no instructions are appended after a `br`/`ret`.

#### PHI nodes

Every PHI predecessor must have an incoming value, or the verifier rejects the
module. Emitters: `if` expressions (`If.cpp:70-133`), `switch` expressions
(`SwitchStatement.cpp:116`), variant pattern matching
(`PatternMatchExpr.cpp:101`), logical `&&`/`||` (`LLVM.cpp:799`), and `in`
expressions (`LLVM.cpp:1322`).

#### Short-circuit `&&` / `||`

`Expression::llvm_logical_expr` (`LLVM.cpp:754`) emits a diamond: evaluate operand
one, branch to operand-two (`&&`) or end (`||`), evaluate operand two, PHI the
result. `Expression::llvm_conditional_branch` (`:909`) does this recursively for
conditions, so `if(a && b)` does not materialize an `i1`.

#### `switch`, variants, pattern matching

`SwitchStatement::code_gen` (`SwitchStatement.cpp:163`) auto-dereferences
references and, for a variant expression, loads the `i32` tag and switches on it
(`VariantDefinition::load_type_int`, `:62`); `auto_default_case` covers all
members. Case constants are normalized with `implicit_cast_constant` (`:237`).
`PatternMatchExpr` (`PatternMatchExpr.cpp:50`) reads the tag, `icmp eq` the member
index, and loads the payload via `get_param_pointer` (`VariantDefinition.cpp:79`).

#### `break` / `continue`, `unreachable`, unwind

`break`/`continue` destroy inside-loop locals (`dispatch_destruct_jobs`) then
branch to `current_loop_exit`/`current_loop_continue` (`LLVM.cpp:2431`, `:1963`);
`loop_body_gen` saves those around a body (`Codegen.cpp:1249`). `unreachable` is
emitted via `CreateUnreachable` (`LLVM.cpp:1969`). `create_func`
(`Codegen.cpp:475-491`) adds `UWTable` in `Debug`/`DebugComplete` and `NoUnwind`
otherwise or when `-fno-unwind-tables` is set. `ThrowStatement` is
`[UNIMPLEMENTED]` (`LLVM.cpp:2690`) and `TryCatch` has no landing pads.

### Arrays

`ArrayValue::initialize_allocated` (`ArrayValue.cpp:37`) zero-fills the array
(`llvm.memset`) when the literal is partial, then stores explicit elements — so
empty/partial literals zero-initialize like C. Constant arrays become
`llvm::ConstantArray` (`:115`). Array destruction is a reverse loop with a PHI
(`Codegen::loop_array_destructor`, `Codegen.cpp:871`), completed by
`LLVMArrayDestructor`'s destructor (`:862`).

### Strings

Non-array `StringValue` uses `CreateGlobalStringPtr`; a char-array `[N]char`
becomes a private global with the bytes plus NUL padding; string-initialized globals
use `CreateGlobalString` and inherit linkage/constness
(`LLVM.cpp:551-576`, `VarInit.cpp:55-65`).

### Variants

Canonical layout (`VariantDefinition.cpp:34`):

```llvm
%variant = type { [<inherited structs>…], i32, { <largest member's raw struct> } }
```

The `i32` discriminator sits after inherited structs, before the payload (index
`direct_inh_composed_structs()`); the payload wraps the **largest** member, smaller
members bitcast/GEP'd in (`get_param_pointer`, `:79`). `direct_child_index` maps a
member name to its tag; destructors switch on the tag (`FunctionDecl.cpp:783`).

### Interfaces, Dynamic Dispatch and Vtables

- An interface object is a `{ ptr, ptr }` fat pointer: object + vtable
  (`LLVM.cpp:282`; `pack_fat_pointer`, `Codegen.cpp:1051`). A vtable is a
  `GlobalVariable` of function pointers in declaration order, inherited interfaces
  first (`InterfaceDefinition.cpp:184`, `:235`), named via `mangle_vtable_name`.
- Dynamic dispatch (`FunctionCall.cpp:442`) loads object + vtable, computes
  `vtable_function_index(linked)`, GEPs/loads the callee, then `call_with_callee`.
- Impl functions live in `gen.trait_impl_func_map` keyed by
  `{interface, for, func}`; default methods look themselves up there
  (`FunctionCall.cpp:506-533`). Cleared per module (`ASTCompiler.cpp:351`).

### Generics / Monomorphized Instances

Generic decls are monomorphized before codegen (`generics` skill); each concrete
decl's `generic_instantiation` index changes the mangled name (`__cgs__N`/`__cfg_N`).
Per-module instantiations are declared/implemented in dedicated loops
(`ASTCompiler.cpp:220-300`), then cleared (`:343`).

### Closures / Lambdas

- A non-capturing lambda is a `PrivateLinkage` nested function pointer
  (`create_nested_function`, `Codegen.cpp:677`).
- A capturing lambda packs `{ fn_ptr, capture_struct_ptr }` (`Codegen.cpp:1051`);
  `capture_struct_type` is an anonymous struct of captures (`LambdaFunction.cpp:173`);
  by-value struct captures are memcpy'd with the drop flag cleared (`:36-48`); a
  generated `lambda_cap_destr` destructs them (`:71`). `CapturingFunctionType` takes a
  hidden capture pointer at arg 0 (`FunctionType.cpp:53`); `call_capturing_lambda`
  loads `get_fn_ptr`/`get_data_ptr` (`FunctionCall.cpp:540`).

### Global Variables and Constants

- `VarInitStatement::code_gen_global_var` (`VarInit.cpp:52`) creates the
  `GlobalVariable`; linkage from `AccessSpecifier` (`:29`); `@thread_local` sets
  `LocalExecTLSModel` (`:46`); `const` constant initializers make it `const` (`:82`).
- `dso_local` is set for **non-extern** globals only (`:42-45`); the external path
  clears it (`:195-202`); `comptime const` strings materialize lazily (`:205-231`).

### Async Functions / Coroutines (LLVM)

Lowered by `compiler/backend/LLVMCoroutine.{h,cpp}` (not `LLVM.cpp`). When symres
has wrapped an `async func`'s return type to `FutureHandle<T>`,
`FunctionDeclaration::code_gen_body` calls `gen_llvm_async_fn` before the normal
body emission; `AwaitExpression::llvm_value` routes to `gen_llvm_await` only when
`gen.current_coro != nullptr` and the awaited operand's type differs from the
resolved result type (otherwise `await` is transparent/eager).

Generated functions (design §9):

- **ramp**: `coro.id`/`coro.alloc`/`coro.size`/`coro.begin` with the
  `presplitcoroutine` **enum** attribute, an initial lazy suspend, the body, a
  final suspend, and a shared `coro.end(i1 false)` return path;
- **`foo_poll`**: resumes the coroutine, returns `Poll.Ready(result)` or
  `Poll.Pending`; on completion the wrapper state becomes `0xFFFFFFFF`;
- **`foo_drop`**: frees the frame. A **completed** coroutine (`0xFFFFFFFF`) has
  no live locals, so `emit_drop_fn` frees the wrapper + coroutine frames directly
  instead of calling `coro.destroy` (B22 — `coro.destroy` re-runs the body);
  not-started (`0`) and suspended (`site+1`) use `coro.destroy`;
- a `FutureTable<T>` vtable.

Frame ownership: the returned handle's `frame` field points at **our own
wrapper** `{ coro, state, cx, result }` (fixed offsets 0/1/2/3); the LLVM
coroutine frame is separate and referenced by `coro`. Normal-completion does
**not** free the frame (the returned handle owns it); `drop` frees both once.

- **No-await fast path**: a plan with no await sites lowers eagerly via
  `gen_llvm_async_eager_fn` (tiny heap frame, always-`Ready` poll, freeing drop,
  no coroutine intrinsics).
- **Debug info (B15-W fixed)**: the async lowering emits correct debug info.
  `gen_llvm_async_fn` / `gen_llvm_async_eager_fn` open the async function's own
  `DISubprogram` around the ramp body (`start_function_scope(decl, ramp)`) and a
  synthetic one around each generated `__poll` / `__drop`
  (`DebugInfoBuilder::start_generated_function_scope`), and the cancellation
  destructor call carries a `!dbg` location (`emit_destroy_type`). `CoroSplit`
  then gives every split clone (`resume`/`destroy`/`cleanup`) its own
  `DISubprogram` and remaps the locations. Previously `gen.di` was disabled for
  the whole lowering to avoid invalid scopes.
- **B27 gotcha (fixed):** `gen_llvm_async_fn` must capture the caller's
  `current_function` at entry and restore it on every exit path — capturing it
  after installing the ramp leaked the coroutine as `current_function`, so
  module-level `var`s after an `async func` in the same file emitted as *locals*
  with no initializer (`@x = internal global i32`), an invalid module that
  crashed `AlwaysInlinerPass`.

Known LLVM limits, previously worked around and now **fixed**: a plain
non-variant struct payload (B23), a large struct variant such as
`Result<Response, std::string>` through a `FutureHandle` (B26), and awaiting a
combinator inside a coroutine that is itself polled as a task (B25). Small
variants (`Result<int, string>`, `Option<...>`) and pointer/int payloads are
fine.

Async **closures** are not lowered here (the C backend emits the diagnostic;
LLVM keeps the eager bootstrap). See `lang/docs/async-remaining-work.md` for the
remaining items (AC, POSIX-EPOLL).

> **B23, B25, B26 and B15-W fixed.** `Codegen::writeReturnStmtFor`'s coroutine
> branch used to store a `StructValue` alloca pointer into the frame's struct
> result slot; it now byte-copies every struct-like return value
> (`value->llvm_pointer`, materializing a `StructValue` via `llvm_value`). This
> also resolved the large struct-variant payload (B26). Regression tests:
> `lang/tests/async/struct_payload_test.ch`,
> `lang/tests/async/variant_payload_test.ch`. **B25 fixed:** `gen_llvm_await`
> reloads the caller's `Context*` inside the await loop, so a spawned coroutine
> that awaits a combinator no longer forwards a stale `Context` after a resume
> (`lang/tests/async/spawn_combinator_test.ch`). **B15-W fixed:** async debug
> info is emitted and split clones get their own `DISubprogram`s (see above);
> regression `async_coroutine_debug_info_compiles` in
> `lang/tests/negative/src/async.ch` compiles the coroutine shape in
> `debug_complete`. The remaining async items are only the infrastructure/feature
> gaps AC and POSIX-EPOLL — read `lang/docs/async-remaining-work.md`.

## Debug Info Generation

The `DebugInfoBuilder` generates DWARF debug information:

- **Source locations**: Every LLVM instruction has debug metadata pointing to the source position
- **Function debug info**: `DISubprogram` for each Chemical function
- **Variable debug info**: `DILocalVariable` for local variables
- **Type debug info**: `DIDerivedType`, `DICompositeType` for structs, arrays, pointers

```cpp
// Pattern (DebugInfoBuilder::declare, DebugInfoBuilder.cpp:483):
llvm::DILocalVariable* var = builder->createAutoVariable(scope, name, file, line, type);
builder->insertDeclare(alloca, var, builder->createExpression(), loc, inst);
```

Additional source-verified details: `isEnabled` comes from `debug_info`/`-g` and is
on for `Debug`/`DebugComplete` (`DebugInfoBuilder.h:31-37`). One `DICompileUnit` is
created per file with vendor language code `0x8001` (`:76`); `module_init` sets the
Debug Info Version / CodeView / Dwarf Version module flags
(`Codegen.cpp:278-294`). `di_loc` is 1-based (`:124`); `diScopes` stacks compile unit
→ file → function; `finalize` clears caches (`:272`). Struct types use real field
offsets from `DataLayout` (`create_struct_type`, `:300`) and self-referential types
use `createReplaceableCompositeType` (`:281`). Each file/instantiation brackets
codegen with `start_file_scope`/`end_file_scope` (`ASTCompiler.cpp:159-320`).

### How Modes Affect Output

`configure_emitter_opts` (`Codegen.cpp:1597`) maps `OutputMode`:

| Mode | `is_debug` | `lto` | `is_small` | `assertions_on` | LLVM opt level |
|---|---|---|---|---|---|
| `Debug` / `DebugQuick` | true | false | false | false | `O0` |
| `DebugComplete` | true | false | false | **true** | `O0` + verifier |
| `ReleaseFast` | false | **true** | false | false | `O3` |
| `ReleaseSmall` | false | **true** | **true** | false | `Oz` |

`DebugComplete` registers `VerifierPass` at pipeline start and end
(`Codegen.cpp:1481-1492`); release adds `AddDiscriminatorsPass` (`:1495-1501`); debug
disables unrolling/SLP/loop-vectorization/interleaving/merge-functions (`:1441-1446`).

## Driver / Clang Integration

### Backend selection (LLVM vs 2c)

`LabBuildCompiler::process_modules` (`LabBuildCompiler.h:345`) picks
`use_c(job) ? process_job_tcc(job) : process_job_gen(job)`. `use_c` is true when
`options->use_c` or the job is a TCC job (`:299`); `--use-tcc` implies `use_c`,
`--use-c` sets it only. On the `COMPILER_BUILD` (LLVM) build `use_c` means
"translate to C + embedded Clang" (`use_embedded_clang`, `:328`); on `TCCCompiler`
it means TinyCC. `process_job_gen` is the LLVM IR path.

### `compiler/backend/CLANG.cpp` — `CTranslator`

`CLANG.cpp` is **not** the codegen driver. It parses `#include`d C headers
(`import c …`) into Chemical AST via the embedded Clang frontend:
`ClangLoadFromCommandLine` (`:949`) builds a `clang::ASTUnit`;
`make_type`/`make_struct`/`make_func`/`make_var_init` translate decls;
`CTranslator::translate` (`:1297-1345`) is called by `process_module_gen` for module
headers (`LabBuildCompiler.cpp:1089-1121`). `ClangCodegen`/`mangled_name`
(`:1071-1188`) handles `@cpp` bridging, separate from `NameMangler`.

### Embedded Clang compile + link

- `compile_c_file_to_object` (`Codegen.cpp:1965`) builds a clang command
  (`-target`, `-resource-dir`, `-I`, `-g`, `-c -o`) and calls
  `chemical_clang_main2` (`:1673`) over `chemical_clang_main` (`clang_driver.cpp`).
- `clang_link_objects` (`:1862`) builds the link command (objects, `-L`, `-l`,
  `-shared`, `-fsanitize=…`, `-g`, `-no-pie`, `-rpath`) and invokes embedded clang.
- `lld_link_objects` (`:1740`) does the same via embedded LLD
  (ELF/COFF/MinGW/Mach-O/WASM; `invoke_lld`, `:1715`) when `--use-lld` is set
  (`link_objects_now`, `LabBuildCompiler.cpp:2151`).
- `compile_c_to_obj_w_opts` (`LabBuildCompiler.cpp:708`) chooses clang vs TinyCC for
  the `--use-c`/`--use-tcc` pipeline.

### Optimization levels & LTO

`save_as_file_type` (`Codegen.cpp:1392`) builds a `PassBuilder`; the level is
chosen at `Codegen.cpp:1536-1553`: debug → `O0`, release-small → `Oz`, otherwise
`O3` (`buildPerModuleDefaultPipeline`, or `buildLTOPreLinkDefaultPipeline` with
LTO). Sanitizer passes append at optimizer-last (`:1504-1533`).

### Emitting IR / asm / objects

`--out-ll-all`/`--out-asm-all` populate every module's `llvm_ir_path`/`asm_path`
(`LabBuildCompiler.cpp:2038-2053`). `--debug-ir` uses
`save_to_ll_file_for_debugging`, which prints **pre-optimization** IR
(`Codegen.cpp:1658`); otherwise `.ll` is emitted post-optimization.
`--out-ll`/`--out-bc`/`--out-obj`/`--out-asm` set the matching
`CodegenEmitterOptions` paths.

## Parallelization Strategies

Currently, the LLVM backend runs **per-function** and **per-module** in a mostly serial fashion within each compilation job. Future parallelization opportunities:

1. **Per-function codegen**: Each function's LLVM IR generation could run in parallel, with shared context
2. **Per-module codegen**: If a job compiles multiple modules, each module's codegen is independent
3. **LLVM optimization passes**: LLVM's own `ModulePassManager` can parallelize within a module

**Current limitation**: `llvm::IRBuilder<>` is not thread-safe per-context. Options:
- Use multiple `LLVMContext` instances (one per thread)
- Use `llvm::orc::ThreadSafeModule` for thread-safe module ownership

### Actual IR Generation Pipeline & Caching (source-verified)

- `process_modules` dispatches `use_c(job) ? process_job_tcc : process_job_gen`
  (`LabBuildCompiler.h:345`). `process_job_gen` creates the `GlobalInterpretScope`,
  `Codegen`, and `LLVMBackendContext`, prepares the target machine, and per module
  calls `gen.module_init(...)` then `processor.compile_module(...)`
  (`LabBuildCompiler.cpp:2005-2034`, `:1083`, `:1181`).
- Parse/symres/typeverify are parallel per file, but **LLVM codegen is serial** and
  module-by-module within a job (`LabBuildCompiler.cpp:2061-2118`). Within a module,
  `compile_module` runs four serial loops: external-declare deps → declare own →
  declare+implement instantiations → compile own bodies (`ASTCompiler.cpp:138-353`).
- `gen.mod_ptr_cache` is per-module and cleared after each module (`ASTCompiler.cpp:347`);
  `gen.ctx_ptr_cache` lives for the whole `LLVMContext`.
- **Job-level caching** is content-based: if no module changed, `process_job_gen`
  returns early and re-links the previous objects without re-parsing
  (`LabBuildCompiler.cpp:1974-1999`). `--no-cache` disables it.

## LLVM Gotchas

### 1. `dso_local` and External Declarations

**Rule**: When a global variable is declared in another module (`submod_extern_globe_var`), do **NOT** set `dso_local` on the LLVM global value.

```cpp
// WRONG — linker rejects dso_local + external:
auto* gv = new llvm::GlobalVariable(*module, type, false,
    llvm::GlobalValue::ExternalLinkage, nullptr, name);
gv->setDSOLocal(true);  // BUG: linker error for cross-module references

// CORRECT:
auto* gv = new llvm::GlobalVariable(*module, type, false,
    llvm::GlobalValue::ExternalLinkage, nullptr, name);
// No setDSOLocal() call
```

Relevant files: `ast/statements/VarInit.cpp`, `compiler/backend/LLVM.cpp`

Source-verified: `global_var_set_defaults` sets `dso_local` only when
`!stmt->is_extern()` (`VarInit.cpp:42-45`), and
`VarInitStatement::code_gen_external_declare` explicitly does
`gv->setDSOLocal(false)` for initializer-less external globals (`:195-202`).
Functions, by contrast, are always created with `fn->setDSOLocal(true)` in
`create_func` (`Codegen.cpp:482`) — this is safe because cross-module function
references are still resolved by name and a function's own declaration is emitted
in its defining module.

### 2. Struct Assignment: Temp + Destruct + Memcpy Pattern

The LLVM backend assigns structs via a three-step pattern:

```llvm
; Step 1: Bitwise copy source into a stack temp
%temp = alloca %struct.Type
call void @llvm.memcpy.p0.p0.i64(%temp, %src, size, align)

; Step 2: Call destructor on destination
call void @Type_destruct(%dest)

; Step 3: memcpy temp onto destination
call void @llvm.memcpy.p0.p0.i64(%dest, %temp, size, align)
```

**This breaks self-referencing pointers**. If a struct has a pointer field pointing to one of its own members (e.g., `function`'s `fn_data_ptr`), the bitwise copy produces a dangling pointer — the temp shares the same pointer, but after `memcpy` over the destination, the temp is destroyed. The destination's pointer now points to freed memory.

**Possible fixes** (none implemented yet):
- `@reflat` annotation to skip destruct+memcpy
- `@move` hook for custom move semantics
- Forbid self-referencing pointers inside value types

Source-verified location: `AssignStatement::code_gen` (`compiler/backend/LLVM.cpp:2718-2819`),
`Codegen::copy_or_move_struct` (`:2694`). Note the temp is **not** destroyed in the
LLVM lowering — the pattern is alloca temp → fill → destruct old lhs → memcpy →
restore drop flag. The danger is the bitwise copy of a self-referential pointer
field, exactly as the `llvm_backend`/`AGENTS.md` guidance describes.

### 3. Uninitialized Variables and PHI Nodes

LLVM requires `UndefValue` for uninitialized phis:

```cpp
// CORRECT pattern:
auto* phi = builder->CreatePHI(type, numIncoming);
phi->addIncoming(val1, block1);
phi->addIncoming(UndefValue::get(type), block2);  // uninitialized path
```

Check `IRBuilder::CreatePHI` usage in `LLVM.cpp` when adding new PHI-based constructs.

**Correction / clarification (source-verified):** the only `UndefValue` currently
emitted is a *default return* for non-void, non-primitive returns in
`create_return_of_type` (`compiler/Codegen.cpp:759`) — used to give weak stubs a
valid terminator. No current PHI relies on `UndefValue`; instead every PHI
predecessor supplies a real incoming value (`If.cpp:70-133`,
`SwitchStatement.cpp:71-120`, `PatternMatchExpr.cpp:95-103`, `LLVM.cpp:799`,
`LLVM.cpp:1322`). If you add a new PHI-based construct, you must emit an incoming
value for **every** predecessor block or the verifier will fail.

### 4. Alloca Creation

Allocas are created where codegen needs them via `LLVMGen::CreateAlloca`, which forwards to the extern-C helper `LLVMGenCreateAlloca` (and `LLVMGenCreateAllocaTyped`, which also applies `@maxalign` alignment). These insert at the current `IRBuilder` insertion point — there is no `CreateEntryBlockAlloca`/entry-block hoisting helper:

```cpp
// In LLVMGen.cpp:
llvm::Value* LLVMGenCreateAlloca(LLVMGen* gen, llvm::Type* type, SourceLocation location) {
    const auto allocaInst = gen->builder->CreateAlloca(type);
    gen->di.instr(allocaInst, location);
    return allocaInst;
}
```

Source-verified: `compiler/backend/LLVMGen.cpp:14-28`. `LLVMGenCreateAllocaTyped`
computes the type with `type->llvm_type(*gen->codegen)` and sets alignment from
`chemical_llvm_type_align` when `> 1`.

### 5. GEP (GetElementPtr) Indices

When accessing struct fields, the GEP indices must be correct:

```cpp
// For struct S { int a; float b; }
// Accessing 'b' via S* ptr:
// GEP indices: [0, 1]  (0 = deref pointer, 1 = field index)
auto* gep = builder->CreateStructGEP(ptr, 1);  // field index 1 = b
```

All GEPs use `gen.inbounds` (default `true`) to mark `inbounds` — out-of-bounds
access is therefore UB, matching C. Field indices for named members come from
`add_child_index` (structs/variants), never guess them manually.

### 6. Variant (Tagged Union) Lowering

A variant's canonical LLVM type is a struct laid out as `[ inherited structs..., i32 discriminator, { <payload> } ]`:

```llvm
; canonical layout (the largest member determines the payload/alignment):
%variant = type { i32, { <largest member's fields> } }
```

- `i32 discriminator` — which case is active (an enum index), placed after any inherited struct fields, not at offset 0
- payload — a struct wrapping the member's raw struct; the canonical layout uses the largest member and each variant member's payload is accessed via GEP/bitcast

Pattern matching generates:
1. Load discriminator (`i32`)
2. `icmp eq` with case value
3. `br` to matching case block

Source-verified: `VariantDefinition::llvm_type_with_member`
(`ast/structures/VariantDefinition.cpp:34-55`); index of the tag is
`direct_inh_composed_structs(this)` and the payload index is `1 +` that. The
canonical type always uses the largest member; smaller payloads are reached by
bitcast (`get_param_pointer`, `:79-102`).

### 7. `bool` Is `i1` in LLVM (even in aggregates)

`BoolType::llvm_type` returns `i1` for locals, fields and globals alike
(`LLVM.cpp:132`). The `i8`/`int` promotion is a 2c/C detail. `CreateCondBr`
promotes non-`i1` conditions against zero (`Codegen.cpp:1239`).

### 8. Mangled Names Have No Separators

Container path + own name are concatenated with no `_` (only the module prefix adds
one): `struct Point { func sum }` → `<module>_Pointsum`. Always use
`gen.mangler.mangle(...)` (`name_mangling` skill).

### 9. Caches Are Cleared Between Modules

`compile_module` clears `gen.mod_ptr_cache` and `gen.trait_impl_func_map` after
each module (`ASTCompiler.cpp:347-351`). Never reuse a cached
`llvm::Function*`/`GlobalVariable*` across modules — look it up via
`known_func`/`get_llvm_data`, which re-declares on a miss.

### 10. `redirect_return` / Cleanup Blocks

Destructors create a cleanup block and set `gen.redirect_return`
(`FunctionDecl.cpp:542`); `DefaultRet`/`FunctionRet` branch to it instead of
returning (`Codegen.cpp:1214-1228`). Reset it after the destructor body (`:780`).

### 11. Weak Stubs Need a Terminator

`declare_weak_function` emits a default return so a static interface method is
valid if not overridden (`Codegen.cpp:763-778`); `define_function` converts the
weak stub into a strong definition (`:724-746`).

### 12. `inbounds` Is On By Default

`gen.inbounds` defaults to `true` (`Codegen.h:289`), so out-of-range indexing is
UB. Do not rely on LLVM to bound-check.

## Debugging LLVM IR

### Dumping IR

```bash
# Pass --out-ll-all to dump LLVM IR for all modules:
cmake-build-debug/Compiler "lang/compiled/temp.ch" --out-ll-all --build-dir "lang/compiled" -o "lang/compiled/temp.exe" --mode debug_complete --debug-ir
```

IR is written to `lang/compiled/modules/main/llvm_ir.ll`.

### Recommended Debugging Workflow

1. **Rebuild** after any `.cpp`/`.h` change: `./scripts/build.sh --llvm`.
2. **Isolate** the case in `lang/compiled/temp.ch` (AGENTS.md "Debugging: Isolating
   a Single Test Case").
3. **Emit validated IR** with `--assertions` (verifier before/after optimization),
   `--debug-ir` (pre-optimization IR instead of crashing) and `-fno-unwind-tables`:
   ```bash
   cmake-build-debug/Compiler "lang/compiled/temp.ch" --out-ll-all \
       --build-dir "lang/compiled" -o "lang/compiled/temp.exe" \
       --mode debug_complete --debug-ir --assertions -fno-unwind-tables
   ```
4. **Inspect** `lang/compiled/modules/main/llvm_ir.ll`; **GDB** on crash:
   `gdb -batch -ex run -ex bt -ex "info registers" -ex "x/16i \$pc" ./lang/compiled/temp.exe`.
5. **Add `std::cerr` logging** in the suspect `code_gen`/`llvm_value`, rebuild, repeat.

### Common LLVM Verifier Errors

| Error | Cause |
|-------|-------|
| `PHI node entries do not match predecessors` | Missing phi entry for a block |
| `Instruction does not dominate all uses` | Variable used before definition in dominance order |
| `dso_local and external linkage don't match` | Setting dso_local on externally-defined globals |
| `expected instruction opcode` | Wrong IR type used in CreateXxx call |
| `Terminator found in the middle of a basic block` | Emitting after a `br`/`ret`; use the safe `Codegen::CreateBr`/`CreateRet` wrappers |
| `value does not have the expected type` | Missing `implicit_cast` / wrong `llvm_type` |

### Quick Checks

1. **Alloca created at wrong insertion point**: `LLVMGen::CreateAlloca` inserts at the current builder position
2. **GEP wrong indices**: Verify field indices match struct layout
3. **Type mismatch**: Check `CreateCall` arg types match function signature
4. **Phi missing incoming**: Every predecessor must have an incoming value
5. **Break/continue in wrong context**: Must be inside a loop
6. **sret mismatch**: struct-returning calls pass an alloca first
   (`FunctionCall::struct_return_in_args`, `FunctionCall.cpp:429`) and the callee
   returns `void` (`FunctionType.cpp:22`)
7. **Terminator already emitted**: check `has_current_block_ended`
8. **Extern globals**: use `code_gen_external_declare`, not `code_gen_global_var`

## Performance Considerations

1. **Type cache**: `LLVMBackendContext` caches Chemical-to-LLVM type conversions — avoid recomputing
2. **Function cache**: Generated function declarations are cached for extern functions
3. **Alloca hoisting**: All allocas in entry block → better LLVM optimization
4. **GEP simplification**: Use `CreateStructGEP` instead of manual index computation
5. **memcpy vs load+store**: LLVM can optimize memcpy; use it for struct copies
6. **Avoid redundant casts**: `CreateBitCast` is cheap but avoid chains of them

**Correction (source-verified):** the caches are `Codegen::ctx_ptr_cache`
(node → `llvm::Type*`) and `Codegen::mod_ptr_cache` (node → `llvm::Value*`), not
fields on `LLVMBackendContext` (`Codegen.h:179-184`). `mod_ptr_cache` is cleared per
module; `ctx_ptr_cache` lives for the `LLVMContext`. Alloca hoisting is **not**
performed — `LLVMGenCreateAlloca` inserts at the current builder position
(`LLVMGen.cpp:14`).

## Related Skills

- **C Codegen (2c)** (`c_codegen`) — the alternative backend (`process_job_tcc`),
  including the `(*({ ... }))` sret pattern.
- **Name Mangling** (`name_mangling`) — `NameMangler`, `__cgs__N`/`__cfg_N`, vtable
  symbols, and the `dso_local` cross-reference.
- **Annotations** (`annotations`) — `@inline`, `@extern`, `@no_mangle`,
  `@constructor`/`@delete`, `@thread_local`, `@maxalign`.
- **Generics** (`generics`) — monomorphization/mangling of instantiations.
- **Symbol Resolution** (`symres`) and **Type Verification** (`type_verification`) —
  the passes before codegen.
- **Build System (Lab)** (`build_system`) — job dispatch, `LabBuildCompilerOptions`,
  output modes, `--out-ll-all`, `--use-c`/`--use-tcc`.
- **Performance** (`performance`) — arena allocation and parallelization patterns.
- **Interpreter Internals** (`interpreter`) — the comptime evaluator
  (`Codegen::eval_comptime`, `Codegen.cpp:781`).
