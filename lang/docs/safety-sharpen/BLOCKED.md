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
