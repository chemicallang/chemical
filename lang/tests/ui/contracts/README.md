# UI frozen contract module

`chemical.mod` + `src/main.ch` define every public declaration frozen in
`lang/docs/ui/API.md`. They are compile-checked with:

```bash
./scripts/ui-test.sh --tcc  --filter contract
./scripts/ui-test.sh --llvm --filter contract
```

The module uses `@extern` prototypes and empty/trivial bodies; it never runs UI
code. Positive declarations are listed in `lang/docs/ui/CONTRACT_TESTS.md`.
Negative misuse tests that need the library (forged handles, stale IDs, Fatal
errors) are scheduled in P1.2/P1.13 rather than silently skipped.
