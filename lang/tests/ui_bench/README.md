# UI benchmark scaffold (P0.3b)

Scenes: `empty`, `static-1k`, `list-100k` in `scenes/*.json`; schema in
`scenes/schema.json`. The runner emits deterministic counters only, labels
`renderer:"none"`, and does **not** claim frame times. Real timing starts at
P1.10.

```bash
./scripts/ui-bench.sh --scenario empty --tcc
./scripts/ui-bench.sh --scenario static-1k --llvm
./scripts/ui-bench.sh --scenario list-100k --tcc --compare
```

Baselines: `baselines/reference-linux-x86_64.json`, one entry per scene
(`scene_hash`, counters). `--compare` fails on a >2% counter regression and
refuses without a baseline (exit 3).

`--update-baseline` is a parent-approved data change and requires
`CHEMICAL_UI_UPDATE_BASELINE=1`:

```bash
CHEMICAL_UI_UPDATE_BASELINE=1 ./scripts/ui-bench.sh --scenario static-1k --tcc --update-baseline
```
