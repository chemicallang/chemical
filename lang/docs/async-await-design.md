# Async/Await Design for Chemical

> **Status: Implementation Design — revised September 14, 2026 (third review)**
>
> **TL;DR for the implementer:** the protocol, the runtime handle, the
> `FutureHandle<T>` static-vtable materialization (D13), and the `block_on<T>(
> FutureHandle<T>)` shape were **compiled and run** on the real compiler under
> **both backends** (Sections 1.4, 1.7, 1.8; probes `lang/compiled/async_probe`,
> `lang/compiled/async_mat`, `lang/compiled/async_final`). Four compiler
> blockers found in the first two reviews are fixed (B1 generic loop-expression
> result types, B2 impl-only method calls, B6 generic destructible structs by
> value, B7 2c loop-expression double-destroy); the full TCC suite passes
> (2185/2185). Start with Sections 1.4–1.8, 2 (D13), 4.2–4.6 (the future type
> model), §8.7 (completion cleanup), §9.0 (corrected coroutine recipe), §10.0,
> 14 (performance), and 17 (Phase 0).
>
> **The third review made four changes that an implementer must not miss:**
>
> 1. **Type-model fix (Section 4.6).** An `async func f(...) : T` returns a
>    concrete `FutureHandle<T>`, **not** the `Future<T>` interface. `Future<T>`
>    remains a trait that *hand-authored* future types implement; `await`
>    materializes them into a `FutureHandle<T>`. The previous text (and §16.6's
>    "wrap in `Future<T>`") was internally contradictory and is corrected.
> 2. **Completion cleanup bug (Section 8.7).** On the normal `return` path the
>    frame's still-live locals must be destroyed **before** the state is set to
>    `STATE_DONE`. `STATE_DONE` means "destructors already ran"; the drop thunk
>    may only free the frame. The old text implied `STATE_DONE` skipped all
>    destructors, which leaks every local live at `return`.
> 3. **`coro.end` semantics were wrong (Section 9.0).** The destroy/cleanup path
>    must use `coro.end(..., i1 true, ...)`; the normal path uses `i1 false`;
>    exactly **one** fallthrough `coro.end` is allowed. Two `i1 false` ends
>    crash LLVM's `CoroSplit` with *"Only one coro.end can be marked as
>    fallthrough"* at `-O0/-O1`. The old §9.0 item 5 and Appendix E had this
>    inverted (they claimed `i1 true` segfaults — it does not; the old recipe
>    does).
> 4. **The LLVM backend cannot move destructor-bearing results (B9, §1.8).**
>    Even `func f() : std::string { return std::string("X") }` crashes LLVM
>    codegen at current HEAD. This blocks `block_on<T>` on LLVM for any `T` with
>    a destructor (`std::string`, structs, …) and must be fixed before Phase 3.
>    TCC is unaffected, so the runtime library can be developed and tested on the
>    fast backend first.
>
> Two more backend bugs found in the third review are documented: **B8**
> (`dyn` dispatch of a struct-returning method omits the hidden sret argument in
> the 2c backend) and the LLVM destroy-path brittleness. The design still
> mandates that backends emit the poll loop and that `await` goes through a
> `FutureHandle<T>` vtable (D13).
>
> This document supersedes the August 25, 2026 draft. The draft was a good
> outline but contained factual errors about the codebase (wrong method names,
> wrong file paths, invalid Chemical syntax) and, most importantly, recommended
> a **blocking thread-per-await** implementation as the destination. That is not
> performant and does not meet the language's zero-cost goal.
>
> This revision is written to be followed literally by an implementer. It
> contains the exact extension points in the real codebase, the exact runtime
> protocol, the exact frame layout rules, the exact lowering recipes for both
> backends, and the exact safety rules. Where the previous draft and reality
> disagreed, reality wins and the correction is called out.
>
> **Read this whole document before writing code.** Several decisions interact
> (lazy vs eager, heap frames vs pinned frames, two lowerings vs one, `Send`
> and thread model). Implementing a piece without understanding the interacting
> pieces will create work that must be thrown away.

---

## Table of Contents

