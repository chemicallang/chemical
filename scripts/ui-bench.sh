#!/usr/bin/env bash
# Run the UI bench scaffold (P0.3a: only the `empty` scenario).
#
#   scripts/ui-bench.sh --scenario empty
#   scripts/ui-bench.sh --scenario empty --json
#   scripts/ui-bench.sh --scenario empty --compare   # needs a baseline
#
# Exit codes: 0 pass, 1 counter regression/test failure, 2 usage, 3 no baseline.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BACKEND="tcc"
SCENARIO="empty"
COMPARE=0
JSON=0
BUILD_DIR="$ROOT/lang/tests/ui_bench/build"
MOD="$ROOT/lang/tests/ui_bench/chemical.mod"
EXE=""

usage() {
    cat <<'EOF'
Usage: scripts/ui-bench.sh [options]

  --scenario <name>   Scenario to run (P0.3a: only 'empty')
  --compare           Compare counters to baselines/<scenario>.json (fails >2%)
  --json              Print the benchmark JSON (and a JSON verdict)
  --tcc | --llvm      Backend (default: --tcc)
  --help              Show this help
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --scenario) shift; [ $# -gt 0 ] || { echo "ui-bench: --scenario needs a name" >&2; exit 2; }; SCENARIO="$1" ;;
        --scenario=*) SCENARIO="${1#*=}" ;;
        --compare) COMPARE=1 ;;
        --json) JSON=1 ;;
        --tcc) BACKEND=tcc ;;
        --llvm) BACKEND=llvm ;;
        --help|-h) usage; exit 0 ;;
        *) echo "ui-bench: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

if [ "$SCENARIO" != "empty" ]; then
    echo "ui-bench: unknown scenario '$SCENARIO' (P0.3a has only 'empty')" >&2
    exit 2
fi

mkdir -p "$BUILD_DIR"
BUILD_LOG="$BUILD_DIR/ui-bench-build.log"
EXE="$BUILD_DIR/ui_bench_${BACKEND}.exe"
if [ "$BACKEND" = "tcc" ]; then
    if ! "$ROOT/cmake-build-debug/TCCCompiler" "$MOD" -o "$EXE" \
            --mode debug_quick >"$BUILD_LOG" 2>&1; then
        cat "$BUILD_LOG" >&2
        echo "ui-bench: TCCCompiler build failed" >&2
        exit 1
    fi
else
    if ! "$ROOT/cmake-build-debug/Compiler" "$MOD" -o "$EXE" \
            --mode debug_quick >"$BUILD_LOG" 2>&1; then
        cat "$BUILD_LOG" >&2
        echo "ui-bench: Compiler build failed" >&2
        exit 1
    fi
fi

if [ ! -x "$EXE" ]; then
    echo "ui-bench: missing executable '$EXE'" >&2
    exit 1
fi

CANDIDATE="$BUILD_DIR/result_${SCENARIO}_${BACKEND}.json"
"$EXE" --scenario "$SCENARIO" --json >"$CANDIDATE"
if ! python3 -c 'import json,sys; json.load(open(sys.argv[1]))' "$CANDIDATE" >/dev/null 2>&1; then
    echo "ui-bench: benchmark did not emit valid JSON" >&2
    exit 1
fi

if [ "$COMPARE" = "1" ]; then
    BASELINE="$ROOT/lang/tests/ui_bench/baselines/${SCENARIO}.json"
    if [ ! -f "$BASELINE" ]; then
        echo "ui-bench: --compare requires $BASELINE (P0.3b adds baselines)" >&2
        if [ "$JSON" = "1" ]; then
            printf '{"scenario":"%s","backend":"%s","status":"no-baseline","exit_code":3}\n' "$SCENARIO" "$BACKEND"
        fi
        exit 3
    fi
    set +e
    python3 - "$BASELINE" "$CANDIDATE" <<'PY'
import json, sys
base = json.load(open(sys.argv[1]))
cand = json.load(open(sys.argv[2]))
bc = base.get("counters", {})
cc = cand.get("counters", {})
bad = []
for key, bv in bc.items():
    if not isinstance(bv, (int, float)):
        continue
    cv = cc.get(key)
    if not isinstance(cv, (int, float)):
        bad.append((key, bv, cv, "missing"))
    elif cv > bv * 1.02:
        bad.append((key, bv, cv, "regression"))
if bad:
    for key, bv, cv, why in bad:
        print(f"ui-bench: {key}: baseline={bv} candidate={cv} ({why})", file=sys.stderr)
    sys.exit(1)
print(f"ui-bench: counters within 2% of baseline ({len(bc)} keys)")
PY
    cmp_rc=$?
    set -e
    if [ "$cmp_rc" -ne 0 ]; then
        if [ "$JSON" = "1" ]; then
            printf '{"scenario":"%s","backend":"%s","status":"fail","exit_code":1}\n' "$SCENARIO" "$BACKEND"
        fi
        exit 1
    fi
fi

if [ "$JSON" = "1" ]; then
    cat "$CANDIDATE"
else
    cat "$CANDIDATE"
    echo "ui-bench: PASS ($SCENARIO, $BACKEND)"
fi
