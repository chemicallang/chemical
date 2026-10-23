#!/usr/bin/env bash
# Safety compile-time/size baseline.
#
# Usage:
#   ./scripts/safety-baseline.sh [--update]
#
# Records the release compiler sizes and the wall time of the safety audit on
# the safety test module. The check fails on significant growth (size >5% and
# >64 KiB, or audit wall time >25%). This is a lightweight gate; the full
# benchmark corpus lives in scripts/bench-*.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

UPDATE=false
if [ "${1:-}" = "--update" ]; then
    UPDATE=true
fi

TCC="${TCC_COMPILER:-cmake-build-debug/TCCCompiler}"
LLVM="${LLVM_COMPILER:-cmake-build-debug/Compiler}"
BASELINE="${SAFETY_PERF_BASELINE:-lang/docs/safety/perf-baseline.json}"
MODULE="${SAFETY_AUDIT_MODULE:-lang/tests/safety/chemical.mod}"

file_size() {
    if [ -f "$1" ]; then
        stat -c%s "$1" 2>/dev/null || stat -f%z "$1" 2>/dev/null || echo 0
    else
        echo 0
    fi
}

TCC_SIZE="$(file_size "$TCC")"
LLVM_SIZE="$(file_size "$LLVM")"

START_NS="$(date +%s%N)"
if ! "$SCRIPT_DIR/safety-audit.sh" "$MODULE" > /tmp/chemical-safety-perf.out 2>&1; then
    echo "error: safety audit failed during baseline measurement" >&2
    tail -30 /tmp/chemical-safety-perf.out >&2
    exit 1
fi
END_NS="$(date +%s%N)"
AUDIT_MS=$(( (END_NS - START_NS) / 1000000 ))

mkdir -p "$(dirname "$BASELINE")"

python3 - "$BASELINE" "$UPDATE" "$TCC_SIZE" "$LLVM_SIZE" "$AUDIT_MS" <<'PY'
import json
import os
import sys

baseline_path, update, tcc_size, llvm_size, audit_ms = sys.argv[1], sys.argv[2] == "true", int(sys.argv[3]), int(sys.argv[4]), int(sys.argv[5])
data = {
    "tcc_compiler_bytes": tcc_size,
    "llvm_compiler_bytes": llvm_size,
    "safety_audit_ms": audit_ms,
}

if update:
    with open(baseline_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, sort_keys=True)
        f.write("\n")
    print(f"updated {baseline_path}")
    print(json.dumps(data, sort_keys=True))
    sys.exit(0)

try:
    base = json.load(open(baseline_path, "r", encoding="utf-8"))
except FileNotFoundError:
    print(f"error: baseline not found at {baseline_path}; run with --update", file=sys.stderr)
    sys.exit(1)

failures = []
for key, current, minimum_delta in (
    ("tcc_compiler_bytes", tcc_size, 64 * 1024),
    ("llvm_compiler_bytes", llvm_size, 256 * 1024),
):
    before = int(base.get(key, 0))
    if before <= 0:
        continue
    delta = current - before
    if delta > minimum_delta and current > before * 1.05:
        failures.append(f"{key}: {before} -> {current} (+{delta})")

before_ms = int(base.get("safety_audit_ms", 0))
if before_ms > 0 and audit_ms > before_ms * 1.25:
    failures.append(f"safety_audit_ms: {before_ms} -> {audit_ms}")

print("safety performance baseline:")
for key, value in data.items():
    print(f"  {key:24s} {value}")

if failures:
    print("\nERROR: safety baseline regressed:", file=sys.stderr)
    for failure in failures:
        print(f"  {failure}", file=sys.stderr)
    print("Investigate the regression; use --update only for an intentional, reviewed change.", file=sys.stderr)
    sys.exit(1)

print("\nsafety performance: within baseline")
PY
