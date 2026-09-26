---
name: C Codegen (2c)
description: Comprehensive guide to the C translation backend (2c) — how Chemical AST is translated to C code, key patterns, gotchas, and optimization strategies.
---

# C Codegen (2c Backend)

The "2c" backend translates the Chemical AST into C code. This is used by `TCCCompiler` and is also the foundation for the TinyCC JIT compilation path. The 2c backend produces readable, portable C that can be compiled with any C compiler.

> **Not to be confused with `compiler/ctranslator/CTranslator`.** That class does the
> **reverse** direction: it uses Clang to parse C headers and builds Chemical AST nodes
> (`make_type`, `make_struct`, …) so `cstd`/header bindings can be imported. The 2c backend
> described here turns a fully resolved Chemical AST **into** C text.

## Architecture

### Pipeline

```
Type-checked AST → ToCAstVisitor (structure/declaration level) → BufferedWriter → C source file
```

The 2c pass is driven by `ASTProcessor` in four phases per module, then the whole buffer is
finalized and compiled. See [Compiler Pipeline / Lifecycle](#compiler-pipeline--lifecycle)
below.

### Key Files

| File | Purpose |
|------|---------|
| `preprocess/2c/2cASTVisitor.h` | Main C codegen visitor declaration — `ToCAstVisitor` class |
| `preprocess/2c/2cASTVisitor.cpp` | Main implementation (~8,170 lines) — translates all AST nodes to C |
| `preprocess/2c/SubVisitor.h` | Base class and utilities for C translation sub-visitors |
| `preprocess/2c/CTopLevelDeclVisitor.h` | Top-level declaration visitor (`CTopLevelDeclarationVisitor`) — functions, structs, globals |
| `preprocess/2c/CDestructionVisitor.h` | Visitor for destructor code generation (`CDestructionVisitor`, `DestructionJob`) |
| `preprocess/2c/2cBackendContext.h` | `ToCBackendContext` — the `BackendContext` shim (emit/mem_copy/supports/atomics) |
| `preprocess/2c/BufferedWriter.h` | `BufferedWriter` and `ScratchString<N>` efficient buffered output |
| `compiler/ASTProcessor.cpp` | Drives the four phases (`declare_module`/`implement_module`) |
| `compiler/lab/LabBuildCompiler.cpp` | Owns the visitor, finalizes and compiles the C (`compile_c_to_obj_w_opts`) |
| `integration/libtcc/LibTcc.cpp` | TinyCC compilation of the generated C |
| `integration/libtcc/LibTccInteg.h` | `compile_c_string` / `compile_adding_file` / `tcc_link_objects` |
| `integration/libtcc/TCCMode.h` | `TCCMode { None, Debug, DebugComplete }` |
| `compiler/ctranslator/CTranslator.{h,cpp}` | **Reverse** direction: C/clang → Chemical AST (not the 2c backend) |

### `ToCAstVisitor` — state and shape

`ToCAstVisitor : public NonRecursiveVisitor<ToCAstVisitor>, public ASTDiagnoser`
(`2cASTVisitor.h:30`). It is the only wide, mutable state holder; the sub-visitors
(`CTopLevelDeclarationVisitor tld`, `CDestructionVisitor destructor`) hold a reference back
to it (`preprocess/2c/SubVisitor.h`).

Notable state (`2cASTVisitor.h`):

| Field | Meaning |
|-------|---------|
| `BufferedWriter writer` | Single in-memory output buffer (4 MiB default, doubling growth) |
| `NameMangler& mangler` | Runtime C identifiers (see `name_mangling` skill) |
| `loc_man`, `comptime_scope`, `binder`, `coreNodes`, `implsIndex` | `#line` locations; target data + comptime eval; CBI/core/interface indices |
| `bool pass_structs_to_initialize` | Enables the sret convention (default `true`) |
| `bool cpp_like`, `line_directives`, `minify`, `is64Bit` | Output knobs (`cpp_like` emits `bool` not `_Bool`) |
| `bool array_types_as_subscript`, `nested_value` | Arrays as `[N]` (default false → pointer); suppress trailing `;` |
| `fat_pointer_type`, `current_assignable`, `return_redirect_block` | `__chemical_fat_pointer__`; result temp; `return`→`goto` target |
| `local_allocated`, `destructible_refs`, `aliases` maps + `lambda_num` | Materialized temps, post-call destruction, aliases, lambda counter |
| `unimplemented_static_interfaces`, `current_func_type`, `current_scope` | Stub list and current context |

### Compiler Pipeline / Lifecycle

The translator is owned by `LabBuildCompiler` and reused across modules: construct
`ToCAstVisitor(...)` + `ToCBackendContext` (installed as `global.backend_context`,
`LabBuildCompiler.cpp:1759`), call `prepare_translate()` (`:3280`) to emit the self-contained
preamble, then per module run `fwd_declare` (`:234`, `:283`), `declare_type_aliases` (`:253`),
`declare_before_translation` (`:201`), `translate_after_declaration` (`:208`),
`file_level_reset()` (`:3407`) and `reset()` (`:3415`). Finally `end_translate()` (`:3398`)
emits stubs for unimplemented static interfaces, and the buffer is written (`--emit-c` /
`-o file.c`) or compiled (`compile_c_to_obj_w_opts`, `LabBuildCompiler.cpp:708`). Generic
instantiations are translated between module file passes (`ASTProcessor.cpp:1697`, `:1705`,
`:1745`), then `clear_current_module_instantiations()` (`:1769`).

### `BufferedWriter`

`BufferedWriter` (`preprocess/2c/BufferedWriter.h`) is a fast append-only byte buffer:
default 4 MiB, doubling growth (`reserve_total` / `ensure_total_capacity_for`), with
`append`/`append_char`/`append_unsigned_u64`/`append_signed_i64`/`append_float`
(`to_chars` + forced `.0f`)/`append_double`/`append_file` and `operator<<` overloads.
`move_range` shifts a region in place (no realloc) for incremental/single-file merges;
`finalize()` appends `\0`, `un_finalize_unsafely()` removes it, `flush_to_file` writes in one
`fwrite`. `ScratchString<N>` (`:455`) is a stack-buffer subclass convertible to
`chem::string_view` / `std::string_view` — used for mangled names (`ScratchString<128>`).

### Name Generation (C identifiers)

Two distinct layers:

1. **Runtime symbol names** are produced by `NameMangler` and written through
   `ToCAstVisitor::mangle(node)` / `mangle(func)` (`2cASTVisitor.h:226`). See the
   [Name Mangling skill](../name_mangling/SKILL.md) for the full algorithm. In short:
   - top-level names get the module prefix `<scope>_<module>_`,
   - nested names concatenate the container path with **no separator** (`geoPointsum`),
   - generic functions append `__cfg_N`, generic containers `__cgs__N`,
   - `@extern`/`@no_mangle` emit the bare name.
2. **Synthesized C identifiers** always use `__chx__` to avoid user collisions:
`__chx__lv__<N>` local temps (`:3434`), `__chx__` escaping a C-keyword name (`:465`),
`__chx_struct_ret_param_xx` / `this` sret / ctor self (`:372`, `:510`),
`__chx_interface_self` (`:373`), `__chx__vt_621827` variant discriminator (`:1408`),
`__chx__dstctr_clnup_blk__` destructor cleanup target (`:4541`),
`continue_<encoded_location>` `for-in` labels (`:3505`), `__chemda_<rand>_<N>` lambdas
(`:2396`), `__chemical_fat_pointer__` (`:3266`) and `__chx_<mangled>_vt_t` (`:3024`).
`write_c_id` (`:465`) checks `is_c_keyword` (`:386`): a Chemical variable named `int`
becomes `__chx__int` in **both** its declaration and every reference.

### Forward Declarations

Before bodies are translated, the declaration pass emits everything else must see:
`fwd_declare(ASTNode*)` (`:234`) emits `struct`/`union <mangled>;` (recursing into
namespaces); `fwd_declare(BaseType*)` (`:283`) walks pointer/reference/linked/dynamic/array/
generic types. `CTopLevelDeclarationVisitor` then emits full definitions, aliases, and
prototypes; `declare_struct_def_only` (`:2844`) calls `early_declare_composed_variables`
first so member types from other modules are declared before this struct (avoids
"incomplete type"). `early_declare_*_def` guard on `has_declared` so each type is emitted
once. Nested `impl` blocks are not top-level nodes, so `VisitStructDecl` walks
`def->evaluated_nodes()` and calls `VisitImplDecl` (`:2937`).

### Include Management / Self-contained preamble

The generated C does **not** `#include <stdint.h>` / `<stdbool.h>` / `<stddef.h>`.
`prepare_translate()` (`:3280`) emits a self-contained preamble instead:

- `bool`/`true`/`false` macros (`_Bool`/`1`/`0`, guarded by `__cplusplus`), `NULL`, and an
  `offsetof` macro implemented without `size_t`.
- Fixed-width integer typedefs matching the target ABI (`long`/`unsigned long` on LP64,
  otherwise `long long`), selected from `comptime_scope.target_data.is64Bit` / `.win64`.
- The `__chemical_fat_pointer__` typedef (`declare_fat_pointer`, `:3265`) and
  `extern` declarations for `malloc`/`free`.
- `__chem_stdcall`/`__chem_dllimport`, `__chx_align(N)`/`__chx_align_struct(N)`,
  `__chx_thread_local` portability macros and `#pragma` suppression of
  `-Wincompatible-pointer-types`.

C headers imported via `cstd`/`.h` bindings are handled earlier by `CTranslator` and are
not re-included here; the preamble plus the translated typedefs make the output standalone.

## Key Translation Patterns

### 1. Struct Translation

Chemical structs become C structs:

```chemical
struct Point {
    var x : int
    var y : int
}
```

```c
typedef struct Point {
    int32_t x;
    int32_t y;
} Point;
```

Structs with a required alignment emit `__chx_align_struct(N) <mangled> { ... };`
(`declare_struct_def_only`, `:2851`). Inheritance is emitted as an embedded member named
after the base (`:2861`). Member functions are emitted **after** the struct body by the
body pass via `visit_struct_contained_nodes` (`:4853`).

**Struct methods** become C functions with the `self` pointer as the first parameter:

```chemical
func p.sum(&self) : int {
    return self.x + self.y;
}
```

```c
int32_t Point_sum(Point* self) {
    return self->x + self->y;
}
```

> Because mangling concatenates with no separator, the real symbol is `main_Pointsum`
> (module + `Point` + `sum`), not `Point_sum`. See `name_mangling` for why.

### 2. Function Translation

Chemical functions become C functions with name mangling applied:

| Chemical Function | C Name |
|-------------------|--------|
| `func foo()` in module `main` | `main_foo` |
| `func Namespace::foo()` in module `main` | `main_Namespacefoo` (containers are concatenated with **no** `_` separator) |
| `generic func foo<T>()` | `main_foo__cfg_N` (generic function instance) |
| `struct Box<T>` | `main_Box__cgs__N` (generic container) |
| `@extern func printf()` | `printf` (no mangling at all) |
| `@no_mangle func bar()` | `bar` (mangled to nothing) |

Top-level functions come from `func_decl_with_name` (`:4318`); methods from
`contained_func_decl` (`:4506`); only functions with a body and not `comptime` are emitted.
`@inline`/`@noinline` drive `write_function_attrs` (`:595`); `extern` is written first, then
`static` for non-public linkage (`:616`). Functions returning function pointers use the
declarator syntax from `func_that_returns_func_proto` / `func_ret_func_proto_after_l_paren`
(`:2487`, `:2477`).

### 3. Struct-Returning Functions

Functions returning structs use the sret (struct return) pattern via compound expressions.
When `pass_structs_to_initialize` is on (default) and the return type is struct-like
(`struct`/`variant`/`union`), the return type is rewritten to `void` and a hidden pointer is
prepended (`accept_func_return` `:570`, `func_type_params` `:532`):

```c
// Chemical:
func create_point(x : int, y : int) : Point { ... }

// C — hidden sret pointer:
void create_point(Point* __chx_struct_ret_param_xx, int32_t x, int32_t y);

// Using the result in an expression:
(*({ struct Point __chx__lv__0; create_point(&__chx__lv__0, 1, 2); &__chx__lv__0; }))
```

Constructors differ: the hidden parameter is `struct <Parent>* this` instead of
`__chx_struct_ret_param_xx` (`write_struct_return_param`, `:510`).

The full in-depth treatment (including argument-destruction deps and captured/`std::function`
returns) is in [The Struct-Return Compound-Expression Pattern](#the-struct-return-compound-expression-pattern).

**Warning**: When the result is discarded (expression-statement context), `gcc -Wall` emits `-Wunused-value` on the `*` dereference. This is a pre-existing pattern throughout generated C.

### 4. Method Chains and `&self`

In a method chain like `a.b().c()`:

1. `b()` is called on `a` — generates receiver variable
2. `c()` is called on the result of `b()` — if `c()` has no `&self` parameter, **do not** create a receiver variable

**Before the fix**: `ToCAstVisitor` created an unused `struct Type* __chx__recv__N` variable, producing "unused variable" warnings.

**After the fix**: Check if the called function has a `self` parameter before creating the receiver variable.

**Actual fix location:** `access_chain()` in `preprocess/2c/2cASTVisitor.cpp:5571`, at the
branch `:5590`:

```cpp
if(!func_decl->has_self_param() && first->val_kind() == ValueKind::FunctionCall) {
    // calling a function on a returned struct, but the function doesn't take
    // &self, the struct expression still needs to be evaluated for side effects
    // generate: ({ func_call(); mangled_name; })
    visitor.write("({ ");
    accept_opt_nestable(visitor, first, false);
    visitor.write("; ");
    visitor.mangle(linked);
    visitor.write("; })");
} else {
    visitor.mangle(linked);
}
```

So a no-`&self` method on a returned struct is **not** given a receiver; only the receiver
expression's side effects are preserved. Extension functions take their receiver as
`params[0]`, handled by `write_implicit_args` / `write_self_arg` (`:1679`). See the full
section below.

### 5. Pointer and Reference Translation

| Chemical | C |
|----------|---|---|
| `*int` | `int32_t*` |
| `*mut int` | `int32_t*` (no const) |
| `*char` | `const char*` |
| `&int` (ref) | `int32_t*` (same as pointer) |
| `[10]int` | `int32_t[10]` |

Precise rules from the type visitors:

- `VisitPointerType` (`:7888`): `const ` when `!is_mutable`, then the pointee, then `*`.
- `VisitReferenceType` (`:7901`): pointee then `*` when mutable else `*const`.
- `VisitStringType` (`:7986`): `char*`.
- `VisitArrayType` (`:7760`): the element type then `[N]` **only when
  `array_types_as_subscript` is true**; otherwise the array decays to `*` (the default).
- `VisitVoidType` (`:8018`): `void`; `VisitAnyType` (`:7738`) also emits `void`.

### 6. Variant Translation

Variants become a struct with a discriminator and union:

```chemical
variant Option<int> {
    Some(value : int)
    None()
}
```

```c
struct Option__cgs__N {          // N = generic instantiation id
    int __chx__vt_621827;        // discriminator (0-based case index)
    union {
        struct { int32_t value; } Some;
        struct { } None;
    };                           // anonymous union, not a named `__data` member
};
```

`CTopLevelDeclarationVisitor::declare_variant_def_only` (`:2951`) emits inherited structs,
then `int __chx__vt_621827;`, then `union { struct { fields } Member; ... };`.
Discriminator reads/writes use `.__chx__vt_621827` (`do_patt_mat_expr_cond` `:4614`,
`VisitIsValue` `:4960`, `write_switch_expr` `:5114`). Constructing a member uses a compound
literal `(struct V) { .__chx__vt_621827 = idx, ... }` (`write_variant_call`, `:1746`).

### 7. Name Generation

- **C keyword escaping**: `write_c_id` prefixes `__chx__` (e.g. a Chemical name that is a C keyword becomes `__chx__int`); C keywords that are also Chemical keywords are included (`2cASTVisitor.cpp:465`, list at `:386`)
- **Temp variables**: `__chx__lv__N` from `get_local_temp_var_name()` (`2cASTVisitor.cpp:3434`)
- **Struct-return parameter**: `__chx_struct_ret_param_xx` for sret pointers; constructors use `this` (`2cASTVisitor.cpp:372`, `2cASTVisitor.cpp:510`)
- **Variant discriminator**: `__chx__vt_621827` (`2cASTVisitor.cpp:1408`)
- **Destructor cleanup block**: `__chx__dstctr_clnup_blk__` (`2cASTVisitor.cpp:4541`)
- **Loop continue labels**: `continue_<encoded_location>` for `for-in` loops (`2cASTVisitor.cpp:3505`)

### 8. Control Flow Translation

| Construct | C Pattern |
|-----------|-----------|
| `if/else` | `if(...) { ... } else { ... }` |
| `while` | `while(...) { ... }` |
| `do-while` | `do { ... } while(...)` |
| `for` | `for(...; ...; ...) { ... }` |
| `switch` | `switch(...) { case ...: ... }` |
| `break` | `break;` after destroying in-loop locals |
| `continue` | `continue;` (inside `for-in`: `goto continue_<encoded_location>;`) |
| Defer | N/A (no defer in Chemical) |

**Break/continue implementation**: `break` first runs `destruct_till_loop_scope_above()` to destroy locals created inside the loop, then emits a plain C `break;`. `continue` emits a plain C `continue;`, except inside a `for-in` loop where the lowered loop body needs a `goto` to a unique `continue_<encoded_location>` label (`2cASTVisitor.cpp:3468`, `2cASTVisitor.cpp:3501`).

### 9. Async Functions / Coroutines (2c)

`async func f(...) : T` is lowered in `preprocess/2c/2cASTVisitor.cpp` into a
hand-written C state machine (no `llvm.coro.*`; this is the reference lowering).
The compiler only does this when symres wrapped the return type to
`FutureHandle<T>` (see the `symres` skill) and `lowers_async()` includes `"C"`.

Generated per async function:

- a **frame struct** `struct <name>__chx_frame { uint32_t __state; T __result; ... }`
  plus a **frame-resident drop flag per destructible local**
  (`uint8_t __chx_drop_<id>`) so a moved-then-cancelled local drops once;
- a **ramp** that allocates the frame (`chemical_async_frame_alloc`), runs the
  body, and returns `FutureHandle<T>{ frame, &vtbl }`;
- a **poll** function that resumes at the saved state and returns
  `Poll.Ready(frame->__result)` when done;
- a **drop** function that runs the cancellation switch (live-local
  destructors) and frees the frame;
- a **static `FutureTable<T>`** whose `poll`/`drop` fields point at those
  concrete functions.

Key mechanics:

- `writeReturnStmtFor` redirects the body's `return e` (via `async_ramp_body`) to
  `frame->__result = e; goto <done>`, so the body is emitted once.
- `await e` (`VisitAwaitExpression`) drives the child handle's `poll` inline;
  on `Pending` it saves the state, stores the child in a frame slot, and
  `return`s to the caller (the executor re-polls). Await sites become
  `switch`/`goto` suspension points.
- Cancelling a suspended frame drops live locals **and** the in-flight child
  future (`__chx_child_i`), then frees the frame.
- A no-await async function takes an **eager fast path** (an always-`Ready` poll).
- The frame's protocol type names are resolved structurally from the wrapped
  return type (`resolve_async_c_types_from_handle`), so no mangled CI name is
  hard-coded.
- Async **closures** are *not* lowered: symres emits
  `async closures are not yet supported; use a named async func instead`.

Known 2c limit: reading a field of a **struct-typed parameter** inside an async
body emits `frame->slot->field` where the slot is stored by value (B24) — pass a
pointer/int handle instead.

> **Pending 2c async fixes (worked around, not fixed):** B24 (struct-typed async
> parameter field access) and async-closure lowering (currently diagnosed). See
> `lang/docs/async-remaining-work.md` for the actionable worklist.

## Translation Reference by AST Category

### Structs, Unions, Variants

- **Struct**: `declare_struct_def_only` (`:2844`) → `struct <mangled> { <base members>; <fields>; };`. Alignment via `__chx_align_struct(N)`. Member functions are emitted later by `visit_struct_contained_nodes` (`:4853`), which routes `FunctionDecl` children through `contained_func_decl` (`:4506`). `VisitStructDecl` (`:4866`) is the body pass.
- **Union**: `declare_union_def_only` (`:2894`) → `union <mangled> { ... };`. Same member-function handling.
- **Variant**: `declare_variant_def_only` (`:2951`) → `struct <mangled> { int __chx__vt_621827; union { struct {...} Case; ... }; };`. Payload access is `Member.field`; the discriminant is checked with `.__chx__vt_621827 == idx` (`is`), `switch`, or pattern-match lowering.
- **Anonymous struct/union**: `VisitStructType` (`:7990`) / `VisitUnionType` (`:8004`) emit inline `struct Name { ... }`; `VisitUnnamedStruct` (`:4836`) / `VisitUnnamedUnion` (`:4821`).

### Functions, Methods, and Generics

Top-level functions go through `func_decl_with_name` (`:4318`): prototype
`declare_func_with_return` (`:2497`) + `scope(...)` body, skipped when `comptime` or bodyless.
Methods/interface impls go through `contained_func_decl` (`:4506`): `static <ret>
<mangled>(...) { ... }`, opening the destructor cleanup block for `@delete` and
default-initializing members/bases for generated constructors (`:4448`). Extension functions
take the receiver as `params[0]`, prepended at call sites by `write_implicit_args` (`:1679`);
functions returning function pointers use `func_that_returns_func_proto` (`:2487`); `@extern`
gets `extern ` and no body (`:617`).

Generics never reach the wire by their generic name: `GenericInstantiator` creates one
concrete declaration per instantiation and `NameMangler` names it (`__cgs__N` containers,
`__cfg_N` functions). `VisitGenericType` (`:7817`) visits the instantiated `referenced` type;
`VisitLinkedType` resolves a `GenericTypeParam` to its specialized type, erroring with
`[GENERIC_TYPE_PARAMETER_NOT_SPECIALIZED_COMPILER_BUG]` otherwise (`:7954`). See
[Generics](../generics/SKILL.md).

### Arrays and Strings

Arrays are emitted as pointers by default (`VisitArrayType`, `:7760`); dimensions are written
by `write_type_post_id` (`:356`) only in subscript contexts (casts, `sizeof`, `zeroed`).
`sizeof` flips `array_types_as_subscript` around the type visit (`emit_sizeof_of_type`,
`:5217`); `zeroed` for an array becomes `{0}` because TinyCC rejects array compound literals
(`VisitZeroedValue`, `:8030`). `string`/`*char` are `char*` (`:7986`); literals are written by
`write_str_value` (`:3441`) with full C escaping via `write_escape_encoded` (`:155`).

### Control Flow, Loops, and Value Forms

Statements are emitted by `VisitIfStmt` (`:4621`), `VisitWhileLoopStmt` (`:5283`),
`VisitDoWhileLoopStmt` (`:3742`), `VisitForLoopStmt` (`:3754`), `VisitForInLoopStmt`
(`:3781`), `VisitSwitchStmt` (`:6409`) and `VisitLoopBlock`.

Value forms (a construct used as an expression) use GNU statement expressions and the
`current_assignable` temp: `writeIfStmtValue` (`:5041`) → nested ternary;
`writeSwitchStmtValue` (`:5122`) and `writeLoopStmtValue` (`:5191`) → `({ T r; ...; r; })`.
The `Visit...Value` wrappers dispatch into these (`VisitIfValue` `:5088`,
`VisitSwitchValue` `:5187`, `VisitLoopValue` `:5213`).

`loop_scope(...)` sets `destructor.loop_job_begin_index` to the current job size (`:1820`)
and restores it; `destruct_till_loop_scope_above()` destroys down to that index (`:3650`).
`for-in` bodies are lowered so `continue` can `goto continue_<encoded_location>` (`:3501`).

### Destructors ("defer"-like cleanup)

Chemical has no `defer`; RAII is expressed with `@delete`. Declaring/initializing a
destructible value queues a `DestructionJob` (`:3460`); `visit_scope` (`:4788`) and
`visit_value_scope` (`:4747`) discharge them at scope exit (`dispatch_jobs_from_no_clean`).
`break`/`continue` call `destruct_till_loop_scope_above` (`:3468`, `:3501`); `return` calls
`destruct_scopes_above` via `writeReturnStmtFor` (`:3638`, `:3654`). A `@delete` body uses a
`goto` cleanup target (`return_redirect_block = "__chx__dstctr_clnup_blk__"`) that calls
member/base destructors (`:4537`, `:4431`, `:4404`); `delete`/`dealloc` lower to
`VisitDeleteStmt` (`:4884`) and `VisitDeallocStmt` (`:4936`).

### Lambdas / Capturing Functions

`write_lambda_function` (`:2396`) emits `__chemda_<rand>_<N>`, an optional
`struct <name>_cap { captures... };` + `_cap_destr`, then a static function taking the
capture struct (`void* this` when capturing). Capturing function *types* are the two-pointer
`__chemical_fat_pointer__` (`:7804`, `:7813`); calls are lowered by
`write_capturing_function_call` (`:5996`) via `get_fn_ptr` / `get_data_ptr`.

### Pattern Matching and `is` / `in`

- `is` on a variant member → discriminant comparison `(expr.__chx__vt_621827 ==/<!= idx)`
  (`VisitIsValue`, `:4942`).
- `is` against a type that has **no linked declaration** (a function type, a pointer, a
  primitive) is false, and true when negated. Both generators read the discriminant of the
  checked type's declaration, so they must not be reached with a null node:
  `IsValue::get_comp_time_result()` (`ast/values/IsValue.cpp`) settles those at compile time
  (`is_negating`), and `VisitIsValue` / `IsValue::llvm_value` also null-check `linked` rather
  than segfaulting on `linked->kind()`.
- `if(cond is Member(v))` materializes a pointer to the tested value (`do_patt_mat_expr`,
  `:4588`), then tests the discriminant (`:4605`).
- `switch(expr)` on a variant switches on `.__chx__vt_621827` (`write_switch_expr`, `:5096`).
- `x in Variant { ... }` → a `switch` inside `({ ... })` (`VisitInValue`, `:4974`).

## The Struct-Return Compound-Expression Pattern

Because C cannot return a struct-value expression inline without a temporary, 2c uses GNU
compound statement expressions plus a hidden sret parameter (expands gotcha #1). With
`pass_structs_to_initialize == true` and a struct-like return type:

- **Prototype/definition**: return type becomes `void`; the *first* parameter is
  `<ReturnType>* __chx_struct_ret_param_xx` (or `struct <Parent>* this` for constructors) —
  `write_struct_return_param` (`:510`), `func_type_params` (`:532`).
- **Call site** (`writeStructReturningFunctionCall`, `:5954`):

```c
(*({ struct Point __chx__lv__0; create_point(&__chx__lv__0, x, y); &__chx__lv__0; }))
```

The expression yields the `Point` value. If the call already owns a temp (tracked in
`local_allocated`, e.g. an outer initializer), that name is reused (`:5963`). Captured /
`std::function` returns use the same pattern with a `__chemical_fat_pointer__` temp
(`write_capturing_function_call` `:5996`, `struct_returning_capture_call` `:6066`) and go
through `get_fn_ptr(instance)` / `get_data_ptr(instance)`. Calls whose arguments include
temporaries needing destruction are wrapped by `calculate_arg_destruction_deps` (`:5909`) /
`write_destruct_vars_for_deps` (`:5926`):

```c
({ struct Arg __chx__lv__0; func(&__chx__lv__0, args); Arg_destroy(&__chx__lv__0); <ret>; })
```

This is what makes `make_temp().to_view()` and reference-of-temporary arguments correct.

## Method Chains and `&self` Receiver Handling

`access_chain` (`:5571`) lowers `a.b.c().d()` step by step: `chain_value_accept` (`:5416`)
writes each identifier (resolving struct-member paths via `write_path_to_member`),
`write_accessor` writes `.`/`->` between steps, and a chain starting with a call returning a
destructible struct is wrapped `({ T* tmp = &call(); U saved = tmp->rest; destroy(tmp); saved; })`
(`write_destructible_call_chain_values`, `:5482`).

**The `&self` fix** (`:5590`): when the final member is a `FunctionDecl` **without** a
`self` parameter and the receiver is a struct-returning function call, the receiver is still
evaluated for side effects but no receiver variable is created:
`({ func_call(); mangled_name; })`. If the method does have `self`, the receiver chain is
written normally and the implicit `self` argument is added at the call site by
`write_implicit_args` (`:1679`); extension functions (`isExtensionFn()`) are treated like
methods.

### `(*ptr).member` — an explicit dereference followed by a member access

The parser turns `(*ptr).member` into `AccessChain[ DereferenceValue(ptr), member, ... ]`
(`parseAccessChainAfterValue` in `parser/utils/Expression.cpp`): the dereference is the chain's
*first element*, not a wrapper around the member access.

`write_accessor` sees that element as pointer-like (its `linked_node()` delegates to the inner
value) and writes `->`. So `chain_value_accept` **must not** also write the dereference's own
`*`: `*ptr->member` is read by C as `*(ptr->member)` and fails to compile whenever the member
is not itself a pointer (`pointer expected`).

The rule (in `chain_value_accept`, via the `chain_accessor` decision that also backs
`write_accessor`):

| accessor that follows | emitted |
|-----------------------|---------|
| `->` | the pointer itself — the accessor performs the dereference: `ptr->member` |
| `.`  | `(*ptr)` — parenthesized, so the dereference binds tighter: `(*ptr).member` |

The `*` is still written when the dereference is *not* followed by an accessor (`*ptr` as a
value) and when it wraps a whole chain (`*(ptr->field)`), so the special case only applies to a
`DereferenceValue` chain element that has a `next` element.
Regression tests: `lang/tests/common/src/references/deref_member.ch`.

## Type Translation Table

From the `Visit*Type` methods in `2cASTVisitor.cpp`:

| Chemical type | C emitter | Source |
|---------------|-----------|--------|
| `i8/16/32/64`, `u8/16/32/64`, `int`/`uint`, `long`/`ulong` | `int8_t`…`uint64_t`, `int`/`unsigned int`, `long`/`unsigned long` | `VisitIntNType`, `:7822` |
| `char`/`short`/`long long`/`uchar`… | `char`/`short`/`long long`/`unsigned …` | `:7822` |
| `i128`/`u128` | `__int128`/`unsigned __int128` | `:7836` |
| `bool` | `_Bool` (or `bool` when `cpp_like`) | `VisitBoolType`, `:7775` |
| `float`/`double`/`long double`/`f128` | `float`/`double`/`long double`/`__float128` | `:7783`, `:7787`, `:7795`, `:7791` |
| complex | `_Complex <elem>` | `:7799` |
| `void` / `any` | `void` | `:8018`, `VisitAnyType`, `:7738` |
| `*T` | `const T*` (immutable) / `T*` | `VisitPointerType`, `:7888` |
| `&T` | `T*` / `T*const` | `VisitReferenceType`, `:7901` |
| `string` | `char*` | `VisitStringType`, `:7986` |
| `[N]T` | `T*` (decay, default) or `T[N]` when subscripted | `VisitArrayType`, `:7760` |
| `struct S` / `variant V` / `union U` | `struct <mangled>` / `struct <mangled>` / `union <mangled>` | `VisitLinkedType`, `:7945`, `:7950` |
| enum | underlying integer type | `:7938` |
| interface | `void*` (unbound) or `struct <impl>` (active impl) | `:7927`, `:7923` |
| `dyn Interface` / capturing fn | `__chemical_fat_pointer__` (+ `*` for capturing) | `VisitDynamicType`, `:7982`, `:7806` |
| function type | `<ret>(*)(<params>)` | `VisitFunctionType`, `:7804` |
| function type that returns a function type | nested declarator, see below | `func_type_with_id` |
| alias / opaque | resolved alias / `<mangled>` fallback | `:7964`, `:7975` |

### Nested function declarators (function returning a function)

A function (or function pointer) whose *return type* is itself a function type cannot be
written by putting the two declarators next to each other — C puts the name in the innermost
group, so each level of function return types wraps the whole declarator and the final return
type ends up in front:

```c
int(*(*fn)(int(*p)(int x)))(int x)   // fn : (p : (x:int)=>int) => (x:int)=>int
```

Writing the return type as an id-less prototype instead produces an invalid declaration
(`int(*)(int x)(*fn)(...)`), which is what `func_type_with_id` used to emit for every such
declaration — TinyCC then fails with `error: identifier expected` or
`incompatible types for redefinition`.

Because the base return type goes at the *front* but is only known after walking the chain,
the declarator is written into the buffer and parts are moved with
`BufferedWriter::move_range(fromStart, fromEnd, index)`:

- `wrap_nested_func_return(visitor, ret_type, decl_start)` — the single implementation. Walks
  the function return types, prepending `(*` at `decl_start` and appending `)(<params>)` for
  each level, then writes the final return type and moves it in front of `decl_start`.
- `func_type_with_id` calls it with `type->returnType` when the innermost group
  `(*id)(<params>)` has been written (variables, parameters, struct members, array elements).
- `func_that_returns_func_proto` calls it with `retFunc->returnType` —
  `func_ret_func_proto_after_l_paren` already covers the declared function's own list plus one
  level of its return type.

Any change here must keep the `k = 1` case byte-identical (`int(*fn)(<params>)`): it is the
path every ordinary function-typed variable takes.

### Types that resolve to a function declaration

`FunctionDeclaration` **is a** `FunctionType` (`FunctionTypeBody → FunctionType → BaseType`)
and `known_type()` returns `this`, so a generic function instantiation used as a type
argument (`apply<int>`) has a type whose declaration is a function. Its *mangled name* denotes
a function and cannot be used where a type is expected (`apply__cfg_0 x` is not a declaration),
so 2c resolves those types through `resolved_function_type`:

- `resolved_function_type(type)` returns the function type for a plain function type, and for
a `LinkedType`/`GenericType` whose linked node is a `FunctionDecl` (the instantiation). It
intentionally does **not** canonicalize through type aliases, so aliases keep their C name.
- Used by `type_with_id` (parameters, uninitialized locals), `func_type_with_id` (nested
returns), `value_init_default`'s `default:` arm (function-typed locals), and
`declare_func_with_return` (a function whose return type is an instantiation).
- `VisitLinkedType`'s `FunctionDecl` case and `VisitGenericType` (when the referenced node has
no members container) fall back to an id-less function declarator for genuinely abstract
positions such as casts.

### Pattern Matching and `is` / `in`

## Inline Assembly, `@extern` / `@no_mangle`, Static & Global Vars

### Inline assembly

`asm("template" : outputs : inputs : clobbers)` lowers via `VisitInlineAsmStmt` (`:3705`)
to GCC extended asm, accepted by GCC/Clang/TinyCC:

```c
__asm__ __volatile__("template" : "=r"(out) : "r"(in) : "cc", "memory");
```

Atomic operations that cannot be expressed in plain C are emitted as CAS loops via inline
asm by `ToCBackendContext` (`2cBackendContext.h`); unsupported atomics report an error
asking for `%runtime_value()` inline asm (`2cASTVisitor.cpp:8142`).

### `@extern` / `@no_mangle` and globals

Both annotations set the `no_mangle` flag; `@extern` additionally marks the node externally
defined. Functions get `extern ` and no body (`accept_func_return_with_name`, `:617`);
structs/interfaces/vars are declared without the module prefix and referenced by their exact
C name (see [Name Mangling](../name_mangling/SKILL.md)). `var_init_top_level` (`:1577`)
emits globals as:

```c
[__chx_thread_local] [static|extern] [volatile] <type> <mangled> = <value>;
```

`static` when not linkage-public, `extern` for `@extern`, `volatile` for `@volatile`,
`__chx_thread_local` for thread-locals; comptime globals are skipped. Local variables go
through `var_init` (`:1598`).

## How TCCCompiler Compiles the Generated C

`compile_c_to_obj_w_opts` (`LabBuildCompiler.cpp:708`) is the single dispatch point.
**Embedded Clang** (`COMPILER_BUILD` and `use_embedded_clang(job)`): write `Translated.c` and
compile via `compile_c_file_to_object(...)` with the bundled Clang and
`options->resources_path` — the `Compiler` target in C-translation mode (`--use-c`).
**TinyCC** (default for `TCCCompiler`, always for JIT/CBI/`ToCTranslation`):
`compile_adding_file(exe, Translated.c, obj, ...)` (`LibTcc.cpp:286`) when `emit_c`, else
`compile_c_string(exe, program, obj, ...)` (`LibTcc.cpp:251`). `use_embedded_clang`
(`LabBuildCompiler.h:328`) is false for TinyCC jobs, for `options->use_tcc`, and without
`COMPILER_BUILD`.

### Include paths, runtime headers, linking

- `tcc_new_state` (`LibTcc.cpp:17`) resolves the TinyCC install relative to the compiler
  executable (`lib/tcc` in DEBUG, `packages/tcc` in release), adding `<tcc>/include` (plus
  `<tcc>/include/winapi` on Windows) and `<tcc>/lib`; `compile_adding_file` also adds each
  module's `include_dirs`.
- The generated C is self-contained (see [Include Management](#include-management--self-contained-preamble)); `malloc`/`free` come from the preamble. JIT runs (`TCC_OUTPUT_MEMORY`) call `prepare_tcc_state_for_jit` (`:214`).
- Multiple objects are linked by `tcc_link_objects` (`LibTcc.cpp:342`); the Clang path uses
  `clang_link_objects` / `lld_link_objects` (`LabBuildCompiler.cpp:2179`).

## Debug / Emit Workflow

The `.c` extension (or `-o file.c`) makes the compiler translate only, writing the C to that
path (`LabJobType::ToCTranslation`). The canonical invocation:

```bash
cmake-build-debug/TCCCompiler "lang/compiled/temp.ch" -o "lang/compiled/temp.c" -v -bm-modules
```

| Output / flag | Behavior |
|---------------|----------|
| `-o file.c` | `LabJobType::ToCTranslation` — translate only, write to the given path |
| `--emit-c` / `--keepc` | Keep generated per-module C (`partial.2c.c`, `partial.2h.c`, `Translated.c`) |
| `--incremental` | Per-file C translation (not merged) |
| `--minify-c` | `minify = true` (no indentation/newlines) |
| `--mode debug_complete` / `-g` | `line_directives` (`#line`) + debug info |
| `--use-tcc` / `--use-c` | TinyCC path / translate-to-C + embedded Clang |
| `.ll` / `.o` / `.bc` / `.s` | LLVM backend outputs (not 2c) |

Internals: each module's C is appended to `partial.2c.c` / `partial.2h.c` and cached; the
final merged program is written to `<build-dir>/Translated.c` (`LabBuildCompiler.cpp:434`,
`:446`, `:1876`). `build.lab` itself is also translated, to `build.lab.c` (`:3088`).

## Codegen State Management

### ToCBackendContext

`ToCBackendContext : public BackendContext` (`2cBackendContext.h`) holds a `ToCAstVisitor*`
and implements `name()` ("C"), `emit`, `forget`, `mem_copy`, `supports`, `destruct_call_site`
(calls `visitor->destruct_scopes_above(nullptr)`), and the `atomic_*` / `signal_fence`
helpers. It is installed on `GlobalInterpretScope::backend_context` so macros, comptime
intrinsics, and the build system can emit text and query `supports(CompilerFeatureKind::InlineAsm)`.

### Scoping

Scope is tracked for unique variable names, `for-in` continue labels, nested/anonymous
struct names, and mangled function names. Concretely `current_scope` tracks the AST scope,
`destructor.destruct_jobs` is the stack of pending destructor jobs (discharged by
`visit_scope`), and `local_allocated` maps already-materialized values to their C temp names.

## Gotchas

### 1. Compound Expression Wrapping

Functions returning structs use the `(*({ ... }))` pattern. When the result is discarded:

```c
// Generated when discarding a struct return:
(*({ struct Type __tmp; func(&__tmp, args...); &__tmp; }));
// gcc -Wall: warning: unused value
```

**Correction:** The current code does **not** wrap discarded results in `(void)`. The
`-Wunused-value` warning is a known, benign artifact only under `gcc -Wall`; the TinyCC and
Clang paths used by the compiler do not treat it as an error. If you need a warning-free C
output, change the discard path in `VisitValueWrapper` / `visit_wrapped_value`
(`2cASTVisitor.cpp:7628`) or wrap the compound expression in `(void)`.

### 2. Bool-to-Int Promotion

Chemical `bool` is `i1` in LLVM but `int` in C. The codegen must:

```c
// Chemical bool:
bool flag = true;

// C translation:
int32_t flag = 1;  // bool promoted to int32_t
```

Precisely: `VisitBoolType` emits `_Bool` (or `bool` with `cpp_like`), and
`prepare_translate` defines `bool`/`true`/`false` macros so the output stays self-contained.
Logical results are written as `1`/`0` (e.g. `VisitIsValue`, `:4971`).

### 3. Zero-Length Arrays

Chemical allows zero-length arrays. In C, this requires flexible array member or zero-length array extension:

```c
// Chemical: var arr : [0]int
// C (GCC extension): int32_t arr[0];
```

### 4. Variadic Function Declarations

Chemical does NOT support variadic function declarations for user-defined functions. Only `@extern` C functions use variadic args:

```c
// Chemical:
@extern func printf(format : *char, ...) : int

// C:
extern int printf(const char* format, ...);
```

### 5. Anonymous Scopes

Chemical supports bare `{ }` as a scope expression. These must generate a compound statement:

```c
// Chemical:
var x = {
    var tmp = 42;
    tmp
};

// C:
int32_t x = ({ int32_t tmp = 42; tmp; });
```

### 6. Hexadecimal Float Literals

Chemical supports hex float literals. C99+ supports `0x` prefix for floats:

```c
// Chemical: 0x1.921fb6p+1f
// C: 0x1.921fb6p+1f
```

### 7. `loop` Value Must Emit with `loop_scope`

A `loop { ... break }` value is lowered inside a compound expression. Its body must be emitted with `loop_scope` (not plain `scope`) so that `loop_job_begin_index` is set and `break`/`continue` destroy only the jobs created *inside* the loop body. Using `scope` left that index stale, so `destruct_till_loop_scope_above()` destroyed every live local/parameter in the enclosing function on each `break`/`continue` (and again at scope exit) — see `writeLoopStmtValue` (`2cASTVisitor.cpp:5191`).

### 8. `impl` Blocks Nested Inside a Struct Need Explicit Declaration

`impl Interface for T` blocks written inside a struct body are not top-level nodes, so the top-level declaration pass never reaches them. `CTopLevelDeclarationVisitor::VisitStructDecl` walks `def->evaluated_nodes()` and calls `VisitImplDecl` for each nested `ImplDecl`, emitting prototypes before any caller (e.g. a generic instantiation) references them (`2cASTVisitor.cpp:2937`).

### 9. Compound-Literal Lifetime and Unused Temps

The `(*({ ... }))` result is a **value**, not an address: the underlying `__chx__lv__N`
temporary lives only to the end of the enclosing full expression, so never take `&` of the
whole expression or store it. When a value must outlive the statement, the translator first
allocates a named temp via `local_allocated` / `allocate_struct_by_name_no_init` (`:672`)
and reuses it (`:5963`). `__chx__lv__`/sret/capture temps can also appear unused on some
control-flow paths; `-Wall` may report them, but they are benign.

### 10. Arrays Default to Pointers

`array_types_as_subscript` is `false` by default, so `[N]T` in a normal type position decays
to `T*`. If a declaration needs a real C array, the emitter sets
`array_types_as_subscript = true` around the visit (see `emit_sizeof_of_type`, `:5217`, and
`VisitZeroedValue`, `:8034`). Forgetting this produces pointer-to-pointer confusion.

### 11. Forward-Declaration Ordering and `return_redirect_block`

Struct member types from other modules must be declared before the struct body:
`declare_struct_def_only` calls `early_declare_composed_variables` (`:2848`) and
`early_declare_container` for variants (`:2952`). Wire any new container-like node into the
early-declaration pass or C rejects the incomplete type. Separately, returns inside a
destructor body are rewritten to `goto __chx__dstctr_clnup_blk__` (`:4542`); constructs that
emit a `return` while `return_redirect_block` is set must save/restore it.

## Performance Optimization

The `BufferedWriter` (4 MiB default, doubling) minimizes write calls; the preamble is fixed
and self-contained (no per-file header churn); comptime evaluation removes dead branches;
and `has_declared` guards emit each struct/union/variant once. The backend is currently
serial within a job. Per-function and per-module codegen are independent and are natural
parallelization targets, but the shared `BufferedWriter` and `NameMangler` serialize output
today. The visitor is already reused across modules (`reset()`) and frees file-level AST
memory per file (`file_level_reset()`).

## Debugging C Output

### Common C Compiler Warnings from Generated Code

| Warning | Cause | Fix |
|---------|-------|-----|
| `unused variable` | Unused receiver for method chain without `&self` | Check receiver requirement in `access_chain` (`:5590`) |
| `unused value` | Discarded struct-returning expression | Benign; optionally wrap in `(void)` |
| `incompatible pointer types` | Wrong type in a cast | Check type conversion in codegen |
| `implicit declaration` | Missing forward declaration | Declare before use or use extern |

### Reading the output

Add `--minify-c` to inspect tokenized output and `--emit-c` to keep the per-module
`partial.2c.c` / `Translated.c` files.

- `#line` directives appear only when `line_directives` is on (`-g` / debug mode).
- `__chx__lv__N` counters reset via `reset()`; `__chx__vt_621827` is intentionally greppable.
- Run `gcc -fsyntax-only -Wall` on the emitted `Translated.c` for a fast sanity check.

## Extending the 2c Backend

When adding a new AST node to the 2c backend:

1. **Add a visitor method** in `2cASTVisitor.cpp`
2. **Declare the visitor** in `2cASTVisitor.h`
3. **Add any new helper** in `CDestructionVisitor.h` or `CTopLevelDeclVisitor.h` if needed
4. **Register in `preprocess/visitors/NonRecursiveVisitor.h`** — add a default `VisitXxx` method and a `case` in `VisitNodeNoNullCheck` / `VisitValueNoNullCheck`.
5. **Wire declaration passes** for runtime types/symbols: `fwd_declare` / `declare_type_aliases` / `CTopLevelDeclarationVisitor`.

> ⚠️ If you add a CBI-exposed enum value (`ASTNodeKind`, `TokenType`), update the matching
> `.ch` binding in `lang/libs/compiler/src/` at the same time — see the `CRITICAL Enum Sync
> Rule` in `AGENTS.md` and the `cbi_plugin_api` skill. A mismatch shifts every subsequent
> value and SIGSEGVs all CBI plugins.

### Template for a New Visitor

```cpp
void ToCAstVisitor::VisitNewNode(NewNode* node) {
    VisitNode(node->dependency);              // dependencies first
    writer.write("/* chemical new node */");  // generate C
    for(auto child : node->children) VisitNode(child);
}
```

## Cross-Links

- **Naming**: [`name_mangling`](../name_mangling/SKILL.md) — runtime symbols, `__cfg_N`/`__cgs__N`, vtables.
- **Other backend**: [`llvm_backend`](../llvm_backend/SKILL.md) — LLVM lowering, sret, struct assignment.
- **Build & CLI**: [`build_system`](../build_system/SKILL.md), [`cli_entrypoint`](../cli_entrypoint/SKILL.md) — driving 2c, flags, output routing.
- **AST/API**: [`compiler_api`](../compiler_api/SKILL.md) — node/value/type hierarchy, visitor tiers.
- **Generics**: [`generics`](../generics/SKILL.md) — monomorphization and instantiation indices.
- **Plugins**: [`compiler_bindings`](../compiler_bindings/SKILL.md), [`cbi_plugin_api`](../cbi_plugin_api/SKILL.md), [`macro_code_gen`](../macro_code_gen/SKILL.md), [`json_serialization`](../json_serialization/SKILL.md).
- **Testing/interpreter**: [`testing`](../testing/SKILL.md), [`interpreter`](../interpreter/SKILL.md).
