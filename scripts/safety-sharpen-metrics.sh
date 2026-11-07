#!/usr/bin/env bash
# Safety-sharpen size/compile-time metrics.
#
# Usage:
#   ./scripts/safety-sharpen-metrics.sh [--update]
#
# Records release TCCCompiler sizes plus a small compile-time sample. Read-only
# unless --update rewrites lang/docs/safety-sharpen/metrics.json. The hard gate
# is 5,000,000 bytes for the release TCCCompiler (4 MB goal).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

UPDATE=false
if [ "${1:-}" = "--update" ]; then
    UPDATE=true
fi

TCC="${TCC_COMPILER:-cmake-build-release/TCCCompiler}"
LLVM="${LLVM_COMPILER:-cmake-build-release/Compiler}"
METRICS="${SAFETY_SHARPEN_METRICS:-lang/docs/safety-sharpen/metrics.json}"
LIMIT=5000000
SAMPLE_MODULE="${SAFETY_SHARPEN_SAMPLE_MODULE:-lang/tests/safety/chemical.mod}"

file_size() {
    if [ -f "$1" ]; then
        stat -c%s "$1" 2>/dev/null || stat -f%z "$1" 2>/dev/null || echo 0
    else
        echo 0
    fi
}

TCC_SIZE="$(file_size "$TCC")"
LLVM_SIZE="$(file_size "$LLVM")"
STRIPPED=0
if [ -f "$TCC" ]; then
    TMP_STRIP="$(mktemp "${TMPDIR:-/tmp}/chemical-tcc-strip.XXXXXX")"
    cp "$TCC" "$TMP_STRIP"
    if command -v strip >/dev/null 2>&1; then
        strip "$TMP_STRIP" 2>/dev/null || true
    fi
    STRIPPED="$(file_size "$TMP_STRIP")"
    rm -f "$TMP_STRIP"
fi

COMPILE_MS=0
if [ -x "$TCC" ] && [ -f "$SAMPLE_MODULE" ]; then
    START_NS="$(date +%s%N)"
    TMP_BUILD="$(mktemp -d "${TMPDIR:-/tmp}/chemical-sharpen-build.XXXXXX")"
    if ! "$TCC" "$SAMPLE_MODULE" --check --no-cache --build-dir "$TMP_BUILD" >/tmp/chemical-sharpen-metrics.out 2>&1; then
        echo "error: sample compile failed" >&2
        tail -30 /tmp/chemical-sharpen-metrics.out >&2
        rm -rf "$TMP_BUILD"
        exit 1
    fi
    rm -rf "$TMP_BUILD"
    END_NS="$(date +%s%N)"
    COMPILE_MS=$(( (END_NS - START_NS) / 1000000 ))
fi

python3 - "$METRICS" "$UPDATE" "$TCC_SIZE" "$LLVM_SIZE" "$STRIPPED" "$COMPILE_MS" "$LIMIT" <<'PY'
import json
import os
import sys
import datetime

path, update, tcc, llvm, stripped, compile_ms, limit = (
    sys.argv[1], sys.argv[2] == "true", int(sys.argv[3]), int(sys.argv[4]),
    int(sys.argv[5]), int(sys.argv[6]), int(sys.argv[7])
)
data = {
    "release_tcc_compiler_bytes": tcc,
    "release_tcc_compiler_stripped_bytes": stripped,
    "release_llvm_compiler_bytes": llvm,
    "safety_sample_compile_ms": compile_ms,
    "size_limit_bytes": limit,
    "measured_at": datetime.date.today().isoformat(),
}
if update:
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, sort_keys=True)
        f.write("\n")
    print(f"updated {path}")
else:
    try:
        old = json.load(open(path, "r", encoding="utf-8"))
    except FileNotFoundError:
        old = {}
    print("safety-sharpen metrics:")
    for key, value in data.items():
        print(f"  {key:38s} {value}")

if not update:
    print(f"release size gate: {tcc} / {limit} bytes")
    if tcc > limit:
        print(f"ERROR: release TCCCompiler is {tcc - limit} bytes over the hard ceiling", file=sys.stderr)
        sys.exit(1)
    print("release size gate: PASS")
PY
