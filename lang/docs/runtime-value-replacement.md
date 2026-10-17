# Replacing `%runtime_value` with comptime parameters

Status: design proposal (Phase 5 enabler)
Motivation: `%runtime_value` / `%runtime_block_value` are the only reason the
AST interpreter is still reachable at lowering time (see
`lang/docs/mir-interpretation-plan.md` §5.4 and the Phase-5 thread). They make a
`comptime func` return an **AST template** instead of a value, which a MIR
execution engine cannot represent. This document audits every use and proposes a
MIR-native replacement that deletes the mechanism — and, with it, the AST
interpreter's last lowering-time job.

> **Implementation status (2026-10).** Step 1 is in the working tree and green:
> `MIRLowerComptime.cpp` inlines a comptime template call instead of interpreting
> it — it evaluates the arguments in the caller's context, swaps to the callee's
> parameter bindings (runtime operands become places, comptime operands fold to
> constants), lowers the body with an inline-return frame, and publishes the
> folded comptime parameter *names* into the interpret scope
> (`MIRLowerer::comptime_binding_names()`) so a nested comptime intrinsic
> (`intrinsics::size`, `satisfies`, `llvm_mem_order`, …) folds with the callee's
> parameters visible. Covered: **free functions and methods** (the
> `atomic.ch` wrappers, `std`'s `append_expr`, the `lab`/`test` helpers). All
> suites green with the hook active and **no fallback** — `MIRTests` 4417,
> `--tcc` 2307/2307, `--llvm` 2305/2305, `--interpret` 1896/1896, `--libs`
> 718/719.
>
> **Still on the interpreter:** **pure comptime functions** (value folding and
> `comptime if`/array-size resolution at symres time). So the interpreter cannot
> be deleted yet; see §5.4 and §8.

> **Migration progress (2026-10).** Source migration is underway, one unit per
> commit, `--tcc`/`--llvm`/`--interpret` green at each step. **All 149**
> `%runtime_value` / `%runtime_block_value` uses are gone — no site remains.
>
> **The explicit replacement is `@expand`** (commit `add @expand: explicit
> compile-time code functions`). A `comptime func` is either *evaluated* to a
> value or *lowered* at its call site; the form did not say which, and inference
> is unsound (it cannot tell `lab.curr_dir` from `comptime_fnv1_hash`). `@expand`
> makes it a declared fact — the compiler never inspects the body:
>
> ```chemical
> @expand
> comptime func curr_dir() : std::string {
>     const loc = intrinsics::get_call_loc(9999)
>     return curr_dir_of(loc, intrinsics::size(loc))   // no %runtime_value
> }
> ```
>
> Migrated: **atomic.ch (120)**, **string_view.make**, **string.make** (both
> overloads), **append_expr**, **test_runner**, **test_runner_handle**,
> **lab.curr_dir / get_my_path / rel_path_to**, **panic**, **stream.print** /
> **println**, **span.make2** (both overloads), **universal_test**'s runners,
> and the constructor fixtures (`comptime/basic`, `functions/constructors`,
> `destructors/basic`, `capturing_call`, `temp_arg_destruct`, `for_in_custom`).
>
> **`@expand` is a *splice*, not a hygienic inline** (commit `mir: @expand
> splices in the caller's scope`): the body is lowered in the **caller's**
> scope, with the callee's bindings layered on top (a journal restores the
> caller's scope exactly). This is what the expressive-string, array-size and
> lambda cases needed — they all resolve in the caller's context.
>
> Compiler work this required:
> 1. raw-kind parameter detection (`BaseType::canonical()` unwraps the markers,
>    so the old check never matched);
> 2. call-value indexing past the receiver slot (`fd->params` includes `self`);
> 3. publishing comptime **locals**, not just parameters;
> 4. the inline establishing the **call site** (call node on the interpreter call
>    stack) so `intrinsics::get_call_loc` resolves to the call, not the macro body;
> 5. a nested-inline **use-after-free** fix — the inline `return` held
>    `ComptimeInlineFrame&` across `lower_expr`, which can reallocate
>    `inline_frames_` and leave the reference dangling;
> 6. **lambda isolation** — a lambda materialized inside a splice inherited the
>    splice's `inline_frames_`/`comptime_bindings_`, so its `return` branched to
>    the *caller's* block and stored into a caller place (an instruction in the
>    lambda's function referencing a place it does not own);
> 7. **a splice is synchronous** — `async_` is cleared for the spliced body, so a
>    `comptime func`'s `return` in an `async` caller is an inline-return, not an
>    async return.
>
> **Done.** The last two were not `%runtime_value` problems:
> - **`function.make`** — a splice is synchronous (`async_ = false` for its
>   body): a `comptime func`'s `return` inside an `async` caller was being
>   treated as an async return, completing the caller on the first poll. With
>   the lambda-isolation fix below, it migrates.
> - **`ASTBuilder.allocate`** — became a **normal generic function** rather
>   than a splice. Its only compile-time needs are the generic `T`,
>   `sizeof`/`alignof` and `get_child_fn<T>`, which all fold in a normal generic
>   function; the comptime splice deadlocked the `--tcc --plugins` build (the
>   allocator's shared mutex in the CBI path). Verified by `--plugins`
>   (1296/1296).

---

## 1. What `%runtime_value` does today

Declared only inside a `comptime func`:

```chemical
comptime func atomic_load_u64(x : %runtime<*u64>, order : memory_order = seq_cst) : u64 {
    comptime if(intrinsics::get_backend_name() == "C") {
        return %runtime_value(__chx__load_u64_dispatch(x)) as u64   // template A
    } else {
        return %runtime_value(intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope)) as u64
    }
}
```

- `%runtime_value(<expr>)` — "the value of this comptime call is the AST
  expression `<expr>`; lower it at the **call site**."
- `%runtime_block_value { <stmts> }` — the same, for a block.
- `%runtime<T>` — "this comptime-func parameter is a **runtime** operand"
  (opt-in); everything else in a `comptime func` is comptime.
- `%maybe_runtime<T>` — "the argument may be comptime or runtime."

Mechanically: the AST interpreter runs the function and returns a `RuntimeValue`
holding `underlying` plus `captured_refs` (the `%runtime` operands, bridging to
the caller's argument expressions). `MIRLowerExpr.cpp` handles it by lowering
`underlying`:

```cpp
case ValueKind::RuntimeValue:
    return lower_expr(static_cast<RuntimeValue*>(value)->underlying, error);
```

So the comptime function's real job is **compile-time code shaping**: fold the
comptime inputs, keep the runtime inputs as operands, and produce a call/block.
That is *exactly* what a normal function with comptime parameters does under MIR
lowering — which is the replacement (§4).

---

## 2. Inventory (verified in the tree)

**148** `%runtime_value` and **1** `%runtime_block_value` uses.

| File | count | category |
|---|---:|---|
| `lang/libs/atomic/src/atomic.ch` | 120 | backend-dependent intrinsic selection |
| `lang/libs/lab/src/lab.ch` | 3 | source-location-driven helper |
| `lang/libs/universal_test/src/runner.ch` | 2 | comptime-collection → runtime runner |
| `lang/libs/test/src/main.ch` | 2 | comptime-collection → runtime runner |
| `lang/libs/std/src/string.ch` | 2 (+1 block) | implicit ctor / expressive-string |
| `lang/libs/std/src/stream.ch` | 2 | expressive-string materialization |
| `lang/libs/std/src/span.ch` | 2 | implicit ctor argument adaptation |
| `lang/libs/compiler/src/ASTBuilder.ch` | 2 | compiler-intrinsic wrapper |
| `lang/libs/std/src/string_view.ch` | 1 | implicit ctor argument adaptation |
| `lang/libs/std/src/panic.ch` | 1 | compiler/runtime support |
| `lang/libs/std/src/function.ch` | 1 | comptime closure construction |
| `lang/tests/...` (5 files) | 5 | tests of the mechanism |
| `lang/libs/atomic/arch/x86/asm.ch` | 1 | comment/reference |

Distinct callees inside `%runtime_value(...)`:

- **`intrinsics::llvm::atomic_*` (72)** and **`__chx__*_dispatch` (24)** — atomics.
- `constructor` / `constructor2` / `make2` / `builder` / `view_of` / `make_test_runner_handle` / `make_universal_runner_handle` / `run_test_runner` / `run_universal_tests` / `raw_panic` / `curr_dir_of` / `appended_str` / `runtime_sum` / `actual` / `first` / `second` — constructor/runner adaptation.
- `intrinsics::expr_str_block_value` (3) — expressive strings.

**Six categories:**

1. **Backend/op selection** (atomics, 120) — pick an intrinsic or a dispatch
   call from comptime flags; splice the runtime pointer/value.
2. **Implicit-constructor adaptation** (`span`, `string_view`, `string`) —
   forward to a runtime `constructor` with adjusted args (`size`, `.data()`).
3. **Expressive-string materialization** (`string.append_expr`, `stream`,
   `string.make(expr)`) — turn a comptime `%expressive_string` into runtime
   appends; the sole `%runtime_block_value`.
4. **Comptime-collection runners** (`test`, `universal_test`) — `get_tests()`
   at comptime, then run a runtime runner.
5. **Source-location helpers** (`lab.curr_dir`, `get_my_path`, `rel_path_to`) —
   `get_call_loc` at comptime, then a runtime string build.
6. **Compiler-intrinsic wrappers** (`ASTBuilder.allocate`, `function.make`,
   `panic`) — fold an intrinsic, then emit a runtime call.

Every one of them is "fold the comptime inputs, keep the runtime inputs, return
a call/block".

---

## 3. Why it is not MIR-friendly

The result is **not a value** — it is an unlowered AST expression. A MIR engine
returns raw bits; it cannot construct a template. That is the architectural
blocker: `%runtime_value` is code generation expressed as a comptime value, so
the AST interpreter (which can return an AST `Value*`) is load-bearing. As long
as the mechanism exists, `compiler/Interpreter/Core.cpp` cannot be deleted.

---

## 4. Proposed replacement: comptime parameters

**Observation:** in all 148 uses, the returned template is a *call/block over
(comptime-folded) constants and (runtime) operands*. Under MIR lowering, that is
a normal function with **comptime parameters**. So:

### 4.1 The new surface

Introduce `comptime` **parameters** — the inverse of `%runtime<T>`:

```chemical
// today (comptime func + template)
comptime func atomic_load_u64(x : %runtime<*u64>, order : memory_order = seq_cst) : u64 {
    comptime if(intrinsics::get_backend_name() == "C") {
        return %runtime_value(__chx__load_u64_dispatch(x)) as u64
    } else {
        return %runtime_value(intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope)) as u64
    }
}

// proposed (normal function + comptime parameter)
func atomic_load_u64(x : *u64, comptime order : memory_order = seq_cst) : u64 {
    comptime if(intrinsics::get_backend_name() == "C") {
        return __chx__load_u64_dispatch(x)
    } else {
        return intrinsics::llvm::atomic_load(x, llvm_mem_order(order), scope)
    }
}
```

The rewrite rules are mechanical:

| today | proposed |
|---|---|
| `comptime func f(...)` with a returning template | `func f(...)` (normal) |
| parameter `p : %runtime<T>` | parameter `p : T` (runtime) |
| parameter `p : T` (comptime in a comptime func) | parameter `comptime p : T` |
| `return %runtime_value(X)` | `return X` |
| `return %runtime_block_value { B }` | the body block `B` |

### 4.2 Semantics: specialize per comptime argument

A function with `comptime` parameters is **lowered to MIR like any other
function**, specialized once per distinct comptime-argument tuple — exactly how
Chemical already specializes generics per type argument. At a call site:

- each **comptime** argument is resolved to a constant (the existing constant
  folder handles literals, `%expressive_string`, and comptime intrinsics);
- each **runtime** argument becomes an ordinary MIR operand;
- the callee is interned as `(decl, comptime-arg-tuple)` and lowered once.

`%maybe_runtime<T>` keeps its meaning but on a plain parameter: a comptime
argument folds, a runtime argument passes through.

### 4.3 Category mapping (worked)

- **Atomics (120):** as above. The MIR lowerer already emits `AtomicLoad` /
  `AtomicStore` / `AtomicCmpXchg` / `AtomicRMW` for `intrinsics::llvm::atomic_*`
  (`MIRLowerCall.cpp::lower_atomic_intrinsic`), and `comptime if` is already
  folded. `order` becomes a comptime parameter; the LLVM path uses the MIR
  atomic opcodes, the C path calls `__chx__*_dispatch`. **120 sites collapse to
  plain functions.**
- **`span` / `string_view` / `string` ctors:**
  `comptime func make(value : %literal_string) { return %runtime_value(constructor(value, size(value))) }`
  → `func make(comptime value : %literal_string) { return constructor(value, size(value)) }`.
- **`string.make(expr)` (the one `%runtime_block_value`):**
  ```chemical
  func make(comptime expr : %expressive_string) {
      var str = std::string()
      str.append_expr(expr)
      return str
  }
  ```
  The function body *is* the block; no `%runtime_block_value`.
- **`test` / `universal_test` runners:**
  `func test_runner(argc : int, argv : **char) : int { const t = get_tests(); return run_test_runner(span(t), argc, argv) }`
  — `get_tests()` is a comptime intrinsic the lowerer folds; `t` becomes a
  compile-time constant; `argc`/`argv` are ordinary runtime params.
- **`lab` helpers / `ASTBuilder.allocate` / `panic`:** identical shape — fold the
  comptime intrinsic, keep the runtime call.

---

## 5. Compiler changes

1. **Parser/AST** — add a `comptime` parameter marker (a `FunctionParam` flag, or
   a `%comptime<T>` type mirroring `%runtime<T>`). During migration,
   `%runtime_value(X)` can parse to plain `X` and `%runtime_block_value { B }` to
   a block, so **existing sources compile unchanged** (a transparent shim).
2. **Symres/typecheck** — comptime params do not take a runtime argument slot;
   the call's comptime arguments are checked as constants (they already are, for
   the current mechanism).
3. **MIRLowerer** — at a call to a function with comptime params:
   - resolve comptime args to constants; intern the specialization by
     `(decl, comptime-arg-tuple)`;
   - lower the callee body once per tuple (the existing generic-instantiation
     flow is the template for this);
   - remove the `fd->is_comptime() → comptime_eval_` branch for such calls.
4. **Remove** `RuntimeValue` / `RuntimeBlockValue` from lowering (and eventually
   from the AST); remove `%runtime`/`%maybe_runtime` **type** wrapping in favour
   of plain parameters plus the `comptime` marker.
5. **Result** — comptime-value specialization is the only genuinely new compiler
   feature. Everything else the replacement needs already exists:
   `comptime if` folding, atomic MIR opcodes, implicit constructors, generic
   specialization, the constant folder.

---

## 6. Performance

- **Zero interpretation.** Comptime parameters fold to constants at the call
  site; the body is ordinary MIR — inlineable and optimizable by the existing
  passes. Today the template is interpreted then re-lowered; the replacement
  skips the interpretation entirely.
- **Specialization is bounded** by the number of distinct comptime-argument
  tuples (typically 1–2 per backend — e.g. one memory-order enum value).
- **Atomics get faster on the LLVM path** (real `AtomicLoad`/`AtomicRMW` MIR
  opcodes with constant order/scope) and stay identical on the C path (the
  `__chx__*_dispatch` call).

---

## 7. Migration path

1. Add `comptime` parameters + specialization; make `%runtime_value`/`%runtime_block_value`
   transparent shims. **All suites stay green** (nothing changes semantically).
2. Migrate `atomic.ch` (120 sites) — mechanical, then delete its `%runtime` uses.
3. Migrate `std` (`string`, `string_view`, `span`, `stream`, `function`,
   `panic`), `lab`, `test`, `universal_test`, `compiler/ASTBuilder`.
4. Remove the shims, `RuntimeValue`/`RuntimeBlockValue`, and `%runtime`/`%maybe_runtime`.
5. Delete the AST interpreter's statement path and `compiler/Interpreter/Core.cpp`
   (Phase 5 DoD).

After step 3 there are no `%runtime_value` uses left; after step 4 the AST
interpreter has no lowering-time job. The residual symres-time expression
evaluator (`comptime if` conditions, array lengths, intrinsic `call` overrides)
remains, as it must — see `mir-interpretation-plan.md` §5.4.

---

## 8. Residual / non-goals

- **Pure comptime value functions** (`satisfies`, `is_same_type`, arithmetic
  folds) still evaluate via the symres-time folder; the replacement targets
  only the *template-returning* functions, which are the blocker.
- **`%runtime_value` as a user-facing test fixture** (`lang/tests/common/src/comptime/basic.ch`)
  is updated in step 3/4 to the new surface; the covered semantics (branch
  selection over runtime operands, `%runtime` splice) are preserved by
  specialization.
- This proposal does **not** change `%maybe_runtime`'s comptime-or-runtime
  meaning; it only moves it from a comptime-func-only marker to a plain
  parameter.
