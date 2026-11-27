# P0.1a — named function → `std::function` conversion: report

**Status:** **FIXED** on both backends. No commits, branch stays `main`, no public
API/doc changes, no ABI change.

**Scope:** the P0.1 blocker #1 (`lang/docs/ui/evidence/P0.1/BLOCKER.md` §1).
The repro `lang/tests/ui_probe/bugs/named_fn/` now prints `g=3` / `named_fn PASS`
under both `TCCCompiler` and `Compiler`.

**Implementation size:** **249 added lines** in compiler code (51
`MIRLowerCall.cpp`, 188 `MIRLowerLambda.cpp`, 10 `MIRLowerer.h`), plus tests.
No change to `default_function_instance`, the capturing-call ABI, or
`API.md`/`CONTRACTS.md`.

---

## 1. Root cause

`std::function` is `%capture<T, default_function_instance>`. A lambda literal is
recognized by `MIRLowerer::lower_arg_converted` and wrapped by the instance's
comptime `make` constructor. A **named** function is not a `LambdaFunction`; it
arrives as an `AccessChain`/`Identifier`/`GenericInstIdentifier` value whose
`linked_node()` is a `FunctionDeclaration`. The type checker correctly accepted
the function type against `CapturingFunctionType`, but lowering fell through to
plain `lower_expr`, producing a raw function address. The assignment then cast
that pointer to the instance struct.

Before (TCC translation, `before_tcc_error.log`):

```c
/* generated C from the failing case */
__chx_p0 = ((struct std_stddefault_function_instance)((ui_probe_bug_named_fn_inc)));
```

`ui_probe_bug_named_fn_inc` has type `void(int,int)`, so TCC rejected the C cast.
LLVM accepted the bitcast and the call path invoked it with the capturing ABI
`fn(void* data, int, int)`, shifting every argument (`g=16777217`).

---

## 2. Fix

In `MIRLowerer::lower_arg_converted`, before the lambda path:

1. Detect a value whose `linked_node()` is a `FunctionDeclaration` and whose
   wrapper chain resolves to a plain reference (`UnsafeValue`, `CastedValue`,
   `ReferenceOfValue`, `AddrOfValue`; the value itself must be `Identifier`,
   `AccessChain`, or `GenericInstIdentifier` — a call result is not a named
   function).
2. Check the target `CapturingFunctionType::get_func_type()` with
   `FunctionType::equal_type`; a mismatch returns a normal lowering error (the
   normal type checker also rejects it first — see the negative test).
3. Emit a **capturing-ABI thunk** `(void* data, [sret], params...) -> ret` whose
   body forwards the real parameters to the named function:
   - scalar return: `ret callee(args...)`
   - void: `callee(args...); ret void`
   - aggregate return: `call_sret` into a temp, then `memcpy` the temp through
     the caller's sret pointer
4. Construct the instance with the audited constructor:
   `default_function_instance.make2(thunk_ptr, null, null, 0, 0)`.
   The null capture pointer takes `make2`'s existing zero-allocation path
   (`fn_data_ptr = null`, `dtor = null`, `alloc_base = null`, `is_heap = false`).
5. Return the temp as the conversion result, so var-init/assignment/call-arg/
   field/return/struct-literal paths all use the same instance value.

No implicit C cast is generated and no new public API exists. The thunk is
internal-linkage MIR emitted alongside the materialized lambda definitions.
`std::typed_function`, `std::Fn`, `std::FnMut`, and `std::FnOnce` share the same
`default_function_instance`, so they use the same path; `FnMut`/`FnOnce` named
functions behave as ordinary non-capturing callables (no capture to mutate/move).

Generic instantiations (`generic_id<int>`) work because `linked_node()` is the
instantiated `FunctionDeclaration` and `intern_function` uses its concrete
signature.

---

## 3. Files changed

| File | Change |
|---|---|
| `compiler/mir/MIRLowerCall.cpp` | Detect named-function conversion in `lower_arg_converted`; signature check; dispatch to the thunk/instance builder |
| `compiler/mir/MIRLowerLambda.cpp` | `materialize_named_function_instance`: capturing-ABI thunk MIR + `make2` instance construction |
| `compiler/mir/MIRLowerer.h` | Helper declaration |
| `lang/tests/src/basic/lambda/named_function.ch` | New regression tests (9) |
| `lang/tests/negative/src/safety_diagnostics.ch` | Wrong-signature negative test |
| `lang/tests/ui_probe/bugs/named_fn/src/main.ch` | Comment updated: proof, not a failing repro |
| `lang/docs/ui/evidence/P0.1/BLOCKER.md`, `REPORT.md` | Historical note: blocker #1 resolved in P0.1a |
| `lang/docs/ui/evidence/P0.1a/REPORT.md`, `logs/` | This evidence |

No `API.md`, `CONTRACTS.md`, `BENCHMARKS.md`, or other protected document was
edited.

---

## 4. Commands and results

### Repro (exact requested commands)

```bash
cmake-build-debug/TCCCompiler lang/tests/ui_probe/bugs/named_fn/chemical.mod \
    -o /tmp/named_fn_tcc.exe --mode debug_quick && /tmp/named_fn_tcc.exe
# g=3
# named_fn PASS   (exit 0)

cmake-build-debug/Compiler lang/tests/ui_probe/bugs/named_fn/chemical.mod \
    -o /tmp/named_fn_llvm.exe --mode debug_quick && /tmp/named_fn_llvm.exe
# g=3
# named_fn PASS   (exit 0)
```

