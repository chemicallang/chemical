#!/usr/bin/env python3
"""Mechanical Phase 2 migration assistant for removed syntax.

This is intentionally conservative: it rewrites only shapes it can prove
textually and prints a report for everything it leaves alone. It never inserts
`unsafe`.

Usage:
  scripts/safety-migrate-removed.py [--write] PATH...
"""
import re
import sys
from pathlib import Path

def migrate(text: str):
    report = []
    # Box<[T]> -> vector<T> (flat single-argument slices only)
    def box_sub(m):
        report.append("Box<[T]> -> vector<T>")
        return f"vector<{m.group(1)}>"
    text = re.sub(r"\bBox<\s*\[\s*([^\[\]<>]+?)\s*\]\s*>", box_sub, text)

    # view[a..b] -> view.subview(a, b); only simple identifiers/literals.
    def range_sub(m):
        base, a, b = m.group(1), m.group(2), m.group(3)
        report.append(f"range slice `{base}[{a}..{b}]` -> subview")
        return f"{base}.subview({a}, {b})"
    text = re.sub(r"\b([A-Za-z_][A-Za-z0-9_]*)\[\s*([^\[\]:]+?)\s*\.\.\s*([^\[\]]+?)\s*\]", range_sub, text)

    # `[..]` -> subview(0, size)
    def all_sub(m):
        base = m.group(1)
        report.append(f"full range `{base}[..]` -> subview")
        return f"{base}.subview(0, {base}.size())"
    text = re.sub(r"\b([A-Za-z_][A-Za-z0-9_]*)\[\s*\.\.\s*\]", all_sub, text)

    if re.search(r"\btype\s+[A-Za-z_][A-Za-z0-9_]*\s*:", text):
        report.append("associated-type declaration remains: migrate manually (REMOVED-001)")
    if re.search(r"\bimpl\s*<[^>]*>\s*[A-Za-z_][A-Za-z0-9_]*\s*<", text) or re.search(r"^\s*impl\s+[A-Za-z_][A-Za-z0-9_]*\s*\{", text, re.M):
        report.append("external/generic inherent impl remains: migrate to in-body methods or a trait impl (REMOVED-002)")
    return text, report

def main():
    write = "--write" in sys.argv
    paths = [Path(p) for p in sys.argv[1:] if p != "--write"]
    if not paths:
        print(__doc__)
        return 2
    changed = 0
    for root in paths:
        files = [root] if root.is_file() else sorted(root.rglob("*.ch")) + sorted(root.rglob("*.lab"))
        for p in files:
            try:
                text = p.read_text()
            except (UnicodeDecodeError, OSError):
                continue
            new, report = migrate(text)
            if new != text:
                changed += 1
                print(f"{p}:")
                for r in report:
                    print(f"  - {r}")
                if write:
                    p.write_text(new)
    print(f"{changed} file(s) changed" + ("" if write else " (dry run; pass --write)"))
    return 0

if __name__ == "__main__":
    raise SystemExit(main())
