# Safety Sharpen — Blocked

## P1 baseline: TCC `--server` codegen failure — RESOLVED

- Fixed in `MIREmitter` field-address emission: pointer-to-pointer bases are
  dereferenced explicitly (`(*base)->field`) and pointer bases are parenthesized,
  so nested `HeaderMap` cleanup emits valid C.
- `./scripts/test.sh --tcc --no-build --server` now passes `10/10`.

## P2.2: RAII File destructor — RESOLVED

- Fixed in MIR: returning a pattern-bound payload (directly or as a
  variant-constructor argument) clears the source drop flag, and
  `clear_moved_ref` now handles ordinary local identifiers.
- `fs::File` has its `@delete` RAII destructor restored; `fs::read_file`/
  `write_file` still close explicitly and the destructor is idempotent through
  `valid`.
- Non-consuming patterns (`var Some(value) = option`) remain aliases and do not
  move the source; consuming paths use the explicit move/return machinery.
- Verified with a `Result<File, IoError>` returned through a helper and dropped
  after `write_all`, plus existing libs/process/fs tests.

## Server wave - compiler gaps found (2026-11-13) — ALL FIXED (2026-11-13)

These blocked the remaining server/transport shape changes. Each was verified
with a minimal repro, fixed, and is now covered by a regression test in
`lang/tests/negative/src/generic_bodies.ch`.

1. **`Option<&'a Struct>` payloads inside generic bodies (fixed).**
   Three separate defects:
   - `SymResLinkBody`'s generic-declaration link pass skipped `using`
     statements, so an unqualified `Option`/`string`/`vector` was unresolved
     inside every generic body (`couldn't find symbol`); the fully qualified
     `std::` spelling worked around it. The pass now declares the file's
     `using` imports.
   - A generic variant constructor call written in a generic body stayed
     linked to the generic declaration, so `var Some(v) = opt` bound the
     declaration's own parameter (`T`) instead of the argument, and `v.id` /
     `*v` failed. A constructor call with concrete generic arguments *and*
     concrete argument types is now materialized at body-link time (mirroring
     the existing `Box<int>` early materialization); a partially applied shape
     (`Option<T>`) still defers to the instantiator.
   - Variant-constructor inference was handed the master implementation
     instead of the generic declaration, so `Option.Some(value)` could not
     infer its payload (`couldn't infer type for generic parameter at index
     0`); `is_node_parent_of` now also accepts a container member, and an
     already-concrete member is no longer re-instantiated.
   A generic variant's declared default type now also wins over inference from
   an argument literal (`Variant<T = ulong>.Some(10)` instantiates with
   `ulong`), which `lang/tests/common/src/decls/variants/basic.ch` asserts.
2. **`&'a self` receivers (fixed).** The parser now accepts an optional
   lifetime after `&` (and after `mut`) in the receiver position and stores it
   on the reference type, so `func f(&'a self) : Option<&'a T>` parses and type
   checks. Still rejected: `&'a self` in a non-generic function whose lifetime
   is not bound (the usual lifetime rules apply).
3. **Async interface methods keep their future (fixed).**
   `FunctionTypeBody::inner_return_type` only unwrapped a `FutureHandle<T>`
   whose referenced node was already a concrete `StructDecl`. An async *generic*
   function's return type is still linked to the generic declaration when its
   body is checked, so the inner type was reported as the handle itself and
   `return n` failed with `value with type 'int' does not satisfy type
   'FutureHandle<elided, int>'`. Both the declaration and the concrete struct
   are accepted now, which unblocks `AsyncRead`/`AsyncWrite`.
4. **`using`-imported names inside generic instantiation (fixed).** Same fix as
   item 1: the generic-declaration link pass declares the file's `using`
   imports. The imports are *not* injected into the generic instantiator's
   table — doing that made `using namespace std` shadow unrelated types during
   instantiation (it broke `[T]` span conversions).
5. **Consuming `self` (fixed earlier in this wave).** `func f(self)` used to
   copy the receiver, destroy the callee copy, and destroy the caller copy too
   (double drop). `MIRLowerer::append_receiver_arg` now moves a by-value
   aggregate receiver (`MoveOut` + `mark_moved`) and rejects moves out of a
   pointer/reference projection (`SAFE-MOVE-004`).

### Remaining known gap (not blocking the server wave)

- A parameter whose type is a *generic variant instantiation*
  (`func set(&mut self, value : Option<&'a Chain>)` on a generic struct) is
  still lowered as `void*` in the C backend when the method is emitted for an
  instantiation. Reference-typed parameters (`value : &'a Chain`) work, which is
  what the TLS configuration needs; passing the variant by value needs a MIR
  type-mapping fix.
