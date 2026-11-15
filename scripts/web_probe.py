#!/usr/bin/env python3
"""Web-stack probe: measures the artifacts a web build emits.

Phase 0 of `lang/docs/web-stack-plan.md` §3.19. Every metric here answers a
question the plan makes a promise about: how much of a page bundle is runtime,
how much is per-instance dispatch text, how much is duplicated between pages,
whether anything is minified, whether asset names are content-hashed, and
whether production HTML violates the CSP rule (no inline <script>).

Usage:
    python3 scripts/web_probe.py analyze <dir> [--strict] [--json]
    python3 scripts/web_probe.py compare <page-a.js> <page-b.js>

Exit code is 0 unless --strict is given and a gate is violated, so the script
is usable both as a report and as a CI gate.
"""

import gzip
import io
import json
import os
import re
import sys

# ---------------------------------------------------------------------------
# JavaScript scanning
# ---------------------------------------------------------------------------

STATES = ("code", "line_comment", "block_comment", "single", "double", "template")


def scan_js(src):
    """Walk JS source once, yielding spans of comments and measuring whitespace.

    Deliberately a scanner, not a parser: it only needs to know when it is
    inside a string so that a `//` inside a string literal is not mistaken for
    a comment. Regex literals are not modelled; the emitted universal runtime
    and component bodies contain none that embed `//` or `/*`.
    """
    comment_bytes = 0
    blank_line_bytes = 0
    indent_bytes = 0
    trailing_ws_bytes = 0
    state = "code"
    i = 0
    n = len(src)
    while i < n:
        c = src[i]
        if state == "code":
            if c == "/" and i + 1 < n and src[i + 1] == "/":
                state = "line_comment"
                start = i
                while i < n and src[i] != "\n":
                    i += 1
                comment_bytes += i - start
                continue
            if c == "/" and i + 1 < n and src[i + 1] == "*":
                state = "block_comment"
                start = i
                i += 2
                while i < n - 1 and not (src[i] == "*" and src[i + 1] == "/"):
                    i += 1
                i = min(i + 2, n)
                comment_bytes += i - start
                continue
            if c == "'":
                state = "single"
            elif c == '"':
                state = "double"
            elif c == "`":
                state = "template"
            i += 1
            continue
        if state == "line_comment":
            state = "code"
            continue
        if state == "block_comment":
            if c == "*" and i + 1 < n and src[i + 1] == "/":
                i += 2
                state = "code"
                continue
            i += 1
            continue
        # inside a string
        if c == "\\":
            i += 2
            continue
        if (state == "single" and c == "'") or (state == "double" and c == '"') or (
            state == "template" and c == "`"
        ):
            state = "code"
        i += 1

    for line in src.split("\n"):
        stripped = line.strip()
        if not stripped:
            blank_line_bytes += len(line) + 1
            continue
        body = line.lstrip(" ")
        indent_bytes += len(line) - len(body)
        trailing_ws_bytes += len(line) - len(line.rstrip())

    return {
        "comment_bytes": comment_bytes,
        "blank_line_bytes": blank_line_bytes,
        "indent_bytes": indent_bytes,
        "trailing_ws_bytes": trailing_ws_bytes,
    }


def call_spans(src, callee):
    """Byte length of every `<callee>(...)` call, with balanced parens."""
    total = 0
    count = 0
    start = 0
    needle = callee + "("
    while True:
        i = src.find(needle, start)
        if i == -1:
            break
        depth = 0
        j = i + len(callee)
        n = len(src)
        in_str = None
        while j < n:
            c = src[j]
            if in_str:
                if c == "\\":
                    j += 2
                    continue
                if c == in_str:
                    in_str = None
            elif c in ("'", '"', "`"):
                in_str = c
            elif c == "(":
                depth += 1
            elif c == ")":
                depth -= 1
                if depth == 0:
                    j += 1
                    break
            j += 1
        total += j - i
        count += 1
        start = j
    return count, total


DEF_RE = re.compile(r"(?m)^function ([A-Za-z_$][A-Za-z0-9_$]*)\(")


def split_bundle(src):
    """Split an emitted page bundle into defs / runtime / call-site regions.

    The emitter currently produces: component definitions, then the hydration
    runtime, then the per-page dispatch calls. If the shape changes the probe
    reports the regions it can still find rather than failing to parse.
    """
    lines = src.split("\n")
    def_spans = []
    for line in lines:
        m = DEF_RE.match(line)
        if m:
            def_spans.append((m.group(1), len(line) + 1, line))

    def_line_count = sum(1 for l in lines if DEF_RE.match(l))
    first_dispatch = next(
        (i for i, l in enumerate(lines) if "window.$__uni_dispatch(" in l), None
    )
    def_bytes = sum(span[1] for span in def_spans)
    if first_dispatch is None:
        runtime_bytes = 0
        callsite_bytes = 0
    else:
        runtime_bytes = sum(len(l) + 1 for l in lines[def_line_count:first_dispatch])
        callsite_bytes = sum(len(l) + 1 for l in lines[first_dispatch:])
    return {
        "defs": def_spans,
        "def_bytes": def_bytes,
        "def_count": len(def_spans),
        "runtime_bytes": runtime_bytes,
        "callsite_bytes": callsite_bytes,
    }


