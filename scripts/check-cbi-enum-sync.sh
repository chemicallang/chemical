#!/usr/bin/env bash
# CBI enum sync gate.
#
# The compiler (C++) and the Chemical plugin bindings (lab.ch) both spell the
# CBIFunctionType enum. A value inserted in only one of them shifts every later
# value, which crashes CBI plugins. This gate parses the names/order from both
# and fails on any mismatch. Values are implicit (sequential), so equal order
# means equal values.
#
#   ./scripts/check-cbi-enum-sync.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

python3 - <<'PY'
import re
import sys

cpp_path = "compiler/cbi/model/CBIFunctionType.h"
chem_path = "lang/libs/lab/src/lab.ch"

def strip_comments(text):
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text

def enum_body(text, pattern):
    m = re.search(pattern, text)
    if not m:
        raise SystemExit(f"check-cbi-enum-sync: could not find enum pattern {pattern!r}")
    body = text[m.end():]
    depth = 1
    out = []
    for ch in body:
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                break
        out.append(ch)
    return "".join(out)

def parse_names(body):
    body = strip_comments(body)
    names = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", body)
    return names

with open(cpp_path, encoding="utf-8") as f:
    cpp = enum_body(f.read(), r"enum\s+class\s+CBIFunctionType[^{]*\{")
with open(chem_path, encoding="utf-8") as f:
    chem = enum_body(f.read(), r"enum\s+CBIFunctionType[^{]*\{")

cpp_names = parse_names(cpp)
chem_names = parse_names(chem)

if cpp_names == chem_names:
    print(f"ok: CBIFunctionType has {len(cpp_names)} values in sync")
    sys.exit(0)

print("check-cbi-enum-sync: CBIFunctionType values differ", file=sys.stderr)
print(f"  {cpp_path}: {cpp_names}", file=sys.stderr)
print(f"  {chem_path}: {chem_names}", file=sys.stderr)

for i, (a, b) in enumerate(zip(cpp_names, chem_names)):
    if a != b:
        print(f"  first difference at index {i}: C++={a!r} Chemical={b!r}", file=sys.stderr)
        break
else:
    print("  one enum has extra trailing values", file=sys.stderr)
sys.exit(1)
PY
