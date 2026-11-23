# JavaScript backend — ABI decision

**Status:** decided · **Date:** 2026-11-15 · **Phase:** 1, step 0 (the gate before any emitter code)
**Plan:** [`web-stack-plan.md`](web-stack-plan.md) §3.7 · **Proposal:** [`proposals/0007-javascript-backend.md`](proposals/0007-javascript-backend.md)

---

## 1. Decision

**Route A — emulated linear memory.** The JS backend emits JavaScript that executes MIR against a
`Uint8Array` heap addressed by byte offsets taken from `MIRLayout`. Type layout, field offsets,
union/variant storage, `sizeof`/`alignof`/`offsetof`, `memcpy`/`memset` and every aggregate rule
are the *same* rules the C backend uses; only the instruction encoding changes.

Route B (promote non-escaping places to JS values, map aggregates to JS objects,
`@runtime_provided(js)` std types) is **rejected for v1** and should not be revisited without new
information. It is not a matter of effort: the MIR it would have to consume is address-shaped in
ways that a value model cannot represent without changing MIR itself.

### Why (evidence, not preference)

| fact | source | consequence for B |
|---|---|---|
| Unions store every field at offset 0; variants are `bases + int tag + union of payloads`. LLVM needs `MIRAggregatePlan` overlap groups to emit them. | `MIRLayout.h:97-160`, `:177-298` | A "struct = JS object with named fields" cannot represent N field names at one offset, nor the inactive variant case. |
| `packed`, `explicit_alignment`, `sizeof`/`offsetof`/`alignof` are observable through MIR opcodes and are emitted as constants. | `MIRTypeTable.h:92-101`, `MIRInstruction.h:58-60` | Promoted values would have to reproduce byte-identical layout anyway — i.e. emulate memory, while also being objects. |
| `place_escaped` fires on **any** use other than a direct operand (`MIRFacts.cpp:208-221`), including passing a place as a call argument. | `MIRFacts.h:106-120` | Most places are "escaped" in practice, so promotion would help a minority of code. |
| Lifetime is tracked **per place** (`init_state`, `field_wise_drop`, `SetDrop`/`Drop`/`Destroy`, move paths). | `MIRFunction.h:39-72`, `MIRInstruction.h:40-50` | A value model has nowhere to hang per-place drop flags. |
| `MemCpy`/`MemSet` are raw byte ops; atomics and `InlineAsm` are address-based by construction. | `MIRInstruction.h:49-50`, `:113-123` | They cannot be expressed over JS values at all. |
| There is **no MIR-to-MIR pass framework**; no pass rewrites any opcode. | `compiler/mir/` (no `*Pass`) | Route B needs a new pass first, plus emitter support for whatever it cannot promote. |
| `MIRInterpreter` (2,240 lines) already executes MIR on an address-based engine, missing only 4 async ops + `InlineAsm`. | `MIRInterpreter.cpp` | Route A has a working reference implementation of every opcode's semantics, in-repo. |
| `std::string` is an SSO tagged union over `malloc/realloc/free`; `vector`/`map` are `{ptr,size,cap}` structs. | `std/src/string.ch:52-72`, `vector.ch:17-23` | Under Route A the **entire stdlib compiles unchanged**. Under B every std type needs a second implementation. |

The deciding sentence: Route A makes the stdlib, layout, unions, variants, drop flags, `sizeof` and
`memcpy` *work by construction*; Route B makes them each a project. For a v1 that must be reliable
on day 1, that settles it.

## 2. Target profile

The JS job uses the **native LP64 layout**, deliberately:

- `is64Bit = true`, `pointer_size = 8`, alignment/offsets from the same `MIRModuleLayout`
  (`MIRLayout.h:44-52`) the native job uses, filled from the same `TargetData`.
- Therefore `sizeof`/`alignof`/`offsetof` printed by a program are **identical** under `--tcc` and
  `--js`. Differential C-vs-JS conformance then needs no exclusions — the strongest possible gate,
  and it costs nothing at runtime.
- Cost: more heap bytes per struct than a 32-bit profile. Irrelevant at page sizes, and it buys
  conformance.

## 3. Memory model