def analyze_js(path):
    raw = io.open(path, encoding="utf-8", errors="replace").read()
    regions = split_bundle(raw)
    dispatch_count, dispatch_bytes = call_spans(raw, "window.$__uni_dispatch")
    scanner = scan_js(raw)
    minifiable = (
        scanner["comment_bytes"]
        + scanner["blank_line_bytes"]
        + scanner["indent_bytes"]
        + scanner["trailing_ws_bytes"]
    )
    return {
        "path": path,
        "bytes": len(raw),
        "gzip": len(gzip.compress(raw.encode("utf-8"), 6)),
        "def_count": regions["def_count"],
        "def_bytes": regions["def_bytes"],
        "runtime_bytes": regions["runtime_bytes"],
        "callsite_bytes": regions["callsite_bytes"],
        "dispatch_count": dispatch_count,
        "dispatch_bytes": dispatch_bytes,
        "get_element_by_id": raw.count("getElementById"),
        "esm_import": len(re.findall(r"(?m)^\s*import[\s{*'\"]", raw)),
        "esm_export": len(re.findall(r"(?m)^\s*export[\s{*]", raw)),
        "dynamic_import": len(re.findall(r"\bimport\s*\(", raw)),
        "source_map_comment": "sourceMappingURL" in raw,
        "minifiable_bytes": minifiable,
        "defs": regions["defs"],
    }


# ---------------------------------------------------------------------------
# HTML checks
# ---------------------------------------------------------------------------

SCRIPT_RE = re.compile(r"<script\b([^>]*)>", re.I)
LINK_RE = re.compile(r"<link\b([^>]*)>", re.I)
ATTR_RE = re.compile(r"([a-zA-Z_:][-a-zA-Z0-9_:.]*)\s*=\s*(\"([^\"]*)\"|'([^']*)'|([^\s>]+))")
HASHED_RE = re.compile(r"\.[0-9a-f]{8,}\.(js|css)(\?|$)")


def attrs_of(tag_body):
    out = {}
    for m in ATTR_RE.finditer(tag_body):
        out[m.group(1).lower()] = m.group(3) or m.group(4) or m.group(5) or ""
    return out


def analyze_html(path):
    raw = io.open(path, encoding="utf-8", errors="replace").read()
    inline = 0
    module_scripts = 0
    external = []
    for m in SCRIPT_RE.finditer(raw):
        a = attrs_of(m.group(1))
        if "src" in a:
            external.append(a["src"])
            if (a.get("type") or "").lower() == "module":
                module_scripts += 1
        else:
            inline += 1
    stylesheets = []
    for m in LINK_RE.finditer(raw):
        a = attrs_of(m.group(1))
        if "stylesheet" in (a.get("rel") or "").lower() and "href" in a:
            stylesheets.append(a["href"])
    return {
        "path": path,
        "bytes": len(raw),
        "gzip": len(gzip.compress(raw.encode("utf-8"), 6)),
        "inline_scripts": inline,
        "module_scripts": module_scripts,
        "external_scripts": external,
        "stylesheets": stylesheets,
        "unhashed_assets": [u for u in external + stylesheets if not HASHED_RE.search(u)],
    }


# ---------------------------------------------------------------------------
# Cross-page duplication
# ---------------------------------------------------------------------------


def line_multiset(src):
    counts = {}
    for line in src.split("\n"):
        if line.strip():
            counts[line] = counts.get(line, 0) + 1
    return counts


def compare_pages(a_path, b_path):
    a = io.open(a_path, encoding="utf-8", errors="replace").read()
    b = io.open(b_path, encoding="utf-8", errors="replace").read()
    ca, cb = line_multiset(a), line_multiset(b)
    dup_bytes = 0
    dup_lines = 0
    for line, count in ca.items():
        shared = min(count, cb.get(line, 0))
        if shared:
            dup_bytes += shared * (len(line) + 1)
            dup_lines += shared
    # component definitions present in both, byte-identical
    defs_a = {name: text for name, _n, text in split_bundle(a)["defs"]}
    defs_b = {name: text for name, _n, text in split_bundle(b)["defs"]}
    common = set(defs_a) & set(defs_b)
    identical = sorted(n for n in common if defs_a[n].rstrip() == defs_b[n].rstrip())
    identical_bytes = sum(len(defs_a[n]) + 1 for n in identical)
    return {
        "a": a_path,
        "b": b_path,
        "a_bytes": len(a),
        "b_bytes": len(b),
        "duplicate_lines": dup_lines,
        "duplicate_bytes": dup_bytes,
        "duplicate_share_of_b": (dup_bytes / len(b)) if b else 0.0,
        "defs_common": len(common),
        "defs_identical": len(identical),
        "defs_identical_bytes": identical_bytes,
        "defs_only_in_a": sorted(set(defs_a) - set(defs_b))[:10],
        "defs_only_in_b": sorted(set(defs_b) - set(defs_a))[:10],
    }


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------


