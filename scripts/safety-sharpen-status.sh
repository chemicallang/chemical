#!/usr/bin/env bash
# Safety-sharpen plan status.
#
# Usage:
#   ./scripts/safety-sharpen-status.sh [--quiet]
#
# Reads lang/docs/safety-sharpen/STATUS.md (the evidence table) and
# lang/docs/safety-sharpen/gates.json (job -> required gate map), then prints:
#   * implemented/total and percentage per phase and overall
#   * the next incomplete job
#   * blocked jobs with the recorded reason
#
# Exits nonzero when STATUS.md is malformed (bad columns, unknown status,
# duplicate/missing IDs, unknown phase) or when gates.json is invalid / a mapped
# gate script no longer exists. Stale evidence on individual completed rows is
# reported as a warning (older rows predate the timestamp rule) and does not
# fail the gate.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

QUIET=false
[ "${1:-}" = "--quiet" ] && QUIET=true

exec python3 - "$ROOT_DIR" "$QUIET" <<'PY'
import json
import os
import re
import sys

root = sys.argv[1]
quiet = sys.argv[2] == "true"
status_path = os.path.join(root, "lang/docs/safety-sharpen/STATUS.md")
gates_path = os.path.join(root, "lang/docs/safety-sharpen/gates.json")

errors = []
warnings = []

if not os.path.isfile(status_path):
    print(f"error: missing {status_path}", file=sys.stderr)
    sys.exit(2)

try:
    with open(gates_path, "r", encoding="utf-8") as f:
        gates = json.load(f)
except (OSError, json.JSONDecodeError) as exc:
    print(f"error: cannot read gates.json: {exc}", file=sys.stderr)
    sys.exit(2)

phases = gates.get("phases", {})
job_gates = gates.get("jobs", {})
if not isinstance(phases, dict) or not isinstance(job_gates, dict):
    print("error: gates.json must contain 'phases' and 'jobs' objects", file=sys.stderr)
    sys.exit(2)

STATUS_VALUES = {"[ ]": "not started", "[~]": "in progress", "[x]": "done", "[!]": "blocked"}
ID_RE = re.compile(r"^P\d+(?:\.\d+)?[a-z]?$")
DATE_RE = re.compile(r"\b\d{4}-\d{2}-\d{2}\b")
SHA_RE = re.compile(r"\b[0-9a-f]{7,40}\b")

rows = []
with open(status_path, "r", encoding="utf-8") as f:
    for lineno, raw in enumerate(f, 1):
        line = raw.rstrip("\n")
        if not line.startswith("|"):
            continue
        cells = [c.strip() for c in line.strip().strip("|").split("|")]
        if len(cells) < 2:
            continue
        if cells[0] in ("ID", "---") or set(cells[0]) <= set("-: "):
            continue
        if not ID_RE.match(cells[0]):
            # A prose table row that is not a job row is ignored.
            continue
        if len(cells) != 7:
            errors.append(f"STATUS.md:{lineno}: job row '{cells[0]}' has {len(cells)} columns, expected 7")
            continue
        job_id, phase, area, job, status, owner, evidence = cells
        if status not in STATUS_VALUES:
            errors.append(f"STATUS.md:{lineno}: job '{job_id}' has unknown status '{status}'")
            continue
        if phase not in phases:
            errors.append(f"STATUS.md:{lineno}: job '{job_id}' has unknown phase '{phase}'")
            continue
        if not owner:
            errors.append(f"STATUS.md:{lineno}: job '{job_id}' has no owner")
            continue
        rows.append({
            "id": job_id, "phase": phase, "area": area, "job": job,
            "status": status, "owner": owner, "evidence": evidence,
            "line": lineno,
        })

if not rows:
    errors.append("STATUS.md: no job rows found")

seen = {}
for row in rows:
    if row["id"] in seen:
        errors.append(
            f"STATUS.md:{row['line']}: duplicate job ID '{row['id']}' "
            f"(first at line {seen[row['id']]['line']})")
    seen[row["id"]] = row

# Job -> gate coverage and gate-command existence.
for row in rows:
    jid = row["id"]
    if jid not in job_gates:
        errors.append(f"gates.json: no gate mapped for job '{jid}'")
        continue
    cmds = job_gates[jid]
    if not isinstance(cmds, list) or not cmds:
        errors.append(f"gates.json: job '{jid}' must map to a non-empty list of gate commands")
        continue
    for cmd in cmds:
        if not isinstance(cmd, str) or not cmd.strip():
            errors.append(f"gates.json: job '{jid}' has an empty gate command")
            continue
        first = cmd.split()[0]
        if first.startswith("./scripts/") or first.startswith("scripts/"):
            path = os.path.join(root, first[2:] if first.startswith("./") else first)
            if not os.path.isfile(path):
                errors.append(f"gates.json: job '{jid}' maps to missing gate script '{first}'")
    if row["status"] in ("[x]", "[~]") and row["evidence"]:
        ev = row["evidence"]
        if not DATE_RE.search(ev):
            warnings.append(f"STATUS.md:{row['line']}: '{jid}' evidence has no YYYY-MM-DD timestamp")
        if not SHA_RE.search(ev) and not any(c.split()[0].startswith("./scripts/") for c in cmds):
            warnings.append(f"STATUS.md:{row['line']}: '{jid}' evidence has no commit SHA")

# Per-phase summary.
overall_done = overall_total = 0
phase_stats = {}
for pid in sorted(phases, key=lambda p: int(p)):
    phase_stats[pid] = {"done": 0, "total": 0, "in_progress": 0, "blocked": 0}
for row in rows:
    st = phase_stats[row["phase"]]
    st["total"] += 1
    if row["status"] == "[x]":
        st["done"] += 1
    elif row["status"] == "[~]":
        st["in_progress"] += 1
    elif row["status"] == "[!]":
        st["blocked"] += 1
    overall_total += 1
    if row["status"] == "[x]":
        overall_done += 1

def pct(done, total):
    return (100.0 * done / total) if total else 0.0

if not quiet:
    print("Safety sharpen plan status")
    print("==========================")
    for pid in sorted(phases, key=lambda p: int(p)):
        st = phase_stats[pid]
        print(f"Phase {pid} — {phases[pid]}")
        print(f"  implemented {st['done']}/{st['total']} "
              f"({pct(st['done'], st['total']):.1f}%), "
              f"in progress {st['in_progress']}, blocked {st['blocked']}")
    print(f"Overall: {overall_done}/{overall_total} ({pct(overall_done, overall_total):.1f}%)")
    print()

next_job = next((r for r in rows if r["status"] in ("[ ]", "[~]")), None)
if next_job:
    print(f"Next incomplete job: {next_job['id']} — {next_job['job']} "
          f"({STATUS_VALUES[next_job['status']]})")
else:
    print("Next incomplete job: none")

blocked = [r for r in rows if r["status"] == "[!]"]
if blocked:
    print("Blocked jobs:")
    for row in blocked:
        reason = row["evidence"] or "(no reason recorded)"
        print(f"  {row['id']}: {reason}")
else:
    print("Blocked jobs: none")

if warnings:
    print()
    print("Warnings:")
    for w in warnings:
        print(f"  warn: {w}")

if errors:
    print()
    print("Malformed STATUS/gates:", file=sys.stderr)
    for e in errors:
        print(f"  error: {e}", file=sys.stderr)
    sys.exit(1)

sys.exit(0)
PY
