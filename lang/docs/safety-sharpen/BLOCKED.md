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

## Server wave — compiler gaps found (2026-11-13)

These block the remaining server/transport shape changes. Each is verified with
a minimal repro; none is a placeholder.

1. **`Option<&'a Struct>` cannot be instantiated.** Both
   `std::Option.Some<&'a CertificateChain>(chain)` (explicit type argument) and
   the inference form `std::Option.Some(chain)` fail with
   `couldn't infer type for generic parameter at index 0` when the payload is a
   reference to a struct. `Option<&'a int>` works. This forced the TLS trust
   store to be *owned* by `Client` instead of a lifetime-witness field on
   `TlsConfig`; a fully lifetime-typed `TlsConfig<'a>` needs this fixed.
2. **`&'a self` receivers are rejected by the parser** (`expected a identifier
   right after '&' ... as a 'self' parameter`), so a lifetime-generic method
   cannot return a value borrowing `self` (e.g. `tls_config(&'a self) ->
   TlsConfig<'a>`).
3. **Async methods in interfaces lose their future.** With
   `interface ARead { async func read_it(&mut self, buf : &mut [u8]) : int }`
   plus an impl and a generic caller, the call yields `int` and `await` fails
   with `value with type 'int' does not satisfy type 'FutureHandle<elided, int>'`.
   `AsyncRead`/`AsyncWrite`/`AsyncSeek` (needed by the async server loop) cannot
   exist until interface methods carry their coroutine type.
4. **`using`-imported names are unresolved inside generic instantiation.**
   Inside a generic struct constructor, unqualified `Option`, `string`,
   `vector`, and `Result` fail to resolve; the fully qualified `std::` spelling
   is required.
5. **Consuming `self` (fixed in this wave).** `func f(self)` used to copy the
   receiver, destroy the callee copy, and destroy the caller copy too (double
   drop). `MIRLowerer::append_receiver_arg` now moves a by-value aggregate
   receiver (`MoveOut` + `mark_moved`) and rejects moves out of a
   pointer/reference projection (`SAFE-MOVE-004`).
