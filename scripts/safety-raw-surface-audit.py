#!/usr/bin/env python3
"""Soundness gate: a public function that ACCEPTS a raw pointer must be
`unsafe func`.

Why: a raw-pointer parameter is a caller assertion that the pointer (and any
length paired with it) is valid. A declaration that is not `unsafe func` makes
that assertion available to safe code, so safe code can reach an out-of-bounds
read/write through the callee's `unsafe` body without writing `unsafe` itself.

A raw-pointer *return* is fine: returning a pointer does not trust the caller.

Criterion (multi-line aware): the text between the declaration's parentheses
mentions `*T` / `*mut T` / `*const T`. Exits non-zero when any offender exists.
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DECL = re.compile(r"\bpublic\s+(comptime\s+)?(func)\b\s*(<[^>]*>)?\s*([A-Za-z_][A-Za-z0-9_]*)")
RAWPTR = re.compile(r"\*\s*(mut\s+|const\s+)?[A-Za-z_]")
# A `%runtime<T>` parameter is produced by an `@expand` template, not passed by
# the caller, so it is not a caller assertion about pointer validity.
RUNTIME = re.compile(r"%(?:maybe_)?runtime<[^<>]*(?:<[^<>]*>[^<>]*)*>")


def mask_comments(text):
    out = []
    for line in text.splitlines(keepends=True):
        idx = line.find("//")
        if idx >= 0:
            out.append(line[:idx] + " " * (len(line) - idx))
        else:
            out.append(line)
    return "".join(out)


def params_of(text, start):
    open_idx = text.find("(", start)
    if open_idx < 0:
        return None
    depth = 0
    for i in range(open_idx, len(text)):
        if text[i] == "(":
            depth += 1
        elif text[i] == ")":
            depth -= 1
            if depth == 0:
                return open_idx, i
    return None


def main():
    offenders = []
    for path in sorted(glob.glob(os.path.join(ROOT, "lang/libs/**/*.ch"), recursive=True)):
        rel = os.path.relpath(path, ROOT).replace(os.sep, "/")
        text = open(path, "rb").read().decode("utf-8", "replace")
        masked = mask_comments(text)
        for m in DECL.finditer(masked):
            before = masked[max(0, m.start() - 20):m.start()]
            if re.search(r"\bunsafe\s+$", before):
                continue
            span = params_of(masked, m.end())
            if not span:
                continue
            o, c = span
            name = m.group(4)
            # `__chc_*` are compiler runtime ABI symbols (abort/bounds/enum
            # failure hooks) emitted by codegen and by `panic`/`assert`
            # expansions. They are not called from source, and requiring
            # `unsafe` would force every expansion site to be unsafe.
            if name.startswith("__chc_"):
                continue
            if not RAWPTR.search(RUNTIME.sub("", masked[o:c + 1])):
                continue
            line = text[:m.start()].count("\n") + 1
            offenders.append("%s:%d: %s" % (rel, line, name))

    if offenders:
        sys.stderr.write(
            "error: %d public function(s) accept a raw pointer without `unsafe func`:\n"
            % len(offenders)
        )
        for o in offenders:
            sys.stderr.write("  " + o + "\n")
        sys.stderr.write(
            "       declaring such a function safe lets safe code reach undefined behaviour;\n"
            "       mark it `unsafe func`, or take a span/view instead of a raw pointer\n"
            "       (a raw-pointer *return* is fine and is not counted).\n"
        )
        return 1
    print("RAW_UNSAFE_GATE: every public function that accepts a raw pointer is `unsafe func`")
    return 0


sys.exit(main())
