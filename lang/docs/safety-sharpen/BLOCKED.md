# Safety Sharpen — Blocked

## P1 baseline: TCC `--server` codegen failure

- Command: `./scripts/test.sh --tcc --no-build --server`
- Symptom: generated C at `lang/tests/build/chemical-server-tests.dir/Translated.c`
  contains `std_stdvector__cgs__8delete((&(&(&__chx_p10)->headers)->headers));`
  and TCC reports `struct or union expected`.
- Scope: `http::FileServer::serve_http` by-value `ResponseWriter` cleanup. This
  is a pre-existing MIR/C-emitter issue, not caused by the Phase 1 feature
  removals (no changed file is on that path).
- Action: fix in Phase 2 codegen/API work; all other Phase 1 gates pass.

## P2.2: RAII File destructor blocked by pattern-payload move

- Repro: `lang/tests`/temp `helper() -> Result<D,E>` + `wrap()` pattern-destructures `Ok(d)` and returns `d`; the source Result still drops the payload (`drops == 1`).
- Consequence: a `File` with `@delete` is closed by the moved-from Result, so `fs::create_file(...)` + `File.write_all` fails (`EBADF`). A `File` without `@delete` works, so Phase 2 currently requires explicit `fs::close_file`.
- Partial fix: returning a pattern-bound payload (directly or as a `Result.Ok(...)` argument) now clears the source variant drop flag (`MIRLowerStmt.cpp`). This fixes move-out through a wrapper.
- Remaining: materializing a *destructible aggregate* payload into an owned local with a shallow copy double-frees owned containers; it needs a real field move. `File` therefore uses explicit `fs::close_file`; restore `@delete` after the field move lands.