```
JS:  const HEAP = new Uint8Array(INITIAL_BYTES);   // grown by replacement
     const DV   = new DataView(HEAP.buffer);
     pointers are JS numbers (byte offsets); 0 == null
```

| MIR | JS |
|---|---|
| `Alloca` | bump-allocate `size` bytes in the current frame region; `StorageLive`/`StorageDead` are no-ops (frame reset at return) |
| `Load` / `Store` | `DV.getInt32/getUint8/...` at `addr + offset`, sized by the type record |
| `FieldAddr` / `IndexAddr` | `addr + MIRFieldInfo.offset` / `addr + i * elem_size` |
| `Gep` | same arithmetic (builder-API only today, no emitter case) |
| `AddressOf` | the place's address (a number) |
| `MemCpy` / `MemSet` | `HEAP.copyWithin` / `HEAP.fill` |
| `SizeOf`/`AlignOf`/`OffsetOf` | constant-folded from the type tables |
| `Copy`/`Move`/`Init`/`Assign`/`Drop`/`Destroy`/`SetDrop` | byte copy + `@delete` call; on a GC target `Drop` still runs destructors (they matter for `std::string`: `free`), so drop glue is **not** a no-op — see §6 |
| `Call` / `CallIndirect` | direct call / function-table index; aggregates passed as pointers exactly as MIR already encodes them |
| `Call` with sret | nothing special: the sret pointer is an ordinary operand (MIR prepends it) |
| `Switch` | `switch` on the tag (not yet lowered by MIR — CondBr chains today) |
| `DynamicCall` | **diagnostic** (fat pointer, `MIRInstruction.h:100-103`), v1 |
| `Async*` (4 ops) | **diagnostic**, v1 (own phase, `web-stack-plan.md` §4 Phase 6) |
| `Atomic*`, `InlineAsm`, `Throw` | **diagnostic**, v1 |

Allocator: bump allocator for allocas (frame-scoped), and `malloc`/`realloc`/`free` implemented over
the same heap in the runtime shim, because `std` calls them. A free-list is a Phase-5+ optimisation;
v1 correctness only needs grow-never-free + frame reset. Bounds: every `DV` access goes through a
checked helper in development builds (`--web-checks`), and unchecked in production.

## 4. Value representation

| Chemical type | JS representation | notes |
|---|---|---|
| `bool`, `u8`/`i8` … `u32`/`i32` | `number` | masked on narrowing stores (`& 0xFF`, `\| 0`) |
| `u64`/`i64`/`size_t`/`usize` | `number` | **exact to 2^53**, stored as 8 bytes; see §5 |
| `f32`/`f64` | `number` | `Math.fround` on f32 stores |
| pointers | `number` (byte offset, `0` = null) | |
| structs/unions/variants/arrays | heap bytes | never JS objects |
| `string`/`string_view`/`&str` | heap bytes (a struct) | views are `{ptr,len}` in the heap, as today |
| functions | JS functions | one per MIR function, named by the mangled symbol |
| closures (`std::function`) | object with a function + captured environment pointer | existing `function` struct already carries `fn_data_ptr`; the shim turns it into a call |

## 5. The 64-bit integer bound (a correction to the plan)

`size_t` is 64-bit (`cstd/src/common/std_types.ch:22-28`), and `vector`/`unordered_map` are built on
it. Any policy that "rejects `i64`/`u64` in client code" would reject the standard library itself,
so the v1 plan text is corrected here:

- `u64`/`i64` are **supported**, lowered to `number`, and stored as 8 bytes.
- Arithmetic is exact for values up to `2^53` (f64 mantissa). Beyond that, a `u64` silently loses
  precision. This is the one documented soundness bound in the JS backend.
- **Unsigned 64-bit values saturate rather than wrap** (§5.1): this is not cosmetic — the stdlib's
  own guards are written as `x >= (size_t)-1`.
- It is not a hidden trap: `--web --check` warns on `i64`/`u64` **values read from the outside
  world** (JSON numbers, `Date.now`, user input), where the bound can actually be hit; sizes,
  lengths, counters and durations never reach it.
- Exact 64-bit arithmetic (`BigInt`) remains a later, additive change (§3.20 deferred), and it will
  be a representation change only — no source migration.