def pct(part, whole):
    return f"{(100.0 * part / whole):.1f}%" if whole else "0.0%"


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    mode = argv[1]
    strict = "--strict" in argv
    as_json = "--json" in argv
    args = [a for a in argv[2:] if not a.startswith("--")]

    if mode == "compare":
        if len(args) != 2:
            print("compare needs two .js files")
            return 2
        result = compare_pages(args[0], args[1])
        if as_json:
            print(json.dumps(result, indent=2))
        else:
            r = result
            print(f"pages: {os.path.basename(r['a'])} ({r['a_bytes']} B) vs "
                  f"{os.path.basename(r['b'])} ({r['b_bytes']} B)")
            print(f"  duplicated bytes: {r['duplicate_bytes']} "
                  f"({pct(r['duplicate_bytes'], r['a_bytes'])} of page A, "
                  f"{pct(r['duplicate_share_of_b']*100, 100)} of page B)")
            print(f"  shared component defs: {r['defs_common']}, byte-identical: "
                  f"{r['defs_identical']} ({r['defs_identical_bytes']} B)")
        return 0

    if mode != "analyze":
        print(f"unknown mode {mode}")
        return 2

    directory = args[0] if args else "lang/compiled/components/output"
    if not os.path.isdir(directory):
        print(f"not a directory: {directory}")
        return 2

    js_files = sorted(
        os.path.join(directory, f) for f in os.listdir(directory) if f.endswith(".js")
    )
    html_files = sorted(
        os.path.join(directory, f) for f in os.listdir(directory) if f.endswith(".html")
    )
    js = [analyze_js(p) for p in js_files]
    html = [analyze_html(p) for p in html_files]

    violations = []
    for h in html:
        if h["inline_scripts"]:
            violations.append(
                f"{os.path.basename(h['path'])}: {h['inline_scripts']} inline <script> "
                f"(CSP rule)"
            )
        for u in h["unhashed_assets"]:
            violations.append(f"{os.path.basename(h['path'])}: unhashed asset {u}")

    pages = [j for j in js if j["def_count"] > 0]
    worst = pages[0] if pages else None
    if worst:
        for j in pages:
            for other in pages:
                if j is other:
                    continue
                dup = set(n for n, _b, _t in j["defs"]) & set(
                    n for n, _b, _t in other["defs"]
                )
                if dup and len(pages) > 1:
                    violations.append(
                        f"{os.path.basename(j['path'])} duplicates {len(dup)} component "
                        f"defs with {os.path.basename(other['path'])}"
                    )
                    break

    if as_json:
        print(json.dumps({"js": js, "html": html, "violations": violations}, indent=2,
                         default=str))
        return 1 if (strict and violations) else 0

    print(f"web probe — {directory}")
    print()
    header = (f"{'file':<28} {'bytes':>9} {'gzip':>8} {'defs':>5} {'runtime':>9} "
              f"{'dispatch':>9} {'calls':>6} {'minify':>7}")
    print(header)
    print("-" * len(header))
    for j in js:
        print(f"{os.path.basename(j['path']):<28} {j['bytes']:>9} {j['gzip']:>8} "
              f"{j['def_count']:>5} "
              f"{j['runtime_bytes']:>8} ({pct(j['runtime_bytes'], j['bytes']):>5}) "
              f"{j['dispatch_bytes']:>8} ({pct(j['dispatch_bytes'], j['bytes']):>5}) "
              f"{j['dispatch_count']:>6} {pct(j['minifiable_bytes'], j['bytes']):>7}")
    print()
    if html:
        print(f"{'page':<28} {'bytes':>9} {'inline js':>9} {'module':>7} {'css':>4}")
        for h in html:
            print(f"{os.path.basename(h['path']):<28} {h['bytes']:>9} "
                  f"{h['inline_scripts']:>9} {h['module_scripts']:>7} "
                  f"{len(h['stylesheets']):>4}")
        print()

    if len(pages) >= 2:
        c = compare_pages(pages[0]["path"], pages[1]["path"])
        print(f"cross-page duplication ({os.path.basename(c['a'])} vs "
              f"{os.path.basename(c['b'])}):")
        print(f"  duplicated bytes: {c['duplicate_bytes']} "
              f"({pct(c['duplicate_share_of_b'], 1.0)} of page B)")
        print(f"  shared defs: {c['defs_common']}, identical: {c['defs_identical']} "
              f"({c['defs_identical_bytes']} B)")
        print()

    esm = sum(j["esm_import"] + j["esm_export"] for j in js)
    maps = sum(1 for j in js if j["source_map_comment"])
    print(f"ecosystem: ESM statements={esm}, source maps={maps}, "
          f"pages with inline JS={sum(1 for h in html if h['inline_scripts'])}")

    if violations:
        print()
        print(f"violations ({len(violations)}):")
        for v in violations:
            print(f"  - {v}")
        if strict:
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
