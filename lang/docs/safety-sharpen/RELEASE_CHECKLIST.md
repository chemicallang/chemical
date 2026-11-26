# Safety-sharpen release checklist

Run these from the repository root on the release commit. A gate with a stored
baseline (`metrics.json`, `audit-baseline.json`, `api/*.txt`) fails on growth;
never delete an entry to make it pass.

## Build and binary gates

1. `./scripts/build.sh --tcc` (debug compiler used by the suites).
2. `./scripts/build.sh --llvm` (second backend; required for TSAN).
3. `cmake --build cmake-build-release --config Release --target TCCCompiler -j $(nproc)`.
4. `./scripts/safety-sharpen-metrics.sh` — release TCCCompiler must stay under
   the 5,000,000-byte hard ceiling; record the 4 MB-goal delta.
5. `./scripts/safety-sharpen-status.sh` — STATUS rows have evidence.

## Safety / API gates

- `./scripts/test.sh --tcc --safety`
- `./scripts/test.sh --tcc --negative`
- `./scripts/test.sh --tcc --libs`
- `./scripts/test.sh --tcc --api`
- `./scripts/safety-audit.sh` (libs baseline; no unsafe growth)
- `python3 scripts/safety-raw-surface-audit.py` (known pre-existing offenders
  `universal_cbi::body_check`/`hydration_check_body`, `web::App::handle`,
  `webserve::serve` are recorded in `NONBREAKING_DEFERRED.md`; fix them before a
  release that claims the raw-typed safe surface is closed)
- `./scripts/safety-sharpen-api-audit.sh` (reviewed allowlist only)
- `./cmake-build-debug/MIRTests`

## Language / backend gates

- `./scripts/test.sh --tcc` and `./scripts/test.sh --llvm` (main suite)
- `./scripts/test.sh --tcc --interpret` (interpreter parity)
- `./scripts/test.sh --tcc --async` and `./scripts/test.sh --llvm --thread-tsan`
- `./scripts/test.sh --tcc --fuzz` and `./scripts/test.sh --tcc --cbor`
- area suites touched by the release: `--plugins`, `--regexp`, `--process`,
  `--server`, `--removed`, and the GUI compile-only checks recorded in TARGETS.

## API-stability notes

- The public safe surface is **additive** within this release line: new
  `try_*` forms, new diagnostics, and opt-in annotations may be added; existing
  public names, parameter/return shapes, and error variants do not change.
- The plain public name is the safe entry point; raw-pointer/FFI forms live
  under `<lib>::raw`/`<lib>::ffi` and are `unsafe func`. Do not introduce a
  second `safe_`/`*_safe` spelling.
- Regenerate the affected `lang/docs/safety-sharpen/api/<lib>.txt` snapshot for
  any public addition and review the diff; snapshots are the review gate.
- Effect annotations (`[no_panic]`, `[no_alloc]`, `[no_unsafe]`,
  `[no_blocking]`, `[pure]`) are opt-in restrictions and do not change a
  caller's obligations unless the caller also opts in, except that a callee's
  declared `[no_*]` may be *relied on* by an already-restricted caller.
- Deferred breaking work is listed in `NONBREAKING_DEFERRED.md`; it must not be
  pulled into a non-breaking release.
