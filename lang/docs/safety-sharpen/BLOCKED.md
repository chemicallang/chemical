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
