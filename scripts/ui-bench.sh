#!/usr/bin/env bash
# Run the UI bench scaffold and compare deterministic counters.
#
#   scripts/ui-bench.sh --scenario static-1k --tcc
#   scripts/ui-bench.sh --scenario static-1k --compare
#   CHEMICAL_UI_UPDATE_BASELINE=1 scripts/ui-bench.sh --scenario static-1k --update-baseline
#
# Exit codes: 0 pass, 1 regression/failure, 2 usage/validation, 3 no baseline.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKEND="tcc"
SCENARIO="empty"
COMPARE=0
UPDATE=0
JSON=0
BUILD_DIR="$ROOT/lang/tests/ui_bench/build"
MOD="$ROOT/lang/tests/ui_bench/chemical.mod"
SCENES="$ROOT/lang/tests/ui_bench/scenes"
BASELINE="$ROOT/lang/tests/ui_bench/baselines/reference-linux-x86_64.json"

usage() {
    cat <<'EOF'
Usage: scripts/ui-bench.sh [options]

  --scenario <name>     empty | static-1k | list-100k
  --compare             Compare counters to baselines/reference-linux-x86_64.json
  --update-baseline     With CHEMICAL_UI_UPDATE_BASELINE=1, rewrite the scene entry
  --json                Print the benchmark JSON (and a JSON verdict)
  --tcc | --llvm        Backend (default: --tcc)
  --help                Show this help

Baseline updates are parent-approved data changes; the env gate prevents an
accidental local rewrite.
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --scenario) shift; [ $# -gt 0 ] || { echo "ui-bench: --scenario needs a name" >&2; exit 2; }; SCENARIO="$1" ;;
        --scenario=*) SCENARIO="${1#*=}" ;;
        --compare) COMPARE=1 ;;
        --update-baseline) UPDATE=1 ;;
        --json) JSON=1 ;;
        --tcc) BACKEND=tcc ;;
        --llvm) BACKEND=llvm ;;
        --help|-h) usage; exit 0 ;;
        *) echo "ui-bench: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

SCENE="$SCENES/$SCENARIO.json"
if [ ! -f "$SCENE" ]; then
    echo "ui-bench: unknown scenario '$SCENARIO' (no $SCENE)" >&2
    exit 2
fi

if ! python3 - "$SCENE" "$SCENES/schema.json" <<'PY'
import json, re, sys
scene, schema = sys.argv[1], sys.argv[2]
d = json.load(open(scene)); s = json.load(open(schema))
errs = []
def check(v, sch, path="$"):
    t = sch.get("type")
    kinds = {"object":dict,"array":list,"string":str,"integer":int,"number":(int,float),"boolean":bool}
    if t and not isinstance(v, kinds[t]):
        errs.append(f"{path}: expected {t}"); return
    if isinstance(v,str):
        if "minLength" in sch and len(v) < sch["minLength"]: errs.append(f"{path}: too short")
        if "pattern" in sch and not re.search(sch["pattern"], v): errs.append(f"{path}: bad pattern")
    if isinstance(v,dict):
        for k in sch.get("required",[]):
            if k not in v: errs.append(f"{path}: missing {k}")
        props=sch.get("properties",{})
        for k,sub in props.items():
            if k in v: check(v[k],sub,f"{path}.{k}")
        if sch.get("additionalProperties") is False:
            for k in v:
                if k not in props: errs.append(f"{path}: unknown field {k}")
    if isinstance(v,list) and "items" in sch:
        for i,item in enumerate(v): check(item,sch["items"],f"{path}[{i}]")
check(d,s)
if errs:
    print("\n".join(errs), file=sys.stderr); sys.exit(1)
PY
then
    echo "ui-bench: invalid scene $SCENE" >&2
    exit 2
fi

SCENE_HASH="$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1]))["scene_hash"])' "$SCENE")"
SCENE_NAME="$(python3 -c 'import json,sys;print(json.load(open(sys.argv[1]))["name"])' "$SCENE")"
if [ "$SCENE_NAME" != "$SCENARIO" ]; then
    echo "ui-bench: scene name '$SCENE_NAME' does not match '$SCENARIO'" >&2
    exit 2
fi

