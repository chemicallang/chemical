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
- Root cause: MIR pattern payload reads are aliases/addresses into the source variant; returning the bound identifier does not clear the source payload drop flag.
- Action: fix pattern-payload move tracking in MIR (or materialize destructible payloads into owned locals and clear the source field), then restore `File` RAII.
