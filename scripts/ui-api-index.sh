#!/usr/bin/env bash
# Extract public declarations from API.md code fences into a stable JSON index.
#
# P0.3a is a scaffold: P0.2 refines the schema and gates removals/renames.
# Identical repeated declarations are recorded under `identical_duplicates`
# (API.md currently repeats UiCtx.set_text_scale); conflicting duplicates are
# fatal.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/lang/docs/ui/ui_api_index.json"
SRC="$ROOT/lang/docs/ui/API.md"
CHECK=0

usage() {
    cat <<'EOF'
Usage: scripts/ui-api-index.sh [--out <path>] [--check] [--help]

Reads lang/docs/ui/API.md, extracts declarations from Chemical code fences, and
writes a stable sorted JSON index (default: lang/docs/ui/ui_api_index.json).
--check writes to a temp file and fails if the committed index differs (removed,
renamed, or changed signature/default). Exits 1 on a conflicting duplicate key.
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --out) shift; [ $# -gt 0 ] || { echo "ui-api-index: --out needs a path" >&2; exit 2; }; OUT="$1" ;;
        --out=*) OUT="${1#*=}" ;;
        --check) CHECK=1 ;;
        --help|-h) usage; exit 0 ;;
        *) echo "ui-api-index: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

[ -f "$SRC" ] || { echo "ui-api-index: missing $SRC" >&2; exit 1; }
CHECK_TMP=""
if [ "$CHECK" = "1" ]; then
    CHECK_TMP="$(mktemp "${TMPDIR:-/tmp}/ui-api-index.XXXXXX.json")"
    OUT="$CHECK_TMP"
fi
mkdir -p "$(dirname "$OUT")"

python3 - "$SRC" "$OUT" <<'PY'
import json
import re
import sys

src_path, out_path = sys.argv[1], sys.argv[2]
lines = open(src_path, encoding="utf-8").read().splitlines()

decl_re = re.compile(
    r'^\s*(?:public\s+)?(?:comptime\s+)?'
    r'(func|struct|interface|variant|union|enum|type|const|namespace)\b')
name_re = re.compile(
    r'\b(?:func|struct|interface|variant|union|enum|type|const|namespace)\s+'
    r'(?:<[^>]*>\s*)?([A-Za-z_][A-Za-z0-9_]*)')

def norm(text):
    return " ".join(text.split())

def name_of(text):
    m = name_re.search(text)
    if m:
        return m.group(1)
    # `type Name<T> = ...` or punctuation-heavy declarations
    m = re.search(r'([A-Za-z_][A-Za-z0-9_]*)\s*(?:<|=|:)', text)
    return m.group(1) if m else "?"

decls = []
fence = 0
in_code = False
lang = ""
scope = []  # (kind, name, brace_depth_at_open)
depth = 0
i = 0
while i < len(lines):
    raw = lines[i]
    stripped = raw.strip()
    if stripped.startswith("```"):
        if not in_code:
            in_code = True
            lang = stripped[3:].strip()
            fence += 1
        else:
            in_code = False
            lang = ""
        i += 1
        continue
    if in_code and lang in ("chemical", "ch", ""):
        # Track namespace/type scopes by brace depth. Declarations and closes
        # are normally one line each in API.md.
        m = decl_re.match(raw)
        if m:
            kind = m.group(1)
            start = i
            text = raw.strip()
            # A function signature can wrap: consume until parentheses balance,
            # and include a leading `:` return on the following line.
            parens = text.count("(") - text.count(")")
            while parens > 0 and i + 1 < len(lines):
                i += 1
                nxt = lines[i].strip()
                text += " " + nxt
                parens += nxt.count("(") - nxt.count(")")
            if i + 1 < len(lines) and lines[i + 1].strip().startswith(":"):
                i += 1
                text += " " + lines[i].strip()
            name = name_of(text)
            owner = "::".join(s[1] for s in scope)
            if kind != "namespace":
                members = []
                if "{" in raw:
                    body_depth = 0
                    j = i
                    body_lines = []
                    while j < len(lines):
                        line = lines[j]
                        body_depth += line.count("{") - line.count("}")
                        body_lines.append(line)
                        if body_depth <= 0 and j >= i:
                            break
                        j += 1
                    for line in body_lines:
                        stripped_member = line.strip()
                        if not stripped_member or stripped_member.startswith("//"):
                            continue
                        if stripped_member in ("{", "}"):
                            continue
                        # drop the outer braces from the first/last lines
                        stripped_member = stripped_member.strip("{").strip("}").strip()
                        if stripped_member:
                            members.append(norm(stripped_member))
                decls.append({
                    "kind": kind,
                    "name": name,
                    "owner": owner,
                    "signature": norm(text),
                    "members": members,
                    "fence": fence,
                    "line": start + 1,
                    "key": f"{kind}|{owner}|{name}",
                })
            if kind in ("struct", "interface", "variant", "union", "enum", "namespace"):
                scope.append((kind, name, depth))
        depth += raw.count("{") - raw.count("}")
        while scope and depth <= scope[-1][2]:
            scope.pop()
    i += 1

# Stable sorted output. Identical duplicate keys are recorded; a conflicting
# duplicate key (same kind/owner/name, different signature) fails the index.
by_key = {}
for d in decls:
    by_key.setdefault(d["key"], []).append(d)
identical = []
conflicts = []
unique = []
for key in sorted(by_key):
    group = by_key[key]
    if len(group) > 1:
        sigs = {g["signature"] for g in group}
        if len(sigs) > 1:
            conflicts.append((key, [g["line"] for g in group]))
            continue
        identical.append({"key": key, "lines": [g["line"] for g in group]})
    unique.append(group[0])

if conflicts:
    for key, locs in conflicts:
        print(f"ui-api-index: conflicting duplicate key {key!r} at lines {locs}", file=sys.stderr)
    sys.exit(1)

unique.sort(key=lambda d: (d["key"], d["line"]))
out = {
    "schema": 1,
    "source": "lang/docs/ui/API.md",
    "count": len(unique),
    "identical_duplicates": identical,
    "declarations": unique,
}
with open(out_path, "w", encoding="utf-8") as f:
    # Compact generated data: P0.2 refines the schema/format.
    json.dump(out, f, separators=(",", ":"), sort_keys=True)
    f.write("\n")
print(f"ui-api-index: {len(decls)} declarations, {len(identical)} identical duplicate(s) -> {out_path}")
PY

if [ "$CHECK" = "1" ]; then
    BASELINE="$ROOT/lang/docs/ui/ui_api_index.json"
    if [ ! -f "$BASELINE" ]; then
        echo "ui-api-index: --check needs a committed baseline at $BASELINE" >&2
        exit 1
    fi
    if cmp -s "$OUT" "$BASELINE"; then
        echo "ui-api-index: --check OK (index matches baseline)" >&2
        rm -f "$CHECK_TMP"
    else
        echo "ui-api-index: API index changed; review the diff and update the baseline" >&2
        diff -u "$BASELINE" "$OUT" | head -80 >&2 || true
        rm -f "$CHECK_TMP"
        exit 1
    fi
fi
