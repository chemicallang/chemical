#!/usr/bin/env bash
# Verify a UI packet manifest/evidence bundle and run the harness self-test.
#
#   scripts/ui-verify.sh --self-test
#   scripts/ui-verify.sh --packet P0.3a
#   scripts/ui-verify.sh --packet P0.3a --evidence /tmp/evidence --json
#   scripts/ui-verify.sh --write-status
#
# No network access. Exit codes: 0 pass, 1 verification failure, 2 usage.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JSON=0
PACKET=""
EVIDENCE=""
SELF_TEST=0
WRITE_STATUS=0
SCHEMA="$ROOT/lang/docs/ui/packets/schema.json"
STATUS_SCHEMA="$ROOT/lang/docs/ui/STATUS.schema.json"
STATUS_JSON="$ROOT/lang/docs/ui/STATUS.json"
STATUS_MD="$ROOT/lang/docs/ui/STATUS.md"

usage() {
    cat <<'EOF'
Usage: scripts/ui-verify.sh [options]

  --packet <ID>       Verify lang/docs/ui/packets/<ID>.json and its evidence
  --evidence <dir>    Override the manifest's evidence directory
  --self-test         bad_packet.json must fail, good_packet.json must pass;
                      also validates STATUS.json and the schemas
  --write-status      Regenerate STATUS.json from STATUS.md (no verification)
  --json              Print one JSON result object
  --help              Show this help
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --packet) shift; [ $# -gt 0 ] || { echo "ui-verify: --packet needs an ID" >&2; exit 2; }; PACKET="$1" ;;
        --packet=*) PACKET="${1#*=}" ;;
        --evidence) shift; [ $# -gt 0 ] || { echo "ui-verify: --evidence needs a dir" >&2; exit 2; }; EVIDENCE="$1" ;;
        --evidence=*) EVIDENCE="${1#*=}" ;;
        --self-test) SELF_TEST=1 ;;
        --write-status) WRITE_STATUS=1 ;;
        --json) JSON=1 ;;
        --help|-h) usage; exit 0 ;;
        *) echo "ui-verify: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

# Keep stdout for JSON; human logs go to stderr in --json mode.
exec 3>&1
if [ "$JSON" = "1" ]; then exec 1>&2; fi

status_write() {
    python3 - "$STATUS_MD" "$STATUS_JSON" <<'PY'
import json, re, sys
md, out = sys.argv[1], sys.argv[2]
entries = []
for line in open(md, encoding="utf-8"):
    if not line.startswith("|"):
        continue
    cells = [c.strip() for c in line.strip().strip("|").split("|")]
    if len(cells) != 7 or cells[0] in ("Packet", "--------"):
        continue
    m = re.match(r'^(P[0-9]+(?:\.[0-9]+)?[a-z]?)\s*(.*)$', cells[0])
    if not m:
        continue
    def clean(v):
        v = v.strip().strip('`')
        return None if v in ("", "—", "-") else v
    entries.append({
        "packet": m.group(1), "label": m.group(2).strip(),
        "state": cells[1], "commit": clean(cells[2]), "tag": clean(cells[3]),
        "evidence": clean(cells[4]), "bench": clean(cells[5]),
        "notes": clean(cells[6]),
    })
with open(out, "w", encoding="utf-8") as f:
    json.dump({"schema": 1, "source": "lang/docs/ui/STATUS.md", "entries": entries},
              f, separators=(",", ":"), sort_keys=True)
    f.write("\n")
print(f"ui-verify: wrote {len(entries)} STATUS entries to {out}", file=sys.stderr)
PY
}

if [ "$WRITE_STATUS" = "1" ]; then
    status_write
    exit 0
fi

