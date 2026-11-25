#!/usr/bin/env bash
# Component corpus typing gate (component body typing, Phase 5b).
#
# Two things are proved here:
#
#   1. Typed rules are strict over `lang/libs/components`: the client and server
#      passes of a `--web` app that imports the library compile all 196 component
#      bodies with zero WEB-BODY-001/002/006/008 diagnostics. A regression is a
#      red build, not a count.
#
#   2. WEB-BODY-005 (untyped edge) does not grow silently. The per-component
#      counts are pinned in `lang/tests/web/component-body-baseline.txt`; any
#      component above its baseline fails. New components must enter at 0 or be
#      added to the baseline deliberately (the corpus is not "005 strict" yet:
#      the remaining edges are genuine untyped locals/params/refs).
#
# It also proves a plain native build of `components` (no `--web`) still works
# and runs the same checker: `components` imports `dom` for its declarations
# only, the index is built, and no typed diagnostic appears there either.
#
#   ./scripts/web-component-typing-test.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-component-typing-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
    exit 1
fi
compiler="$(cd "$(dirname "$compiler")" && pwd)/$(basename "$compiler")"

baseline="lang/tests/web/component-body-baseline.txt"
fixture="lang/tests/web/fixtures/components_corpus"
out="lang/compiled/web-component-typing-gate"

fail=0
check() {
    if [ "$2" = "1" ]; then
        echo "ok   $1"
    else
        echo "FAIL $1" >&2
        fail=1
    fi
}
has() { grep -qF -- "$2" "$1" 2>/dev/null && echo 1 || echo 0; }
count() { grep -cF -- "$2" "$1" 2>/dev/null || true; }
typed_count() {
    local file="$1"
    echo $(( $(count "$file" 'WEB-BODY-001') + $(count "$file" 'WEB-BODY-002') \
        + $(count "$file" 'WEB-BODY-006') + $(count "$file" 'WEB-BODY-008') ))
}

rm -rf "$out"
mkdir -p "$out"

# ---------------------------------------------------------------- native consumer
echo "== native consumer: dom is declarations-only, the same checker runs"
native_log="$out/native.log"
native_build="$out/native_build"
if CHEM_DEBUG_BODY=1 "$compiler" lang/libs/components/chemical.mod --build-dir "$native_build" \
        -o "$out/components.a" --no-cache > "$native_log" 2>&1; then
    check "a native build of lang/libs/components succeeds" 1
else
    check "a native build of lang/libs/components succeeds (see $native_log)" 0
    tail -30 "$native_log" >&2
fi
check "the native build checks every component" \
      "$([ "$(count "$native_log" '[body] component=')" -ge 196 ] && echo 1 || echo 0)"
check "the native build has no WEB-BODY-001/002/006/008" \
      "$([ "$(typed_count "$native_log")" = "0" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- both web sides
echo
echo "== both --web sides: strict typed rules over the corpus"
web_log="$out/components_web.log"
web_build="$out/components_web_build"
web_out="$out/components_web.exe"
if CHEM_DEBUG_INDEX=1 CHEM_DEBUG_BODY=1 "$compiler" "$fixture/chemical.mod" --web -v \
        --build-dir "$web_build" -o "$web_out" --no-cache > "$web_log" 2>&1; then
    check "the components_corpus --web fixture builds" 1
else
    check "the components_corpus --web fixture builds (see $web_log)" 0
    tail -30 "$web_log" >&2
fi
check "the build completed both sides" "$(has "$web_log" 'web build complete')"
check "the server job indexed dom" \
      "$(grep -qE 'universal_index_module module=dom ' "$web_log" 2>/dev/null && echo 1 || echo 0)"
check "no WEB-BODY-001/002/006/008 on either side" \
      "$([ "$(typed_count "$web_log")" = "0" ] && echo 1 || echo 0)"
if [ "$(typed_count "$web_log")" != "0" ]; then
    grep -E 'WEB-BODY-(001|002|006|008)' "$web_log" | head -20 >&2
fi

client_count=$(awk '/\[lab\] web build: client side/{s="c"} /\[lab\] web build: server side/{s="s"} s=="c" && /\[body\] component=/{n++} END{print n+0}' "$web_log")
server_count=$(awk '/\[lab\] web build: server side/{s="s"} s=="s" && /\[body\] component=/{n++} END{print n+0}' "$web_log")
check "the client pass checked every component" "$([ "${client_count:-0}" -ge 196 ] && echo 1 || echo 0)"
check "the server pass checked every component" "$([ "${server_count:-0}" -ge 196 ] && echo 1 || echo 0)"
echo "client components: ${client_count:-0}  server components: ${server_count:-0}"

awk '
    /\[lab\] web build: client side/ { s = "client"; next }
    /\[lab\] web build: server side/ { s = "server"; next }
    s == "client" && /\[body\] component=/ { print }
' "$web_log" | sed -E 's/.*component=([^ ]+) untyped=([0-9]+)/\1 \2/' | sort > "$out/measured.txt"
echo "measured components: $(wc -l < "$out/measured.txt")  total 005: $(awk '{s+=$2} END {print s+0}' "$out/measured.txt")"

if [ ! -f "$baseline" ]; then
    echo "FAIL missing baseline $baseline" >&2
    fail=1
else
    python3 - "$baseline" "$out/measured.txt" <<'PY'
import sys

baseline_path, measured_path = sys.argv[1], sys.argv[2]

def load(path):
    out = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            name, value = line.split()
            out[name] = int(value)
    return out

baseline = load(baseline_path)
measured = load(measured_path)

failed = False
for name in sorted(measured):
    now = measured[name]
    was = baseline.get(name)
    if was is None:
        if now > 0:
            print(f"FAIL new component '{name}' introduces {now} untyped edge(s); add it to the baseline deliberately")
            failed = True
        continue
    if now > was:
        print(f"FAIL component '{name}' untyped edges grew: {was} -> {now}")
        failed = True

removed = sorted(set(baseline) - set(measured))
if removed:
    print("note: baseline components no longer measured: " + ", ".join(removed))

if failed:
    sys.exit(1)
print(f"ok   WEB-BODY-005 baseline holds for {len(measured)} components")
PY
    check "the WEB-BODY-005 baseline does not grow" "$([ $? = 0 ] && echo 1 || echo 0)"
fi

# ---------------------------------------------------------------- cleanup
rm -rf "$fixture/build" "$fixture/client/build" "$fixture/shared/build" "$fixture/src/build"
if [ "$fail" = 0 ]; then
    rm -rf "$out"
else
    echo "web-component-typing-test: artifacts kept in $out" >&2
fi

if [ "$fail" != 0 ]; then
    echo "web-component-typing-test: FAILED" >&2
    exit 1
fi
echo "web-component-typing-test: typed rules strict, 005 baseline holds, both sides checked"