### 5.1 Saturation is load-bearing (found by the emitter's first std program)

`std::string` guards its capacity with

```chemical
if(new_capacity >= ((0 as size_t) - 1)) { panic("string capacity overflow") }
```

`(size_t)-1` is `SIZE_MAX`, which is *not* representable in a JS number. The first JS build compared
`capacity >= -1` — because `0 - 1` wraps to `-1` — which is true for every capacity, and the emitted
program panicked inside `std::string` on the first append.

The fix is to saturate: an unsigned 64-bit result that would exceed `2^53` becomes `2^53`
(`Number.MAX_SAFE_INTEGER`), so `capacity >= SIZE_MAX` is *false*, as it must be, while signed `i64`
stays exact to `-2^53..2^53`. Saturation is confined to values the ABI already declares inexact, and
it makes the standard library's overflow guards behave correctly instead of firing.

## 6. Destructors are not a no-op

The plan's earlier "destructors are no-ops on JS" line is also corrected: `std::string` frees heap
memory in `@delete`. Dropping that glue would leak on every string. So `Drop`/`Destroy`/`SetDrop`
emit real calls on the JS target, exactly as on C. Ownership and borrow checking stay identical;
GC only removes the *need* to free, it does not remove the code that does.

## 7. Modules, names, and the boundary to native

- One Chemical module → one ESM module. Exports are the mangled symbol names
  (`MIRSymbolTable::mangled_view`), so `shared` logic compiled for both jobs exposes the same names.
- `@extern` declarations whose symbol is a browser global (`document`, `window`, `fetch`,
  `localStorage`) are emitted as references to that global, not as imports. The `dom` module
  declares them; `MIRTargetSupport` records which symbols are available per target.
- `@extern` to a C symbol cannot be satisfied in JS: `MIRTargetSupport` must reject it at the call
  site with the symbol name and its source location. This is what stops a `shared` module from
  accidentally depending on a native-only library (the `shared` restriction in the plan's §3.1 is
  enforced by this, not by convention).

## 8. Diagnostics

`MIRTargetSupport` (new, `compiler/mir/MIRTargetSupport.{h,cpp}`) will hold per-target facts:
symbol availability, type availability, opcode availability. `MIRPortability` is not usable (zero
call sites; it classifies `Call`/`Load`/`Store` as `NativeOnly`).

Rules:
- Check **before** emitting a function, so the output is all-or-nothing (the C emitter's
  `emit_function_c` already returns `false` + message transactionally; mirror that).
- Report the unsupported construct with the MIR instruction, the callee/type name, and the
  MIR source index → Chemical source location. "async is not supported" alone is useless to an
  agent; "`await fetch_user` at client/api.ch:42: the `Async*` form is not supported by the JS
  backend in v1" is actionable.

**What the emitter does today** (before the shared table exists): each unsupported instruction
produces

```
js backend: inline assembly is not supported by the JS backend
  (in 'js_backend_diagnostics_uses_asm_a' at …/main.ch:10:14)
```

and emission **continues** past a failed function, so one build reports every offending site
(`scripts/js-backend-diagnostics.sh` asserts exactly that: two sites, two positions). The first
version reported only the first error, which costs an agent one build per fix.

Not yet: reporting from `--check`/LSP without running the emitter (needs the shared table), and
`@extern`-availability diagnostics (a `shared` module calling a native-only symbol).

`--check` is supported though: `chemical <mod> --check --emit-js <dir>` lowers to MIR, runs the
rules, reports every offending site with a position, and writes **nothing** — that is the CI answer
to "does this module compile to JavaScript?" before a bundle exists.

A callee the backend cannot resolve is now explained by *why*, which matters because the three
causes need different fixes:

| situation | message |
|---|---|
| in the backend's libc map (`printf`, `malloc`, `strtol`, …) | emitted as `rt.printf`, … |
| a `__chc_*` checked-operation helper | `rt.trap("<helper>", …)` |
| `@extern` C function with no JS equivalent (`getenv`) | *native symbol 'getenv' has no JavaScript equivalent (declared @extern); a client or shared module must not depend on it* |
| lowered in the program but missing from the reachable set | *reported as an internal error* — it means the reachability walk missed an edge |



