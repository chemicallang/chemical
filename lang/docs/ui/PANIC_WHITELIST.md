# Chemical UI — panic whitelist

**Status:** no allowed abort sites in `lang/libs/ui*` for P1.13.

Every UI failure path is expected to return a typed `UiError` and degrade to a
safe placeholder or the standard keyed error card. The runtime does not catch a
panic (Chemical has no unwinding), so an abort is a release-blocking defect.

| ID | Callsite | Invariant | Mechanical proof | Test keeping it unreachable | Owner | Expiry | Approval |
|----|----------|-----------|------------------|-----------------------------|-------|--------|----------|
| — | — | — | — | — | — | — | — |

The audit commands below scan `lang/libs/ui*` and `lang/compiled/ui_*` for
`panic`/`unwrap`/`expect`/`unreachable`, unchecked access
(`unchecked`/`take_unchecked`/`get_unchecked`) and raw-pointer patterns in
safe paths. P1.13 removed the only unchecked access in the UI library
(`semantics.ch` string escaping) rather than whitelisting it:

```bash
./scripts/ui-lint.sh
./scripts/ui-lint.sh --fixtures
```

The fault-injection suite exercises callback, allocation/OOM, font/image decode,
layout, stale identity, protocol and missing-resource failures; all must return
a typed value or error card without reaching an abort.