Raw: `logs/after_repro_tcc.log`, `logs/after_repro_llvm.log`; before:
`logs/before_tcc_error.log`.

### Regression tests (`lang/tests/src/basic/lambda/named_function.ch`)

Covered: void call, scalar return, zero parameters, assignment after
declaration, return from a function, struct field initialization, generic
instantiation `generic_id<int>`, aggregate (sret) return, and a capturing/
non-capturing lambda regression.

```bash
./scripts/test.sh --tcc --no-build --skip-sequential --test-names \
  named_function_to_std_function_return,named_function_to_std_function_void,\
named_function_zero_parameters,named_function_assignment_and_return,\
named_function_typed_and_fn_aliases,named_function_aggregate_return,\
named_function_generic_instantiation,named_function_struct_field,\
capturing_lambda_path_still_works
# Total: 9 | Passed: 9 | Failed: 0

./scripts/test.sh --llvm --no-build --skip-sequential --test-names <same>
# Total: 9 | Passed: 9 | Failed: 0
```

Raw: `logs/tests_tcc.log`, `logs/tests_llvm.log`.

### Wrong-signature negative test

`neg_named_function_signature_mismatch` assigns `(int,int)=>int` to
`std::function<(int)=>int>` and asserts a normal type error
(`does not satisfy type`), not a crash:

```bash
./scripts/test.sh --tcc  --no-build --negative --skip-sequential --test-names neg_named_function_signature_mismatch  # PASS
./scripts/test.sh --llvm --no-build --negative --skip-sequential --test-names neg_named_function_signature_mismatch  # PASS
```

Raw: `logs/negative_tcc.log`, `logs/negative_llvm.log`.

### Broader regression

```bash
./scripts/test.sh --tcc  --no-build      # Total 2495 Passed 2495 Failed 0
./scripts/test.sh --llvm --no-build      # Total 2493 Passed 2493 Failed 0
./lang/tests/ui_probe/run-probes.sh      # P0.1 probes: PASS=27 FAIL=0
```

The P0.1 probes include the GTK window probe, which passes lambdas through
`std::function` callbacks, so the existing lambda/capture path is exercised on
both backends. Raw: `logs/full_suite_tcc.log`, `logs/full_suite_llvm.log`,
`logs/ui_probes.log`.

### Real window handler regression

The P0.1 GTK window probe previously crashed when a **named function** was passed
to `w.on_resize(...)`. It now deliberately uses a named handler for window 1 and
a lambda for window 2; both route independently:

```bash
./cmake-build-debug/TCCCompiler lang/tests/ui_probe/window_probe/chemical.mod \
    -o /tmp/win_tcc.exe --mode debug_quick --no-cache && /tmp/win_tcc.exe
# P0.1 window resize1=5 resize2=12 events=0 created_ms=114
# P0.1 window_create_userdata_run PASS
# (same with Compiler: resize1=5 resize2=11 created_ms=113)
```

Raw: `logs/window_named_handler.log`.

---

## 5. Residual limitations / risk

- **Function-pointer values are not covered.** `var fp : (a:int,b:int)=>int =
  named; var f : std::function<...> = fp` still reaches the old cast
  (TCC rejects the generated C; LLVM passes garbage). The fixed path is a direct
  named function value (including `AccessChain`, `generic_id<int>`, and
  `&named`/`unsafe(named)` wrappers), not a runtime function-pointer variable.
  Follow-up options: a capture-owning thunk that stores the pointer, or a
  normal type-check rejection. This packet leaves the direct named-function
  requirement green and records the gap rather than widening scope.
- **Variadic named functions** are rejected at MIR lowering
  (`a variadic named function cannot be stored in std::function`) because the
  thunk has no portable way to forward `...`. Non-variadic signatures are exact.
- **Methods with `self`** are rejected (`a method with self cannot be used as a
  plain function value`); use a lambda capturing/forwarding `self`.
- **Custom `%capture` instance types without `make2`** are rejected with a
  lowering error. `std::function`, `std::typed_function`, `std::Fn`, `FnMut`,
  `FnOnce` all use `default_function_instance.make2`.
- **Async named functions** were not added as a test; the thunk would return the
  `FutureHandle` through the sret path. Async closures have their own FnOnce
  ABI; a named async function assigned to `FnOnce` is not part of P0.1a.
- **Aggregate return** uses a temp + `memcpy` through the sret pointer. It is
  correct for ordinary aggregates (tested with a two-int struct) and leaves the
  temp undestroyed, which is the ownership-transfer equivalent of a move. A
  self-referential aggregate (the known LLVM struct-assignment hazard in
  `AGENTS.md`) was not tested; that hazard predates this fix.
- **Code size:** one internal thunk is emitted per conversion site (not cached
  across sites). This is small and avoids cross-function `lambda_defs_` lifetime
  issues; deduplicating per named function is a future optimization.
- No performance claim: the thunk adds one direct call and one unused `void*`
  parameter, matching the existing non-capturing-lambda ABI.
- The fix is **249 LOC** of compiler code, under the packet's ~250 LOC bound,
  and does not change any ABI layout or public API.