## 9. The spike — **done**

`scripts/js-abi-spike.sh` compiles `lang/compiled/js_abi_spike` with the native backend, runs
`scripts/js_abi_spike/spike.mjs` against the emulated ABI, and requires **byte-identical output**.
Result: pass, with **8,163 bytes** of hand-written JS (comments included).

```
point_size=8          sizeof(Point)              struct, two ints
vec_size=24           sizeof(vector<Point>)      {ptr,size,cap}
string_size=32        sizeof(std::string)        union{...}+state
sso=hello|5           SSO path                   bytes live inside the struct
heap=xxxx…(40)|40     heap path                  malloc'd buffer, state '2'
vec=2|10              push_back + element loads  1+2+3+4
shape=circle|7        variant tag + payload      tag 0 = Circle, payload at +4
```

What the spike proved, concretely:

- **The type table reproduces native layout.** The JS derives `sizeof` for `Point`,
  `vector<Point>` and `string` from field kinds and alignment and lands on 8 / 24 / 32 — the same
  numbers the C backend prints. Layout is not a JS-side approximation.
- **`std::string`'s SSO/heap union works on the emulated heap**, including the transition at
  `STR_BUFF_SIZE = 16` and the `state` byte (`'0'` constant, `'1'` SSO, `'2'` heap), with
  `malloc`/`realloc` implemented over the same `Uint8Array`.
- **`vector<T>` of a struct** addresses elements at `data + i * sizeof(T)` and field loads go through
  `DataView` at the layout offsets.
- **Variants** are `tag` then the payload union, and the tag is what selects the case.

What it deliberately does **not** prove, and what the emitter's differential test must: that *std's
own MIR* runs on this ABI. The spike encodes the same operations by hand; the emitter will encode
them from MIR, and the C-vs-JS differential suite (§6 of the plan) is what verifies the general case.
That is the correct split — a spike that reimplemented std would prove nothing about std.

Measured cost of the model, for the record: 8.2 KB of JS for the data model plus allocator, and
`BRK`-style bump allocation is enough for the spike; the emitting backend will need a real
`malloc`/`free` over the same heap because the stdlib calls them.

## 10. The emitter's surface, measured

`CHX_DUMP_MIR=<path>` on an interpretation job writes the fully lowered program
(82,609 lines for the test suite: 1,939 types, 2,605 constants, 3,311 symbols,
3,165 functions). A census of instruction shapes over that program is the
emitter's work list — **76 distinct shapes**, and the head of the distribution is
small:

| count | shape |
|---|---|
| 8,012 | `%v = const.int c` |
| 5,681 | `store %v, p` |
| 5,108 | `%v = load p, ty` |
| 4,599 | `p = alloca ty` |
| 3,583 | `%v = compare a, b, c, ty` |
| 2,861 | `return %v` |
| 2,620 | `cond_br %v, bb, bb` |
| 2,590 | `store p, c, %v` (field store) |
| 2,584 | `storage_live p` |
| 2,432 | `alloca ty` |
| 2,386 | `br bb` |
| 2,289 | `%v = const.string c` |
| 2,182 | `p = call s, …, …` |
| 2,009 | `%v = function_addr s, ty` |
| 1,492 | `%v = load %v, c, ty` (field load) |
| 1,415 | `const.int c` |
| 1,167 | `%v = binary a, b, c, ty` |
| 1,140 | `%v = const.bool c` |
| 1,133 | `move_init p, p, ty` |
| 1,026 | `%v = address_of p, ty` |
| 988 | `p = call s, p` |
| 923 | `param ty` |

The long tail (317 `field_addr`, 295 `drop`, 275 `param`, 218 `call`, 207
`const.string`, 176 `index_addr`, 159 `global_addr`, 113 `borrow_mut`, 92
`memset`, 78 `move_out`, 39 `sizeof`, 36 `destroy`, 28 `dynamic_call`, 28
`alignof`, 20 `call_indirect`, 20 `memcpy`, 15 `offsetof`) says the first working
emitter must cover roughly **20 opcodes**, not 60, and that `dynamic_call` /
`call_indirect` / async are the natural "diagnose, do not emit" set.

