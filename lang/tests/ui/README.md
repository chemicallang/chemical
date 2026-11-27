# UI test suite (P0.3b)

This is a test-only scaffold. It does not implement the production `UiHarness`
or a renderer; P6.1 replaces `ui_test_core` while keeping the API.

```bash
./scripts/ui-test.sh --tcc  --filter scaffold   # package markers
./scripts/ui-test.sh --tcc  --filter core       # headless tree/core model
./scripts/ui-test.sh --tcc  --filter golden     # image + golden compare
./scripts/ui-test.sh --llvm --filter core
./scripts/ui-test.sh --llvm --filter golden
```

`core.ch` models stable `NodeId`/`Key`, parent/child, bounds, text, semantics,
focus/scroll, reorder state preservation, stale IDs, frame/advance/settle with a
bounded frame count and `CoreError.Timeout`, and JSON snapshots.

`golden` uses the committed `fixtures/pixels/checker_128.png` through `image`;
`assert_golden` writes `.expected.png`, `.actual.png`, and `.diff.png` on failure.
`CHEMICAL_UI_UPDATE_GOLDENS=1` writes only when explicitly set and prints
`updated golden`.

`contracts/` is the P0.2 frozen declaration module, built by
`ui-test --filter contract`.
