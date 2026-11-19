#!/usr/bin/env bash
# Shared chunk-graph verifier for the web gates (`web-app-test.sh`,
# `web-golden-test.sh`).
#
# The unit graph (lang/docs/web-stack-plan.md §3.13, Phase 4 item 7) promises two
# things about a built site that are checkable from the artifacts alone:
#
#   1. **closure** — every component a page dispatches (`$__uni_c(id,'name',…)`)
#      is defined in the file that dispatches it or in the shared bundle, so the
#      page hydrates;
#   2. **no duplicates** — no definition ships in two files, so no route
#      downloads a component it already has.
#
# Deliberately independent of the compiler: it reads only the built bytes, so it
# keeps working if the graph derivation changes.
#
# Usage: `source scripts/web-graph.sh; graph_check <site dir>`

_web_graph_py=""
for _candidate in python3 python; do
    if command -v "$_candidate" >/dev/null 2>&1 && "$_candidate" -c 'import sys' >/dev/null 2>&1; then
        _web_graph_py="$_candidate"
        break
    fi
done

# Print one line per problem and return non-zero; print nothing and return zero
# when the graph is closed and duplicate-free.
graph_check() {
    if [ -z "$_web_graph_py" ]; then
        echo "graph_check: no python interpreter found" >&2
        return 2
    fi
    "$_web_graph_py" - "$1" <<'PY'
import os, re, sys
root = sys.argv[1]
assets = os.path.join(root, 'assets')
if not os.path.isdir(assets):
    print('no assets directory at %s' % assets)
    sys.exit(1)
files = {}
for name in sorted(os.listdir(assets)):
    if name.endswith('.js'):
        with open(os.path.join(assets, name), 'r', encoding='utf-8') as fh:
            files[name] = fh.read()
bundles = [n for n in files if re.fullmatch(r'app\.[0-9a-f]+\.js', n)]
if len(bundles) != 1:
    print('expected exactly one shared bundle, found %d' % len(bundles))
    sys.exit(1)
bundle = bundles[0]
defs = {n: set(re.findall(r'function ([A-Za-z0-9_$]+)\(props\)', t)) for n, t in files.items()}
problems = []
seen = {}
for name, names in defs.items():
    for d in sorted(names):
        if d in seen:
            problems.append('duplicate definition of %s in %s and %s' % (d, seen[d], name))
        seen[d] = name
for name, text in files.items():
    refs = set(re.findall(r"\$__uni_c\(\d+,'([A-Za-z0-9_$]+)'", text))
    for ref in sorted(refs):
        if ref not in defs[name] and ref not in defs[bundle]:
            problems.append('%s dispatches %s but neither it nor %s defines it' % (name, ref, bundle))
for p in problems:
    print(p)
sys.exit(1 if problems else 0)
PY
}