Practical consequences for the emitter:

- **Places dominate.** `alloca`/`load`/`store`/`field_addr`/`index_addr` are 30%+
  of all instructions, which is exactly why Route A was chosen: they are byte
  arithmetic, not a value model.
- **`storage_live`/`storage_dead`/`set_drop` are ~27% of instructions and are
  no-ops** on this ABI, so the emitter's hot path stays small.
- **`function_addr` (2,009) appears in ordinary code**, not only in callbacks, so
  closures and indirect calls cannot be deferred past step 2.
- **`memcpy`/`memset` (112)** map onto `HEAP.copyWithin` / `HEAP.fill`.

## 11. Reachability, not "every function"

The first emitter printed every function of every linked module. It could not even compile a program
that uses `printf`: `cstd` contains thousands of platform functions (`GetStdHandle`, …) that would
have to be emitted, and that nothing calls.

`reachable_functions(module, entry)` walks `call` / `init` / `drop` / `destroy` / `function_addr`
references from `main` over the whole lowered program (the JS job uses one shared MIR program,
exactly like the interpretation job), and only those functions are emitted. On the std fixture that is
**26 of 908** lowered functions. This is not an optimization bolted on later: it is what makes a
bundle small enough to serve, and it is the same pass that would otherwise be a "dead code" problem
for `#web` output.

## 12. The emitter as built (Phase 1, step 1)

`compiler/mir/MIREmitterJs.{h,cpp}` is the real thing: ~1,500 lines that print a program's MIR as
ES modules, with no AST access. `--emit-js <dir>` runs the normal front end (parse, symres, type
check) and swaps the back end in `do_js_job`; each bundle is `<program>.mjs` + `index.mjs` +
`runtime.mjs`.

**Gates, both green** (`scripts/js-backend-test.sh`; native and JS output must be byte-identical):

| fixture | what it covers | bundle |
|---|---|---|
| `js_backend` | ints, loops, recursion, `i8`/`u32` wrap, `div`/`rem` sign rules, pointer store/load | 3 files, 25 KB |
| `js_backend_std` | `std::string` SSO→heap, `vector<Point>` growth, struct fields, variant tag + payload | 3 files, 105 KB (26 of 908 functions reachable) |
| `js_backend_closure` | capturing lambdas in `std::function`, a closure passed as a parameter, an owned string capture, `for-in` | 3 files, 105 KB |
| `js_backend_json` | `json::parse` → `JsonValue` variant walk → `json::stringify`, `ordered_map<string, …>` keys | 3 files, 350 KB (109 functions) |

Each bundle also carries `<program>.mjs.map`: one segment per emitted instruction line, mapping to the
Chemical file/line/column it came from (`scripts/js_backend/sourcemap_check.mjs` decodes and verifies
it). That is what keeps a stack trace in a browser pointing at Chemical source instead of at
generated JavaScript.

**Shapes emitted**, per MIR construct:

| MIR | JS |
|---|---|
| `alloca ty` | one `rt.frame_alloc(size, align)` at function entry, released by `rt.frame_mark`/`frame_release` |
| `load`/`store` | `rt.ld(p, size, signed)` / `rt.st(p, size, signed, v)`; field/index forms project from the type table's offsets, a plain form is the place's own storage |
| aggregate `load` | `frame_alloc` + `memcpy` (a MIR value is one word, so the value *is* the address) |
| `binary`/`compare`/`unary`/`cast` | one call into the runtime (`rt.binop`, `rt.cmp`, `rt.unop`, `rt.cast`) with bits/signedness as constants, so C's promotions live in one tested place |
| pointer `+`/`-` | `rt.padd`/`rt.psub`/`rt.pdiff` scaled by the pointee size |
| `br`/`cond_br`/`switch` | `$pc` + `for(;;) switch($pc)`, one `case` per block |
| `call`/sret | direct call; the sret place is the first argument |
| `init`/`drop`/`destroy` | ctor/dtor calls with the place address; array dtors iterate in reverse |
| `const.string` | `rt.cstr(new Uint8Array([...]))` at module load |
| globals | `rt.alloc` + the `MIRConstExpr` tree (int/float/bool/null/string/aggregate/variant/negate/function-ref) |
| `function_addr` | the symbol id as a number; `$fns[id]` resolves it for indirect calls |
| `call_indirect` | `rt.indirect($fns, addr)(env, args…)` — a closure's capture environment is the first argument |
| `__chc_*` helpers | `rt.trap("<helper>", …)`, so a trap names the check that fired |
| externs | libc subset the stdlib uses: `printf`, `malloc`/`realloc`/`free`, `mem*`, `str*`, `ctype`, `strtol`/`strtod` |

