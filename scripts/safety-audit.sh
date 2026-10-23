#!/usr/bin/env bash
# Read-only safety audit baseline check.
#
# Usage:
#   ./scripts/safety-audit.sh [--update] [module]
#
# Default module: lang/tests/safety/chemical.mod
# Default baseline: lang/docs/safety/audit-baseline.json
#
# The compiler emits one JSON object per module (JSONL) for
# --safety-report json. This script sums the totals and compares them with the
# checked-in baseline. Any growth fails unless --update is passed.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

UPDATE=false
if [ "${1:-}" = "--update" ]; then
    UPDATE=true
    shift
fi

MODULE="${1:-lang/tests/safety/chemical.mod}"
COMPILER="${COMPILER:-cmake-build-debug/TCCCompiler}"
BASELINE="${SAFETY_AUDIT_BASELINE:-lang/docs/safety/audit-baseline.json}"

if [ ! -x "$COMPILER" ]; then
    echo "error: compiler not found at $COMPILER" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/chemical-safety-audit.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT
OUT="$TMP_DIR/audit.out"
EXE="$TMP_DIR/audit.exe"

if ! "$COMPILER" "$MODULE" --no-cache --safety-report json -o "$EXE" > "$OUT" 2>&1; then
    echo "error: safety audit compilation failed" >&2
    tail -40 "$OUT" >&2
    exit 1
fi

mkdir -p "$(dirname "$BASELINE")"

python3 - "$OUT" "$BASELINE" "$UPDATE" <<'PY'
import json
import sys

out_path, baseline_path, update = sys.argv[1], sys.argv[2], sys.argv[3] == "true"
modules = {}
for line in open(out_path, "r", encoding="utf-8", errors="replace"):
    line = line.strip()
    if not line.startswith("{"):
        continue
    try:
        obj = json.loads(line)
    except json.JSONDecodeError:
        continue
    if "module" not in obj or "report" not in obj:
        continue
    report = obj["report"]
    modules[obj["module"]] = report.get("totals", {})

if not modules:
    print("error: no JSON safety report found in compiler output", file=sys.stderr)
    sys.exit(1)

totals = {}
for counts in modules.values():
    for key, value in counts.items():
        totals[key] = totals.get(key, 0) + value

data = {"modules": modules, "totals": totals}
if update:
    with open(baseline_path, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, sort_keys=True)
        f.write("\n")
    print(f"updated {baseline_path}")
    print(json.dumps(totals, sort_keys=True))
    sys.exit(0)

try:
    baseline = json.load(open(baseline_path, "r", encoding="utf-8"))
except FileNotFoundError:
    print(f"error: baseline not found at {baseline_path}; run with --update", file=sys.stderr)
    sys.exit(1)

base_totals = baseline.get("totals", {})
growth = []
for key in sorted(set(totals) | set(base_totals)):
    now = totals.get(key, 0)
    before = base_totals.get(key, 0)
    if now > before:
        growth.append((key, before, now))

print("safety audit totals:")
for key in sorted(totals):
    print(f"  {key:24s} {totals[key]}")

if growth:
    print("\nERROR: safety surface grew:", file=sys.stderr)
    for key, before, now in growth:
        print(f"  {key}: {before} -> {now}", file=sys.stderr)
    print("Review the new occurrences; if intentional, run with --update.", file=sys.stderr)
    sys.exit(1)

print("\nsafety audit: no growth")
PY
