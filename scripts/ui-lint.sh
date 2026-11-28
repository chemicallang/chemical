#!/usr/bin/env bash
# Minimal P0.3b UI lint. P5.9 expands the rule set.
#
#   scripts/ui-lint.sh                 # scan the repo targets
#   scripts/ui-lint.sh --fixtures      # clean fixture passes, each bad fixture fails
#   scripts/ui-lint.sh --path DIR      # scan a path
#   scripts/ui-lint.sh --json
#
# Rules: UI-KEY-001 (list loop without key), UI-PANIC-001 (panic/unwrap/expect/
# unreachable and unchecked access in ui* or compiled/ui_*), UI-RAW-001 (raw
# pointer in app/view code without an unsafe host-boundary comment).
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec python3 - "$ROOT" "$@" <<'PY'
import json, os, re, sys
root = sys.argv[1]
args = sys.argv[2:]
path = None
as_json = False
fixtures = False
for i,a in enumerate(args):
    if a == "--path" and i+1 < len(args): path = args[i+1]
    elif a == "--json": as_json = True
    elif a == "--fixtures": fixtures = True
    elif a in ("--help","-h"):
        print("scripts/ui-lint.sh [--path DIR] [--fixtures] [--json]"); sys.exit(0)
    else:
        print(f"ui-lint: unknown argument {a}", file=sys.stderr); sys.exit(2)

panic_re = re.compile(r'\b(panic|unwrap|expect|unreachable)\b|\b(?:take_|get_)?unchecked\b')
raw_re = re.compile(r'ui::raw|as\s+\*mut|as\s+\*const|&raw\s+mut|&raw\b')
list_start = re.compile(r'\bfor\s*\(|\bwhile\s*\(')
visible_call = re.compile(r'\bb\.(text|row|column|grid|image|icon|virtual_list)\s*\(')
key_call = re.compile(r'\b(b\.key|key_of|key_pair|\bkey)\s*\(')

def scan_file(p, rules):
    findings=[]
    try: lines=open(p,encoding='utf-8').read().splitlines()
    except Exception: return findings
    base=os.path.basename(p)
    norm = p.replace(os.sep,"/")
    is_fixture = "/fixtures/lint/" in norm
    in_contract = ("lang/tests/ui/contracts" in norm or "/ui_probe/" in norm or ("/tests/" in norm and not is_fixture))
    for n,line in enumerate(lines,1):
        stripped=line.strip()
        if stripped.startswith("//"): continue
        if "UI-KEY-001" in rules and not in_contract:
            if list_start.search(line):
                window=[]
                for j in range(n, min(n+8, len(lines)+1)):
                    window.append(lines[j-1])
                block="\n".join(window)
                if visible_call.search(block) and not key_call.search(block):
                    findings.append({"rule":"UI-KEY-001","path":p,"line":n,"message":"repeated visible node loop has no b.key/key_of/key_pair"})
        if "UI-PANIC-001" in rules and not in_contract:
            if panic_re.search(line):
                findings.append({"rule":"UI-PANIC-001","path":p,"line":n,"message":"panic/unwrap/expect/unreachable in UI code"})
        if "UI-RAW-001" in rules and not in_contract:
            if raw_re.search(line):
                prev=lines[n-2].strip() if n>1 else ""
                if "unsafe" not in line and "host boundary" not in prev and "host boundary" not in line:
                    findings.append({"rule":"UI-RAW-001","path":p,"line":n,"message":"raw pointer in app/view code without an unsafe host-boundary comment"})
    return findings

def all_ch(d):
    if os.path.isfile(d): return [d]
    out=[]
    for dp,_,fs in os.walk(d):
        if "/build/" in dp.replace(os.sep,"/"): continue
        for f in fs:
            if f.endswith(".ch"): out.append(os.path.join(dp,f))
    return sorted(out)

def scan(paths, rules):
    fs=[]
    for p in paths:
        for f in all_ch(p): fs += scan_file(f, rules)
    return fs

def ui_dirs(*bases):
    out=[]
    for base in bases:
        if not os.path.isdir(base): continue
        for name in os.listdir(base):
            if name == "ui" or name.startswith("ui_"):
                out.append(os.path.join(base,name))
    return out

def paths_for(rule):
    if path:
        return [path]
    if rule == "UI-KEY-001":
        # scaffold/list code only; not every app in the tree
        return [p for p in [os.path.join(root,"lang/tests/ui/src")] if os.path.exists(p)] + ui_dirs(os.path.join(root,"lang/compiled"))
    if rule == "UI-PANIC-001":
        return ui_dirs(os.path.join(root,"lang/libs"), os.path.join(root,"lang/compiled"))
    return ui_dirs(os.path.join(root,"lang/compiled"))

rules=["UI-KEY-001","UI-PANIC-001","UI-RAW-001"]

if fixtures:
    clean = os.path.join(root,"lang/tests/ui/fixtures/lint/clean")
    bads = {
        "bad_key": ("UI-KEY-001", os.path.join(root,"lang/tests/ui/fixtures/lint/bad_key")),
        "bad_unchecked": ("UI-PANIC-001", os.path.join(root,"lang/tests/ui/fixtures/lint/bad_unchecked")),
        "bad_panic": ("UI-PANIC-001", os.path.join(root,"lang/tests/ui/fixtures/lint/bad_panic")),
        "bad_raw": ("UI-RAW-001", os.path.join(root,"lang/tests/ui/fixtures/lint/bad_raw")),
    }
    ok=True
    clean_findings=scan([clean], rules)
    if clean_findings: ok=False
    result={"clean":"pass" if not clean_findings else "fail","bad":{}}
    for name,(rule,badpath) in bads.items():
        f=scan([badpath],[rule])
        passed=any(x["rule"]==rule for x in f)
        result["bad"][name]="fail" if passed else "pass"
        ok = ok and passed
    if as_json:
        print(json.dumps({"fixtures":result,"status":"pass" if ok else "fail"}, sort_keys=True))
    else:
        print(f"ui-lint fixtures: clean={result['clean']} " + " ".join(f"{k}={v}" for k,v in result["bad"].items()))
        print("ui-lint: PASS" if ok else "ui-lint: FAIL")
    sys.exit(0 if ok else 1)

findings=[]
for rule in rules:
    findings += scan(paths_for(rule), [rule])
status="pass" if not findings else "fail"
if as_json:
    print(json.dumps({"findings":findings,"counts":{r:sum(1 for f in findings if f["rule"]==r) for r in rules},"status":status}, sort_keys=True))
else:
    for f in findings:
        print(f"{f['path']}:{f['line']}: {f['rule']}: {f['message']}")
    print(f"ui-lint: {status} ({len(findings)} findings)")
sys.exit(0 if not findings else 1)
PY