**Two real bugs the gate caught, both now fixed** (they are the reason the gate exists):

1. `Init`/`Drop`/`Destroy` carry the callee symbol at operand *1* (destination place first), so
   reachability missed every constructor and destructor.
2. A plain `load p, ty` reads the place's own storage; only field/index loads dereference a
   pointer-typed place. Getting that wrong read one pointer level too deep, which looked like a
   corrupt pointer rather than a codegen slip.
3. A function address stored in a closure is read back through memory, so it must be a *number* (the
   symbol id), not a JS function object — casting a function to `u64` stored `NaN`.

## 13. What this means for the emitter's shape

- `MIREmitterJs.{h,cpp}` is a dispatcher over MIR exactly like `MIREmitter.cpp`, sharing the
  **AST-free** `MIRDeclPrinter` (529 lines) behind a printer interface; it must not use
  `MIRDeclEmitter` (630 lines, AST-based).
- Expected size, from the C emitter's breakdown: ~2,300 lines of body emission (60 opcodes minus the
  ~10 diagnosed ones), ~350 of type/declarator printing (JS-shaped, so much simpler: no declarator
  syntax, just `{size, align, fields[]}` descriptors), ~60 of constants, ~100 of naming.
- No AST access anywhere; no re-derivation of layout or ABI (that is the whole point of Route A).

## 14. Interface dispatch (`dyn`) — landed 2026-11-23

`dyn I` is a fat pointer `{ first: self, second: vtable }` in the same layout the C backend
uses, so nothing about the value model changed — only where the vtable's bytes come from.

- **Tables are heap blocks of function addresses.** For every `MIRModule::vtables.defs` record
  the emitter allocates `ptr_size * slots` bytes and stores each slot's *symbol id* (a function
  address is a MIR symbol id; see §12). `$g<sym>` names that block, so `&vtable` — which the
  lowerer writes into `second` — is an ordinary address.
- **The slot index comes from MIR, not from the call site.** `MIRVtableRecord::slots` hold the
  method name per slot; `dynamic_call`'s operands carry the vtable *type* name and the method name,
  so the emitter resolves the index and reports a clear diagnostic when a method is not a slot of
  that interface (rather than emitting a call to slot 0).
- **The call is `rt.indirect(rt.fns, <vtable[slot]>)(self, args…)`** — the same indirect-call path
  closures use, so a null slot fails with the runtime's "unresolved function address (0)" instead
  of `undefined is not a function`.
- **Reachability follows the tables.** An impl method that is only ever named by a vtable slot has
  no call instruction pointing at it; `reachable_functions` seeds its work list from the slot
  targets, otherwise the bundle would omit the method and the table would name a function the
  emitter never printed.

Verified differentially by `lang/tests/web/fixtures/js_backend_dyn` (one interface, two impls, a
method returning `*char` and one returning `int`, both dispatched call sites): identical output
under both backends.

## 15. Async — why it is not a switch to flip (decided: §17, promise-based)

**Resolved 2026-11-23:** the promise-based design below was implemented (see §17). This section
is kept as the decision record.

MIR carries the *lowered* coroutine: a frame struct, a state number per `await` site, spill lists
of live locals, a poll protocol (`Ready`/`Pending`), and a `FutureHandle<T>` vtable
(`AsyncFrameAlloc`, `AsyncAwait`, `AsyncFinish`, `AsyncRampFinish`). The native backends hand that
to LLVM's `CoroSplit`, or (for C) emit the equivalent by hand; the JS backend has nothing to hand
it to.

Two designs are open, and the choice is a semantic one, not an implementation detail:

1. **Promise-based.** `await x` becomes `x.then(...)`; an `async func` becomes an `async function`.
   This is what the browser wants (microtask ordering, `fetch`, `setTimeout`), and it makes
   `web::Client`'s completion handlers redundant rather than the only option. It requires the
   *front end* to keep an await-bearing function as structured control flow (or a JS state machine
   over the existing frame), because the poll protocol cannot be expressed with `async/await` without
   re-deriving the source shape.
2. **Poll-based, in JS.** Emit the frame as a heap object, each resume pass as a switch on the state,
   and an executor that drives polls. Faithful to the MIR, but it also means porting the `async`
   library's epoll/kqueue reactor to JS — which has no file descriptors. A browser deployment wants
   (1) here regardless.

Either way the work is: a lowering of the four `Async*` opcodes (or a front-end path that avoids
them), a scheduler, and the `FutureHandle` representation — which is why the plan defers it to
Phase 6 rather than to the v1 backend. Until then the diagnostic is per-site and names the
function and position, so a client module that reaches for `await` fails loudly instead of
silently doing the wrong thing.

## 16. Formatting minification (release builds) — landed 2026-11-23

A release build (`--mode release_fast|release_small|release_safe`) writes compact modules; a
debug build keeps the formatting and the source map. The rule is deliberately crude so it cannot
surprise anybody: **release means no formatting, and therefore no map** — a map whose lines no
longer match the file is worse than no map at all.

- `mir::minify_js_source` is a scanner, not a regex: strings and template literals are copied byte
  for byte (so `"// not a comment"` survives), `//` and `/* */` comments are dropped, and a run of
  whitespace becomes **one space only where two tokens would otherwise merge** — two word
  characters (`return x`), or `+ +` / `- -`. Everything else collapses to nothing (`a = 1;` →
  `a=1;`).
- It is applied to every emitted module, to `index.mjs`, and to `runtime.mjs`. The runtime is
  scanned safely because it has no regex literals (the one construct this pass cannot classify);
  that property is worth keeping in mind when the runtime grows.
- **Identifiers keep their names.** This is formatting only; renaming locals (`$f22` → `a`) is the
  plan's separate "identifier mangling" item, and it is what a debugger needs to see.
- The release build must still *behave* identically, so `scripts/js-backend-test.sh` runs the
  whole differential comparison a second time in `release_small` and then asserts three things
  about the result: no comment survived, no source map was written, and the bundle is smaller.

Measured with the gate on the six fixtures (bundle = emitted modules + `index.mjs` + runtime):

| fixture | debug | release (minified) | ratio |
|---|---|---|---|
| `js_backend` | 42,293 B | 24,931 B | 58.9% |
| `js_backend_dyn` | 39,994 B | 23,331 B | 58.3% |
| `js_backend_std` | 198,075 B | 154,861 B | 78.2% |
| `js_backend_closure` | 197,975 B | 155,293 B | 78.4% |
| `js_backend_iter` | 141,140 B | 119,817 B | 84.9% |
| `js_backend_json` | 508,779 B | 374,613 B | 73.6% |

The fixtures that shrink least are the ones dominated by table data already written without
formatting; the modules themselves still shed ~40% (e.g. `main.mjs` 2,135 → 1,229 B). The dom
client in release mode was also run against its host harness (clicks, fetch) with the minified
runtime — same output, `runtime.mjs` 28,162 → 16,458 B.

## 17. Promise-based async/await — landed 2026-11-23

A JS build has real promises, so async is *not* translated into the coroutine protocol. Each
async body becomes a JS `async` arrow, `await` becomes a JS `await`, and a `FutureHandle<T>`
became **a promise token**: an integer naming a slot in the runtime's promise table, written into
the handle's first word. MIR keeps the coroutine shape (native backends need it); only the
emission changes.

The mapping, faithfully:

