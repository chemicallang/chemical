# UI bench baselines

`reference-linux-x86_64.json` holds one deterministic scaffold counter entry for
`empty`, `static-1k`, and `list-100k` (P0.3b). These are **not** frame-time
baselines; real timing starts at P1.10.

`--compare` fails on a >2% counter regression or a changed `scene_hash` and
refuses without this file. `--update-baseline` is a parent-approved data change
and requires `CHEMICAL_UI_UPDATE_BASELINE=1`.
