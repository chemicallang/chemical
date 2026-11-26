# Safety Sharpen — deferred to the breaking branch

Items on the Phase 3 worklist that **cannot** be implemented without rejecting
code that compiles today, removing/renaming a public API, or otherwise breaking
the stable safe-API release. Each entry states the reason and the intended
breaking-branch task. Additive work stays in the main branch.

Status: P3.1 added the synchronous effect checker, module policy defaults and
generic propagation without breaking existing code. The remaining items below
need either a semantics decision or a breaking diagnostic.

| Item | Why deferred | Breaking-branch task |
|---|---|---|
| **P3.1 async effect propagation** | An `async func` lowers to a coroutine ramp plus generated poll/drop/resume functions. The ramp always allocates a frame (`[no_alloc]` is inherently impossible), its div-zero/OOM guards are compiler-generated aborts, and `await` reaches the child through generated poll calls. Attributing those operations to the source effect set is a coroutine-specific analysis, not the call-site bitset the synchronous checker uses. Today an effect annotation on an async function is rejected explicitly instead of being silently ignored. | Add an async-effect model: annotate the frame allocator as an alloc effect, treat compiler-generated guards as part of the allocation effect, and prove `await` through the child's `FutureHandle` contract. Then accept `[no_panic]`/`[no_unsafe]`/`[no_blocking]` on async functions and test TCC+LLVM. |
| **P3.1 module defaults on plugin-generated declarations** | The `chemical.mod` policy is applied after parsing. Functions that CBI macros add during symres/generics (for example `#json` impls) are created later and therefore keep their explicit attributes only. Applying the policy to them requires threading the module policy into the CBI/macro emission boundary. | Thread the module effect policy into `ASTBuilder`/macro emission so a generated function inherits the default, with a macro regression test. |
| **P3.1 `[no_panic]` on generic `vector::try_*` / `string` growth** | `try_reserve` calls the unannotated `malloc`/`realloc`; the compiler emits `__chc_div_zero_fail` for the element-size division even after the `elem_size == 0` branch proves it nonzero; `try_push` also runs the generic `T` destructor, whose effect set is unknown for an unbounded `T`. Annotating these now would either be unsound or require a false proof. | 1) annotate audited libc externs (`malloc`/`realloc`/`memcpy`) `[no_panic]`; 2) elide the div-zero guard for a provably nonzero divisor; 3) add an effect bound for destructors (`T: NoPanicDrop`) or make `try_push` return before constructing `T`'s drop glue. Then annotate the `try_*` family. |
| **P3.1 `[noreturn]` enforcement** | `[noreturn]` is accepted as a documented property but has no checker: a function that falls through does not yet error. This is a new rejection, not a widening. | Add a divergence check at MIR lowering for declarations carrying `noreturn`, with tests for `return`, fallthrough and `panic`/`unreachable` bodies. |