| MIR | JS |
|---|---|
| ramp (`async_body`) with no await site | the ramp *is* the async function: `const $p = (async () => { … })(); rt.promise_put(sret, $p)` |
| ramp with await sites (`async_deferred`) | allocates the frame (heap — see below), stores params, then `rt.promise_put(sret, <poll>(0, frame, 0))` |
| poll (`async_poll`) — the body lives here when the function awaits | `const $p = (async () => { … })(); return $p` |
| `AsyncAwait` | `<result> = await rt.promise_get(rt.ld(<child handle>, 8, 0))` — no state store, no spills, no poll |
| `AsyncFinish` (poll: `Ready(frame->__result)`) | `return rt.ld(frame + offset(__result), …)` |
| `AsyncRampFinish` (native: build `{frame, vtbl}`) | eager ramp: `return <__result>`; deferred ramp: hand over the poll's promise and `return` |

Three details that are easy to get wrong, and are now covered by the fixture:

1. **The body is in the poll whenever the function awaits.** `eager = async_plan_.sites.empty()`:
   with no await site the lowering inlines the body into the ramp, otherwise the ramp only sets
   the frame up. `MIRFunction::async_body` / `async_poll` / `async_deferred` now carry that fact so
   no backend re-derives it from opcodes.
2. **Poll parameters are `(scratch, frame, ctx)`.** The first parameter is the poll-base scratch the
   coroutine writes `Ready` into, *not* the frame — passing the frame first silently makes every
   parameter read garbage (it looked like "async returns wrong numbers", not like a pointer bug).
3. **Frame slots must alias, and async storage is heap storage.** A `FrameField` place *is* a frame
   field; a pointer backend aliases the two, JS cannot, so each such place is declared without
   storage and bound to `frame + offset(field)` once the frame is known — at `AsyncFrameAlloc` in
   the ramp, and at the store that lands the frame parameter in the poll. Places are allocated with
   `rt.malloc`, not `rt.frame_alloc`: a suspended arrow outlives the synchronous caller whose frame
   the mark/release discipline reclaims.

`async func main` is the entry point: `index.mjs` exports an **async** `$run()` that allocates a
handle, calls the ramp, and awaits the token. `run.mjs` awaits `$run()`. `async::block_on` is
deliberately *not* available in a JS build — it polls, and there is no reactor to poll against; a
JS client awaits instead.

**Verified** by `scripts/js-backend-async-test.sh` (in `js-backend-suites.sh`): fixture
`js_backend_async` awaits a plain async call, a two-level chain, awaits inside a `while` loop, and
evaluates awaits left-to-right (`inc=21 / twice=22 / loop=10 / order=240`), and the gate then
asserts the emitted module really contains async arrows + `await rt.promise_get` and that no
poll-protocol await leaked into the bundle.

**Limits (diagnosed, not silent):** an await whose result is an aggregate, and an async function
whose result is an aggregate, are rejected with a message. Instants (`await <plain value>`) are
built by the async library as coroutine futures; `rt.promise_get` throws a named error on such a
handle instead of resolving garbage.

### 17.1 A host future: `await dom::fetch_status(url)`

The promise path is not testable without a future the *host* creates, so `dom` gained the
awaitable form of its fetch:

```chemical
async func load_status(url : string_view) : int {
    var status = await dom::fetch_status(url)   // 0 when the request failed
    return status
}
```

`dom::fetch_status` returns `core::async::FutureHandle<'static, int>`; underneath,
`raw::chx_dom_fetch_status` is an extern whose aggregate return travels through the hidden sret
pointer, so the emitted call is `rt.dom.fetch_status(handle, url)` and the host does

```js
promise_put(handle, fetch(url).then(r => r.status).catch(() => 0));
```

That is the whole host contract for a future in a JS build: **create the promise, hand its token
over through the sret slot.** Nothing polls, so there is no `Context`, no waker, and no vtable.

Fixture `lang/tests/web/fixtures/dom_async` is a `--web` app whose client entry point is an
`async func main` that awaits three requests — success (`200`), an error status (`404`), and a
network failure that rejects (`0`). `scripts/web-dom-test.sh` builds both jobs, asserts the bundle
contains `async` arrows and no `async.await` (so it cannot pass through the coroutine path), runs
it under `scripts/js_backend/dom_harness.mjs` (whose `fetch` is a real promise, rejecting for
`/api/down`) and diffs the three lines. The harness now awaits `$run()`, because an awaiting entry
resolves only after its I/O settles.
