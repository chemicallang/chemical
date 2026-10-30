#!/usr/bin/env bash
# Read-only safety audit baseline check.
#
# Usage:
#   ./scripts/safety-audit.sh [--update] [module.mod ...]
#
# With no module arguments, the audit compiles every module listed in
# lang/docs/safety/audit-modules.txt with `--check` (no emission or link) and
# aggregates the compiler's JSON safety reports by module name.
#
# The default baseline is lang/docs/safety/audit-baseline.json. Any growth
# fails unless --update is passed; --update rewrites the whole-tree baseline.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT_DIR"

UPDATE=false
if [ "${1:-}" = "--update" ]; then
    UPDATE=true
    shift
fi

MODULES_FILE="${SAFETY_AUDIT_MODULES:-lang/docs/safety/audit-modules.txt}"
COMPILER="${COMPILER:-cmake-build-debug/TCCCompiler}"
BASELINE="${SAFETY_AUDIT_BASELINE:-lang/docs/safety/audit-baseline.json}"

if [ ! -x "$COMPILER" ]; then
    echo "error: compiler not found at $COMPILER" >&2
    exit 1
fi

MODULES=()
if [ "$#" -gt 0 ]; then
    MODULES=("$@")
else
    if [ ! -f "$MODULES_FILE" ]; then
        echo "error: audit module list not found at $MODULES_FILE" >&2
        exit 1
    fi
    while IFS= read -r line; do
        line="${line%%#*}"
        line="$(echo "$line" | xargs)"
        [ -n "$line" ] && MODULES+=("$line")
    done < "$MODULES_FILE"
fi

if [ "${#MODULES[@]}" -eq 0 ]; then
    echo "error: no audit modules configured" >&2
    exit 1
fi

TMP_DIR="$(mktemp -d "${TMPDIR:-/tmp}/chemical-safety-audit.XXXXXX")"
trap 'rm -rf "$TMP_DIR"' EXIT
OUT="$TMP_DIR/audit.out"
: > "$OUT"

for module in "${MODULES[@]}"; do
    echo "auditing $module" >&2
    if ! "$COMPILER" "$module" --check --no-cache --build-dir "$TMP_DIR/build" \
            --safety-report json >> "$OUT" 2>&1; then
        echo "error: safety audit compilation failed for $module" >&2
        tail -40 "$OUT" >&2
        exit 1
    fi
done

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
    name = obj["module"]
    totals = obj["report"].get("totals", {})
    # The same dependency graph is compiled from several suite entry points.
    # Keep the most conservative count per module instead of counting it twice.
    previous = modules.get(name)
    if previous is None:
        modules[name] = totals
    else:
        merged = dict(previous)
        for key, value in totals.items():
            merged[key] = max(merged.get(key, 0), value)
        modules[name] = merged

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
    print(f"modules: {len(modules)}")
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

print(f"safety audit modules: {len(modules)}")
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