mkdir -p "$BUILD_DIR"
BUILD_LOG="$BUILD_DIR/ui-bench-build.log"
EXE="$BUILD_DIR/ui_bench_${BACKEND}.exe"
if [ "$BACKEND" = "tcc" ]; then
    if ! "$ROOT/cmake-build-debug/TCCCompiler" "$MOD" -o "$EXE" \
            --mode debug_quick --no-cache >"$BUILD_LOG" 2>&1; then
        cat "$BUILD_LOG" >&2; echo "ui-bench: TCCCompiler build failed" >&2; exit 1
    fi
else
    if ! "$ROOT/cmake-build-debug/Compiler" "$MOD" -o "$EXE" \
            --mode debug_quick --no-cache >"$BUILD_LOG" 2>&1; then
        cat "$BUILD_LOG" >&2; echo "ui-bench: Compiler build failed" >&2; exit 1
    fi
fi
[ -x "$EXE" ] || { echo "ui-bench: missing $EXE" >&2; exit 1; }

CANDIDATE="$BUILD_DIR/result_${SCENARIO}_${BACKEND}.json"
"$EXE" --scenario "$SCENARIO" --json --scene-hash "$SCENE_HASH" >"$CANDIDATE"
python3 -c 'import json,sys;json.load(open(sys.argv[1]))' "$CANDIDATE" >/dev/null 2>&1 || {
    echo "ui-bench: benchmark did not emit valid JSON" >&2; exit 1
}

if [ "$UPDATE" = "1" ]; then
    if [ "${CHEMICAL_UI_UPDATE_BASELINE:-0}" != "1" ]; then
        echo "ui-bench: --update-baseline requires CHEMICAL_UI_UPDATE_BASELINE=1 (parent approval)" >&2
        exit 2
    fi
    mkdir -p "$(dirname "$BASELINE")"
    python3 - "$BASELINE" "$SCENARIO" "$CANDIDATE" <<'PY'
import json, os, sys
path, scenario, cand = sys.argv[1:4]
base = {"schema": 1, "profile": "reference-linux-x86_64", "scenes": {}}
if os.path.isfile(path):
    base = json.load(open(path))
base.setdefault("scenes", {})[scenario] = json.load(open(cand))
json.dump(base, open(path, "w"), indent=2, sort_keys=True)
open(path, "a").write("\n")
print(f"ui-bench: updated {scenario} in {path}", file=sys.stderr)
PY
    echo "ui-bench: baseline updated (parent-approved data change)"
    exit 0
fi

if [ "$COMPARE" = "1" ]; then
    if [ ! -f "$BASELINE" ]; then
        echo "ui-bench: --compare requires $BASELINE" >&2
        [ "$JSON" = "1" ] && printf '{"scenario":"%s","status":"no-baseline","exit_code":3}\n' "$SCENARIO"
        exit 3
    fi
    set +e
    python3 - "$BASELINE" "$CANDIDATE" "$SCENARIO" <<'PY'
import json, sys
base = json.load(open(sys.argv[1])); cand = json.load(open(sys.argv[2])); scenario = sys.argv[3]
entry = base.get("scenes", {}).get(scenario)
if entry is None:
    print(f"ui-bench: no baseline for {scenario}", file=sys.stderr); sys.exit(1)
if entry.get("scene_hash") != cand.get("scene_hash"):
    print(f"ui-bench: scene hash changed for {scenario}; update the baseline explicitly", file=sys.stderr); sys.exit(1)
bc = entry.get("counters", {}); cc = cand.get("counters", {})
bad = []
for k, bv in bc.items():
    if not isinstance(bv, (int,float)): continue
    cv = cc.get(k)
    if not isinstance(cv, (int,float)):
        bad.append((k,bv,cv,"missing"))
    elif cv > bv * 1.02:
        bad.append((k,bv,cv,"regression"))
for k,bv,cv,why in bad:
    print(f"ui-bench: {k}: baseline={bv} candidate={cv} ({why})", file=sys.stderr)
if not bad:
    print(f"ui-bench: counters within 2% of baseline ({len(bc)} keys)", file=sys.stderr)
sys.exit(1 if bad else 0)
PY
    rc=$?
    set -e
    if [ "$rc" -ne 0 ]; then
        [ "$JSON" = "1" ] && printf '{"scenario":"%s","status":"fail","exit_code":1}\n' "$SCENARIO"
        exit 1
    fi
fi

if [ "$JSON" = "1" ]; then cat "$CANDIDATE"; else cat "$CANDIDATE"; echo "ui-bench: PASS ($SCENARIO, $BACKEND)"; fi