verify_one() { # verify_one <manifest> [evidence-override]
    local manifest="$1" override="${2:-}"
    python3 - "$manifest" "$SCHEMA" "$ROOT" "$override" "$STATUS_JSON" "$STATUS_SCHEMA" <<'PY'
import hashlib, json, os, re, subprocess, sys

manifest_path, schema_path, root, override, status_json, status_schema = sys.argv[1:7]
errors = []

def load(path):
    try:
        return json.load(open(path, encoding="utf-8"))
    except Exception as e:
        errors.append(f"{path}: {e}")
        return None

def type_ok(v, t):
    if t == "object": return isinstance(v, dict)
    if t == "array": return isinstance(v, list)
    if t == "string": return isinstance(v, str)
    if t == "integer": return isinstance(v, int) and not isinstance(v, bool)
    if t == "number": return isinstance(v, (int, float)) and not isinstance(v, bool)
    if t == "boolean": return isinstance(v, bool)
    if t == "null": return v is None
    return True

def validate(value, schema, path="$"):
    if schema is True: return
    if schema is False:
        errors.append(f"{path}: not allowed")
        return
    if "oneOf" in schema:
        matches = 0
        for sub in schema["oneOf"]:
            before = len(errors)
            validate(value, sub, path)
            if len(errors) == before:
                matches += 1
            else:
                del errors[before:]
        if matches != 1:
            errors.append(f"{path}: expected exactly one oneOf branch, got {matches}")
        return
    if "const" in schema and value != schema["const"]:
        errors.append(f"{path}: expected {schema['const']!r}")
    if "enum" in schema and value not in schema["enum"]:
        errors.append(f"{path}: {value!r} not in {schema['enum']}")
    t = schema.get("type")
    if t is not None:
        types = t if isinstance(t, list) else [t]
        if not any(type_ok(value, x) for x in types):
            errors.append(f"{path}: expected type {t}, got {type(value).__name__}")
            return
    if isinstance(value, str):
        if "minLength" in schema and len(value) < schema["minLength"]:
            errors.append(f"{path}: too short")
        if "pattern" in schema and not re.search(schema["pattern"], value):
            errors.append(f"{path}: does not match {schema['pattern']!r}")
    if isinstance(value, dict):
        for key in schema.get("required", []):
            if key not in value:
                errors.append(f"{path}: missing required key {key!r}")
        props = schema.get("properties", {})
        for key, sub in props.items():
            if key in value:
                validate(value[key], sub, f"{path}.{key}")
        if schema.get("additionalProperties") is False:
            for key in value:
                if key not in props:
                    errors.append(f"{path}: unexpected key {key!r}")
        elif isinstance(schema.get("additionalProperties"), dict):
            for key, sub in value.items():
                if key not in props:
                    validate(sub, schema["additionalProperties"], f"{path}.{key}")
    if isinstance(value, list) and "items" in schema:
        for i, item in enumerate(value):
            validate(item, schema["items"], f"{path}[{i}]")

schema = load(schema_path)
manifest = load(manifest_path)
if schema is None or manifest is None:
    print("\n".join(errors), file=sys.stderr)
    sys.exit(1)
validate(manifest, schema)
if errors:
    print("\n".join("ui-verify: " + e for e in errors), file=sys.stderr)
    sys.exit(1)

packet = manifest["packet"]
def resolve(p):
    return p if os.path.isabs(p) else os.path.join(root, p)

for p in manifest["artifacts"]:
    if not os.path.exists(resolve(p)):
        errors.append(f"missing artifact {p}")
for cmd in manifest["commands"]:
    for p in cmd["artifacts"]:
        if not os.path.exists(resolve(p)):
            errors.append(f"missing command artifact {p} (command {cmd['id']})")

ev = manifest["evidence"]
if ev is not None:
    evdir = override or ev["dir"]
    evdir_abs = resolve(evdir)
    if ev["required"]:
        if not os.path.isdir(evdir_abs):
            errors.append(f"missing evidence dir {evdir}")
        for name in ev["files"]:
            if not os.path.isfile(os.path.join(evdir_abs, name)):
                errors.append(f"missing evidence file {os.path.join(evdir, name)}")
    if ev.get("commit"):
        try:
            subprocess.run(["git", "-C", root, "rev-parse", "--verify", ev["commit"] + "^{commit}"],
                           check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        except Exception:
            errors.append(f"evidence commit {ev['commit']} does not exist")
    for rel, expected in ev.get("hashes", {}).items():
        p = os.path.join(evdir_abs, rel)
        if not os.path.isfile(p):
            errors.append(f"hashed evidence file missing {p}")
            continue
        got = hashlib.sha256(open(p, "rb").read()).hexdigest()
        if got != expected:
            errors.append(f"hash mismatch for {p}: {got} != {expected}")

bench = manifest["benchmarks"]
if bench["compare"]:
    bdir = os.path.join(root, "lang/tests/ui_bench/baselines")
    if not os.path.isdir(bdir) or not any(f.endswith(".json") for f in os.listdir(bdir)):
        errors.append("benchmarks.compare=true but no baseline JSON exists")

if os.path.isfile(status_json):
    st = load(status_json)
    ss = load(status_schema)
    if st is not None and ss is not None:
        validate(st, ss)

if errors:
    print("\n".join("ui-verify: " + e for e in errors), file=sys.stderr)
    sys.exit(1)
print(f"PASS {packet} ({manifest['state']})", file=sys.stderr)
PY
}

status_check() {
    python3 - "$STATUS_MD" "$STATUS_JSON" "$STATUS_SCHEMA" <<'PY'
import json, re, sys
md, js, schema = sys.argv[1:4]
try:
    data = json.load(open(js, encoding="utf-8"))
except Exception as e:
    print(f"ui-verify: cannot read {js}: {e}", file=sys.stderr); sys.exit(1)
# Regenerate in-memory and compare.
entries = []
for line in open(md, encoding="utf-8"):
    if not line.startswith("|"): continue
    cells = [c.strip() for c in line.strip().strip("|").split("|")]
    if len(cells) != 7 or cells[0] in ("Packet", "--------"): continue
    m = re.match(r'^(P[0-9]+(?:\.[0-9]+)?[a-z]?)\s*(.*)$', cells[0])
    if not m: continue
    def clean(v):
        v = v.strip().strip('`'); return None if v in ("", "—", "-") else v
    entries.append({"packet": m.group(1), "label": m.group(2).strip(), "state": cells[1],
                    "commit": clean(cells[2]), "tag": clean(cells[3]), "evidence": clean(cells[4]),
                    "bench": clean(cells[5]), "notes": clean(cells[6])})
expected = {"schema": 1, "source": "lang/docs/ui/STATUS.md", "entries": entries}
if data != expected:
    print("ui-verify: STATUS.json is stale; run scripts/ui-verify.sh --write-status", file=sys.stderr)
    sys.exit(1)
print(f"ui-verify: STATUS.json matches STATUS.md ({len(entries)} entries)", file=sys.stderr)
PY
}

if [ "$SELF_TEST" = "1" ]; then
    status_check
    for schema in "$SCHEMA" "$STATUS_SCHEMA" "$ROOT/lang/docs/ui/evidence/schema.json"; do
        python3 -c 'import json,sys; json.load(open(sys.argv[1]))' "$schema" || { echo "ui-verify: invalid schema $schema" >&2; exit 1; }
    done
    bad="$ROOT/lang/tests/ui/fixtures/bad_packet.json"
    good="$ROOT/lang/tests/ui/fixtures/good_packet.json"
    set +e
    verify_one "$bad" >/dev/null 2>&1
    bad_rc=$?
    verify_one "$good" >/dev/null 2>&1
    good_rc=$?
    set -e
    if [ "$bad_rc" -eq 0 ]; then
        echo "ui-verify: self-test FAIL: bad packet was accepted" >&2; exit 1
    fi
    if [ "$good_rc" -ne 0 ]; then
        echo "ui-verify: self-test FAIL: good packet was rejected" >&2; exit 1
    fi
    echo "ui-verify: self-test PASS (bad red, good green)"
    if [ "$JSON" = "1" ]; then
        printf '{"self_test":"pass","bad_packet":"reject","good_packet":"accept","status":"pass"}\n' >&3
    fi
    exit 0
fi

if [ -z "$PACKET" ]; then
    echo "ui-verify: --packet <ID> or --self-test is required" >&2
    usage >&2
    exit 2
fi
manifest="$ROOT/lang/docs/ui/packets/$PACKET.json"
if [ ! -f "$manifest" ]; then
    echo "ui-verify: missing manifest $manifest" >&2
    exit 1
fi
set +e
verify_one "$manifest" "$EVIDENCE"
rc=$?
set -e
if [ "$rc" -ne 0 ]; then
    if [ "$JSON" = "1" ]; then
        printf '{"packet":"%s","status":"fail","exit_code":1}\n' "$PACKET" >&3
    fi
    exit 1
fi
echo "ui-verify: PASS $PACKET"
if [ "$JSON" = "1" ]; then
    printf '{"packet":"%s","status":"pass","exit_code":0}\n' "$PACKET" >&3
fi
