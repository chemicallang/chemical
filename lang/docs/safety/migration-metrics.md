# Safety Migration Metrics

The canonical baseline is [`audit-baseline.json`](audit-baseline.json). The
entry-point list is [`audit-modules.txt`](audit-modules.txt).

## How it is collected

```bash
./scripts/safety-audit.sh            # check against the checked-in baseline
./scripts/safety-audit.sh --update   # re-measure the whole tree
```

For every module in `audit-modules.txt`, the script runs:

```bash
TCCCompiler <module> --check --no-cache --safety-report json
```

`--check` parses, resolves, type-checks, lowers to MIR and runs the structural
MIR verifier without emitting C/LLVM or linking. The compiler emits one JSON
report per module. The script merges duplicate module reports conservatively
(per-metric maximum), sums the unique modules, and compares the totals to the
baseline.

`--update` rewrites the baseline. Any growth fails the read-only check; a
reviewed migration that intentionally adds an unsafe boundary must ship with an
updated baseline and a justification in the same change.

## Current whole-tree totals (94 modules)

| Metric | Count |
|---|---:|
| `unsafe_blocks` | 3664 |
| `unsafe_values` | 4890 |
| `raw_pointer_types` | 9254 |
| `raw_pointer_derefs` | 974 |
| `raw_pointer_addrs` | 10009 |
| `raw_pointer_casts` | 2853 |
| `extern_decls` | 873 |
| `extern_calls` | 1256 |
| `variadic_calls` | 276 |
| `inline_asm` | 57 |
| `mutable_globals` | 141 |
| `extern_globals` | 7 |
| `panic_calls` | 86 |
| `allocator_calls` | 167 |
| `dealloc_stmts` | 176 |
| `delete_stmts` | 22 |
| `destruct_stmts` | 23 |
| `memory_calls` | 82 |
| `new_values` | 7 |
| `abort_calls` | 0 |
| `unreachable_stmts` | 0 |
| `union_field_accesses` | 0 |

## Largest modules by unsafe surface

| Module | `unsafe_blocks` | `unsafe_values` | Raw pointer types | Extern calls |
|---|---:|---:|---:|---:|
| `::css_parser` | 462 | 57 | 672 | 18 |
| `::tls_integration_tests` | 453 | 2823 | 857 | 157 |
| `::tls` | 373 | 1007 | 738 | 164 |
| `::common_tests` | 290 | 4 | 235 | 17 |
| `::universal_cbi` | 228 | 15 | 850 | 3 |
| `::std` | 160 | 112 | 252 | 122 |
| `::window` | 123 | 24 | 315 | 88 |
| `::process` | 104 | 78 | 54 | 110 |
| `::html_cbi` | 91 | 14 | 376 | 2 |
| `::lib_tests` | 87 | 131 | 211 | 47 |
| `::webview` | 82 | 25 | 224 | 70 |
| `::page` | 77 | 46 | 85 | 38 |
| `::async` | 70 | 26 | 94 | 25 |
| `::webview_tests` | 66 | 7 | 72 | 2 |
| `::fs` | 64 | 41 | 118 | 44 |
| `::http` | 60 | 30 | 133 | 20 |
| `::css_cbi` | 50 | 12 | 202 | 5 |
| `::image` | 50 | 6 | 103 | 0 |
| `::md_parser` | 46 | 15 | 248 | 0 |
| `::test` | 46 | 32 | 81 | 65 |

## Interpretation

- These are **audit counts**, not accepted exceptions. A raw-pointer operation
  outside an `unsafe` boundary is a compile error, so the whole-tree audit
  completing means every reported operation is inside a declared boundary.
- `unsafe_blocks`/`unsafe_values` are concentrated in FFI, crypto/TLS, parser,
  windowing/webview and test-harness adapters. The library-facing safe APIs
  remain safe; the boundaries are internal proofs.
- The tree is still dominated by C-interop surface (`extern_decls`,
  `variadic_calls`, `raw_pointer_types`). Reducing it depends on the
  target-aware C ABI and the FFI-wrapper milestone (M2), not on deleting
  necessary bounds.
- `unsafe` is never an opt-out: it marks a boundary, and borrow/lifetime/init
  checking continues inside it.
- The interpreter and both code generators consume the same MIR safety checks,
  so these counts are not backend-specific.
