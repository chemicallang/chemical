# Deterministic pixel fixture (P0.3b)

`checker_128.png` is a 128×128 RGBA checker/gradient written through the
`image` library by a temporary Chemical generator. It proves the image encode/
decode path; it is **not** a renderer golden (the first renderer goldens are
P1.9/P1.10).

- PNG SHA-256: `424a8b518c53efad05c0ad964382f5dbc395298b2da19c3963b6208fa2d01000`
- Decoded RGBA SHA-256: `2deb38260d75f07d80d888a6ba74b6ea2dd88eb4a09e2ff7bdeb39b86ca2aa38`
- Size: 65,737 bytes

`ui-test --filter golden` re-encodes it, checks the SHA-256, decodes and checks
the RGBA hash, then exercises `ui_test_core::assert_golden` pass/fail/diff.