1. [Current Codebase Assessment](#1-current-codebase-assessment)
2. [Design Decisions (Normative)](#2-design-decisions-normative)
3. [Requirements and How They Are Satisfied](#3-requirements-and-how-they-are-satisfied)
4. [Execution Model and Core Protocol](#4-execution-model-and-core-protocol)
5. [Language Surface](#5-language-surface)
6. [Compiler Representation](#6-compiler-representation)
7. [Await Normalization Pass](#7-await-normalization-pass)
8. [Frame Layout and Drop Planning](#8-frame-layout-and-drop-planning)
9. [LLVM Backend Lowering](#9-llvm-backend-lowering)
10. [C / 2c Backend Lowering](#10-c--2c-backend-lowering)
11. [Interpreter and Comptime](#11-interpreter-and-comptime)
12. [Runtime Library](#12-runtime-library)
13. [Safety Model](#13-safety-model)
14. [Performance Model](#14-performance-model)
15. [Diagnostics](#15-diagnostics)
16. [Extension-Point Checklist (Exact Files)](#16-extension-point-checklist-exact-files)
17. [Implementation Phases](#17-implementation-phases)
18. [Testing Plan](#18-testing-plan)
19. [Edge Cases and Gotchas](#19-edge-cases-and-gotchas)
20. [Deferred Work and Open Questions](#20-deferred-work-and-open-questions)
21. [Appendix A: Corrected Examples](#21-appendix-a-corrected-examples)
22. [Appendix B: File and Symbol Reference Map](#22-appendix-b-file-and-symbol-reference-map)
23. [Appendix C: The Rules an Implementer Must Not Break](#appendix-c-the-rules-an-implementer-must-not-break)
24. [Appendix D: Verification Artifacts](#appendix-d-verification-artifacts)
25. [Appendix E: Verified LLVM Coroutine Reference IR](#appendix-e-verified-llvm-coroutine-reference-ir)

---

## 1. Current Codebase Assessment

This section is the ground truth as of September 14, 2026.

### 1.1 What does not exist

There is **no** async/await in the language today.

- `lexer/TokenType.h` has no `AsyncKw` / `AwaitKw`.
- `ast/base/ValueKind.h` has no `AwaitExpr`.
- No `AwaitExpression` node, no `is_async` flag anywhere.
- No `compiler/effects/` directory; `FX_SUSPENDS` is only mentioned in
  `effect-system-proposal.md`. The effect system is **unimplemented**.
- No `lang/libs/async/` directory.
- No `llvm.coro.*` usage, no coroutine transform. The only `coro` mentions in
  C++ are in `compiler/chem_clang.cpp`, which enumerates Clang AST kinds while
  parsing C headers and is unrelated.

### 1.2 What does exist and must be used

- **`std.concurrent.Future<T>` / `Promise<T>` / `ThreadPool`** live in
  `lang/libs/std/src/concurrency/threadpool.ch`. They are **blocking,
  thread-pool based** and are *not* the async future. They are useful as a
  blocking interop adaptor and as a source of platform threading code.
- **`FunctionDeclaration`** (`ast/structures/FunctionDeclaration.h`) inherits
  `FunctionTypeBody`. Its flags live in `FuncDeclAttributes`. It stores
  `TypeLoc returnType` (public, assignable) inherited from `FunctionType`.
- **`FunctionTypeData`** (`ast/types/FunctionType.h:33`) is a 4-bool struct with
  `static_assert(sizeof(...) <= 8)`. It is the shared flag bag for both
  declarations and lambda function types.
- **`LambdaFunction`** (`ast/values/LambdaFunction.h`) also inherits
  `FunctionTypeBody`, so a single `is_async` bit covers both.
- **Generic synthesis pattern**: to build a `FutureHandle<T>` type node the compiler
  uses:
  `new (allocator.allocate<GenericType>()) GenericType(new (allocator.allocate<LinkedType>()) LinkedType(decl), { TypeLoc(inner, loc) })`.
  Real examples: `ast/values/FunctionCall.cpp:2141` (in `determine_type`; line
  2157 is the plain fallback) and `compiler/symres/SymResLinkBody.cpp:2140,2167`.
- **Core-node caching**: `CoreNodes` (`compiler/symres/CoreNodes.h`) caches
  handles to core declarations; populated by `SymbolResolver::link_core_nodes`.
  This is where a handle to the `Future`/`Poll`/`FutureHandle` declarations must
  live.
- **Value extension machinery**: `preprocess/visitors/NonRecursiveVisitor.h`
  `VisitValueNoNullCheck` is the master `ValueKind` switch. `RepresentationVisitor`
  provides `representation()`. `RecursiveVisitor`, `ToCAstVisitor`,
  `SymResLinkBody`, `TopLevelLinkSignature`, `TypeVerifier`,
  `GenericInstantiator` all dispatch through it.
- **`Value` API**: `getType()` / `setType()` (not `known_type()` /
  `set_known_type()` as the old draft wrote). Constructor is
  `Value(ValueKind, BaseType*, SourceLocation)`. Allocation is
  `new (allocator.allocate<T>()) T(...)`.
- **`TypeLoc`** (`ast/base/TypeLoc.h`) wraps `BaseType const*` + location, and is
  freely assignable. Replacing a return type is `func->returnType = TypeLoc(...)`.
- **TCC backend compiles generated C with TinyCC**, not Clang. See
  `LabBuildCompiler::process_module_tcc` →
  `compile_c_to_obj_w_opts` → `compile_adding_file` / `compile_c_string` in
  `compiler/lab/LabBuildCompiler.cpp`. TinyCC is fast but not an optimizer, so
  generated C for the C backend must be efficient *by construction*.

### 1.3 The one hard architectural constraint

Chemical's AST has **no `goto`/label statement** (see `ast/statements/`). LLVM
builds basic blocks internally; the C backend writes `goto` text directly. This
means an async→state-machine transform **cannot be expressed as an AST rewrite**.
It must live either in a CFG-bearing IR (MIR, not yet implemented) or in each
backend. This document therefore specifies a **small, shared front-end analysis
(the Async Lowering Plan) plus two backend emissions**. A future MIR can unify
them (Section 20).

### 1.4 Empirically Verified Protocol (Phase 0 probe)

Before designing further, the protocol and the runtime handle shape were
**compiled and executed** on the real compiler. Probe modules live in
`lang/compiled/async_probe/` (gitignored). Build/run:

```bash
./cmake-build-debug/TCCCompiler lang/compiled/async_probe/chemical.mod \
    -o lang/compiled/async_probe/probe.exe --mode debug_quick --no-cache
./lang/compiled/async_probe/probe.exe
```

Observed output:

```
[drop] int frame
int await = 99
[drop] string frame
string await = hello-future
unit await done
handles = 2
[drop] int frame
[drop] int frame
```

The probe proves **all** of the following compile and run today on the 2c/TCC
backend:

1. **Protocol types**: `WakerVTable` (function-pointer fields), `Waker` (owns a
   type-erased pointer, `@delete`), `Context`, and the generic variant
   `Poll<T> { Ready(value: T) Pending() }`.
2. **Generic variant construction** `Poll.Ready<T>(v)` and pattern matching
   (`r is Poll.Ready`, `var Ready(value) = r else unreachable`).
3. **Moving a result out of `&mut self`** via
   `std::replace(&mut self.value, zeroed<T>())`.
4. **The runtime handle + vtable exactly as proposed** — a generic
   `FutureHandle<T> { frame: *mut void, vtbl: *mut FutureTable<T> }` with a
   `@delete` that calls `vtbl.drop(frame)`; a generic `FutureTable<T>` holding
   `poll`/`drop` function pointers; and the lowered await loop calling
   `fut.vtbl.poll(fut.frame, cx)`.
5. **Move-only handles**: the handle's `@delete` runs the frame drop **exactly
   once** on scope exit (and not on the return path).
6. **Destructor-bearing results**: a `Poll<std::string>` whose `Ready` value is
   moved out and returned; the frame's `StringFrame` is destroyed via
   `delete f` in its drop function.
7. **Unit futures**: `Poll<Unit>` with a user-defined empty `struct Unit {}`.
8. **Executor-queue shape**: `std::vector<FutureHandle<int>>` accepts move-only
   elements and drops every element when the vector dies.
9. **Placement new / delete** for frames: `new(f) StringFrame { value: v }` and
   `delete f`.

**Design consequence:** the protocol in Section 4.2 and the handle in Section
4.3 are not hypothetical — they are the shapes to implement. Rename the handle
types to avoid colliding with user code (the compiler generates them
internally; suggested internal names `__chx_future_handle<T>` /
`__chx_future_vtable<T>`).

### 1.5 Phase 0 Blockers Found (must fix or design around)

Compiling the probe exposed real compiler limitations. They are recorded here
because they change the implementation plan.

#### B1 — Generic loop-expression result type is not specialized (HIGH)

**Verified.** A `loop { break value }` used as a value (initializer **or**
`return` operand) inside a generic function leaves the loop's result type as the
unspecialized type parameter.

Minimal reproduction: `lang/compiled/b1_probe/` and `lang/compiled/generic_loop_bug/`

```chemical
func <T> pick(flag : bool, a : T, b : T) : T {
    var out : T = loop {
        if(flag) {
            break a
        } else {
            break b
        }
    }
    return out
}

public func main() : int {
    return pick<int>(true, 1, 2)
}
```

Observed:

```
[2cTranslation] error: generic type parameter not specialized, compiler bug
  detected at .../b1_probe/src/main.ch:2:7
```

The generated C contains
`[GENERIC_TYPE_PARAMETER_NOT_SPECIALIZED_COMPILER_BUG] out = ...`, which TinyCC
then rejects.

**Verified scope (variants compiled individually):**

| Variant | Result |
|---------|--------|
| `var out : T = loop { break a }` in `func <T>` | **FAIL** |
| `return loop { break a }` in `func <T>` (no initializer) | **FAIL** |
| `loop { break }` (void) in `func <T>` | OK |
| `var out : int = loop { break 5 }` (concrete result) in `func <T>` | OK |
| `var out : T = if(flag) a else b` (if-expression, generic result) | OK |
| concrete `func` with `loop` expression | OK |

So it is specifically **`LoopValue` whose result type is a generic parameter**;
`if`/`switch` value expressions already handle this correctly.

**Root cause (confirmed by reading the code):** `LoopBlock` caches the first
broken value and copies that stale pointer during instantiation.

- `ast/structures/LoopBlock.h:32-36` `copy_into` does
  `blk->first_broken = first_broken;` with the comment
  `// TODO: should we recalculate in generic instantiation ?`
- `compiler/generics/GenericInstantiator.cpp:370-378`
  `VisitLoopValue` then does `value->setType(first->getType())` on the original
  (uninstantiated) broken value, so the type stays `T`.
- `ast/structures/Scope.cpp:130` `LoopBlock::get_first_broken()` caches
  `first_broken`, so the stale pointer is returned.
- Contrast `VisitIfValue` (`GenericInstantiator.cpp:354`) / `VisitSwitchValue`
  (`:362`), which re-read `get_value_node()` after visiting and therefore see
  the instantiated type.

**Impact:** the await poll loop **must not be written as a Chemical `loop`
expression in generic code**. This is a strong independent justification for
Decision D6: the suspend/poll loop is emitted by each backend (LLVM/2c), never
as Chemical source, and never via a generic Chemical helper.

**FIXED (September 14, 2026).** Two changes:

- `ast/structures/LoopBlock.h` `copy_into`: no longer copies the cached
  `first_broken`; it sets `blk->first_broken = nullptr` so it is recomputed
  lazily from the copied body.
- `compiler/generics/GenericInstantiator.cpp` `VisitLoopValue`: invalidates
  `value->stmt.first_broken = nullptr` after visiting the body, then recomputes
  it, so the type is taken from the instantiated break value.

Regression test: `lang/tests/src/generic/generic_dispatch.ch`
(`test_generic_loop_expression_result`), plus `lang/compiled/b1_probe` and
`lang/compiled/generic_loop_bug`.

#### B2 — Any call to an impl-only method emits the interface symbol (HIGH)

**Verified — broader than originally thought.** A call to a method defined *only*
inside an `impl Interface for T` block emits a call to the interface's symbol,
not the concrete impl function — **with or without** a generic constraint.

Minimal reproduction: `lang/compiled/b2_probe/`

```chemical
public interface Ping {
    func ping(&mut self) : int
}

@direct_init
public struct Counter {
    var n : int

    impl Ping for Counter {
        func ping(&mut self) : int {
            return self.n
        }
    }
}

func call_it(c : *mut Counter) : int {
    return c.ping()                 // concrete type, no generic constraint
}

public func main() : int {
    var c = Counter { n : 7 }
    return call_it(&raw mut c)
}
```

Observed (link failure):

```
tcc: error: unresolved reference to 'b2_probe_Pingping'
```

Generated C (from `--emit-c`) shows the exact mismatch:

```c
// defined (correct):
int b2_probe_Ping_Counter_ping(struct b2_probe_Counter* self) { ... }
const __chx_b2_probe_Ping_vt_t b2_probe_Pingb2_probe_Counter = {
    (int(*)(void* self)) b2_probe_Ping_Counter_ping, ... };
// called (wrong):
return b2_probe_Pingping(c);
```

**Verified scope:**

| Variant | Result |
|---------|--------|
| concrete `func call_it(c : *mut Counter) { c.ping() }` | **FAIL** (`b2_probe_Pingping`) |
| generic `func <P : Ping> call_it(c : *mut P) { c.ping() }` | **FAIL** (same symbol) |
| direct method on the struct (not in an `impl`) | OK |

So this is a general impl-method-call name-resolution bug, not specific to
generics or to async. In the earlier async probe it appeared as
`async_probe_Future__cgs__0poll` vs the correct
`async_probe_Future__cgs__0_CountdownFuture_poll`.

**Impact:** a user-defined future whose `poll` is defined in
`impl Future<T> for F` cannot have `poll` invoked by a source-level method call.
**Mitigation built into the design:** the compiler materializes every future into
a `FutureHandle<T>` with a compiler-synthesized **static vtable** that references
the impl's `FunctionDeclaration*` directly (taking its address), not via a
name-based method call. The await lowering then always calls
`handle.vtbl.poll(...)`, which is verified working (see 1.4 item 4). This
sidesteps B2 entirely and is now **Decision D13**.

**FIXED (September 14, 2026).** The root cause was that `impl Interface for T`
blocks nested inside a container were never indexed during the signature pass, so
the container's `indexes` only ever received the interface's abstract methods
(from `SymResLinkBody::VisitImplDecl`'s `struct_linked->adopt(linked)`). Two
changes:

- `compiler/symres/LinkSignature.cpp` `BuildIndexes`: for `StructDecl` /
  `UnionDecl` / `VariantDecl`, also run `index_implementation` +
  `build_indexes_of_impl` for each nested `ImplDecl` in
  `container->evaluated_nodes()`. The impl's functions are adopted into the
  container before the interface's methods, so calls resolve to the concrete
  implementation and mangle with the concrete type.
- `preprocess/2c/2cASTVisitor.cpp` `CTopLevelDeclarationVisitor::VisitStructDecl`:
  declare the functions of nested `ImplDecl`s too, so a call emitted before the
  definition (e.g. from a generic instantiation) has a C prototype. Without this
  the generated C used an implicit declaration and TCC rejected the later
  definition as an incompatible redefinition.

Regression tests: `lang/tests/src/generic/generic_dispatch.ch`
(`test_impl_only_method_call`) and `lang/compiled/async_generic_poll`. Concrete
and generic-constrained calls to an impl-only method now link and run.

#### B3 — `&mut Concrete` does not coerce to `&mut Interface` (MEDIUM)

```chemical
func drive(f : &mut Future<int>, cx : *mut Context) : int { ... }
var cd = CountdownFuture { ... }
drive(&mut cd, &mut cx)   // TypeCheck error:
                          // value with type '&mut CountdownFuture' does not
                          // satisfy type '&mut Future<int>'
```

**Impact:** do **not** design `await`/`IntoFuture` around passing interface
references. Use the `FutureHandle<T>` vtable value instead (D13).

#### B4 — Syntax constraints discovered (must be respected in all generated/user code)

| Constraint | Correct form | Wrong form |
|-----------|--------------|-----------|
| Function-pointer type parameters must be named | `(data : *mut void) => void` | `(*mut void) => void` |
| No `const` in pointer types | `var vtbl : *WakerVTable` | `var vtbl : *const WakerVTable` |
| Generic function declaration | `func <T> name(...)` | `func name<T>(...)` |
| Generic variant construction | `Poll.Ready<T>(v)` | `Poll<T>.Ready(v)` |
| Move out of `&mut self` | `std::replace(&mut self.x, zeroed<T>())` | `var v = self.x` (error) |
| No global unit type | define `public struct Unit {}` | (no builtin `unit`) |
| `if` not inline in args | assign to a var first | `f(if(c) 1 else 0)` |
| Mutable pointer to write fields | `*mut T` from `malloc(...) as *mut T` | `*T` then assign field (error) |

#### B5 — Name collision with `std::concurrent.Future<T>` (LOW, but must be decided)

`lang/libs/std/src/concurrency/threadpool.ch` already defines a blocking
`std.concurrent.Future<T>` / `Promise<T>`. The async protocol must live under a
distinct qualified name. **Decision:** the compiler protocol is
`core::async::{Future, Poll, Context, Waker, FutureHandle, FutureTable}`;
`std::concurrent.Future` remains the blocking interop type. `await` resolves the
`core::async` types by their fully-qualified `CoreNodes` handles, never by the
unqualified name `Future`. Document this in user-facing docs to avoid confusion,
and never add a blanket `using namespace` that merges the two.

### 1.6 Net Design Changes Forced By The Probe

| Finding | Design change |
|---------|---------------|
| B1 generic loop bug | Backends emit the poll/suspend loop; no Chemical `loop` helper. Already D6; now mandatory. |
| B2 impl-only call bug | **D13**: materialize futures into `FutureHandle<T>` with a compiler-synthesized static vtable; `await` always goes through the handle. |
| B3 no interface coercion | Use `FutureHandle<T>` values, not `&mut Interface`. Reinforces D13. |
| B4 syntax | Generated frames/vtables follow the correct forms in the table. |
| B5 collision | Protocol is `core::async::*`; never unqualified `Future`. |

### 1.7 Second Review (September 14, 2026): Blockers B6/B7, Verified Lowerings

A second, deeper verification pass compiled and ran the exact shapes the design
depends on. It found two more compiler blockers (both fixed) and produced the
verified LLVM coroutine recipe and the verified 2c state-machine text.

#### B6 — Generic destructible struct passed by value into a generic function (HIGH) — FIXED

**Verified.** Passing a generic struct that has a heap-owning destructor by
value into a generic function whose parameter type mentions the function's type
parameter applied to that struct (`H<T>`) failed during symbol resolution:

```
[SymRes:link] error: unknown value being moved, where the struct types don't match
```

Minimal form (this is *exactly* the runtime-library signature
`block_on<T>(handle : FutureHandle<T>) : T`):

```chemical
@direct_init
public struct H<T> {
    var p : *mut void
    @delete func delete(&mut self) { p = null }
}
func <T> take(h : H<T>) : int { return 1 }

public func main() : int {
    var h = H<int> { p : null }
    return take<int>(h)          // <-- error here
}
```

**Verified scope (probe matrix):**

| Case | Shape | Result |
|------|-------|--------|
| A | generic struct, **no** destructor, generic fn | OK |
| B | generic struct **with** `@delete`, **concrete** fn | OK |
| C | generic struct with `@delete`, generic fn with `H<T>` param | **FAIL** |
| D | generic struct with `@delete`, generic fn with `T` param | OK |
| E | generic struct with `@delete`, generic fn with `*mut H<T>` param | OK |
| F | **non-generic** struct with `@delete`, generic fn | OK |

**Root cause.** `SymResLinkBody::mark_moved_value` (around
`compiler/symres/SymResLinkBody.cpp:3652`) only accepts the move when
`expected_node == linked_def` or `is_generic_instantiation(expected_node,
linked_def)`. When instantiating `take<int>`, the parameter type `H<int>` and the
argument's type resolve to **two distinct `StructDefinition` instantiation
nodes** that share the same generic parent; neither check matches, so the move is
rejected.

**Fix (September 14, 2026).** Added `is_same_generic_family(a, b)` next to
`is_generic_instantiation` in `compiler/symres/SymResLinkBody.cpp`: it returns
true when both nodes are members containers sharing the same non-null
`generic_parent`. The move acceptance now also checks this (and the reverse
`is_generic_instantiation` direction):

```cpp
if(is_generic_instantiation(expected_node, linked_def)
   || is_generic_instantiation(linked_def, expected_node)
   || is_same_generic_family(expected_node, linked_def)) {
    final = mark_moved_value(&value, diagnoser);
}
```

Regression tests: `test_generic_struct_by_value_param` in
`lang/tests/src/generic/generic_dispatch.ch`. Verified on both TCC and LLVM.

#### B7 — `loop { … break … }` expressions double-destroy in-scope locals (HIGH, C backend only) — FIXED

**Verified.** In the C/2c backend, a loop *expression* (`var x = loop { … break v
… }`) emitted the destructor of every in-scope destructible local and parameter
on each `break`/`continue` **and** again at scope exit, destroying them two or
more times. The LLVM backend was correct.

Minimal form:

```chemical
@direct_init public struct Probe {
    var p : *mut void
    @delete func delete(&mut self) { printf("DROP\n") }
}
func with_loop_expr(d : Probe) : int {
    var out : int = loop { if(true) { break 5 } else { continue } }
    return out
}
```

Before the fix, `DROP` printed twice for one call; LLVM printed once.

**Root cause.** `ToCAstVisitor::writeLoopStmtValue`
(`preprocess/2c/2cASTVisitor.cpp:5191`, the loop-*expression* emitter) called
`scope(*this, block.body)` instead of `loop_scope(*this, block.body)`. Only
`loop_scope` updates `destructor.loop_job_begin_index`, which
`destruct_till_loop_scope_above()` (used by `break`/`continue`) relies on. With a
stale index, each `break`/`continue` destroyed *all* live locals above the
enclosing function scope, and the normal function-exit cleanup destroyed them
again.

**Fix (September 14, 2026).** `writeLoopStmtValue` now uses `loop_scope`.

**Why this matters for async.** The design emits the poll loop in each backend
(D6/B1), so *generated* await code never goes through a Chemical loop
expression. But user code and the runtime library (`block_on`, `spawn`) are
written in Chemical and will use loop expressions; before this fix they would
double-free destructor-bearing locals (e.g. a `FutureHandle<T>`) on the C
backend, which is the default fast backend.

Regression test: `test_loop_expression_cleanup_once` in
`lang/tests/src/generic/generic_dispatch.ch`.

#### Verified LLVM coroutine recipe (corrects Section 9)

Raw `llvm.coro.*` IR was written and run through the linked LLVM 22.1.8 at
`-O0`, `-O1`, and `-O2`. A **separate** `poll` function calling
`coro.resume`/`coro.done` (the design's §9.4 shape) lowers correctly at all three
levels once the recipe is exact. The recipe required by LLVM 22 (and the
corrections to the current §9 text) is in **§9.0**. The most important
corrections:

1. The ramp function **must** carry the `presplitcoroutine` attribute, or
   `CoroSplit` never runs and the intrinsics survive to the backend.
2. Lazy semantics (D2) require an **explicit initial `coro.suspend`** before the
   user body. Raw LLVM coroutines otherwise run the body up to the first suspend
   at call time. The current §9.2 recipe omits this — a real bug for D2.
3. Every `coro.suspend` needs a preceding `coro.save`.
4. The `switch` after each `coro.suspend` must route its **default** to a shared
   `suspend` block (`coro.end(…, i1 false, …)` then `ret`), and `i8 0` to the
   continuation, `i8 1` to the per-state cleanup. Routing the default to the
   continuation makes the ramp run the body (observed: extra side effects).
5. `coro.end` on the destroy/cleanup path must be `i1 false`; `i1 true` marks an
   unwind path and makes `CoroSplit` emit the destroy function as `unreachable`
   (observed SIGSEGV).
6. **Do not blindly `coro.destroy` after `coro.done`.** The destroy function must
   dispatch on the saved state and, when the coroutine already completed, only
   free the frame (skip user destructors). Otherwise cleanup runs after
   completion (design §13.3's `STATE_DONE` rule applies to the LLVM path too).
7. `coro.alloc` returns **true when the frame must be heap-allocated** (the
   conventional Clang `br i1 %alloc, label %do_alloc` shape is correct).

#### Verified D13 materialization, end-to-end (both backends)

`lang/compiled/async_mat` implements the whole D13 pipeline in Chemical:

- a user `CountdownFuture` whose `poll` is **only** in an
  `impl Future<int> for CountdownFuture` block;
- a compiler-shaped static vtable whose `poll` thunk does
  `(frame as *mut CountdownFuture).poll(cx)` (the concrete impl call — B2 path)
  and whose `drop` thunk destroys the frame;
- a **generic** await loop `func <T> rt_await_generic(fut : FutureHandle<T>, cx)
  : T { var out : T = loop { … break value … } }` (the `block_on<T>` shape, B1 +
  B6 paths);
- a `Poll<Unit>` future (B5 / void path) and a cancellation path that polls once
  (Pending) then drops the handle.

Observed on **TCC and LLVM**:

```
materialized await = 42 (expect 42)
drops after await = 1 (expect 1)
unit await done, drops = 1 (expect 2)     # see note
cancellation: pending as expected
drops after cancel = 2 (expect 3)         # see note
```

> Note on the drop counts: a `Unit` future uses `frame == null`, and
> `FutureHandle::@delete` intentionally skips `vtbl.drop` when `frame == null`.
> So the unit handle adds no drop; the "expect 2/3" labels in the probe are
> illustrative. The meaningful assertions are: the awaited int frame drops
> **exactly once**, and the cancelled frame drops **exactly once**.

#### Verified 2c state-machine text (TinyCC)

The design's §10.1 generated-C shape (frame struct + `for(;;) switch(state) goto`
+ per-state drop `switch` + vtable) was compiled and run with the bundled
`./lib/tcc/tcc` (`/tmp/opencode/coro/state.c`; re-verified in the third review).
Results: `complete=105 polls=2`, `drops_after_complete=1`,
`drops_after_cancel=2`. One correction to §10.1:
`state == STATE_DONE` must be a **case that returns `Ready`** (or asserts), not
fall into `default: goto L0`; otherwise polling a completed future would re-run
the body. (The third review additionally clarified that completion must run the
live-local destructors *before* `STATE_DONE` — Section 8.7.)

#### Compiler regression status

- TCC suite: **2185/2185 pass** after B6 + B7 fixes and the four new regression
  assertions (B1 ×4, B2 ×2, B6 ×2, B7 ×1).
- LLVM suite: still aborts on a **pre-existing, unrelated** backend bug — dead
  code after `break`/`continue` produces an LLVM basic block without a
  terminator (`lang/compiled/llvm_deadcode`, trigger at
  `lang/tests/common/src/interp_regressions.ch:318,333`). This is not caused by
  the async work. **However**, the LLVM backend has additional pre-existing
  regressions that do affect async results (B9, below); see §1.8.

### 1.8 Third Review (September 14, 2026): Type Model, `coro.end`, B8/B9

A third pass re-ran every codegen pattern on the real toolchain at the current
commit, re-derived the LLVM coroutine template from Clang's own raw IR, and
audited the language's `dyn` interface machinery. Findings, in order of
severity.

#### The future type model (normative correction)

The second review left an internal contradiction: §5.2/§9.1/§16.6 said an
`async func` returns the `Future<T>` **interface**, while §4.3/D13 said the ramp
produces a concrete `FutureHandle<T>`. An interface value cannot be the return
type here: interface values are `dyn` fat pointers (§1.8 below), the interface
vtable has no drop slot, and `&mut Concrete` does not coerce to `&mut Interface`
(B3). The model is now fixed and stated once in **Section 4.6**:

- `async func f(...) : T` has static return type **`FutureHandle<T>`**.
- `Future<T>` is the trait hand-authored future types implement; it is never a
  parameter/return/variable type of `await`.
- `await e` materializes `e` into a `FutureHandle<T>` and drives it.

Consequently §5.2, §9.1, §12.2 (`spawn`), §16.6 (`CoreNodes::future_handle_type`)
and the lambda wrapping sites are corrected throughout this revision.

#### B8 — `dyn` dispatch of a struct-returning method omits the hidden sret argument (2c backend) (MEDIUM)

**Verified.** A `dyn Interface` whose method returns a struct-like value
(`Poll<T>`, a struct, a variant) generates a C call that omits the hidden
sret argument, so the generated C does not compile:

```chemical
public variant Poll<T> { Ready(value : T) Pending() }
public interface Future<T> { func poll(&mut self, cx : *mut Context) : Poll<T> }
impl Future<int> for MyF { func poll(&mut self, cx) : Poll<int> { ... } }

var d : dyn Future<int> = dyn<Future<int>>(m)
var r = d.poll(&raw mut cx)
```

`--emit-c` shows the mismatch:

```c
typedef struct { void(*poll)(void* self, struct Poll* sret, struct Context* cx); } ..._vt_t;
...
struct Poll r = ((..._vt_t*) d.second)->poll(d.first, &cx);   /* missing sret */
```

Observed: `error: too few arguments to function` from TinyCC (exit before link).
On the LLVM backend the same program **runs correctly** (verified). Probes:
`lang/compiled/async_final/micro/dyn_dispatch.{ch}` (`dyn_prim.ch` returns a
primitive and works on both).

**Impact:** do not build `await` on `dyn Future<T>`. This is the third
independent reason for D13's hand-rolled `{frame, vtbl}` handle (after the
missing drop slot and the missing interface coercion). It also means a future
`executor` that wants heterogeneous tasks must not rely on dynamic dispatch of a
`Poll<T>`-returning method on 2c until B8 is fixed.

#### B9 — LLVM crashes on destructor-bearing by-value returns (HIGH, LLVM only)

**Verified after a clean `./scripts/build.sh --llvm`.** On the current tree the
LLVM backend SIGSEGVs while compiling *any* function that returns a
destructor-bearing value from a temporary or a local, including stdlib calls:

```chemical
func f() : std::string { return std::string("X") }        // crashes
func f(v : std::string) : std::string { return v }         // crashes
var c = std::string("hi").copy()                           // crashes (stdlib)
```

Backtrace (identical for all three; `--mode debug_quick`):

```
SIGSEGV in llvm::DataLayout::getPointerSpec
  llvm::DataLayout::getAlignment
  llvm::IRBuilderBase::CreateLoad
  Value::load_value                         ast/base/Value.cpp:213
  BaseDefMember::llvm_load                  ast/structures/StructDefinition.cpp:142
  VariableIdentifier::llvm_value            compiler/backend/LLVM.cpp:645
  AccessChain::llvm_value                   compiler/backend/LLVM.cpp:1574
  Value::llvm_ret_value                     ast/base/Value.cpp:555
  Codegen::writeReturnStmtFor               compiler/backend/LLVM.cpp:2104
```

`std::string` is a union with an SSO variant (`lang/libs/std/src/string.ch:52`),
so this smells like the documented *temp + destruct + memcpy* struct-assignment
hazard (AGENTS.md → LLVM Backend Gotchas) interacting with a self/inline
referential payload. Whatever the precise cause, it is **not async-specific** and
**blocks the LLVM backend for every `T` with a destructor**, including
`block_on<T>` and `await` of a `Future<T>` whose `T` owns memory.

**Observed consequence on the probe:** `lang/compiled/async_final`, which drives
a `FutureHandle<std::string>` through a generic `rt_await_generic<T>`, prints the
correct value on **TCC** and an **empty string** on LLVM. The TCC path is the
reference result until B9 is fixed.

**Workarounds that did *not* work** (all still crash on LLVM): assigning a
by-value parameter to a local (`var out : T = v`), returning through an
out-pointer (`*out = v`), and the loop-expression form. **Workaround that does
work:** implement `block_on<T>` so that it never materialises a by-value `T`
local — extract the result from the `Poll<T>` payload via pattern matching on a
local `var r = handle.vtbl.poll(...)`, assign once, and return; but the generic
`T` return still hits B9, so on LLVM the runtime's generic result handling must
wait for the fix. Keep the runtime library on TCC until then.

**Rule for the implementer:** treat B9 as a hard prerequisite for Phase 3
(LLVM). Track it separately from async; it is a compiler bug, not a design
problem.

#### Corrected and re-verified LLVM coroutine recipe (supersedes §9.0 item 5 and Appendix E)

The second review's §9.0 recipe put `coro.end(..., i1 false, ...)` on **both**
the normal return path and the destroy/cleanup path. That is invalid in LLVM 22:
`opt` aborts at `-O0/-O1` with

```
LLVM ERROR: Only one coro.end can be marked as fallthrough
```

The canonical structure was recovered directly from Clang's own pre-split IR
(`clang -Xclang -disable-llvm-passes -emit-llvm`, reproduced in `/tmp` and quoted
in §9.0):

- normal return path: a single `coro.end(handle, i1 false, token none)`;
- destroy/cleanup path: `coro.end(handle, i1 true, token none)`.

With that change the reference IR (`/tmp/opencode/coro/ref2.ll`) compiles and
runs at **`-O0`, `-O1`, and `-O2`**, exercising the lazy initial suspend and
normal completion (observed `0` then `1`). Note also that **`llvm.coro.end`
returns `void` in LLVM 22** (Clang 21 emits an `i1` result for LLVM 21); the frame
is freed from the destroy path with `llvm.coro.free`, not from the `coro.end`
result.

**The destroy-while-suspended path is the highest-risk LLVM surface.** The
hand-written toy's destroy path was *not* stable (use-after-free if `free` is
placed before `coro.end`; heap corruption if the frontend frees the raw
`coro.begin` pointer). The implementer must mirror LLVM 22's Clang coroutine
lowering exactly and validate cancellation with a dedicated test; do **not** copy
the old §9.5 text.

#### `dyn`/interface facts that justify D13 (verified)

- Interface vtables contain **only one function-pointer slot per method** — no
  drop/destructor slot (`ast/structures/InterfaceDefinition.cpp:184-221`,
  `preprocess/2c/2cASTVisitor.cpp:3047-3103`). A `dyn` value therefore **owns
  nothing** and destroys nothing; cancellation-by-drop needs `FutureTable::drop`.
- `dyn<Interface>(x)` is a non-owning fat pointer `{ obj, vtbl }`
  (`ast/types/DynamicType.h`, `Value` layout 16 bytes). The object pointer may
  point at a stack temporary with no lifetime tracking.
- The `Delete` trait is `@non_dyn` (`lang/libs/core/ops.ch:14`) precisely because
  interface vtables cannot dispatch destructors.
- A nested `impl Future<T> for FutureHandle<T>` inside a generic struct does
  **not** resolve its methods (verified: `unresolved child 'poll'`), so the
  compiler must recognise `FutureHandle<U>` structurally, not via interface
  satisfaction.

#### Additional verified language facts

- Fieldless variant cases may be written **with or without parentheses**
  (`Pending` and `Pending()` both parse and construct via `Poll.Pending<T>()`).
- A generic function may return a generic concrete struct by value:
  `func <T> make_handle() : FutureHandle<T> { return FutureHandle<T>{...} }`
  compiles and runs on both backends.
- `dyn` dispatch of a **primitive-returning** method works on both backends
  (`dyn_prim.ch`).

#### Third-review verification artifacts

| Artifact | What it validates |
|----------|-------------------|
| `lang/compiled/async_final/` | Full runtime shapes: Waker callbacks, impl-only `poll` (B2), generic `rt_await_generic<T>` (B1+B6), destructor-bearing string result, cancellation drop. Correct on TCC; LLVM string result blocked by B9. |
| `lang/compiled/async_final/micro/` | `pending_paren`/`pending_noparen`, `generic_handle`, `generic_impl` (fails as expected), `dyn_dispatch` (B8), `dyn_prim`. |
| `lang/compiled/async_vs/`, `async_vs2/` | B9 minimization (LLVM string returns/moves). |
| `/tmp/opencode/coro/ref2.ll` | Corrected LLVM coroutine recipe, normal path, O0/O1/O2. |
| `/tmp/opencode/coro/co_raw.ll` | Clang's canonical pre-split coroutine structure (source of the `coro.end` truth). |
| `/tmp/opencode/coro/state.c` | Portable C state machine under TinyCC (`complete=105`, drops 1/2). |

---

## 2. Design Decisions (Normative)

These are decided. Do not re-litigate them mid-implementation.

| # | Decision | Rationale |
|---|----------|-----------|
| **D1** | **Poll-based stackless coroutines.** `await` desugars to a poll loop; suspension is `Pending`. No thread per task. | Only model that meets the zero-cost goal and scales to many concurrent I/O tasks. |
| **D2** | **Lazy futures.** Calling an async function does **not** run the body; the body runs on first `poll`. `spawn` schedules immediately. | Matches LLVM/Clang coroutine default (`initial_suspend`), gives deterministic cancellation, no hidden work at call site. |
| **D3** | **`Future<T>` is the trait hand-authored futures implement; the runtime representation of *every* future (including the return of an `async func`) is `FutureHandle<T>` = `{ frame: *mut void, vtbl: *mut FutureTable<T> }`.** | One representation usable by compiler-generated futures and materialized user futures. Handle is movable and cheap; verified by probe 1.4. See §4.6 / D14. |
| **D4** | **Frames are stable and never move after allocation** (heap, or caller-owned for the elided fast path). No `Pin` type is required. | Eliminates the entire self-referential-move problem. Moving the *handle* is fine; the *frame* is stable. |
| **D5** | **Frames are allocated by the async ramp, not by the `await` site**, through a pluggable `frame_alloc`/`frame_free`. Default backend uses a task arena; falls back to `malloc`. | No hidden per-call `malloc` requirement; executor can pool. |
| **D6** | **Two backend emissions, one shared plan.** LLVM uses `llvm.coro.*`; 2c emits a portable `switch`+`goto` state machine. Both consume the same `AsyncLoweringPlan`. | Each is natural for its backend; shared analysis keeps semantics identical. |
| **D7** | **Default thread model is thread-per-core with per-thread executors.** Tasks never migrate unless explicitly `spawn_send`. `Send` enforcement is a later phase. | Preserves safety today without a `Send`/auto-trait system, while still allowing I/O concurrency. |
| **D8** | **`await` is forbidden in destructors, constructors, `@extern` bodies, and comptime unless the future is provably ready.** | Destructors must be infallible and non-suspending; extern functions have no Chemical frame. |
| **D9** | **Async closures are supported** and produce `FutureHandle<T>`. | They fall out of `LambdaFunction`'s shared `FunctionTypeBody`. |
| **D10** | **The runtime protocol lives in `core`, the executor/IO lives in `lang/libs/async`.** The compiler auto-adds the `core` dependency for modules that use `async` (as it already relies on `core` for operators). | `async` must work without the user importing an executor; no mandatory runtime. |
| **D11** | **All `await` expressions are normalized to statement position before backends.** Backends implement one node, not arbitrary expression-position suspension. | Bounds backend complexity and fixes evaluation order. |
| **D12** | **A blocking `block_on` is the only bridge from sync to async**, used by `main` and interop. It is an executor, not a lowering. | Keeps sync/async boundary explicit. |
| **D13** | **Every awaitable is materialized into a `FutureHandle<T>` with a compiler-synthesized static vtable.** `await e` lowers to a poll loop over the handle's vtable, never to an interface-reference call or a method call on an impl. | Probe B2/B3: impl-only method calls emit the interface symbol in 2c, and `&mut Concrete` does not coerce to `&mut Interface`. The vtable-handle path is verified working (probe 1.4). |
| **D14** | **An `async func f(...) : T` has static return type `FutureHandle<T>`, not `Future<T>`.** `Future<T>` is only the trait hand-authored futures implement; `await` materializes. | Resolves the second review's contradiction. Interface values are non-owning `dyn` fat pointers with no drop slot (§1.8), and `dyn` struct-returning dispatch is broken in 2c (B8). See §4.6. |

---

## 3. Requirements and How They Are Satisfied

| Requirement | Mechanism |
|-------------|-----------|
| **Fast generated code** | No thread per task; one indirect call per `poll`; no allocation on the elided fast path; `Ready`-future fast path skips suspension; LLVM gets `coro` optimization; C state machine written to be efficient. |
| **Fast compilation** | Async lowering is per-function and linear; normalized awaits remove arbitrary suspension positions; async fns with no `await` compile to plain functions; non-async code paths are untouched; no global whole-program fixpoint. |
| **Safe** | Lazy + drop runs live-local destructors; frames stable; `await` context restrictions; `Pending` at comptime is an error; thread model avoids accidental migration; no `Pin` footgun. |
| **Flexible** | `into_future`/`Future` protocol lets users bring their own future and I/O; no mandated runtime; executor pluggable; `select`/channels are libraries. |

---

## 4. Execution Model and Core Protocol

### 4.1 The poll contract

A future is a state machine that is driven by repeated `poll` calls. `poll`
either produces a value (`Ready`) or registers interest and yields (`Pending`).
There is no OS thread and no stack switch during a suspension.

```
poll(future, context):
    Ready(value)  -> the future completed; value is the result
    Pending       -> not done; the future has arranged to be woken
```

The future is responsible for storing the `Waker` from the context (or cloning
it) so it can call `wake()` when progress is possible.

### 4.2 Core types (normative Chemical signatures)

These live in `lang/libs/core/`. Suggested file: `lang/libs/core/async.ch`, added
as a flat `source "async.ch"` line in `lang/libs/core/chemical.mod` (that module
lists each source file directly — there is **no** `src/` subdirectory, unlike
`cstd`/`std`).

> **Availability note.** `core` is not imported by every user module today; it is
> brought in through `import std` (whose `chemical.mod` contains `import core`).
> The compiler already resolves `core` for operator overloading via
> `SymbolResolver::link_core_nodes()` (`compiler/symres/SymbolResolver.cpp:55`).
> For `async` we must guarantee the `core` protocol is resolvable:
> - **Preferred:** auto-add a `core` dependency to any module that contains an
>   `async` declaration or `await` expression (mirroring how the compiler already
>   relies on `core` for operators). This works for freestanding modules too.
> - Do **not** add `import core` to `cstd`: `cstd` is imported by `core`-adjacent
>   low-level code and by freestanding modules, and making it depend on `core`
>   risks an import cycle.
> - If neither is possible, emit the diagnostic in Section 15 rather than
>   crashing in `link_core_nodes`.

```chemical
public namespace core {
    public namespace async {

        public variant Poll<T> {
            Ready(value: T)
            Pending()
        }

        // A cloneable, type-erased wake callback. `data` is the task handle.
        public struct Waker {
            var data  : *mut void
            var vtbl  : *WakerVTable

            public func wake(&self) {
                vtbl.wake(data)
            }

            public func clone(&self) : Waker {
                return vtbl.clone(data)
            }

            @delete
            func delete(&mut self) {
                if(vtbl != null) { vtbl.drop(data) }
            }
        }

        public struct WakerVTable {
            var wake        : (data : *mut void) => void
            var clone       : (data : *mut void) => Waker
            var drop        : (data : *mut void) => void
        }

        public struct Context {
            var waker : Waker
        }

        // The contract a HAND-AUTHORED future implements. An `async func` does
        // NOT return this type: its static return type is `FutureHandle<T>`
        // (Section 4.6). `await` materializes a value of this trait into a
        // `FutureHandle<T>`; it never calls `poll` through an interface
        // reference (probe B3: `&mut Concrete` does not coerce to
        // `&mut Interface`; probe B8: `dyn` struct-returning dispatch is broken
        // in 2c).
        public interface Future<T> {
            func poll(&mut self, cx : *mut Context) : Poll<T>
        }
    }
}
```

> **Chemical-syntax notes for whoever writes this file.**
> - `Poll<T>` is a `variant`; construct with `Poll.Ready<T>(x)` /
>   `Poll.Pending<T>()` (type args attach after the case name). A fieldless
>   case may be written `Pending` or `Pending()`; both parse (verified), and
>   this document uses `Pending()` consistently.
> - Function-pointer parameters must be **named**:
>   `(data : *mut void) => void`.
> - There is no `*const T`; immutable pointer is `*T`. (The earlier text used
>   `*const WakerVTable` — invalid; corrected above.)
> - `wake` takes `&self`: waking mutates the task through the erased `data`
>   pointer, not the `Waker` value itself.
> - Structs holding a `Waker` need an explicit `@delete` (as shown) because
>   `Waker` owns a type-erased allocation.
> - Do **not** write `unsafe var` (removed). Uninitialized locals are declared
>   plainly and only *access* is wrapped in `unsafe(...)`.
> - `if` always needs `else`.

> **There is no `IntoFuture` interface in the design.** The earlier draft's
> `IntoFuture` returning a `Future<T>` interface object cannot work: interface
> objects returned by value / passed by reference are not coercible (B3) and
> constraint calls are miscompiled (B2). Instead, `await` **materializes** its
> operand into the runtime handle below (D13). Adapters like `Result`/`JoinHandle`
> are ordinary types that implement `Future<T>` (or are already handles).

### 4.3 Runtime future handle (validated)

The compiler-generated future and any materialized future share one concrete,
move-only representation. Exact shapes were compiled and run (probe 1.4):

```chemical
// Declared in core/async so generated code can name it. Not user API.
public struct FutureTable<T> {
    var poll : (frame : *mut void, cx : *mut Context) => Poll<T>
    var drop : (frame : *mut void) => void
}

@direct_init
public struct FutureHandle<T> {
    var frame : *mut void
    var vtbl  : *mut FutureTable<T>

    @delete
    func delete(&mut self) {
        if(frame != null) {
            vtbl.drop(frame)
            frame = null
        }
    }
}
```

`@direct_init` is required so `FutureHandle<T>{ frame: ..., vtbl: ... }` is legal
without a constructor. (See AGENTS.md: `@make` without `@direct_init` forbids
`{}` entirely.)

Two producers of a handle:

- **Compiler async function** — the ramp allocates the frame and returns
  `FutureHandle<T>{ frame, vtbl }`; `vtbl.poll`/`drop` are the generated
  functions (Sections 9/10).
- **User type satisfying `Future<T>`** — the compiler hoists the value into a
  compiler-managed slot `__fut` (moved in), synthesizes a **static** vtable whose
  `poll` thunk does `return (__fut as *mut F).poll(cx)`, sets `frame = &raw mut
  __fut`, and adds a `drop` thunk that destroys `__fut`. The handle then owns the
  slot; the normal scope cleanup must not destroy it a second time (the
  materialization moves it).

> The `@delete` on `FutureHandle<T>` is what makes cancellation work: dropping a
> suspended handle calls `vtbl.drop(frame)`, which runs the frame's live-local
> destructors (Sections 8.5, 13.3). Probe 1.4 confirms it runs exactly once.

### 4.4 Await desugaring (normative)

`await E` lowers in three steps:

1. **Normalize** (Section 7): hoist `E` to a statement-boundary temporary.
2. **Materialize** into a `FutureHandle<T>`:
   - If `E : FutureHandle<T>` (compiler-generated ramp call), use it directly.
   - If `E : F` where `F` satisfies `Future<T>`, move `E` into a
     compiler-managed slot, synthesize a static vtable + `poll` thunk like the
     probe's (`return (*frame_as_F).poll(cx)`), and build the handle.
   - Otherwise: diagnostic (Section 15).
3. **Drive** the handle until ready. This loop is emitted by the backend
   (LLVM/2c), **never** written as a Chemical `loop` expression (probe B1):

```
var handle : FutureHandle<T> = <materialized E>
var result : T
loop {
    var r = handle.vtbl.poll(handle.frame, __cx)
    if(r is Poll.Ready) {
        result = move r.value
        break
    } else {
        suspend()          // backend: store state, return Poll.Pending
    }
}
// handle dropped here (moves out result first); drop calls vtbl.drop(frame)
```

The `Poll.Ready` branch is the **fast path**: if the future is already complete,
no suspension is emitted and the whole `await` is a single indirect call.

`suspend()` is compiler-only:
- LLVM: `llvm.coro.save` + `llvm.coro.suspend`; on resume, jump after the await.
- 2c: store the resume state into the frame and `return Poll.Pending`.
- Interpreter: `Poll.Pending` reached → diagnostic (Section 11).

**Why not `IntoFuture`:** see 4.2. Materialization is a compiler step, not an
interface method. This is D13.

### 4.5 Executor task model

A task is `{ FutureHandle<T>, Waker }`. The `Waker.data` points to a task slot
owned by the executor. `wake()` pushes the task back onto the run queue. The
executor calls `handle.vtbl.poll(handle.frame, &context)` until `Ready`, then
reads the value and drops the handle (which drops the frame).

The default executor (Section 12) is per-thread; `spawn` uses the current
thread's executor; `spawn_send` (Phase 4) moves a task to another executor.

### 4.6 The future type model (normative)

This subsection is the single source of truth for "what type is a future". The
second review was ambiguous and an implementer following it literally would
build something that cannot compile. Read this before writing any symres code.

**Three rules.**

1. **`async func f(...) : T` has the static type
   `(...) => FutureHandle<T>`.** The word after `:` is the **body result** type
   `T`. The compiler wraps it, but it wraps it in the concrete
   `FutureHandle<T>`, **not** in the `Future<T>` interface. `FunctionType` /
   `FunctionDeclaration::returnType` therefore holds
   `FutureHandle<T>` after symres.
2. **`Future<T>` is a trait that hand-authored awaitables implement.**
   `impl Future<T> for MyF { func poll(&mut self, cx) : Poll<T> }` is the way a
   user brings their own future. A `Future<T>` value never appears as a
   variable type, parameter type, or return type produced by the compiler; it
   is only ever the *source* of a materialization.
3. **`await e` materializes.** Given an operand of type `S`:
   - if `S` is exactly `FutureHandle<U>`, use it directly (no copy — it is
     move-only);
   - else if `S` satisfies the `Future<U>` trait, move `e` into a
     compiler-managed slot, synthesize a static `FutureTable<U>` and build a
     `FutureHandle<U>`;
   - else emit the "not awaitable" diagnostic (Section 15).

**Why not "`async func` returns `Future<T>`".** The language's interface values
are `dyn` fat pointers, which are:

- **non-owning and non-destructible** — the interface vtable has no drop slot,
  so a returned `Future<T>` could not cancel/drop its frame;
- **not coercible from `&mut Concrete`** (probe B3);
- **broken for struct-returning methods in 2c** (probe B8: `dyn poll` omits the
  sret argument).

`FutureHandle<T>` avoids all three: it owns the frame (`@delete` calls
`vtbl.drop`), it is a plain value type, and its `poll` is a normal function
pointer called as `fut.vtbl.poll(fut.frame, cx)` (a plain indirect call, not a
`dyn` dispatch).

**`FutureHandle<T>` does not implement `Future<T>`.** A generic
`impl Future<T> for FutureHandle<T>` does not resolve its methods in the current
compiler (verified: `unresolved child 'poll'`). The compiler recognises
`FutureHandle<U>` **structurally** for rule 3. Do not try to make it satisfy the
trait.

**Compatibility table (what `await` accepts).**

| Operand static type | Rule | Result |
|---|---|---|
| `FutureHandle<U>` | use directly | `U` |
| `MyF` with `impl Future<U> for MyF` | materialize | `U` |
| `Poll<U>` | error: `Poll` is the *output*, not an awaitable | diagnostic |
| anything else | error | diagnostic |

**API signatures that follow from this model** (corrected throughout the
document):

```chemical
// an async function:
async func fetch(url : *char) : Response      // static type: (*char) => FutureHandle<Response>

// runtime library:
public func block_on<T>(handle : FutureHandle<T>) : T
public func spawn<T>(handle : FutureHandle<T>) : JoinHandle<T>
public struct JoinHandle<T> { var inner : FutureHandle<T> }
// JoinHandle<T> is itself awaitable because it exposes a FutureHandle<T>.
```

> **Naming.** Publicly, `FutureHandle<T>` is the future type users see returned
> by `async func` and passed to `block_on`/`spawn`. Re-export it (Section 5.6)
> as `std::async::Future<T>` **only** if `std::concurrent::Future<T>` (the
> blocking threadpool future, B5) is not also in scope unqualified; do not merge
> the two with a blanket `using namespace`.

---

## 5. Language Surface

### 5.1 Grammar (normative)

```
function_decl   := [access] [comptime] ["async"] "func" ...     // async before func
await_expr      := "await" unary_expr
async_closure   := "async" lambda
```

- `async` may appear wherever `func` may: top level, struct/variant/interface/
  impl/union bodies, inside `type {}` blocks.
- `async` is a hard keyword. Existing uses of the identifier `async` are:
  the HTTP server method suffix `serve_async` (part of an identifier, fine) and
  LSP semantic-token names (fine). No source identifier named exactly `async`
  or `await` exists in the tree, so reserving them is safe.
- `await` binds tighter than any binary operator and looser than call/member
  access, exactly like `!`. `await f()` → `await (f())`;
  `await x.m()` → `await (x.m())`.
- `await` is right-associative: `await await f()` is legal and means
  `await (await f())`.

### 5.2 Declaration semantics

```chemical
async func fetch(url: *char) : std::Result<Response, string> { ... }
```

- The **user-visible return type is `FutureHandle<Result<Response, string>>`**.
  The word after `:` is the **body result type**; the compiler wraps it
  (Section 4.6, D14).
- Inside the body, `return expr` requires `expr : Result<Response, string>`
  (the inner type), **not** `FutureHandle<...>`. Return checking must compare
  against the inner type (Section 15).
- The function's `FunctionType` is
  `(*char) => FutureHandle<Result<Response,string>>`. Function pointers to it
  have that type.

### 5.3 Async closures

```chemical
var fut = async |x: int|() : int => {
    return await work(x)
}
// type: (int) => FutureHandle<int>
```

`LambdaFunction.attrs`-equivalent is `FunctionTypeData::is_async`. A call to an
async closure returns `FutureHandle<T>`.

### 5.4 `select` (Phase 4, library)

`select` is **not** a language statement. It is a library combinator built on
`Future` + `Waker`:

```chemical
var winner = await async::select(fut_a, fut_b)   // returns which + value
```

Do not add `select` to the parser in any early phase.

### 5.5 Entry point

Two supported forms:

1. Sync main, explicit block:
   ```chemical
   public func main() : int {
       return async::block_on(async_main())
   }
   ```
2. Async main with compiler-generated trampoline (Phase 3 convenience):
   ```chemical
   async func main() : int { ... }
   // compiler emits: int main() { return (int) async::block_on(main()); }
   ```
   This requires the `async` library to be linked. If it is not linked, emit a
   diagnostic instead of an undefined symbol.

---

### 5.6 Developer Experience (ergonomics requirements)

The feature is only worth shipping if it is pleasant and hard to misuse. These
are part of the acceptance bar, not optional polish.

1. **Surface names.** Expose the protocol **unqualified for users** even though
   it is declared under `core::async`: re-export `Poll`, `Future`,
   `FutureHandle`, `Context`, `Waker`, `Unit` from a `std` module (suggest
   `std::async` and `std::task`). Today `import std` is universal, so
   `import std` + `using namespace std::async;` must be enough. Never make the
   user remember `core::async::`.
2. **No `Pin`, no `Send`, no `unsafe` on the happy path.** The design already
   eliminates `Pin` (frames are stable by construction) and defers `Send`. Keep
   the first release free of both; a user writing `async func f() : T { … await
   g() … }` must never mention either.
3. **`async func main`.** Emit the block-on trampoline automatically (Phase 3)
   so the simplest program is `async func main() : int { … }`. If the `async`
   library is absent, emit the diagnostic in Section 15, never an undefined
   symbol.
4. **Cancellation is dropping.** Do not add a `cancel()` API in v1. Dropping a
   `FutureHandle<T>` (scope exit, `return`, early `break`) cancels and runs live
   destructors exactly once. Document this prominently; it is the single most
   pleasant thing about the model.
5. **Deterministic testing executor.** Ship `async::test::block_on` and a
   single-threaded `LocalExecutor::run_until_idle` that resolves immediately-ready
   futures without an I/O reactor, so `@test` functions can exercise async code
   deterministically (no sleeps, no flakiness).
6. **Never silently drop a future.** A future value whose result is never awaited
   and whose result has a destructor should warn
   (`unused future; did you mean to await it?`). This is the async analogue of
   `must_use`. At minimum emit it for `FutureHandle<T>`.
7. **Clear diagnostics.** Every diagnostic in Section 15 should include the
   awaited expression's source span and, when known, the concrete future type.
   `await` outside `async` must point at the enclosing `func` and suggest adding
   `async`.
8. **Explain suspension points.** In debug builds, generate a per-await-site
   `state` name (e.g. `state "await at fetch.ch:42"`) in the frame so a debugger
   shows where a task is parked. This is cheap and pays for itself immediately.
9. **IDE/LSP.** `async`/`await` are already valid semantic-token names
   (`core/targets/LSPMain.cpp:77`) and the JS-facing `Async`/`Await` tokens are
   unrelated. Add hover/completion entries for the `std::async` re-exports and a
   signature help for `block_on`/`spawn`.
10. **Documented first example.** Ship the Section 21.1 example in the language
    docs, plus a two-task `select` example and a cancellation example.

### 5.7 Public API surface (concrete, v1)

Everything the user touches, and where it lives. The `core::async` declarations
are compiler-facing; the `std::async` re-exports are the ergonomic surface.

```chemical
// ---------- core (declared in lang/libs/core/async.ch, module `core`) ----------
namespace core::async {
    variant Poll<T>      { Ready(value : T)  Pending() }
    struct  Unit         { }
    struct  Waker        { data : *mut void; vtbl : *WakerVTable
                           func wake(&self); func clone(&self) : Waker
                           @delete func delete(&mut self) }
    struct  WakerVTable  { wake : (data : *mut void) => void
                           clone : (data : *mut void) => Waker
                           drop : (data : *mut void) => void }
    struct  Context      { waker : Waker }
    interface Future<T>  { func poll(&mut self, cx : *mut Context) : Poll<T> }
    struct  FutureTable<T>{ poll : (frame : *mut void, cx : *mut Context) => Poll<T>
                            drop : (frame : *mut void) => void }
    @direct_init
    struct  FutureHandle<T> { frame : *mut void; vtbl : *mut FutureTable<T>
                              @delete func delete(&mut self) }
    @extern func chemical_async_frame_alloc(size : size_t, align : size_t) : *mut void
    @extern func chemical_async_frame_free(ptr : *mut void, size : size_t, align : size_t)
}

// ---------- std (re-exports in lang/libs/std/src/async.ch, module `std`) ----------
namespace std::async {
    // `using` re-exports so `import std` + `using namespace std::async` suffices:
    using core::async::Poll;
    using core::async::Unit;
    using core::async::Waker;
    using core::async::Context;
    using core::async::Future;         // the TRAIT (hand-authored futures only)
    using core::async::FutureHandle;   // the concrete future returned by async funcs
}

// ---------- lang/libs/async (executor library, module `async`) ----------
namespace async {
    func block_on<T>(handle : FutureHandle<T>) : T
    func spawn<T>(handle : FutureHandle<T>) : JoinHandle<T>
    struct JoinHandle<T> { handle : FutureHandle<T> }
    func yield_now() : FutureHandle<Unit>
    func sleep(millis : u64) : FutureHandle<Unit>
    namespace test {
        func block_on<T>(handle : FutureHandle<T>) : T      // deterministic, no reactor
        func run_until_idle(exec : *mut LocalExecutor)
    }
}
```

**Canonical user program (v1):**

```chemical
import std
import async

async func fetch(url : *char) : string {
    await async::sleep(10u)
    return std::string(url)
}

public func main() : int {
    var result = async::block_on(fetch("https://example.com"))
    println(result)
    return 0
}
```

**Name-collision guard (B5).** `std::concurrent::Future<T>` already exists
(blocking threadpool). `std::async::Future<T>` is the async trait. Do not write
`using namespace std::concurrent;` and `using namespace std::async;` in the same
scope, and never merge them in a `std` root namespace. The compiler's own
`await` resolution uses the `CoreNodes` handles, never a name, so it is immune.

**What the user must never write:** `Pin`, `Send`, `unsafe` around `await`,
`core::async::` qualification, `FutureTable`, or `chemical_async_frame_alloc`.

## 6. Compiler Representation

### 6.1 Lexer

- Add to `lexer/TokenType.h`: `AsyncKw`, `AwaitKw`.
- **Append them at the end of the enum, after the existing last token**, and
  mirror identically in `lang/libs/compiler/src/ChemicalTokenType.ch`. Do **not**
  insert them in the keyword block. `Token::isKeyword`
  (`lexer/Token.h:43`) uses strict `>`/`<` against `IndexKwStart=ForKw` /
  `IndexKwEnd=ConstKw`; `WhereKw` already sits outside that range and is handled
  explicitly. Appending avoids shifting `Identifier` and every punctuation
  value, which is exactly the CBI off-by-one hazard documented in AGENTS.md.
- Add the two entries to the keyword map in `lexer/Lexer.cpp` (the
  `std::unordered_map` at ~line 62): `{"async", AsyncKw}`, `{"await", AwaitKw}`.
  Because they are out of the `isKeyword` range, the parser must dispatch on
  them explicitly (Sections 6.3 and 16.3), which is what we want: they can never
  be consumed as identifiers.

### 6.2 AST flags

- `FuncDeclAttributes` (`ast/structures/FunctionDeclaration.h:72`): add
  `bool is_async = false;` and an accessor pair `is_async()` / `set_async(bool)`.
  The struct has default member initializers and is aggregate-initialized
  positionally in the `FunctionDeclaration` constructor with 18 of its 23
  fields (fields 1–18); `is_async` **must be appended at the end** so the
  positional initializers are unaffected. Verified: 23 members today, no
  `is_async`.
  > **CBI note:** `FuncDeclAttributes` is mirrored by `FuncDeclAttributesCBI`
  > (`compiler/cbi/bindings/ASTCBI.h:14`) with converters in
  > `ASTBuilderCBI.cpp` (`FunctionDeclarationgetAttributes`/`setAttributes`).
  > That mirror is **not** auto-updated. Plugins do not need `is_async` for v1,
  > so it is acceptable to leave the CBI struct unchanged, but if a plugin must
  > observe it, add the field to **both** and keep the order identical (AGENTS.md
  > enum-sync rule).
- `FunctionTypeData` (`ast/types/FunctionType.h:33`): add
  `bool is_async = false;`. The struct grows from 4 to 5 bytes and still
  satisfies `static_assert(sizeof(...) <= 8)`. Add a setter `setIsAsync(bool)`
  and set it from the parser for declarations and lambdas. If you instead add it
  to a constructor parameter, these sites construct/forward `data` and must be
  updated consistently: `FunctionType.h:75` (4-arg ctor), `FunctionType.h:89`
  (5-arg/extension ctor), `FunctionType.cpp:250` (`FunctionType::copy`),
  `FunctionDeclaration.h:239`, `LambdaFunction.h:50`, `LexType.cpp:42`,
  `CLANG.cpp:199,207`, `ASTBuilderCBI.cpp:220`. Prefer the setter to avoid the
  blast radius; `copy_into`/`shallow_copy_into` already copy the whole `data`
  struct, so a setter-propagated bit survives copying.

Why both: `FunctionTypeData` is what `FunctionType::copy_into`/`shallow_copy_into`
propagate, so lambda types and instantiated function types keep the bit; the
attribute is what symres/codegen key off on a `FunctionDeclaration`.

### 6.3 Parser

All function parsing funnels through
`Parser::parseFunctionStructureTokens` (`parser/structures/Function.cpp:457`),
which currently begins by consuming `FuncKw` at line 459. Callers pass
`specifier`/`is_comptime` in, so those modifiers are parsed *outside*.

Concrete changes:

1. **Accept `async` before `func`.** At the start of
   `parseFunctionStructureTokens`, `const bool is_async = consumeToken(AsyncKw);`
   (or check `token->type == AsyncKw` and advance), then require `FuncKw`.
   Store on the created declaration via `decl->set_async(is_async)` and
   `decl->data.is_async = is_async`.
2. **Add dispatch cases.** Every caller that switches on the leading token must
   recognize `AsyncKw` and route into the function parser:
   - Top level: `parser/statements/LexStatement.cpp` (`parseTopLevelStatement`
     ~74-143 and `parseTopLevelAccessSpecifiedDecl` ~35-72).
   - Struct members: `parser/structures/Struct.cpp:115,153`.
   - Variant members: `parser/structures/Variant.cpp:111`.
   - Interface/impl/union/inline `type`: all go through
     `Parser::parseContainerMembersInto` (`parser/structures/Struct.cpp:159`).
   Doing this at each switch site is required because `AsyncKw` is not in the
   `isKeyword` range and will otherwise fall through to the default case.
   Alternatively, add a single `peek` helper
   `is_func_start_async()` used by all sites; either is acceptable, but the
   switch-site edits are mandatory regardless.
3. **`await` prefix parsing.** Model it on `Parser::parseNotValue`
   (`parser/utils/Expression.cpp:305`). Add:

   ```cpp
   AwaitExpression* Parser::parseAwaitValue(ASTAllocator& allocator) {
       auto& tok = *token;
       if(tok.type != TokenType::AwaitKw) return nullptr;
       token++;
       // parse the operand at unary precedence so `await f()` and
       // `await x.m()` bind correctly
       auto inner = parseAccessChainOrValueNoAfter(allocator);
       if(!inner) { error("expected an expression after 'await'"); return nullptr; }
       return new (allocator.allocate<AwaitExpression>())
           AwaitExpression(inner, loc_single(tok));
   }
   ```

   Register it in **all** value entry points, because `await` is a keyword and
   `consumeIdentifierOrKeyword` will not swallow it:
   - `parseAccessChainOrValueNoAfter` (`parser/utils/LexValue.cpp:836`)
   - `parseAccessChainOrValue` (`parser/utils/LexValue.cpp:881`)
   - `parseAccessChainOrAddrOf` (`parser/statements/AccessChain.cpp:173`)
   - `parseProvideValue` (`parser/statements/LexStatement.cpp:318`)
   - `parseLhsValue` (`parser/statements/AccessChain.cpp:141`) — to emit a
     clean "cannot assign to an await expression" rather than a parse error.
4. **Async closures.** In `Parser::parseLambdaValue`
   (`parser/values/LambdaValue.cpp:42`) the callers detect `|`/`||`. Detect
   `AsyncKw` at the same value-dispatch sites (`LexValue.cpp:859,903`) before
   the pipe cases: consume `async`, then parse the lambda, then
   `lambda->data.is_async = true`. `async` without a following lambda is an
   error.

### 6.4 `AwaitExpression` node

New file `ast/values/AwaitExpression.h` (+ `.cpp`), modelled on `UnsafeValue`
(transparent, single child) but with a computed result type. It must override
the complete set of extension points listed in Section 16.4.

Key shape:

```cpp
class AwaitExpression : public Value {
public:
    Value* inner;                       // expression producing a Future/awaitable
    BaseType* await_result_type = nullptr;  // T, set in symres

    inline AwaitExpression(Value* inner, SourceLocation loc)
        : Value(ValueKind::AwaitExpr, nullptr, loc), inner(inner) {}

    Value* copy(ASTAllocator& allocator) override {
        return new (allocator.allocate<AwaitExpression>())
            AwaitExpression(inner->copy(allocator), encoded_location());
    }

    ASTNode* linked_node() final { return inner->linked_node(); }

    Value* evaluated_value(InterpretScope& scope) override;  // Section 11

#ifdef COMPILER_BUILD
    llvm::Value* llvm_value(Codegen& gen, BaseType* expected_type = nullptr) final;
    llvm::Type*  llvm_type(Codegen& gen) final;
    // plus assignment/arg/branch/pointer overrides as required by Section 16.4
#endif
};
```

Add `AwaitExpr` to `ValueKind` **at the very end**, and mirror it at the end of
`lang/libs/compiler/src/ast/base/ValueKind.ch`. Add the forward declaration in
`ast/base/ast_fwd.h`. Add the header/source to `CMakeLists.txt`.

**Invariant established by Section 7:** after normalization, `AwaitExpression`
appears only as the initializer of a `VarInitStatement`. Backends may rely on
this.

### 6.5 Async lowering metadata

Do **not** add fields to `FunctionDeclaration` for frame info; the layout is
computed at codegen time from the (already resolved) body and is cached per
`FunctionDeclaration*` in the `Codegen` context, keyed by AST node and generic
instantiation. This keeps generics and parallel codegen working: each
instantiation is a distinct `FunctionDeclaration` with its own cache entry.

The shared plan type (lives in the compiler, e.g.
`compiler/async/AsyncLoweringPlan.h`):

```cpp
struct AwaitSite {
    unsigned resume_state;                 // state to jump to on resume
    std::vector<unsigned> live_drops;      // frame slot ids to drop on cancel
    BaseType* awaited_type;                // T
};

struct AsyncLoweringPlan {
    std::vector<AwaitSite> sites;          // in program order
    unsigned frame_size = 0;
    unsigned frame_align = 0;
    bool needs_frame = false;              // false for non-suspending async fns
    bool result_has_destructor = false;
    // slot -> source variable name (for debugging / C naming)
};
```

This plan is produced by a single pass over the body (Section 8) and consumed by
both backends. It is the contract between them.

---

## 7. Await Normalization Pass

**Goal:** make every suspension point a statement so that both backends only
ever have to handle one shape, and so that evaluation order is unambiguous.

**Placement:** immediately after type verification and generic instantiation,
before codegen. It is a per-function AST rewrite; it can run in parallel across
functions. It must run before any backend-specific codegen.

**Algorithm** (post-order over the body):

1. When an `AwaitExpression` is encountered as the sole initializer of a
   `VarInitStatement`, leave it; it is already normalized.
2. Otherwise, wrap it: create a fresh `VarInitStatement`
   `var __awaitN : T = <AwaitExpression>` at the nearest statement boundary
   before the enclosing statement, and replace the `AwaitExpression` in place
   with an identifier referencing `__awaitN`. `T` is
   `AwaitExpression::await_result_type`.
3. Evaluation order: hoisting must preserve Chemical's left-to-right operand
   evaluation. Visit operands left to right, emitting hoisted temps in that
   order. For `f(await a, await b)` the result is
   `var t1 = await a; var t2 = await b; f(t1, t2)`.
4. Special forms:
   - `if(await c) { A } else { B }` → `var t = await c; if(t) { A } else { B }`.
   - `while(await c) { B }` → `loop { var t = await c; if(!t) { break } B }`.
   - `return await e` → `var t = await e; return t`.
   - `await e` as a bare statement → `var t = await e` (the temp is unused and
     must still be dropped normally).
5. Run only inside `async` function/closure bodies. An `AwaitExpression` in a
   non-async body is a verify error (Section 15), not a normalization target.

**Invariant after this pass:**
> Every `AwaitExpression` node in a compiled body is the initializer of a
> `VarInitStatement`, has static type `T`, and its operand has static type
> `S` where `S` satisfies `Future<T>`. The compiler then materializes `S` into a
> `FutureHandle<T>` (D13) before emitting the poll loop.

This invariant is what lets `AwaitExpression::llvm_value` /
`VisitAwaitExpression` assume statement context.

**Compile-time cost:** one extra traversal per async function only. Non-async
functions are skipped entirely (no `AwaitExpression` can exist in them). No
global analysis.

### 7.6 Implementation notes (concrete)

The pass is a small `RecursiveVisitor`-style rewriter, not a new node type.
Implementation guidance that avoids the common traps:

- **Where it runs.** After `sym_res_module` completes (identifiers + call types
  resolved) and before type verify/codegen; see Section 16.10's pipeline row.
  Running it before type verify is also acceptable and arguably better (verify
  then sees the normalized form), but then the awaitable check must run on the
  original operand before it is replaced; pick one and be consistent.
- **Hoisting machinery.** To insert `var __awaitN : T = <await>` before the
  enclosing statement, use the enclosing `Scope`/`CompoundStatement` node list
  and insert at the statement's index. Give the temp a unique name
  (`__chx_await_<N>`), allocate it with `ASTAllocator`, and make its `linked`
  point at the created `VarInitStatement`. Replace the original `AwaitExpression`
  node (not just its child) with a `VariableIdentifier` whose `linked` is that
  statement, so `getType()` returns `T`.
- **Do not re-order.** Visit operands strictly left-to-right and insert temps in
  that order. For `f(await a, await b, g(await c))` the result must be
  `t1=await a; t2=await b; t3=await c; f(t1,t2,g(t3))`. If Chemical's evaluation
  order is not strict left-to-right for some construct, do not normalize that
  construct blindly; document and test it (Section 19.3).
- **Compound assignments and inc/dec.** `x += await e` and `*p = await e`
  hoist the RHS first, then apply the operator/assignment to the temp. Awaited
  operand of `&`/`&raw`/`sizeof` is a diagnostic (you cannot take the address of
  a suspension point).
- **Conditions.** `if`, `while`, `do-while`, and the condition of a `for` are
  special-cased (algorithm items 1–4). A `switch` value (`switch(await e)`) is
  hoisted like a normal value; a `for-in` range expression containing `await` is
  hoisted before the loop.
- **Multiple awaits in one initializer.** `var x = await a + await b` is *not*
  already normalized (the initializer is not a bare `AwaitExpression`); hoist
  both into temps and rewrite the initializer to `x = t1 + t2`.
- **`await` as the direct initializer** is the only form left untouched, and it
  is what the invariant in Section 7 guarantees.
- **Idempotence.** The pass may be run twice (e.g. a re-emitted generic
  instantiation); it must be idempotent. Detect "initializer is a bare
  `AwaitExpression`" and skip.
- **Generics.** Run normalization on the *instantiated* body, i.e. after
  `GenericInstantiator` finalization, so `T` in `var __awaitN : T` is concrete.
  `GenericInstantiator::VisitAwaitExpression` (Section 16.4 item 10) must
  re-monomorphize `inner` first.
- **Interpreter.** The interpreter does not consume the normalized form
  (Section 11 evaluates `AwaitExpression` directly), so the pass should not
  break interpretation of the same module. Keep `AwaitExpression` a valid AST
  node after normalization even if all compiled uses are VarInit initializers.

### 7.7 Materialization during normalization (optional, recommended)

The pass can also perform D13 materialization (Section 16.7), turning
`var __awaitN : T = <await E>` into
`var __futN : FutureHandle<T> = <materialize E>; var __awaitN : T = await_handle(__futN)`.
Doing it here means backends only ever see the handle-poll shape. Doing it in
the backends keeps the pass smaller but duplicates the logic twice (LLVM + 2c).
**Recommendation:** materialize in the shared pass (one implementation), and
leave both backends to emit only `poll`/`suspend`/`drop`.

---

## 8. Frame Layout and Drop Planning

This section is the heart of correctness. Both backends must produce a frame
and a drop function that obey these rules exactly.

### 8.1 What goes into the frame

For an async function `foo` whose body may suspend:

1. **Header:** `state: u32` (resume index). For LLVM, the coroutine runtime adds
   its own header before ours; our `state` is a normal field we maintain.
2. **Context:** `cx: *mut Context` — the most recent context passed to `poll`,
   stored so nested `poll` calls within a resumption use the current waker.
3. **Parameters:** all parameters, by value (moved in) or as raw pointer/reference
   values (copied). Parameters are stored in the frame at ramp time because they
   are live from state 0.
4. **Live locals:** every local variable (and every temporary that holds an
   awaited child future) that is live across **at least one** suspension point.
   Locals only used within a single resume window stay on the native stack and
   are recomputed on resume — but **only if they are trivially re-evaluable**;
   otherwise they must be in the frame. Simplest correct rule: any local whose
   scope spans an await goes in the frame. (Optimization: locals declared after
   the last await before a return can stay on the stack; see 14.3.)
5. **Awaited child futures:** the `FutureHandle<T>` being awaited at a
   suspension site. It must be a frame slot because it is read again on resume.
6. **Result slot:** storage for `T` written by `return expr`. Present only if
   `T` is non-void.
7. **Drop flags:** `bool` per ambiguous local (see 8.4).

### 8.2 Slot assignment

- Assign frame slots in declaration/creation order.
- Each slot has a type, an alignment, and an owning variable name.
- Frame `size`/`align` are computed with the target `TargetData`, the same way
  struct layout is computed elsewhere.
- For **LLVM**, the frame is the memory pointed at by `coro.begin`; our slots
  are GEPs from `hdl` past the coroutine header. `coro.size` must be set to the
  full size including the header. (See Section 9 for the exact order of
  `coro.id`/`coro.alloc`/`coro.begin`/`coro.promise`.)
- For **2c**, the frame is a generated C `struct` (Section 10.2).

### 8.3 Live-drop sets

The C backend already maintains a **destructor stack per scope**
(`ToCAstVisitor::visit_value_scope(scope, destruct_begin)`,
`2cASTVisitor.h:591`). At each await site, snapshot the current destructor
stack, in reverse order of creation. That snapshot is the `live_drops` list for
that `AwaitSite`. When the future is cancelled (dropped while suspended at that
state), exactly those destructors run, then the frame is freed.

When a local is moved out or explicitly destroyed along the normal path, its
scope handling already removes it from the destructor stack; the snapshot for a
later await will simply not contain it.

### 8.4 Ambiguous liveness → drop flags

A slot's liveness at drop time is **static** if every path reaching a given
state has the same set of live destructible slots. Otherwise it is **ambiguous**
and needs a runtime `bool` drop flag:

- The flag is stored in the frame.
- Set to `true` immediately after the local is initialized, `false` after it is
  moved out or dropped.
- `drop` checks the flag before running the destructor.

This mirrors the existing `set_drop_flag_for_ref` / definite-assignment
machinery. Prefer static sets; use flags only where branches make it ambiguous.

### 8.5 The drop function

`foo_drop(frame)` (cancellation / scope-exit path only):

```c
void foo_drop(void* frame) {
    FooFrame* f = (FooFrame*)frame;
    if (f->state == STATE_DONE) {
        // normal completion already ran every live local's destructor (8.7)
        // and moved the result out; only free the frame.
        free_frame(f);
        return;
    }
    switch (f->state) {
        case 0: goto drop0;
        case 1: goto drop1;
        ...
    }
drop1:
    // slots live at state 1, reverse order
    if (f->flag_t) { destroy_T(&f->t); }
    // fallthrough to earlier scopes
drop0:
    destroy_A(&f->a);
    free_frame(f);
}
```

### 8.6 Initialization and completion protocol

- Ramp: allocate frame, `state = 0`, store parameters, return handle. Body not
  run (D2).
- `poll` entry: store `cx` into the frame, `switch(state)` dispatch, run until
  the next suspend or completion.
- Suspend at site `i`: store `state = i`, return `Poll.Pending`.
- Resume: `state == i` dispatches to the continuation after site `i`.
- `return expr`: evaluate `expr` into the result slot, **run the same destructor
  sequence a normal function runs at `return`** (see 8.7), then set
  `state = STATE_DONE` and return `Poll.Ready(result_moved_out)`.

### 8.7 Completion cleanup (correctness fix)

The second review's `STATE_DONE` rule said "the body completed and the result
was handed to the caller, so only the (empty) frame is freed". That is wrong:
**locals other than the result may still be live at the `return` statement**,
and a normal function destroys them on the way out. Skipping them leaks
everything live at `return` (e.g. a `std::string` local, a buffered writer).

The rule is:

1. On `return expr`, the generated code evaluates `expr` into the result slot
   (or into a temp, mirroring the normal return-move path — see the C backend's
   `writeReturnStmtFor`, `2cASTVisitor.cpp:3654`, and the LLVM backend's
   `Codegen::writeReturnStmtFor`, `LLVM.cpp:2104`).
2. Then it emits the **normal-scope destructor sequence** for every live
   destructible local/parameter, in reverse creation order, exactly as a
   non-async function would at the same point. The result slot is excluded
   because it was just moved out.
3. Only then is `state = STATE_DONE` stored and `Poll.Ready` returned.

`foo_drop` for `STATE_DONE` therefore runs **no user destructors** — the
completion path already did. The per-state `switch` in `foo_drop` is for the
*abnormal* path: dropping a still-suspended future (cancellation, scope exit,
early `break`).

Both backends must follow this ordering. The verified C state machine
(`/tmp/opencode/coro/state.c`) demonstrates the invariant: `fdrops_after_complete`
is 0 (completion handled its own cleanup) and `drops_after_cancel` is 1 (the
drop switch ran the suspended local's destructor exactly once).

> **Result never taken.** If a user drops a `Ready` future without awaiting it,
> the result slot must still be destroyed. Because our `await` desugaring always
> moves the result out immediately at `Ready`, this only matters for hand-held
> futures. Implement it with a `result_taken` bit (Section 13.3): the `Ready`
> path sets it after the move; `foo_drop` destroys the result slot when
> `STATE_DONE && !result_taken`. Keep the flag rather than adding a second
> `STATE_DONE_*` value, for uniformity with 8.4.

### 8.8 Frame layout algorithm (concrete)

The plan computes the frame once per instantiated `FunctionDeclaration`. Exact
procedure:

1. **Collect slots in creation order.** Walk the body. For each parameter, and
   for each local/temporary that is live across at least one `AwaitExpression`
   (the simple rule), create a slot `{ name, BaseType*, is_drop_flag,
   owner_kind }`.
2. **Header slots first:** `state : u32`, `cx : *mut Context`, and (LLVM only)
   an ownership bit `owns_frame : bool`. These are slot 0..2.
3. **Result slot** if the body result is non-void; `result_taken : bool` next to
   it.
4. **Compute size/alignment with `TargetData`.** Mirror the struct layout code:
   `offset = align_up(offset, target.alignof(type))`, then
   `offset += target.sizeof(type)`. Store the per-slot offset in the plan so
   both backends agree.
5. **Frame alignment** = max of all slot alignments (at least 16 on x86-64 for
   the coroutine header). `coro.size` (LLVM) must include the coroutine runtime
   header *plus* our slots; `coro.promise` (or a GEP from `coro.begin`'s result)
   locates our first slot.
6. **For LLVM:** represent the frame as a named `%Foo.Frame = type { ... }` and
   GEP with `CreateStructGEP`; never hand-compute byte offsets in IR.
   For 2c: emit the C `struct FooFrame { ... }` with the same field order, and
   `_Alignof`/`sizeof` are computed by the C compiler (keep the order identical
   to the plan so the two backends have the same layout).
7. **Drop-flag insertion.** For each slot whose liveness at drop time is
   ambiguous (Section 8.4), insert a `bool` slot immediately *before* the
   payload slot (so a single `state` dispatch can guard it) and record it.
8. **Diagnostics.** If a slot type is incomplete/unsized at this point, emit a
   clear error (never emit a byte size of 0 silently).

### 8.9 Slot ID stability across the plan's consumers

`AsyncLoweringPlan::live_drops` stores slot **indices**, not names. Keep the
index assignment deterministic (creation order, stable sort) so a re-emission of
the same function in the same compilation reproduces identical indices. Do not
persist the plan across compiler invocations (generic instantiations may differ);
recompute per compilation (Section 14.1).

---

## 9. LLVM Backend Lowering

### 9.0 Corrected, Verified Coroutine Recipe (read this first)

This subsection supersedes the loose sketches in 9.1–9.6 where they conflict. It
is the exact structure that was assembled into raw IR and executed through the
linked LLVM 22.1.8 at `-O0`, `-O1`, and `-O2`
(`/tmp/opencode/coro/ref2.ll`; the normal-path reference is reproduced in
Appendix E). Follow it literally. The numbered corrections are in Section 1.8.
The correction history matters: the *second* review's recipe was invalid (§1.8
→ "Corrected and re-verified LLVM coroutine recipe").

IR-level shape for `async func foo(x : i32) : i32` with two suspension points
(an initial lazy suspend, one real await):

```llvm
define ptr @foo(i32 %x) presplitcoroutine {          ; (1) attribute is mandatory
entry:
  %id    = call token @llvm.coro.id(i32 0, ptr null, ptr null, ptr null)
  %alloc = call i1   @llvm.coro.alloc(token %id)     ; (7) true == must allocate
  br i1 %alloc, label %alloca, label %after
alloca:
  %size = call i64 @llvm.coro.size.i64()
  %mem  = call ptr @chemical_async_frame_alloc(i64 %size, i64 <align>)
  br label %after
after:
  %phi = phi ptr [ %mem, %alloca ], [ null, %entry ]
  %hdl = call ptr @llvm.coro.begin(token %id, ptr %phi)
  br label %init
init:                                                ; (2) explicit lazy suspend
  %save_init = call token @llvm.coro.save(ptr %hdl)  ; (3) save before every suspend
  %s_init    = call i8    @llvm.coro.suspend(token %save_init, i1 false)
  switch i8 %s_init, label %suspend [                ; (4) default -> suspend block
      i8 0, label %body
      i8 1, label %cleanup ]
body:                                                ; user body up to first await
  %save0 = call token @llvm.coro.save(ptr %hdl)
  %s0    = call i8    @llvm.coro.suspend(token %save0, i1 false)
  switch i8 %s0, label %suspend [ i8 0, label %cont0  i8 1, label %cleanup ]
cont0:                                               ; after the await
  ; ... read result from frame, continue body ...
  %save1 = call token @llvm.coro.save(ptr %hdl)
  %s1    = call i8    @llvm.coro.suspend(token %save1, i1 true)  ; final suspend
  switch i8 %s1, label %suspend [ i8 0, label %fin  i8 1, label %cleanup ]
fin:
  br label %suspend
suspend:                                             ; return to caller
  ; (5) the ONE fallthrough coro.end for this coroutine
  call void @llvm.coro.end(ptr %hdl, i1 false, token none)
  ret ptr %hdl
cleanup:                                             ; destroy-while-suspended
  ; (6) run destructors for the *current* state only (Section 8.5), then:
  call void @llvm.coro.end(ptr %hdl, i1 true, token none)   ; (5) i1 TRUE here
  ret ptr %hdl
}
```

> **CORRECTED (third review).** `coro.end`'s second argument is `unwind`, not
> "fallthrough". Exactly **one** `coro.end` in the coroutine may be `i1 false`
> (the normal return); the destroy/cleanup path uses `i1 true`. The second
> review had `i1 false` on both paths, which makes `opt` abort at `-O0/-O1`
> with *"Only one coro.end can be marked as fallthrough"*. Verified against
> Clang's own pre-split IR (`clang -Xclang -disable-llvm-passes`), which emits
> `i1 false` on the normal path and `i1 true` on cleanup. Also note
> **`llvm.coro.end` returns `void` in LLVM 22** (Clang 21 emitted an `i1` result
> in LLVM 21); free the frame with `llvm.coro.free`, not from a `coro.end`
> result.

The `poll` function (the design's §9.4) is a **separate** function and lowers
correctly:

```llvm
define i1 @foo_poll(ptr %hdl) {          ; returns Ready/Pending; read result from frame
entry:
  call void @llvm.coro.resume(ptr %hdl)
  %done = call i1 @llvm.coro.done(ptr %hdl)
  ; if done: move result out, mark STATE_DONE, free the frame, return Ready
  ; else:    return Pending
  ret i1 %done
}
```

Rules that are easy to get wrong and were all observed to matter:

1. **`presplitcoroutine`** on the ramp or nothing lowers.
2. **Initial `coro.suspend`** for laziness (D2). Without it the body runs at call
   time up to the first real suspend.
3. **`coro.save` before every `coro.suspend`.** Omitting it made the first
   suspend silently not suspend at `-O0` while appearing to work at `-O2`.
4. **Switch default → `suspend` block.** Pointing the default at the
   continuation made the ramp execute the continuation body.
5. **`coro.end` unwind polarity (CORRECTED).** The normal return uses
   `coro.end(hdl, i1 false, token none)`; the destroy/cleanup path uses
   `coro.end(hdl, i1 true, token none)`. There must be exactly **one**
   `i1 false` (`fallthrough`) `coro.end`; two make `CoroSplit` abort with
   *"Only one coro.end can be marked as fallthrough"* at `-O0/-O1`. In LLVM 22
   `coro.end` returns `void`; free an owned frame via `llvm.coro.free`.
6. **`drop` must not run user destructors for a completed coroutine.** Dispatch
   on the stored state; completed ⇒ free the frame only. This is the LLVM
   materialization of §8.7's completion rule and §13.3's `result_taken`. The
   completion path itself runs the live-local destructors before setting
   `STATE_DONE` (Section 8.7) — do not skip them there either.
7. **`coro.alloc` polarity:** true ⇒ heap-allocate via our allocator; false ⇒
   LLVM may pass `null` and place the frame in the caller (`CoroElide`). Record
   in the frame whether the frame is owned (`alloc == true`) so `drop`/`poll`
   know whether they may free it.
8. **Debug and release agree.** The corrected recipe behaves identically at
   `-O0/-O1/-O2`. Do not rely on optimization to make a malformed coroutine
   "work" — the earlier malformed attempts only appeared to work at `-O2`.
9. **The destroy-while-suspended path is the highest-risk surface.** A
   hand-written toy destroy path was unstable: freeing before `coro.end`
   produces a use-after-free (the generated destroy function writes the resume
   index/function pointer to the frame after `coro.end`), and freeing the raw
   `coro.begin` pointer can corrupt the heap (the frame base is not always the
   allocation base; use `coro.free`). Mirror LLVM 22's Clang lowering and add a
   dedicated cancellation test before trusting it. Do **not** copy the old
   §9.5 text.

The 2c backend does not use any of this; it emits the state machine text directly
(§10). A future MIR could share one lowering (Section 20).

### 9.1 Generated functions

Use LLVM's classic coroutine intrinsics. This gives us the state machine, frame
layout, `resume`/`destroy` splitting, and (critically) `CoroElide`, which removes
the frame allocation when the coroutine's lifetime is confined to the caller —
the `await foo()` fast path. Reference model: Clang's C++20 coroutine lowering
(`CGCoroutine.cpp`) and the LLVM LangRef coroutine intrinsics. LLVM 22 is linked;
the verified recipe is §9.0.

> **Probe-mandated note (B1):** the poll/suspend loop is emitted here as basic
> blocks. Do **not** route it through a Chemical `loop` expression in a generic
> helper — generic loop-expression result types were miscompiled (Section 1.5,
> fixed) and the C backend's loop-expression cleanup was also buggy (B7, fixed).
> The generated `AwaitExpression::llvm_value` must build the loop inline.

For `async func foo(a: A) : T` the LLVM backend emits:

1. **`foo` (ramp)** — the user-visible function. Returns `FutureHandle<T>` (sret
   or by-value per the existing ABI rules; the handle is a 2-pointer struct with
   a destructor).
2. **`foo.resume` and `foo.destroy`** — produced by `CoroSplit` from the body.
3. **`foo_poll(frame, cx)`** — our vtable `poll`.
4. **`foo_drop(frame)`** — our vtable `drop`.
5. A **vtable constant** `foo_vtable` (a `FutureTable<T>` global).

### 9.2 Ramp recipe

At the start of the coroutine body (which is `foo` itself):

```
%id    = call token @llvm.coro.id(i32 0, ptr null, ptr null, ptr null)
%size  = call i64  @llvm.coro.size.i64()
%alloc = call i1   @llvm.coro.alloc(token %id)

; conditionally allocate via our allocator
br i1 %alloc, label %do_alloc, label %after_alloc
do_alloc:
  %mem = call ptr @chemical_async_frame_alloc(i64 %size, i64 <align>)
  br label %after_alloc
after_alloc:
  %frame = phi ptr [ null, %entry ], [ %mem, %do_alloc ]

%hdl = call ptr @llvm.coro.begin(token %id, ptr %frame)

; If CoroSplit elides the allocation, %alloc is false and %frame is null;
; coro.begin then places the frame in the caller's storage.
```

The **return object** `FutureHandle<T>` is constructed from `%hdl` plus a
pointer to `foo_vtable` and returned.

> **Correction (see §9.0 item 2):** "because the body is lazy, nothing else runs"
> is only true if the backend also emits an **explicit initial
> `coro.save`/`coro.suspend`** right after `coro.begin`, before any user body
> code. Raw LLVM coroutines otherwise execute the body up to the first suspend at
> call time. The corrected recipe in §9.0 includes this.

### 9.3 Promise / frame fields

`coro.promise` is used to locate our fields. The simplest arrangement is to let
the coroutine promise **be** our frame header: after `coro.begin`, compute
`%p = call ptr @llvm.coro.promise(ptr %hdl, i32 <align>, i1 false)` and GEP into
it for `state`, `cx`, params, locals and result. The exact promise alignment must
be passed consistently.

> Implementer note: `coro.promise` is a hint that survives splitting; if
> getting its alignment exactly right proves brittle, an equivalent and often
> simpler approach is to treat `%hdl` itself as the base and GEP to fields,
> since for the default coroutine ABI `coro.begin`'s result is the frame base.
> Pick one and use it consistently; do not mix.

### 9.4 `foo_poll`

```
define Poll_T @foo_poll(ptr %hdl, ptr %cx) {
  %frame = /* base */
  store ptr %cx, ptr %frame.cx
  call void @llvm.coro.resume(ptr %hdl)
  %done = call i1 @llvm.coro.done(ptr %hdl)
  br i1 %done, label %ready, label %pending
ready:
  %r = load T, ptr %frame.result
  ; mark STATE_DONE / result_taken; free the frame (do NOT re-run user
  ; destructors — see §9.5 and §13.3). If the frame is elided, do not free.
  ret Poll.Ready(%r)
pending:
  ret Poll.Pending
}
```

`llvm.coro.resume` runs the body until it hits `coro.suspend` (returns) or runs
to completion (reaches the final suspend). `llvm.coro.done` distinguishes the
two.

> **Correction (verified).** Do **not** unconditionally `coro.destroy` on the
> `done` branch. The completion path must first run the live-local destructors
> (Section 8.7) before marking `STATE_DONE`; after that, `coro.destroy` would
> re-enter cleanup and run them a second time. Mark the completion in the frame
> and free the frame only. The result was moved out, so no result destructor
> runs either (§13.3 `STATE_DONE`). The `foo_poll` `ready` branch must know
> whether the frame is owned (heap) or elided before freeing.

### 9.5 `foo_drop`

```
define void @foo_drop(ptr %hdl) {
  ; if the coroutine completed (STATE_DONE), the completion path already ran
  ; the live-local destructors (Section 8.7); only free the frame.
  ; otherwise run the per-state destroy path and then free.
  call void @llvm.coro.destroy(ptr %hdl)   ; runs the per-state cleanup
}
```

> **Correction (third review).** The unconditional `coro.destroy` above is only
> correct if the generated destroy path treats the completed state specially
> (skip user destructors; only free). More importantly, the destroy path itself
> is brittle and must be built against LLVM 22's exact ABI:
>
> - The cleanup block's `coro.end` must be `i1 true`, not `i1 false` (§9.0
>   item 5).
> - Do not `free` the frame *inside* the cleanup before `coro.end`: the
>   generated `foo.destroy` writes the resume index/function pointer to the
>   frame *after* `coro.end`, producing a use-after-free (observed).
> - Do not `free` the raw `coro.begin` pointer directly: it is not always the
>   allocation base. Obtain the freeable pointer from `llvm.coro.free`.
> - Because these details were empirically unstable in the toy, treat the
>   destroy path as an **open spike**: implement it by mirroring Clang's
>   LLVM 22 lowering and add an explicit cancellation test (drop a suspended
>   handle; assert each live local's destructor ran exactly once) before
>   relying on it.

The **destroy path** is where `@delete` destructors for live locals run. They
must be emitted as part of the coroutine body's cleanup:
- Around the body, establish a cleanup block that runs the per-state destructor
  sequence.
- `coro.suspend` is emitted with `final = false` at normal awaits and
  `final = true` at the final suspend; the destroy branch of each suspend
  (`i8 1` from `coro.suspend`'s switch) jumps to the appropriate per-state
  cleanup sequence generated from `AsyncLoweringPlan::live_drops`.
- Implement the per-state drop sequence as a chain of cleanup blocks, one per
  await state, mirroring the C `switch` in 8.5. The plan's `live_drops` tells
  you exactly which destructors to call at each state. The cleanup block's
  `coro.end` uses **`i1 true`** (§9.0 item 5).
- Free the frame only if it was heap-allocated (`coro.alloc == true`); an elided
  frame lives in the caller and must not be freed. Use `llvm.coro.free` to
  obtain the freeable pointer.

### 9.6 Await site codegen (`intrinsics::__await_suspend`)

```
; %child is a FutureHandle<T>, already created and stored in the frame
%r = call Poll_T @<child_poll>(ptr %child.frame, ptr %cx)
; switch on the Poll tag
switch i8 %tag, label %pending [
  i8 0, label %ready
]
ready:
  %value = extract T from %r
  br label %after_await
pending:
  %save = call token @llvm.coro.save(ptr %hdl)
  %is = call i1 @llvm.coro.suspend(token %save, i1 false)
  switch i8 %is, label %after_await [ i8 1, label %cleanup ]
cleanup:
  ; destroy path for this state (cancellation)
  br label %destroy
```

On resume, control continues at `after_await`, which computes `Poll.Ready`'s
value. The awaited child handle must be a frame slot so `%child` is reloaded
after the suspend rather than kept in an SSA register (the register does not
survive a suspend).

### 9.7 Fast paths (LLVM)

- **No-await async fn:** if the plan has `needs_frame == false`, do not emit any
  coro intrinsics and do not emit suspend points. The function still returns a
  `FutureHandle<T>`; build a tiny frame (`state = STATE_DONE`, result stored)
  and return a handle to it. **Do not** store that frame inside the returned
  handle: the handle is returned by value and moves, so a self-referential
  `frame` pointer would dangle. The frame must be separately allocated (heap or
  arena) — or placed in the caller by `CoroElide` for the direct-await case.
  This removes the *state machine* overhead for trivial async functions, but not
  the frame allocation unless LLVM elides it.
- **Ready child:** the `Poll.Ready` branch is already the fall-through; LLVM
  will inline/constant-fold if the child poll is known to return Ready.
- **Direct await + CoroElide:** leverage `coro.alloc`/`coro.begin` so LLVM can
  elide the frame when `await foo()` confines the coroutine. Do not defeat it
  with address-taking. Note that `CoroElide` only fires when the coroutine does
  not escape its creator, which is *not* the case for a plain
  `var h = foo(); await h`; it is the same-function `await foo()` shape.

---

## 10. C / 2c Backend Lowering

### 10.0 Verified by TinyCC

The generated-C shape in §10.1 (frame struct + resumable `for(;;) switch(state)
goto` body + per-state destructor `switch` in `drop` + function-pointer vtable)
was compiled and run with the bundled TinyCC
(`./lib/tcc/tcc -run /tmp/opencode/coro/state.c`). Observed:
`complete=105 polls=2`, `drops_after_complete=1`, `first_poll_pending=1`,
`drops_after_cancel=2`. TinyCC handles `switch`, `goto`, compound literals,
designated initializers, and function pointers in structs. The C backend writes
text, so no Chemical loop expression or goto AST is needed (B7 is therefore
irrelevant to generated await code, though it matters for hand-written Chemical
helpers).


The C backend writes text; it can freely emit `switch`, labels and `goto`. TinyCC
supports these standard C constructs. Since TinyCC is **not** an optimizer, the
generated C must be efficient by construction.

> **Probe-mandated notes.**
> - Emit the poll loop as text (`while(1){...}` or the `switch` state machine).
>   Do **not** lower it through a Chemical `loop` expression (B1).
> - Call `poll` through the handle's vtable (`f->vtbl->poll(f->frame, cx)`), or
>   directly on a generated frame's poll function. Do **not** emit a call to an
>   interface's generic `poll` — impl-only method calls currently emit the wrong
>   symbol (B2).
> - Function pointer fields in the generated `FutureTable<T>` must have named
>   parameters in any Chemical surface representation (B4).

### 10.1 Generated C for `async func foo(a: A) : T`

```c
/* frame */
typedef struct FooFrame FooFrame;
struct FooFrame {
    uint32_t state;
    bool     owns_frame;     /* false when CoroElide-style elision is used */
    Context* cx;
    A        a;              /* parameters */
    /* live locals and awaited child futures */
    Future_T child_0;
    Maybe_T  tmp_0;
    bool     flag_tmp_0;     /* only when liveness is ambiguous */
    T        result;
    bool     result_taken;   /* set true when Ready moves the result out */
};

/* forward decls */
static Poll_T foo_poll(FooFrame* f, Context* cx);
static void   foo_drop(FooFrame* f);
static const FutureTable_T foo_vtable = {   /* FutureTable<T> = { poll, drop } */
    (Poll_T(*)(void*,Context*)) foo_poll,
    (void(*)(void*))            foo_drop
};

/* ramp (lazy: body does not run) */
Future_T foo(A a) {
    FooFrame* f = (FooFrame*) chemical_async_frame_alloc(sizeof(FooFrame), _Alignof(FooFrame));
    f->state = 0u;
    f->owns_frame = true;
    f->result_taken = false;
    f->a = a;
    return (Future_T){ .frame = (void*)f, .vtbl = &foo_vtable };
}

static Poll_T foo_poll(FooFrame* f, Context* cx) {
    f->cx = cx;
    for(;;) {
        switch(f->state) {
            case 0u: goto L0;
            case 1u: goto L1;
            /* ... one case per await site ... */
            case STATE_DONE:                       /* result already moved out */
                f->result_taken = true;
                return (Poll_T){ .tag = POLL_READY, .value = f->result };
            default: __builtin_trap();             /* corrupt state, never L0 */
        }
    L0:
        /* ... body up to first await ... */
        f->child_0 = child_create(...);
        f->state = 1u;
        /* fallthrough into poll of child */
    L1:
        {
            Poll_T r = (Poll_T) child_poll(f->child_0.frame, f->cx);
            if (r.tag == POLL_PENDING) { return (Poll_T){ .tag = POLL_PENDING }; }
            f->tmp_0 = r.ready;
        }
        /* ... continue body ... */
        /* on `return expr`: Section 8.7 — move expr to result, THEN run the
           normal-scope destructors of every live local, THEN mark done. */
        f->result = expr;
        destroy_live_locals(f);          /* reverse order, excludes result */
        f->state = STATE_DONE;
        return (Poll_T){ .tag = POLL_READY, .value = f->result };
    }
}

static void foo_drop(FooFrame* f) {
    if (f->state == STATE_DONE) {
        if (!f->result_taken) { destroy_T(&f->result); }
        chemical_async_frame_free(f, sizeof(FooFrame), _Alignof(FooFrame));
        return;
    }
    /* abnormal path: drop the locals live at the suspended state */
    switch(f->state) {
        case 1u: if (f->flag_tmp_0) { destroy_Maybe_T(&f->tmp_0); }
        case 0u: destroy_A(&f->a);
            break;
        default: break;
    }
    chemical_async_frame_free(f, sizeof(FooFrame), _Alignof(FooFrame));
}
```

Notes:
- The trailing `for(;;)` + `switch` is the standard resumable-function idiom:
  the switch runs exactly once per `poll` entry, then the body runs with `goto`
  dispatch; on resume the switch jumps directly to the last label.
- The awaited child handle and all cross-await temporaries are frame fields,
  never C locals.
- `destroy_*` calls are generated by the existing destructor machinery
  (`ToCAstVisitor::CDestructionVisitor`, see Section 10.5).
- **`default:` must trap, not `goto L0`.** Falling back to state 0 on an
  unexpected state re-runs the body from the top; the second review called this
  out. `STATE_DONE` is an explicit case that returns `Ready`.
- The `return expr` branch runs the completion cleanup (Section 8.7) **before**
  `state = STATE_DONE`. This is the single most likely place to leak or
  double-destroy.

### 10.2 Naming and mangling

Frame struct and functions use the existing mangler so that generic
instantiations and nested scopes get unique names. `foo_poll`/`foo_drop`/frame
names are derived from the mangled function name plus a suffix (`__frame`,
`__poll`, `__drop`). Do not hand-roll names.

### 10.3 Where in `2cASTVisitor`

`ToCAstVisitor` is a `NonRecursiveVisitor`. Implement:
- `VisitAwaitExpression` — by the Section 7 invariant it is always in a
  `VarInitStatement` initializer, so it can emit the poll/suspend sequence inline
  and is responsible for registering the child handle as a frame slot.
- A new per-function prologue/epilogue path used by `VisitFunctionDecl` when
  `attrs.is_async` is set: emit the frame struct, ramp, `poll`, `drop`, vtable.
- Hook the existing `visit_value_scope(scope, destruct_begin)` snapshots at await
  sites to build `live_drops`.

The C translator is stateful and single-threaded per module; the frame slot
registry should live on the visitor for the duration of one function's body, not
globally.

### 10.4 Fast paths (2c)

- **No-await async fn:** emit no state machine and no poll/drop switch; emit a
  ramp that allocates a tiny ready frame (`state = STATE_DONE`, result stored)
  and returns a `FutureHandle<T>`. The frame must be separately allocated, not
  stored inside the returned handle (the handle moves). This is the common case
  for trivial async helpers.
- **Direct await with known frame:** optionally stack-allocate the child frame
  at the await site (`FooFrame __f; foo_init(&__f, ...);`) when the awaited
  expression is a direct call to a statically known async function and the
  future does not escape. Guard behind a flag until proven; it changes aliasing
  and is easy to get subtly wrong. **Phase 2 optimization.**
- **No `dyn` dispatch for `poll`** (B8): always call the concrete/generated
  `poll` through the handle's `FutureTable<T>` field, never `dyn Future<T>`.

---

## 11. Interpreter and Comptime

### 11.1 `AwaitExpression::evaluated_value`

The interpreter cannot suspend. It evaluates `inner`, then drives `poll` with a
no-op `Context`:

```cpp
Value* AwaitExpression::evaluated_value(InterpretScope& scope) {
    Value* fut = inner->evaluated_value(scope);
    if (fut == nullptr) return nullptr;
    // Loop poll with a no-op waker.
    for (;;) {
        auto poll_result = call_poll(fut, noop_context(scope));
        if (poll_result is Ready(v)) return v;
        // Pending:
        if (scope.global->is_comptime() || is_interpretation) {
            scope.global->diagnoser.error(this,
                "'await' cannot suspend during comptime evaluation / interpretation");
            return nullptr;
        }
        // Compiled-interpreter fallback: block on the future if it has
        // a blocking adaptor, else error.
        return block_on_fallback(fut, scope);
    }
}
```

Because interpretation tests and comptime code use immediately-ready futures
(pure async functions with no real I/O), the loop resolves on the first
iteration. `Pending` at comptime is a hard error (D8).

### 11.2 Async function calls in the interpreter

An async function is still a normal `FunctionDeclaration`; the interpreter can
call its body directly. For an eager/comptime call, evaluate the body with
`await` handling as above and return the result. The interpreter does **not**
build a frame. This means interpreter semantics are "run to completion or fail",
which is correct for pure async functions.

### 11.3 Comptime rule

- `await` on a future that resolves immediately: allowed.
- `await` that would suspend: diagnostic
  `cannot suspend at compile time`.
- An `async func` used at comptime is only legal if it never actually suspends
  on the given inputs. The interpreter enforces this at the point `Pending` is
  observed.

---

## 12. Runtime Library

Two layers, matching D10.

### 12.1 `core` protocol

`lang/libs/core/async.ch` (new source in `core/chemical.mod`) contains the types
from Section 4.2 plus the runtime handle from Section 4.3 (`FutureHandle<T>`,
`FutureTable<T>`), a `Unit` type, and the frame allocator hooks:

```chemical
// implemented by the compiler; lowered per backend
@extern public func chemical_async_frame_alloc(size: size_t, align: size_t) : *mut void
@extern public func chemical_async_frame_free(ptr: *mut void, size: size_t, align: size_t)
```

There is **no `IntoFuture`** — `await` materializes into `FutureHandle<T>` (D13).

These two hooks are the *only* runtime functions the generated frames need. The
default implementations live in `lang/libs/async` and are linked when the
executor is used; if a program uses async without the library, the compiler must
either link a minimal default or error (Section 15).

> **Naming:** expose the protocol as `core::async::*`. Do **not** name anything
> unqualified `Future` — `std::concurrent.Future<T>` already exists (B5).

### 12.2 `lang/libs/async` executor library

```
lang/libs/async/
├── chemical.mod          // module async; import std; import cstd; import core
└── src/
    ├── main.ch           // re-exports
    ├── frame.ch          // frame alloc/free (task arena + malloc fallback)
    ├── waker.ch          // Waker impls, task waker
    ├── executor.ch       // per-thread run queue, run_until_complete
    ├── block_on.ch       // sync->async bridge
    ├── spawn.ch          // spawn / spawn_send / JoinHandle<T>
    ├── timer.ch          // async::sleep, timer wheel
    ├── select.ch         // select combinator (Phase 4)
    └── channel.ch        // mpsc channel (Phase 4)
```

The task queue can be `std::vector<FutureHandle<T>>`-shaped; the probe verified
move-only handles survive `push` and are all dropped when the container dies
(1.4 item 8). Use concrete per-`T` task queues (or one queue of a fixed
`FutureHandle<Unit>` plus a result slot) since Chemical generics do not mix
heterogeneous `T` in one vector.

`block_on` is a tiny executor:

```chemical
public func block_on<T>(handle : FutureHandle<T>) : T {
    // poll handle.vtbl.poll(handle.frame, &cx) in a loop;
    // on Pending, block the thread on a parker; wake when the waker fires.
}
```

> **B1 reminder:** do not implement `block_on`'s loop as a generic Chemical
> `loop` expression returning `T`. Use a mutable `var out : T` plus a
> `while`/`loop` and assign, or implement `block_on` with an explicit result slot.
> Test whichever form is chosen; the generic loop-result bug is exactly here.
>
> **B9 reminder (LLVM).** On LLVM at current HEAD, *any* by-value return/move of
> a destructor-bearing `T` (including `std::string`) crashes codegen, so
> `block_on<T>` cannot be validated on LLVM until B9 is fixed. Develop and test
> the runtime library on TCC; keep the LLVM-specific result handling behind the
> B9 fix.

`spawn` schedules on the current thread's executor:

```chemical
public func spawn<T>(handle : FutureHandle<T>) : JoinHandle<T>
```

`JoinHandle<T>` wraps a `FutureHandle<T>` and is awaitable through the same
materialization path, so `await async::spawn(f)` works.

### 12.3 Frame allocation strategy

- While inside a task, `chemical_async_frame_alloc` uses the task's bump arena;
  all frames for the task are freed together when the task completes/suspends
  fully. This avoids per-frame `free` in the hot path.
- Outside a task (`block_on` before the first task, or user-created futures),
  fall back to `malloc`/`free`.
- Frames must remain allocated while suspended; a task arena that is only reset
  on task completion satisfies this.

### 12.4 I/O reactor (Phase 4)

- Linux: `epoll`; macOS: `kqueue`; Windows: IOCP.
- `async::net`/`async::tls` wrappers register interest and produce a `Waker` that
  wakes the task on readiness. These are library additions; the language core
  does not know about them.

### 12.5 Executor implementation sketch (Phase 5)

Two executors, same task representation:

```chemical
@direct_init
public struct Task<T> {
    var handle : FutureHandle<T>   // move-only
    var result_slot : *mut T       // where the result lands (for spawn)
    var join_waker : Waker         // optional; wakes a Joiner
}

public struct LocalExecutor {
    var queue : std::vector<TaskHandle>   // FIFO via head index; no atomics
    var head : size_t
    var parker : std::mutex
    var condvar : std::condvar
}
```

> `TaskHandle` is a small type-erased entry (task-slot pointer + a function
> pointer to poll it), since Chemical generics do not mix `T` in one container.
> Use `std::vector`, not a deque — the std library has no deque.

- **`block_on`** (single task, no reactor): poll; on `Pending`, park the current
  thread on a condvar keyed by the task's waker. Do not spin. `cx`'s waker must
  set a "woken" flag and signal the condvar.
- **`LocalExecutor::run_until_idle`**: pop tasks FIFO and poll each; on `Ready`,
  move the result into `result_slot` / `join_waker.wake()`; on `Pending`, leave
  the task parked (its waker pushes it back). `run()` loops until the queue is
  empty and there is nothing to park on (an I/O reactor in Phase 4).
- **`spawn`**: allocate a `Task<T>` in the executor's arena, push it, return a
  `JoinHandle<T>` that shares the task slot. **`JoinHandle<T>` must expose a
  `FutureHandle<T>`** to be awaitable; the cleanest shape is that the join
  future's `poll` returns `Ready` once `result_slot` is set (use an `Option`-like
  flag, not `null`-dereference).
- **Waker data.** For a task, `Waker.data` points at the task slot;
  `wake` pushes the slot's element index back onto the queue (single-threaded,
  so no atomics). `clone`/`drop` are no-ops or a simple refcount owned by the
  executor (not the task).
- **Heterogeneous tasks.** The queue holds `FutureHandle<Unit>`-plus-result-slot
  entries (or a small tagged union), because Chemical generics do not mix
  `T` in one container. The probe verified `std::vector<FutureHandle<int>>`
  accepts move-only handles and drops them all (1.4 item 8).
- **Frame allocator.** `chemical_async_frame_alloc` delegates to the *current*
  task arena (thread-local pointer set by the executor); outside a task it falls
  back to `malloc`. Section 12.3.
- **No `await` executor round-trip on ready.** `run_until_idle` must poll the
  just-spawned task before parking, so an all-ready program makes zero
  scheduler crossings.

---

## 13. Safety Model

### 13.1 `await` context restrictions

Enforced in type verification:
- `await` only inside `async` functions, async closures, and async blocks.
- `await` forbidden in `@delete` destructors.
- `await` forbidden in constructors (`@constructor`).
- `await` forbidden in `@extern` functions (no Chemical frame).
- `await` forbidden at comptime unless it resolves immediately (Section 11.3).

### 13.2 Frames are stable (no `Pin`)

Frames are heap- or caller-stable and never move after allocation (D4). The
`FutureHandle<T>` *handle* may move; moving it just copies `{frame, vtbl}`. This
removes the need for a `Pin`-style type and for "movement after poll" tracking.
Document explicitly: **never** implement an elision that moves a frame after its
first `poll`.

### 13.3 Destructors and cancellation

- Dropping a suspended future runs `drop`, which runs the destructors for the
  live locals at the current state, in reverse creation order (Section 8.5).
- Dropping a completed future runs no user destructors: the completion path
  already ran every live-local destructor before marking `STATE_DONE`
  (Section 8.7), and the result was moved out at `Ready`.
- If a result was never retrieved (user drops a `Ready` future without awaiting),
  it is still dropped: define `STATE_DONE` such that the result slot is dropped
  if not moved out. The await desugaring always moves the result out immediately,
  so this path only matters for hand-held futures. Use a `result_taken` flag or a
  distinct `STATE_DONE_CONSUMED` vs `STATE_DONE_UNCONSUMED` state. Choose the
  flag; it is simpler and uniform with 8.4.

### 13.4 Threads and `Send`

- Default: per-thread executors, no migration (D7). This is safe with raw
  pointers and Chemical's current aliasing model.
- `spawn` runs on the current thread. `spawn_send` (Phase 4) requires an
  explicit `Send` opt-in. Do not silently move futures across threads.
- The `Waker` may be called from another thread only if the task was spawned
  with `spawn_send`; otherwise wakers are thread-local.

### 13.5 Panics and unwinding

Define: a panic during `poll` unwinds out of `poll` to the executor, which runs
`drop` and destroys the task. Frames must be registered so the executor can
destroy them even on panic. If the build uses no-unwind tables
(`-fno-unwind-tables` is used by tests), a panic aborts; destructors may not run.
Document this as a known limitation and recommend unwinding builds for async
servers.

### 13.6 Borrows across await

With stable frames, a reference **into the current frame** stays valid across a
suspend. The remaining hazard is the ordinary escape hazard: a reference to a
local of a *non-coroutine* function that has returned. That is caught by the
existing lifetime/`unsafe` model, not a new async-specific rule.

Rule to document and, later, to enforce:
> An async function may not capture a reference to a local of a synchronous
> caller and keep it across an `await`, because the synchronous frame is gone
> when the future is polled. Pass by value, or use `&mut` only when the referent
> is guaranteed to outlive the future.

Phase 4 may add a targeted borrow-across-await check; v1 relies on the existing
model and `unsafe` markers.

### 13.7 What must never happen

- A `FutureHandle<T>` value with a null `frame` or null `vtbl` being polled/dropped.
- A frame used after `drop`/`STATE_DONE` consumption.
- A suspended future's frame freed by the arena while still suspended.
- `await` compiled to a busy-wait loop in generated code (only the interpreter
  may spin, and only on already-resolving futures).

---

## 14. Performance Model

### 14.1 Compilation performance

- **No global analysis.** All async work is per-function and linear in body
  size: one normalization pass and one frame-plan pass per async function.
- **No-await fast path removes the state machine, not necessarily the allocation.**
  An `async func` with no `await` has `needs_frame == false` and emits no
  suspend points/state switch, but it still returns a `FutureHandle<T>`, so it
  still needs a tiny ready frame (Section 9.7). The frame allocation is removed
  only when LLVM `CoroElide` fires (same-function direct `await`) or the 2c
  direct-await elision applies. Do not advertise "zero allocation" for the
  no-await case.
- **Normalization bounds backend work.** After normalization, backends see at
  most one new node kind in a fixed shape; no combinatorial expression handling.
- **Generics parallelize as today.** Each instantiation is a distinct
  `FunctionDeclaration`; async lowering and frame synthesis run in the existing
  per-module parallel pipeline. The plan cache is per-node, lock-free per
  worker.
- **Non-async code is untouched.** The only shared-table addition is the
  `ValueKind` case (Section 16) and no new global pass runs when there are no
  async functions.
- **Caching.** Generated frames/poll/drop are part of the function's object and
  are covered by the existing module cache keys.
- **Avoid recomputation.** Cache `may_suspend` on the declaration once computed;
  reuse `AsyncLoweringPlan` across re-emission in the same compilation only
  (do not persist across invocations; the key would be brittle).

### 14.2 Generated-code performance

- **One indirect call per `poll`** on the awaited child. No virtual dispatch for
  a concrete compiler future in the direct-await case; one vtable call for
  type-erased futures.
- **No allocation on the elided fast path** (LLVM `CoroElide` for a
  same-function direct `await`; optional 2c direct-await elision). A plain
  `var h = foo(); await h` does **not** elide.
- **Task-arena allocation** otherwise; no `malloc`/`free` per frame in steady
  state. Outside a task, one `malloc`/`free` pair per future lifetime.
- **Ready fast path** avoids suspension entirely; a chain of already-ready
  futures runs synchronously with no executor round-trip.
- **No thread per task**, no context switches for I/O waits.
- **TinyCC note:** the 2c code is not optimized by TinyCC. Keep the generated
  state machine tight: no per-poll heap work, no repeated refcount operations,
  direct field loads/stores, and prefer the no-frame fast path wherever possible.
- **B9 note:** on LLVM, destructor-bearing results (`std::string`, structs) are
  currently miscompiled, so no LLVM performance claim for those paths is
  meaningful until B9 is fixed. TCC numbers are the reference.

### 14.3 Optional optimizations (later, behind flags)

1. **Direct-await frame elision in 2c** (10.4).
2. **Stack allocation of child frames** when the child does not escape.
3. **Devirtualize** `poll` on concrete compiler futures at the call site.
4. **Reuse frames across loop iterations** only if provably dead; risky, defer.
5. **`llvm.coro` accelerations** (already the LLVM base; further flags are
   compiler-internal).
6. **Skip frame slots for locals not live across any await** (8.1 rule 4 note) —
   this is a real win, but implement after the simple correct version and
   verify with tests.

### 14.4 Codegen cost model (grounded in the generated C/IR)

These are structural costs read directly from the generated output of the
verification probes, for an `await` whose child returns `Poll<T>`:

| Operation | LLVM (with `llvm.coro`) | 2c / TinyCC |
|-----------|--------------------------|-------------|
| Complete future (Ready first poll) | 1 call to `vtbl.poll`, tag branch, move result; frame already exists (elided only for same-function direct await) | 1 indirect call (sret), tag branch, move result; the ready frame was allocated by the ramp unless elided |
| Suspend (Pending) | store `cx`, `coro.save`+`coro.suspend`, return Pending | store `state`, return `{PENDING}` |
| Resume | `coro.resume` → `foo.resume`; state reloaded from frame | `switch(state)` → `goto` the continuation label; locals reloaded from frame |
| Child handle across await | spilled into the coroutine frame automatically by `CoroSplit` | an explicit `FooFrame.child_N` field |
| `Poll<T>` return | `sret`/by-value per ABI; LLVM promotes scalars | **hidden sret pointer + a small struct store per poll** (visible in the translated C, e.g. `child_poll(&__chx__lv__4, …)`) |
| Drop / cancel | `coro.destroy` dispatches on state; per-state cleanup | `switch(state)`; per-state destructor chain |

Practical implications for the implementer:

1. **The ready fast path is the hot path.** Most awaits in a well-written program
   resolve without suspending. Optimize it first: no allocation, no waker store,
   no state write on the `Ready` branch. The LLVM recipe's `cori.done`-style
   check must not allocate.
2. **Do not box or clone futures on the ready path.** Materialization should
   reuse a already-heap frame handle directly when the operand already is a
   `FutureHandle<T>`; only synthesize a vtable when the operand is a user
   `Future<T>`.
3. **`Poll<T>` sret on TCC is the main per-poll overhead.** For scalar `T`,
   consider a specialized `Poll<T>` representation (tag + inline value) rather
   than a generic struct returned by hidden pointer; the C backend can emit a
   flat `struct { uint8_t tag; T value; }`. This is a concrete, measurable win
   under TinyCC and does not change semantics.
4. **`Waker` is not free.** `Waker` holds a type-erased pointer and runs
   `vtbl.drop` in `@delete`; on the ready path never construct/store one. Only
   store the context's waker when actually returning `Pending`.
5. **Frame footprint.** Keep the frame to `{ state, cx, params, cross-await
   locals, child handles, result }`. The design's `AsyncLoweringPlan` already
   computes this; the B7 fix and the verified state machine show it can be
   emitted without hidden per-poll heap work.
6. **Destructor-bearing results cost more on cancellation.** `Poll<std::string>`
   needs a frame slot plus per-state drop. Prefer `string_view`/borrowed results
   where the lifetime permits.
7. **No refcounting in the hot path.** `FutureHandle` is move-only and
   non-atomic; `Waker` refcounting only matters when a waker may be called from
   another thread (`spawn_send`, Phase 4).

### 14.5 Comparative assessment

Async is only worth it if it beats the alternatives for the workloads the
language targets (many concurrent network/IO tasks). The alternatives are
`std::concurrent::ThreadPool` (blocking, thread-per-task) and hand-written state
machines.

| Metric | This design (stackless, poll) | `std::concurrent::ThreadPool` | Hand-written state machine |
|---|---|---|---|
| Threads for N idle I/O tasks | ~N/64 (one executor thread per core) | N (one OS thread each) | 0 extra |
| Stack per parked task | frame size (params + cross-await locals + result), typically tens–hundreds of bytes | 8 KiB–1 MiB of native stack | same as this design |
| Context switch per wake | none (queue push + poll) | 1–2 kernel context switches | none |
| Per-await steady cost | 1 indirect `poll` + tag branch; suspend = 2 intrinsic calls (LLVM) or a state store (2c) | mutex/condvar signal + wake | a switch/jump |
| Memory per task | 1 arena slab | 1 thread + promise | 1 frame |
| Cancellation | drop the handle; destructors run once, synchronously | cannot cancel a running thread | manual flag |
| Compile-time cost | per-async-function linear; zero for non-async | none | none (but enormous author cost) |
| Safety | no `Pin`, no data races on the same executor thread | lock-based | manual |

**Where the design wins outright:** thousands of concurrent connections, timers,
and request fan-out. **Where it does not:** CPU-bound parallelism (use the
threadpool/threads) and one-off blocking calls (use `block_on` on a dedicated
thread, or just call the blocking API).

**Cost drivers, in order of importance for the implementer:**

1. **Frame size.** Every cross-await local is spilled. A function that awaits
   once while holding a 4 KiB buffer carries 4 KiB per task. §14.3 item 6
   (skip slots for locals not live across an await) is the single biggest memory
   win but must come after correctness.
2. **`Poll<T>` sret under TinyCC.** A generic `Poll<T>` returned by hidden
   pointer means a struct store/copy per poll. For scalar `T`, a flat
   `struct { uint8_t tag; T value; }` avoids the pointer indirection. Measure
   before/after.
3. **Waker clone/drop on every `Pending`.** If a future stores the waker, each
   `Pending` may clone (refcount) and each drop may decrement. Avoid storing the
   waker for self-waking futures (timers can store a raw pointer + a generation
   counter instead of a refcounted waker).
4. **Indirect call + no inlining across suspend.** A suspend is an optimization
   barrier; keep hot logic outside the async function where possible.
5. **Executor queue contention.** Per-thread queues avoid atomics; `spawn_send`
   crosses threads and pays for atomic work-stealing. Keep the default
   per-thread.

**Compile-time budget (per async function):** one normalization traversal, one
frame-plan traversal, and a constant amount of codegen per await site. No
whole-program analysis. The expected observable effect is that adding async to a
project does not measurably slow non-async compilation; this should be checked
with the existing benchmark harness (Section 18).

### 14.6 Benchmark plan (required for Phase 5 acceptance)

Add these to the benchmark suite so the design's claims are measurable, not
asserted:

1. **Ready-chain throughput:** `await` a chain of K already-ready futures, K =
   1, 10, 1000. Measures the ready fast path; expect ~1 indirect call + branch
   per link, no allocation, and flat scaling vs the threadpool.
2. **Fan-out:** spawn 10k timers that complete after 1 ms. Measures memory per
   task and scheduler overhead; compare against 10k threads (should be orders of
   magnitude better, and bounded by frames not stacks).
3. **Suspend/resume cost:** one future that yields once; measure the full
   `Pending`→executor→`poll` round-trip. This is the number to watch for
   regressions from frame-size growth.
4. **Frame size:** compile the same async function with and without a large
   live-across-await buffer; assert the frame size reported by the plan matches
   the theoretical layout.
5. **Cancellation:** spawn N suspended futures, drop them all; measure
   destructor-exactly-once (correctness) and the cost of the drop switch.
6. **`Poll<T>` representation A/B (TCC):** scalar `T` with sret vs flat struct;
   record the delta. Gate the optimization on the result.
7. **Compile-time:** time compiling the full test suite with and without async
   tests; assert no regression above a small threshold for non-async modules.

Compare each against the equivalent `std::concurrent::ThreadPool` program and
against a hand-written state machine where feasible. Record on the benchmark
dashboard (`.agents/skills/benchmark_dashboard`).


---

## 15. Diagnostics

Add, using the existing `ASTDiagnoser`:

| Situation | Message (suggested) |
|-----------|---------------------|
| `await` outside async | `` `await` can only be used inside an `async` function, `async` closure, or `async` block `` |
| `await` operand not awaitable | `` `await` requires a `FutureHandle<T>` or a type implementing `Future<T>`, found `<type>` `` |
| `async` in destructor | `` an async function cannot be a destructor (suspension is forbidden in `@delete`) `` |
| `async` in constructor | `` an async function cannot be a constructor `` |
| `async` on `@extern` | `` an `@extern` function cannot be `async`; declare it with an explicit `FutureHandle<T>` ABI `` |
| pending at comptime | `` cannot suspend at compile time (the awaited future is not ready) `` |
| `async` keyword misplaced | `` expected `func` or a closure after `async` `` |
| async main without runtime | `` `async func main` requires the `async` library to be linked `` |
| result type mismatch | existing return-type diagnostic, but compared against the **inner** type (unwrap `FutureHandle<T>`) |
| frame has no allocator | `` async code requires `chemical_async_frame_alloc`; link the `async` library `` |

**Critical return-check change:** `ReturnStatement` verification and codegen must
compare the returned value against the **unwrapped inner type** `T`, not
`func->returnType` (which is `FutureHandle<T>` after Section 16.6). Add a helper
`BaseType* inner_return_type(FunctionDeclaration*)` that, when `is_async`, extracts
the single generic argument of the `FutureHandle<T>` return type. Use it everywhere
return checking happens (`compiler/typeverify/TypeVerify.cpp:1595-1628`,
`compiler/symres/SymResLinkBody.cpp:727-756` return-statement handling, and
LLVM/2c return codegen). **This is the single most likely place to introduce a
subtle bug.**

---

## 16. Extension-Point Checklist (Exact Files)

### 16.1 Enum synchronization (read this first)

Adding a `TokenType` or `ValueKind` value requires changing **both** the C++
enum and its TCC-compiled Chemical mirror, in the **same order**, or every CBI
plugin crashes with an off-by-one (AGENTS.md documents the `AsmKw`/`RBrace`
incident). Append new values at the end; never insert in the middle.

| C++ | Chemical mirror |
|-----|-----------------|
| `lexer/TokenType.h` | `lang/libs/compiler/src/ChemicalTokenType.ch` |
| `ast/base/ValueKind.h` | `lang/libs/compiler/src/ast/base/ValueKind.ch` |
| `ast/base/ASTNodeKind.h` | `lang/libs/compiler/src/ast/base/ASTNodeKind.ch` |

Our plan adds `AsyncKw`, `AwaitKw` (TokenType) and `AwaitExpr` (ValueKind). We do
**not** add a new `ASTNodeKind` (we reuse `VarInitStatement`).

### 16.2 Lexer / parser

- `lexer/TokenType.h`: append `AsyncKw`, `AwaitKw`.
- `lang/libs/compiler/src/ChemicalTokenType.ch`: mirror.
- `lexer/Lexer.cpp`: keyword map entries.
- `parser/structures/Function.cpp`: consume `async` before `func`; set flags.
- `parser/statements/LexStatement.cpp`: dispatch `AsyncKw` (top level, provide).
- `parser/structures/Struct.cpp`: dispatch `AsyncKw` (members/container).
- `parser/structures/Variant.cpp`: dispatch `AsyncKw`.
- `parser/utils/Expression.cpp`: add `parseAwaitValue`.
- `parser/utils/LexValue.cpp`: register `await` and `async`-closure in the value
  dispatch sites (`836`, `881`).
- `parser/statements/AccessChain.cpp`: register `await` (`25`, `141`, `173`).
- `parser/values/LambdaValue.cpp`: set `is_async` on lambdas.
- `parser/Parser.h`: declare `parseAwaitValue`.

### 16.3 AST

- `ast/base/ValueKind.h`: append `AwaitExpr`.
- `lang/libs/compiler/src/ast/base/ValueKind.ch`: mirror.
- `ast/base/ast_fwd.h`: forward-declare `AwaitExpression`.
- `ast/values/AwaitExpression.h` + `.cpp`: new node.
- `ast/structures/FunctionDeclaration.h`: `is_async` attribute + accessors.
- `ast/types/FunctionType.h`: `FunctionTypeData::is_async` + setter; update
  constructors and `copy_into`/`shallow_copy_into` flags.
- `CMakeLists.txt`: add the new value header/source (the list of every value
  file, ~lines 470-545).

### 16.4 `ValueKind` switch/visitor sites that must handle `AwaitExpr`

Missing any of these compiles but misbehaves at runtime (release) or throws
(debug):

1. `preprocess/visitors/NonRecursiveVisitor.h` — master switch
   `VisitValueNoNullCheck` (~738) and a `VisitAwaitExpression` forwarder
   (~349-450 region).
2. `preprocess/visitors/RecursiveVisitor.h` — recursion body (recurse into
   `inner`).
3. `preprocess/RepresentationVisitor.h` / `.cpp` — `VisitAwaitExpression`
   (diagnostics and `representation()`).
4. `ast/base/Value.cpp` — the **20** `switch(kind())` helpers. Exact list
   (function + `switch` line): `loadable_llvm_pointer` (59),
   `isValueIntegerLiteral` (606), `isValueLiteral` (616),
   `isValueRValueInBackend` (628), `isValueRValueInFrontend` (696),
   `direct_linked_node` (749), `is_stored_ptr_or_ref` (760),
   `is_ref_value` (786), `is_ref_l_value` (865), `check_is_mutable` (881),
   `is_func_call` (952), `is_ref_moved` (963), `get_chain_id` (976),
   `reference` (988), `get_chain_last_linked` (1048), `get_single_id` (1299),
   `get_last_id` (1312), `set_child_value` (1323), `set_value` (1341),
   `set_value_in` (1359). Decide per-helper: `AwaitExpr` is an rvalue that is
   not assignable, not a ref, not a chain id. Add explicit cases or document the
   default is correct and **test in debug** (the master switch's `#ifdef DEBUG`
   default throws on a missing case).
5. `compiler/symres/SymResLinkBody.h/.cpp` — `VisitAwaitExpression` (declare
   near the value section `SymResLinkBody.h:347-448`; model on
   `VisitDynamicValue`, `SymResLinkBody.cpp:3175`): link `inner`, resolve it
   against the `FutureHandle<U>`/`Future<U>` cases of §4.6, set
   `await_result_type` and `setType(await_result_type)`; handle move semantics of
   the inner future (it is consumed).
6. `compiler/symres/LinkSignature.h/.cpp` — `VisitAwaitExpression`: link the
   inner signature.
7. `compiler/typeverify/TypeVerify.h/.cpp` — `VisitAwaitExpression` (declare
   near `TypeVerify.h:170`; model on `VisitLambdaFunction`,
   `TypeVerify.cpp:1681`): context checks (Section 13.1) using
   `current_func_type` (`TypeVerify.h:31`), verify `inner` is a
   `FutureHandle<U>` or satisfies `Future<U>`, check assignability
   (`is_assignable` default is `true` — add `AwaitExpr` as **not** assignable),
   and definite-assignment interactions. The existing `switch(ValueKind)` in
   `TypeVerify.cpp:738` also needs an `AwaitExpr` case if reached.
8. `preprocess/2c/2cASTVisitor.h/.cpp` — `VisitAwaitExpression` per Section 10.
9. `compiler/backend/LLVM.cpp` — `AwaitExpression::llvm_value` /
   `llvm_type` and any assign/arg/branch overrides per Section 9 and the
   `UnsafeValue` forwarding pattern where appropriate.
10. `compiler/generics/GenericInstantiator.h/.cpp` — `VisitAwaitExpression` to
    re-monomorphize `inner` (mirror `VisitComptimeValue`/`VisitUnsafeValue`).
11. `ast/utils/ASTUtils.cpp` — chain helpers
    (`has_function_call_before`, `get_first_chain_id`, `get_parent_from`,
    `build_parent_chain`) must treat `AwaitExpr` correctly (likely return the
    inner chain or stop). Decide and test.

### 16.5 Async lowering pass

- New: `compiler/async/AwaitNormalizePass.{h,cpp}`.
- New: `compiler/async/AsyncLoweringPlan.{h,cpp}`.
- Wire it after type verification / generic instantiation in the per-module
  pipeline (`compiler/ASTProcessor.cpp` / `SymbolResolver.cpp` orchestration),
  and only for functions/lambdas with `is_async`.

### 16.6 Return-type wrapping (symres)

- `compiler/symres/LinkSignature.cpp` `visit_func_decl` (`:704-737`): when
  `node->attrs.is_async`, build the **`FutureHandle<inner>`** generic (not
  `Future<inner>` — Section 4.6/D14) and assign to `node->returnType`
  **before** `sig.visit(node->returnType)` (`:720`) so the generic gets
  registered/linked. Concretely:

  ```cpp
  sig.visit(node->returnType);
  if(node->attrs.is_async) {
      auto inner = node->returnType.getType();
      auto fh = new (allocator.allocate<GenericType>())
          GenericType(new (allocator.allocate<LinkedType>())
              LinkedType(coreNodes.async.future_handle), { TypeLoc(inner, loc) });
      node->returnType = TypeLoc(fh, node->returnType.getLocation());
  }
  ```

  `TopLevelLinkSignature::VisitGenericType` (`LinkSignature.cpp:572-580`)
  already recurses into the type args via
  `RecursiveVisitor::VisitGenericType` then `register_inline_instantiation`, so
  linking the wrapper also links the inner type. Wrap in this single place so
  every later reader (`LinkSignature.cpp:276`, `SymResLinkBody.cpp:2131`) sees
  the wrapped type.
- `compiler/symres/CoreNodes.h` + `SymbolResolver::link_core_nodes`: add a
  `CoreNodesAsync` sub-struct (parallel to `CoreNodesOps`/`CoreNodesIterable`)
  holding `future_handle`, `future_table`, `poll_type`, `waker`, `context`,
  `unit`, and the `Future::poll` method handle; resolve them under the
  `core`→`async` namespace and populate in `link_core_nodes` after the `stream`
  block. **Do not** cache or use an unqualified `Future` (B5); wrap against
  `future_handle`. Note `link_core_nodes` runs only for the `core` module and
  **after** link-signature/instantiation, so guard against a null handle if a
  dependent module is linked before `core` finishes — fail with the Section 15
  diagnostic rather than dereferencing null.
- Lambda return-type wrapping: `SymResLinkBody.cpp` `materialize_lambda`
  (`:1941`), `link_lambda` (`:2637`), and the deduced-return path (`:2692-2695`)
  where lambda return types are finalized; wrap with `FutureHandle<T>` when the
  lambda's `is_async` bit is set. `GenericInstantiator::FinalizeSignature`
  (`GenericInstantiator.cpp:802`) re-visits generic return types, so an async
  generic function's `FutureHandle<T>` is specialized there.

### 16.7 Materialization + static vtable (D13)

- New compiler support to turn an expression `E` of type `F` that satisfies
  `Future<T>` into a `FutureHandle<T>`. Concrete steps (matching
  `lang/compiled/async_mat` and `async_final`, which are verified):
  1. If `E`'s type is exactly `FutureHandle<U>`, do nothing — use the value
     (move-only; never copy).
  2. Otherwise hoist `E` into a compiler-managed slot `__fut` of type `F`
     (moved in). If `E` is a temporary, this is also what satisfies the
     "temporary destroyed at expression end" rule.
  3. Emit a static `FutureTable<T>` constant whose `poll` is a thunk
     `return (frame as *mut F).poll(cx)` and whose `drop` is a thunk that
     destroys the `F` (calling its `@delete` if it has one, then freeing the
     slot).
  4. Build `FutureHandle<T>{ frame : &raw mut __fut as *mut void, vtbl : &<the table> }`.
- The `poll` thunk is a **direct call** to the concrete `poll`; it must not go
  through the interface generic method (B2) and must not use `dyn` dispatch
  (B8). Verify the emitted C names the concrete function (`..._F_poll`), not
  `Interface__cgs__Npoll`.
- **Do not** attempt `impl Future<T> for FutureHandle<T>`: it does not resolve
  (verified). Recognize `FutureHandle<U>` structurally.
- The synthesized vtable/thunk must be emitted as part of the enclosing
  function's object so the existing module cache keys cover it, and the thunk
  name must come from the mangler (Section 10.2), keyed by the concrete `F`.

### 16.8 Diagnostics

- `compiler/typeverify/TypeVerify.cpp`: context restrictions (13.1), return
  inner-type comparison (15).
- `compiler/symres/SymResLinkBody.cpp`: return inner-type comparison.

### 16.9 Exact parser insertion points (verified)

| Site | File:line | What to add |
|------|-----------|-------------|
| keyword map | `lexer/Lexer.cpp:62-…` | `{"async", AsyncKw}`, `{"await", AwaitKw}` |
| prefix value dispatch (no postfix) | `parser/utils/LexValue.cpp:836-879` | `case AwaitKw: return parseAwaitValue(...)`; `AsyncKw` → `parseLambdaValue` |
| prefix value dispatch (with postfix) | `parser/utils/LexValue.cpp:881-928` | `case AwaitKw: return parseAfterValue(allocator, parseAwaitValue(...))` |
| addr-of / kw dispatch | `parser/statements/AccessChain.cpp:173-241` | `case AwaitKw` before `default` |
| assignment LHS | `parser/statements/AccessChain.cpp:141-171` | `case AwaitKw` → clean "cannot assign to await" error |
| unary fallback | `parser/utils/Expression.cpp:305-366` | already delegates to the dispatch above; no separate change except precedence tests |
| lambda parse | `parser/values/LambdaValue.cpp:42-104` | set `data.is_async`; detect `async` before `|`/`||` |
| function structure | `parser/structures/Function.cpp:457` | consume `AsyncKw` before `FuncKw`; `set_is_async` |
| top-level decl | `parser/statements/LexStatement.cpp:35-143` | dispatch `AsyncKw` |
| struct members | `parser/structures/Struct.cpp:115,153,159` | dispatch `AsyncKw` |
| variant members | `parser/structures/Variant.cpp:111` | dispatch `AsyncKw` |
| `Parser.h` | `parser/Parser.h:1058` | declare `parseAwaitValue` |

### 16.10 Exact AST / visitor insertion points (verified)

| Concern | File:line |
|---------|-----------|
| `ValueKind` append `AwaitExpr` | `ast/base/ValueKind.h:10-68` |
| CBI mirror | `lang/libs/compiler/src/ast/base/ValueKind.ch` |
| forward decl | `ast/base/ast_fwd.h:173-215` |
| `as_await_expression_unsafe()` | `ast/base/Value.h:1002-1170` |
| master value switch | `preprocess/visitors/NonRecursiveVisitor.h:737-897` |
| default no-op method | `NonRecursiveVisitor.h:255-423` |
| by-ptr forwarder | `NonRecursiveVisitor.h:1149-1277` |
| recursive override | `preprocess/visitors/RecursiveVisitor.h:495-509,692-708` |
| `FuncDeclAttributes` append `is_async` | `ast/structures/FunctionDeclaration.h:183` (append at end; ctor positional list `:241` has 18 args for fields 1–18) |
| `FunctionTypeData` flag | `ast/types/FunctionType.h:33-52` (grows 4→5 bytes, `static_assert(sizeof <= 8)` holds) |
| `TypeVerifier` context | `compiler/typeverify/TypeVerify.h:31`, `VisitFunctionDecl` `TypeVerify.cpp:1637`, `VisitLambdaFunction` `:1681` |
| `GenericInstantiator` templates | `GenericInstantiator.cpp:206-244` (lambda), `:354-360` (if), `:370-383` (loop), `:557-561` (comptime), `:622-626` (unsafe) |
| pipeline order | `compiler/ASTProcessor.cpp:430-650` (`sym_res_module`); `LabBuildCompiler.cpp:819/826` (TCC), `:1134/1181` (LLVM) |
| C backend function emission | `preprocess/2c/2cASTVisitor.cpp:4318` (`func_decl_with_name`), `:2329` (`scope`), `:3654` (`writeReturnStmtFor`), `:5191` (`writeLoopStmtValue`) |
| C destruction scopes | `preprocess/2c/CDestructionVisitor.h:39-112`, `2cASTVisitor.cpp:3638` (`destruct_scopes_above`), `:4747` (`visit_value_scope`) |
| C vtable emission pattern | `preprocess/2c/2cASTVisitor.cpp:3122` (`create_v_table`), `:3024` (`vtable_type_name`) |
| C parser dispatch already handles `dyn` | `preprocess/2c/2cASTVisitor.cpp:6189-6232` (and B8's missing sret) |

---

## 17. Implementation Phases

Each phase is independently testable. Do not start a later phase's backend work
before the shared plan exists.

### Phase 0 — Groundwork (no user-visible syntax)

1. Add `AsyncKw`/`AwaitKw` + CBI mirror + lexer map. No parser behavior yet.
2. Add `ValueKind::AwaitExpr` + CBI mirror + `AwaitExpression` node + all
   Section 16.4 extension points with "not yet supported" stubs that produce a
   clear diagnostic.
3. Add `is_async` flags.
4. Add `core::async` protocol types in `lang/libs/core` (`Poll`, `Context`,
   `Waker`, `WakerVTable`, `Future`, `FutureHandle`, `FutureTable`, `Unit`,
   frame alloc hooks). Keep the probe `lang/compiled/async_probe` compiling as a
   smoke test.
5. Add `AsyncLoweringPlan` (empty implementation).
6. ~~**Fix B1**~~ **DONE** (generic loop-expression result-type substitution).
   See Section 1.5 B1. Regression test
   `lang/tests/src/generic/generic_dispatch.ch::test_generic_loop_expression_result`.
7. ~~**Fix B2**~~ **DONE** (calls to impl-only methods emitted the interface
   symbol). See Section 1.5 B2. Regression test
   `lang/tests/src/generic/generic_dispatch.ch::test_impl_only_method_call`.
8. ~~**Fix B6**~~ **DONE** (generic destructible struct passed by value into a
   generic function — the `block_on<T>` shape). See Section 1.7 B6. Regression
   test `generic_dispatch.ch::test_generic_struct_by_value_param`.
9. ~~**Fix B7**~~ **DONE** (2c loop expressions double-destroyed in-scope
   locals). See Section 1.7 B7. Regression test
   `generic_dispatch.ch::test_loop_expression_cleanup_once`.
10. **Fix B8** (2c `dyn` dispatch of a struct-returning method omits the sret
    argument). See Section 1.8 B8. Repro
    `lang/compiled/async_final/micro/dyn_dispatch.ch`; must fail before, compile
    after. Not strictly required for D13 (we avoid `dyn poll`), but it is a
    latent bug in the `dyn` machinery the design touches.
11. **Fix B9** (LLVM SIGSEGV on destructor-bearing by-value returns; blocks
    `block_on<T>` for `T` with a destructor). See Section 1.8 B9. Repro
    `lang/compiled/async_vs2/`. **Hard prerequisite for Phase 3.**
12. **Fix the pre-existing LLVM dead-code terminator bug** (Section 1.7).
    Prerequisite for running the LLVM suite end-to-end.

**Acceptance:** compiler builds; full existing test suite unchanged; both repro
modules compile and run. ✅ (2185/2185 TCC tests pass; D13 materialization probe
`lang/compiled/async_mat` and the extended `lang/compiled/async_final` run
correctly on TCC. LLVM acceptance blocked by B9/dead-code.)

### Phase 1 — Surface + type system + interpreter

1. Parse `async func`, `await expr`, `async` closures.
2. Symres: wrap `async` return type in `FutureHandle<T>` (Section 4.6); resolve `await` to `T`.
3. Typeverify: context restrictions, awaitable check, inner return check.
4. Interpreter: `AwaitExpression::evaluated_value` for ready futures.
5. A **temporary blocking lowering** for compiled `await` (calls
   `Future::block_on`) behind a flag, clearly marked as a bootstrap, so the
   surface can be exercised end-to-end before the real lowering lands. Its
   semantics match "await until ready"; it is not the shipped performance model.

**Acceptance:** async/await examples compile and run in interpreter and compiled
modes; negative tests for all diagnostics.

### Phase 2 — Shared analysis + normalization

1. `AwaitNormalizePass` (Section 7).
2. Frame planning + live-drop analysis (Section 8), producing
   `AsyncLoweringPlan`.
3. No-await fast path.
4. Unit tests for the plan on small functions (states, slots, drop sets).

**Acceptance:** plan is correct on a corpus including nested awaits, awaits in
loops/conditionals, destructor-bearing locals, and generic bodies.

### Phase 3 — LLVM lowering

1. `llvm.coro.*` ramp/begin/promise/suspend/end — follow the **corrected,
   verified recipe in §9.0** exactly (`presplitcoroutine`, initial lazy suspend,
   `coro.save` before each suspend, switch default → suspend block, **exactly one
   `coro.end(i1 false)` on the normal path and `coro.end(i1 true)` on the
   destroy path**, completion cleanup per §8.7 before `STATE_DONE`, no user
   destructors when already completed). Validate at `-O0`.
2. `poll`/`drop`/vtable.
3. Destroy-path destructors from `live_drops`. **This is the highest-risk item**:
   the destroy path must mirror LLVM 22's Clang lowering and pass an explicit
   cancellation test (destructor exactly once) before it is trusted.
4. `CoroElide` fast path for same-function direct await; no-await fast path
   (still allocates a ready frame).
5. Async main trampoline (optional).
6. Gate the Phase 1 blocking lowering off by default.

**Acceptance:** async tests run under `./scripts/test.sh --llvm`; IR assertions
(cancellation, frame size, one `coro.end(i1 false)`); generated IR contains no
frame allocation for same-function direct-await cases. **Blocked by B9
(destructor-bearing returns) and the dead-code-terminator bug; both are
independent of async and tracked separately.**

### Phase 4 — C / 2c lowering

1. Frame struct emission, ramp, `poll`, `drop`, vtable (Section 10).
2. Await sites, live-drop switches.
3. No-await fast path.
4. Optional direct-await elision.
5. Remove the temporary blocking lowering entirely.

**Acceptance:** `./scripts/test.sh --tcc` runs the async tests; behavior matches
LLVM; TinyCC compiles the output.

### Phase 5 — Executor + I/O + advanced library

1. Per-thread executor, `block_on`, `spawn`, `JoinHandle`.
2. Timers (`async::sleep`), channels.
3. `async` net/tls/http wrappers.
4. `select` combinator.
5. `spawn_send` + `Send` opt-in.

### Phase 6 — Safety hardening

1. Borrow-across-await check (targeted).
2. `Send` enforcement for cross-thread.
3. Panic/unwind behavior for async.
4. Effect bit `FX_SUSPENDS` propagation (ties into `effect-system-proposal.md`).

---

## 18. Testing Plan

- **Positive (interpret):** async fn with no await; await of a ready future;
  await in a loop; await in an if/else; await of a generic future; async
  closures; destructor-bearing locals across await; nested awaits.
- **Positive (compiled, LLVM and TCC):** the same corpus, plus:
  - cancellation: drop a suspended future, assert destructors ran exactly once;
  - ready fast path: assert no executor round-trip (observable via a counter);
  - frame stability: take the address of a frame local across an await and use
    it after resume;
  - generic async functions and async struct methods;
  - async main trampoline.
- **Negative:** all Section 15 diagnostics; await outside async; await in a
  destructor; async extern; pending at comptime; non-awaitable operand.
- **Completion cleanup:** an async fn with a destructor-bearing local live at
  `return` must destroy it exactly once on completion (not zero — the old
  `STATE_DONE` rule leaked — and not twice on the subsequent handle drop).
- **Coroutine structure:** grep the emitted LLVM IR for exactly one
  `llvm.coro.end` with `i1 false`; assert `presplitcoroutine` is present; run
  `opt -passes=default<O0>` on the module (not just O2) so malformed coroutines
  fail.
- **IR assertions:** for the LLVM backend, a test that compiles a same-function
  `await foo()` and greps the emitted IR to confirm the frame allocation is
  elided; a separate test asserts a plain `var h = foo(); await h` is **not**
  claimed elided.
- **C output assertions:** for 2c, compile a sample and check the generated C
  contains the frame/poll/drop, that `default:` traps, and that there is no
  unexpected `malloc` in the fast path.
- **`dyn` regression (B8):** `lang/compiled/async_final/micro/dyn_dispatch.ch`
  must compile on 2c once B8 is fixed; `dyn_prim.ch` must keep working.
- **B9 regression (LLVM):** `lang/compiled/async_vs2/` (return/move of
  `std::string`) must compile and run on LLVM once B9 is fixed.
- **Performance benchmarks (Section 14.6):** ready-chain, fan-out,
  suspend/resume, cancellation, and the `Poll<T>` sret A/B.
- **Regression:** the entire existing suite must pass unchanged; async must not
  slow down compilation of non-async code measurably (add a timing check if
  feasible).

Follow the testing skill (`.agents/skills/testing/SKILL.md`). Put language-level
async tests in the main suite only if essential; put heavier ones in a
standalone module if they need the executor library.

---

## 19. Edge Cases and Gotchas

1. **Return-type wrapping is pervasive.** `func->returnType` is
   `FutureHandle<T>` after symres; every return check must unwrap. This is the #1 bug source.
2. **`await` in a condition or loop condition** must be hoisted (Section 7.4).
   Do not let `AwaitExpression` reach a backend inside `If.cpp`/`WhileLoop.cpp`
   logic unnormalized.
3. **Evaluation order.** Hoisting changes nothing only if Chemical evaluates
   operands left to right. Confirm this and encode it in a test.
4. **Void async.** `async func foo() : void`. The result type is represented as
   `Poll<Unit>` with a zero-sized `Unit` (Section 4.2) to keep generics uniform;
   `FutureHandle<Unit>` is the return. Do not special-case `void` in the
   protocol.
5. **`FutureHandle<T>` with a destructor.** `FutureHandle<T>` owns the frame; it
   must have a `@delete` that calls `vtbl.drop(frame)`. Moving a `FutureHandle<T>`
   follows normal move semantics. Never let a copy of the handle exist (it would
   double-drop); `FutureHandle<T>` must be move-only. Mark `@delete` and do not
   provide a copy constructor; confirm Chemical's move semantics null the source.
6. **`Waker` ownership.** `Waker` is cloneable (`vtbl.clone`) and owns its
   `data` via `@delete`/`vtbl.drop`. `Context` holds a `Waker`; poll may clone
   it. Keep the refcount in `data`.
7. **Child future handle across suspend.** Must be a frame slot; an SSA value
   does not survive a suspend.
8. **Nested awaits in the same expression** (`await a + await b`) are hoisted in
   order; ensure temporaries are distinct and dropped.
9. **Await in a loop** reuses the same frame slot for the child handle; ensure
   the previous child is dropped/moved before overwrite.
10. **Cancellation in a loop** must drop the child whose state is current.
11. **`return` inside a loop after an await** must set `STATE_DONE` and not fall
    into loop labels.
12. **Generic frames** must be unique per instantiation; share the mangler.
13. **Async methods** (`&self` receivers): the receiver pointer is a frame slot;
    the frame stability rule keeps it valid. Document the lifetime expectation.
14. **Async closures capturing** follow the normal closure capture struct, which
    becomes part of the frame; ensure captures are moved into the frame, not
    stack-copied after poll starts.
15. **Hand-authored / `dyn` futures.** The compiler materializes a value of a
    type satisfying `Future<T>` into a `FutureHandle<T>` with a **static**
    vtable; it must never route `poll` through a `dyn Future<T>` (B8) or an
    interface constraint call (B2). The materialization takes the future by
    value/move (never copies a move-only future).
16. **`@extern` async** is rejected (no frame/ABI).
17. **`async` at comptime** only if non-suspending.
18. **Frame alignment** must be correct for over-aligned `T`; use `TargetData`.
19. **`STATE_DONE` vs result drop** (13.3): implement the `result_taken` flag.
20. **Zero-await async fns that call other async fns** return the child future
    directly (a form of `async fn forward() -> T { return await f() }` is *not*
    zero-await because it awaits; but `async fn forward() -> T { return f() }`
    is invalid because the body result must be `T`, not `FutureHandle<T>` — it must be
    `return await f()`. State this clearly.)
21. **`async` and `where` clauses / generics** must compose; wrapping happens in
    signature linking where the `where` clause is linked too.
22. **Never** busy-wait in generated code; `Pending` always returns to the
    executor.
23. **Never** allocate in `poll` for a `Pending` result; only the ramp
    allocates.
24. **Completion cleanup runs destructors before `STATE_DONE`** (Section 8.7).
    This is the difference between a leak and a correct program; test it with a
    destructor-bearing local live at `return`.
25. **No-await async fns still allocate a ready frame** (Section 9.7); only
    `CoroElide`/2c direct-await elision removes it. Never store the frame inside
    the returned handle.
26. **Exactly one `coro.end(i1 false)` per coroutine** (Section 9.0); the
    destroy path uses `i1 true`. Two `i1 false` ends crash `CoroSplit` at
    `-O0/-O1`.
27. **`dyn` dispatch of a `Poll<T>`-returning method is broken in 2c** (B8);
    never lower `await` that way.
28. **Destructor-bearing generic results crash LLVM codegen today** (B9); the
    `block_on<T>` / `await` result path must not be claimed working on LLVM
    until B9 is fixed.

---

## 20. Deferred Work and Open Questions

- **MIR unification.** When MIR lands (`mir-design.md`), move the state-machine
  construction into MIR so both backends share one lowering and can drop their
  bespoke emissions. Keep `AsyncLoweringPlan` as the analysis feeding MIR.
- **`Send`/`Sync`.** A full auto-trait system is out of scope. Phase 6 may add an
  explicit `Send` interface for `spawn_send`.
- **Structured concurrency.** `async let`, task groups, cancellation scopes are
  library/phase-6+ features.
- **`select` syntax.** Kept as a library combinator; no parser work.
- **Effect system.** `FX_SUSPENDS` should be populated from `is_async` +
  `may_suspend` when the effect system is implemented; until then, async
  functions are simply "may suspend".
- **Async in interfaces.** Whether interface methods can be `async` needs a
  decision about ABI (returning `FutureHandle<T>` is fine; awaiting inside a default
  method needs a frame). Defer to Phase 6.
- **Wasm/JVM backends.** Poll model maps naturally; note but do not implement.

---

## 21. Appendix A: Corrected Examples

### 21.1 Simple async function

```chemical
import std
import async

async func greet(name: *char) : string {
    await async::sleep(1000u)          // u64
    var s = std::string("Hello, ")
    s.append_view(name)
    return s
}

public func main() : int {
    var greeting = async::block_on(greet("World"))
    println(greeting)
    return 0
}
```

Corrections vs. the old draft: `std::string`, not bare `string`; `sleep` takes a
`u64`; main is sync and calls `block_on` (or use async main in Phase 3). Note
that the old draft's `unsafe var`, `new(x) T()`, and `known_type()` spellings are
all invalid in the current codebase.

### 21.2 Awaiting behind an HTTP-like future

```chemical
async func fetch_data(url: *char) : std::Result<Response, string> {
    var client = http::Client()
    var response = await client.get_async(url)
    if(response is std::Result.Err) {
        return std::Result.Err(response.error)
    }
    var Ok(res) = response else unreachable
    return std::Result.Ok(res.body.read_all())
}
```

### 21.3 Recursion

```chemical
async func countdown(n: int) : int {
    if(n <= 0) {
        return 0
    } else {
        await async::yield_now()
        return await countdown(n - 1)
    }
}
```

Each call gets its own heap frame; recursion is safe because frames are stable.

### 21.4 Async closure

```chemical
var task = async |x: int|() : int => {
    return await work(x)
}
var result = await task(7)
```

---

## 22. Appendix B: File and Symbol Reference Map

Frequently needed real symbols (verify line numbers before editing; they move):

| Symbol | File |
|--------|------|
| `ValueKind` enum | `ast/base/ValueKind.h` |
| `Value` virtuals / `getType`/`setType` | `ast/base/Value.h` |
| `AwaitExpression` (to add) | `ast/values/AwaitExpression.h/.cpp` |
| `FuncDeclAttributes` | `ast/structures/FunctionDeclaration.h:72` |
| `FunctionTypeData` | `ast/types/FunctionType.h:33` |
| `FunctionTypeBody` | `ast/types/FunctionType.h:286` |
| `LambdaFunction` | `ast/values/LambdaFunction.h:32` |
| `TypeLoc` | `ast/base/TypeLoc.h` |
| `GenericType` / `LinkedType` | `ast/types/GenericType.h`, `ast/types/LinkedType.h` |
| Generic synthesis example | `ast/values/FunctionCall.cpp:2141`, `compiler/symres/SymResLinkBody.cpp:2140,2167` |
| Core node caching | `compiler/symres/CoreNodes.h`, `SymbolResolver::link_core_nodes` |
| Signature linking | `compiler/symres/LinkSignature.cpp:704` (`visit_func_decl`) |
| Body linking | `compiler/symres/SymResLinkBody.h/.cpp` |
| Type verification | `compiler/typeverify/TypeVerify.h/.cpp` |
| Master value switch | `preprocess/visitors/NonRecursiveVisitor.h:738` |
| Representation visitor | `preprocess/RepresentationVisitor.h/.cpp` |
| Value kind helpers | `ast/base/Value.cpp` |
| C backend | `preprocess/2c/2cASTVisitor.h/.cpp` |
| C destr. scopes | `ToCAstVisitor::visit_value_scope`, `2cASTVisitor.h:591` |
| LLVM backend | `compiler/backend/LLVM.cpp`, `compiler/Codegen.cpp` |
| Generics | `compiler/generics/GenericInstantiator.h/.cpp` |
| Function parser | `parser/structures/Function.cpp:457` |
| Unary parsers | `parser/utils/Expression.cpp:305` |
| Lambda parser | `parser/values/LambdaValue.cpp` (`parseLambdaValue:42`) |
| Lexer keyword map | `lexer/Lexer.cpp:62` |
| TCC driver / C compile | `compiler/lab/LabBuildCompiler.cpp` (`process_module_tcc`, `compile_c_to_obj_w_opts`) |
| Existing blocking future | `lang/libs/std/src/concurrency/threadpool.ch:106,134` |
| Verified async probe | `lang/compiled/async_probe/` (Section 1.4, Appendix D) |
| Generic-loop bug repro | `lang/compiled/generic_loop_bug/` (Section 1.5 B1) |
| B6 fix (generic struct move) | `compiler/symres/SymResLinkBody.cpp` `is_same_generic_family` + `mark_moved_value` |
| B7 fix (loop-expression cleanup) | `preprocess/2c/2cASTVisitor.cpp` `writeLoopStmtValue` (`loop_scope`) |
| D13 end-to-end probe | `lang/compiled/async_mat/` (Section 1.7) |
| Third-review end-to-end probe | `lang/compiled/async_final/` (Section 1.8) |
| B8 repro (2c `dyn` struct-return) | `lang/compiled/async_final/micro/dyn_dispatch.ch` |
| B9 repro (LLVM destructor-bearing return) | `lang/compiled/async_vs2/` |
| Verified LLVM coroutine recipe | `lang/docs/async-await-design.md` §9.0, Appendix E; `/tmp/opencode/coro/ref2.ll` |
| Clang canonical coroutine IR | `/tmp/opencode/coro/co_raw.ll` |
| Effect proposal | `lang/docs/effect-system-proposal.md` |

---

## Appendix C: The Rules an Implementer Must Not Break

1. **Append-only enums.** New `TokenType`/`ValueKind` values go at the end and
   are mirrored in the `.ch` files.
2. **Frames never move after first poll.** No elision may violate this. A
   returned `FutureHandle<T>` must never point at a frame stored inside itself.
3. **Every `AwaitExpression` reaching a backend is a `VarInitStatement`
   initializer** (Section 7 invariant).
4. **`async func f() : T` returns `FutureHandle<T>`, not `Future<T>`** (D14,
   Section 4.6). Return checks unwrap `FutureHandle<T>` (Section 15).
5. **`Pending` never busy-waits and never allocates.**
6. **`await` goes through a `FutureHandle<T>` static vtable, never through an
   interface reference, a generic-constraint call, or `dyn`** (D13; probes
   B2/B3/B8).
7. **Never write the poll/suspend loop as a Chemical `loop` expression in a
   generic function** (probe B1). Backends emit it.
8. **The LLVM ramp must follow §9.0 exactly** — `presplitcoroutine`, an explicit
   initial suspend, `coro.save` before every `coro.suspend`, switch default →
   `suspend` block, **exactly one `coro.end(i1 false)` on the normal path and
   `coro.end(i1 true)` on the destroy path**. Validate at `-O0`, not just `-O2`.
9. **The completion path runs every live-local destructor *before* setting
   `STATE_DONE`** (Section 8.7). `drop` for `STATE_DONE` runs no user
   destructors; it only frees the frame (and the result if `!result_taken`).
10. **Do not work around B6/B7.** `block_on`/`spawn` take `FutureHandle<T>` by
    value and `block_on` may use a Chemical loop expression; both are fixed and
    covered by tests.
11. **Never use `dyn` dispatch for a struct-returning `poll`** (B8) and never
    rely on `impl Future<T> for FutureHandle<T>` (unresolved).
12. **Do not claim LLVM support for destructor-bearing results until B9 is
    fixed.**
13. **The runtime library targets `core::async` and is re-exported via
    `std::async`; never create an unqualified `Future`** (B5).

## Appendix D: Verification Artifacts

Keep these gitignored probes as living smoke tests; recompile them whenever the
relevant compiler code changes.

| Path | What it validates |
|------|-------------------|
| `lang/compiled/async_probe/` | Full protocol + runtime handle + vtable + move-only drop + destructor-bearing result + unit + vector-of-handles. Expected output in Section 1.4. |
| `lang/compiled/async_mat/` | **D13 end-to-end**: user `Future<int>` impl → synthesized static vtable (concrete `poll` thunk) → materialized `FutureHandle<int>` → **generic** `rt_await_generic<T>` (B1+B6) → Ready, plus a `Poll<Unit>` future and a cancellation drop. Expected output in Section 1.7. Runs on TCC and LLVM (int). |
| `lang/compiled/async_final/` | **Third-review end-to-end**: Waker callbacks, impl-only `poll` (B2), generic `rt_await_generic<T>` (B1+B6), destructor-bearing string result, cancellation exactly once. Correct on TCC; string result blocked on LLVM by B9. |
| `lang/compiled/async_final/micro/` | `pending_paren`/`pending_noparen` (fieldless case syntax), `generic_handle` (generic `FutureHandle<T>` return), `generic_impl` (must fail), `dyn_dispatch` (B8), `dyn_prim` (dyn primitive return works). |
| `lang/compiled/async_vs/`, `lang/compiled/async_vs2/` | B9 minimization: any destructor-bearing by-value return/move (`std::string`, `copy()`, generic `T`) crashes LLVM codegen. |
| `lang/compiled/generic_loop_bug/`, `lang/compiled/b1_probe/` | Minimal repros for B1. |
| `lang/compiled/b2_probe/`, `lang/compiled/async_generic_poll/` | B2 impl-only method dispatch. |
| `/tmp/opencode/b6/*` (matrix) | B6 cases A–F (only C failed before the fix). |
| `/tmp/opencode/b8/` | B7 loop-expression cleanup (TCC printed `DROP` twice before the fix; LLVM once). |
| `/tmp/opencode/coro/ref2.ll` | **Corrected** LLVM coroutine recipe, normal path, O0/O1/O2 (`coro.end` false on normal, true on cleanup). |
| `/tmp/opencode/coro/co_raw.ll` | Clang's canonical pre-split coroutine structure (source of the `coro.end` truth). |
| `/tmp/opencode/coro/state.c` | Verified 2c state-machine text compiled by `./lib/tcc/tcc` (complete=105 polls=2, drops 1/2). |
| `lang/compiled/llvm_deadcode/` | Pre-existing, unrelated LLVM dead-code-terminator crash (must be fixed before the LLVM suite can run). |

Because `lang/compiled/` is gitignored, the verified probe is reproduced below so
the working reference survives a clean checkout. `chemical.mod`:

```
application async_probe
source "src"
import cstd
import std
import core
```

`src/main.ch`:

```chemical
public struct WakerVTable {
    var wake  : (data : *mut void) => void
    var clone : (data : *mut void) => Waker
    var drop  : (data : *mut void) => void
}

public struct Waker {
    var data : *mut void
    var vtbl : *WakerVTable

    public func wake(&self) {
        vtbl.wake(data)
    }

    @delete
    func delete(&mut self) {
        if(vtbl != null) {
            vtbl.drop(data)
            vtbl = null
        }
    }
}

public struct Context {
    var waker : Waker
}

public struct Unit {}

public variant Poll<T> {
    Ready(value : T)
    Pending()
}

public interface Future<T> {
    func poll(&mut self, cx : *mut Context) : Poll<T>
}

public struct FutureTable<T> {
    var poll : (frame : *mut void, cx : *mut Context) => Poll<T>
    var drop : (frame : *mut void) => void
}

@direct_init
public struct FutureHandle<T> {
    var frame : *mut void
    var vtbl  : *mut FutureTable<T>

    @delete
    func delete(&mut self) {
        if(frame != null) {
            vtbl.drop(frame)
            frame = null
        }
    }
}

struct IntFrame {
    var value : int
}

func int_frame_poll(frame : *mut void, cx : *mut Context) : Poll<int> {
    var f = frame as *mut IntFrame
    var v = std::replace(&mut f.value, 0)
    return Poll.Ready<int>(v)
}

func int_frame_drop(frame : *mut void) {
    printf("[drop] int frame\n")
    var f = frame as *mut IntFrame
    dealloc f
}

func make_int_future(v : int) : FutureHandle<int> {
    var f = malloc(sizeof(IntFrame)) as *mut IntFrame
    f.value = v
    var vtbl : *mut FutureTable<int> = malloc(sizeof(FutureTable<int>)) as *mut FutureTable<int>
    vtbl.poll = int_frame_poll
    vtbl.drop = int_frame_drop
    return FutureHandle<int> { frame : f as *mut void, vtbl : vtbl }
}

func rt_await_int(fut : FutureHandle<int>, cx : *mut Context) : int {
    var out : int = loop {
        var r = fut.vtbl.poll(fut.frame, cx)
        if(r is Poll.Ready) {
            var Ready(value) = r else unreachable
            break value
        } else {
            continue
        }
    }
    return out
}

struct StringFrame {
    var value : std::string
}

func string_frame_poll(frame : *mut void, cx : *mut Context) : Poll<std::string> {
    var f = frame as *mut StringFrame
    var v = std::replace(&mut f.value, std::string())
    return Poll.Ready<std::string>(v)
}

func string_frame_drop(frame : *mut void) {
    printf("[drop] string frame\n")
    var f = frame as *mut StringFrame
    delete f
}

func make_string_future(v : std::string) : FutureHandle<std::string> {
    var f = malloc(sizeof(StringFrame)) as *mut StringFrame
    new(f) StringFrame { value : v }
    var vtbl : *mut FutureTable<std::string> = malloc(sizeof(FutureTable<std::string>)) as *mut FutureTable<std::string>
    vtbl.poll = string_frame_poll
    vtbl.drop = string_frame_drop
    return FutureHandle<std::string> { frame : f as *mut void, vtbl : vtbl }
}

func rt_await_string(fut : FutureHandle<std::string>, cx : *mut Context) : std::string {
    var out : std::string = loop {
        var r = fut.vtbl.poll(fut.frame, cx)
        if(r is Poll.Ready) {
            var Ready(value) = r else unreachable
            break value
        } else {
            continue
        }
    }
    return out
}

func unit_poll(frame : *mut void, cx : *mut Context) : Poll<Unit> {
    return Poll.Ready<Unit>(Unit{})
}

func make_unit_future() : FutureHandle<Unit> {
    var vtbl : *mut FutureTable<Unit> = malloc(sizeof(FutureTable<Unit>)) as *mut FutureTable<Unit>
    vtbl.poll = unit_poll
    vtbl.drop = int_frame_drop
    return FutureHandle<Unit> { frame : null, vtbl : vtbl }
}

func rt_await_unit(fut : FutureHandle<Unit>, cx : *mut Context) {
    var done : bool = loop {
        var r = fut.vtbl.poll(fut.frame, cx)
        if(r is Poll.Ready) {
            break true
        } else {
            continue
        }
    }
    if(done) { }
}

public func main() : int {
    var cx = Context { waker : Waker { data : null, vtbl : null } }

    var a = make_int_future(99)
    printf("int await = %d\n", rt_await_int(a, &raw mut cx))

    var b = make_string_future(std::string("hello-future"))
    var s = rt_await_string(b, &raw mut cx)
    printf("string await = %s\n", s.data())

    var u = make_unit_future()
    rt_await_unit(u, &raw mut cx)
    printf("unit await done\n")

    var handles = std::vector<FutureHandle<int>>()
    handles.push(make_int_future(1))
    handles.push(make_int_future(2))
    printf("handles = %d\n", handles.size() as int)

    return 0
}
```

> Note: the probe's `rt_await_*` helpers are intentionally **non-generic**; they
> predate the B1/B6 fixes. The generic equivalent now compiles and runs (see
> `lang/compiled/async_mat`, `rt_await_generic<T>`). The real lowering is still
> backend-emitted (D6/B1) for the reasons in Section 1.5 and to keep the
> suspension points under backend control.

---

## Appendix E: Verified LLVM Coroutine Reference IR

This is the exact IR that compiled and ran correctly at `-O0`, `-O1`, and `-O2`
through the linked LLVM 22.1.8, after the third-review `coro.end` correction.
It is the reference template for `AwaitExpression::llvm_value` / the async ramp
emission (§9.0). `foo` simulates an `async func foo(x : i32) : i32` that awaits
one child and returns `x`; `foo_poll` is the vtable `poll`. Note the explicit
initial suspend in `init`, the `presplitcoroutine` attribute, the single
`coro.end(..., i1 false, ...)` on the normal path, and `coro.end(..., i1 true,
...)` on the destroy/cleanup path. (In a real implementation the cleanup block
dispatches on the saved state, runs the per-state destructors, and frees the
frame with `llvm.coro.free`; here it is empty because the toy frame is trivial.)

> **The destroy-while-suspended path (reached from the `i8 1` arms) is not
> validated by this toy.** See §1.8 / §9.5: it was empirically unstable and
> must be implemented by mirroring Clang's LLVM 22 lowering and tested
> separately.

```llvm
declare token @llvm.coro.id(i32, ptr, ptr, ptr)
declare i64 @llvm.coro.size.i64()
declare i1 @llvm.coro.alloc(token)
declare ptr @llvm.coro.begin(token, ptr)
declare token @llvm.coro.save(ptr)
declare i8 @llvm.coro.suspend(token, i1)
declare i1 @llvm.coro.done(ptr)
declare void @llvm.coro.resume(ptr)
declare void @llvm.coro.end(ptr, i1, token)
declare ptr @malloc(i64)

define ptr @foo(i32 %x) presplitcoroutine {
entry:
  %id = call token @llvm.coro.id(i32 0, ptr null, ptr null, ptr null)
  %alloc = call i1 @llvm.coro.alloc(token %id)
  br i1 %alloc, label %alloca, label %after
alloca:
  %size = call i64 @llvm.coro.size.i64()
  %mem = call ptr @malloc(i64 %size)
  br label %after
after:
  %phi = phi ptr [ %mem, %alloca ], [ null, %entry ]
  %hdl = call ptr @llvm.coro.begin(token %id, ptr %phi)
  br label %init
init:
  ; LAZY: suspend before running user body
  %save_init = call token @llvm.coro.save(ptr %hdl)
  %s_init = call i8 @llvm.coro.suspend(token %save_init, i1 false)
  switch i8 %s_init, label %suspend [ i8 0, label %body  i8 1, label %cleanup ]
body:
  %save0 = call token @llvm.coro.save(ptr %hdl)
  %s0 = call i8 @llvm.coro.suspend(token %save0, i1 false)
  switch i8 %s0, label %suspend [ i8 0, label %resume0  i8 1, label %cleanup ]
resume0:
  ; ... read awaited result from frame, run continuation ...
  %save1 = call token @llvm.coro.save(ptr %hdl)
  %s1 = call i8 @llvm.coro.suspend(token %save1, i1 true)   ; final suspend
  switch i8 %s1, label %fin [ i8 0, label %fin  i8 1, label %cleanup ]
fin:
  br label %suspend
suspend:
  call void @llvm.coro.end(ptr %hdl, i1 false, token none)
  ret ptr %hdl
cleanup:
  ; destroy-while-suspended: run per-state destructors, then free the frame
  ; (5) i1 TRUE here — a second i1 false aborts CoroSplit at -O0/-O1
  call void @llvm.coro.end(ptr %hdl, i1 true, token none)
  ret ptr %hdl
}

define i1 @foo_poll(ptr %hdl) {
entry:
  call void @llvm.coro.resume(ptr %hdl)
  %done = call i1 @llvm.coro.done(ptr %hdl)
  ret i1 %done
}
```

Compilation and execution (from the repository root, LLVM tools in
`out/host/bin/`). This is the exact command sequence used in the third review;
the file also needs the `main` shown in `/tmp/opencode/coro/ref2.ll`:

```bash
out/host/bin/llvm-as /tmp/opencode/coro/ref2.ll -o /tmp/opencode/coro/ref2.bc
for O in O0 O1 O2; do
  out/host/bin/opt -passes="default<$O>" /tmp/opencode/coro/ref2.bc -o /tmp/opencode/coro/ref2_$O.bc
  out/host/bin/llc -relocation-model=static /tmp/opencode/coro/ref2_$O.bc -o /tmp/opencode/coro/ref2_$O.s
  gcc -no-pie /tmp/opencode/coro/ref2_$O.s -o /tmp/opencode/coro/ref2_$O.exe
  /tmp/opencode/coro/ref2_$O.exe    # prints 0 then 1 (pending, then done)
done
```

> Reminder: the `-O0` run is the important one. The second review's recipe (two
> `coro.end(i1 false)`) appeared to work at `-O2` but aborted `opt` at
> `-O0/-O1`; an async lowering that only works under optimization is not
> acceptable.
