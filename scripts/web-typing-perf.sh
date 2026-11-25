#!/usr/bin/env bash
# Performance gate for component body typing (workflow H11 / plan §9).
#
# Measures, on the same machine:
#
#   cold build     `lang/libs/components` with --no-cache (plugin build included)
#   warm build     the same tree with the object cache warm
#   plugin build   a small fixture with -frecompile-plugins
#   bundle gz      canonical gzip -9 of the `components_corpus` --web client
#                  assets (symbol ids / numeric literals normalized; the JS
#                  backend currently numbers symbols by instantiation order,
#                  which is non-deterministic across runs)
#   index calls    IndexModule calls in the --web log
#   peak RSS       the warm build's /usr/bin/time -v maximum resident set
#
# The baseline lives in `lang/tests/web/typing-perf-baseline.json`. A baseline
# for another machine is skipped (host timing is not portable), as is a missing
# baseline. `--update-baseline` records the current machine.
#
# Budgets: warm build <= +2% (or +200 ms, whichever is larger), plugin build
# <= +10% (or +750 ms), bundle bytes = 0 growth. The absolute floor is a
# measurement-noise allowance: a 2% window on a sub-second build is below this
# host's run-to-run jitter. A real regression is much larger than the floor.
# A miss is a red build; the fix is to revert/record a new budget, never to
# weaken this gate.
#
#   ./scripts/web-typing-perf.sh
#   ./scripts/web-typing-perf.sh --update-baseline
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-typing-perf: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
    exit 1
fi
compiler="$(cd "$(dirname "$compiler")" && pwd)/$(basename "$compiler")"

baseline="lang/tests/web/typing-perf-baseline.json"
out="lang/compiled/web-typing-perf"
fixture="lang/tests/web/fixtures/components_corpus"
plugin_fixture="lang/tests/web/fixtures/body_ok_surface"
update=0
[ "${1:-}" = "--update-baseline" ] && update=1

mkdir -p "$out"

now_ms() { date +%s%N; }

# Run a command under /usr/bin/time, store the wall time in a global and its
# maximum RSS in a global. Output goes to the given log. On a host without GNU
# time the wall clock still works; RSS is reported as 0.
LAST_MS=0
LAST_RSS=0
time_bin="/usr/bin/time"
[ -x "$time_bin" ] || time_bin=""
run_timed() {
    local log="$1"; shift
    local timefile="$out/time.txt"
    local start end
    start=$(now_ms)
    if [ -n "$time_bin" ]; then
        "$time_bin" -v -o "$timefile" "$@" > "$log" 2>&1
    else
        "$@" > "$log" 2>&1
    fi
    local status=$?
    end=$(now_ms)
    LAST_MS=$(( (end - start) / 1000000 ))
    if [ -n "$time_bin" ]; then
        LAST_RSS=$(awk '/Maximum resident set size/ {print $NF}' "$timefile" 2>/dev/null || echo 0)
    else
        LAST_RSS=0
    fi
    return $status
}

fail=0
check() {
    if [ "$2" = "1" ]; then
        echo "ok   $1"
    else
        echo "FAIL $1" >&2
        fail=1
    fi
}

# ---------------------------------------------------------------- cold/warm build
echo "== lang/libs/components build"
cold_build="$out/cold_build"
rm -rf "$cold_build"
cold_log="$out/cold.log"
if run_timed "$cold_log" "$compiler" lang/libs/components/chemical.mod \
        --build-dir "$cold_build" -o "$out/components.a" --no-cache; then
    check "cold build succeeds" 1
else
    check "cold build succeeds (see $cold_log)" 0
    tail -20 "$cold_log" >&2
fi
cold_ms=$LAST_MS

# A single warm run is noisy on a shared host. Take the best of five; the
# budget is a regression guard, not a benchmark contest.
warm_ms=0
warm_rss=0
for run in 1 2 3 4 5; do
    warm_log="$out/warm_$run.log"
    if run_timed "$warm_log" "$compiler" lang/libs/components/chemical.mod \
            --build-dir "$cold_build" -o "$out/components.a"; then
        check "warm build $run succeeds" 1
    else
        check "warm build $run succeeds (see $warm_log)" 0
        tail -20 "$warm_log" >&2
    fi
    if [ "$warm_ms" = "0" ] || [ "$LAST_MS" -lt "$warm_ms" ]; then
        warm_ms=$LAST_MS
        warm_rss=$LAST_RSS
    fi
done

# ---------------------------------------------------------------- plugin build
echo
echo "== universal_cbi plugin rebuild"
plugin_build="$out/plugin_build"
rm -rf "$plugin_build"
plugin_log="$out/plugin.log"
if run_timed "$plugin_log" "$compiler" "$plugin_fixture/chemical.mod" \
        --web --build-dir "$plugin_build" -o "$out/plugin.exe" \
        -frecompile-plugins --no-cache; then
    check "plugin rebuild succeeds" 1
else
    check "plugin rebuild succeeds (see $plugin_log)" 0
    tail -20 "$plugin_log" >&2
fi
plugin_ms=$LAST_MS

# ---------------------------------------------------------------- bundle + index
echo
echo "== client bundle and index work"
web_build="$out/web_build"
web_log="$out/web.log"
if CHEM_DEBUG_INDEX=1 run_timed "$web_log" "$compiler" "$fixture/chemical.mod" --web \
        --build-dir "$web_build" -o "$out/web.exe" --no-cache; then
    check "components_corpus --web build succeeds" 1
else
    check "components_corpus --web build succeeds (see $web_log)" 0
    tail -20 "$web_log" >&2
fi
# Canonical size: normalize the generated symbol ids (`$f123`, `$k123`,
# `$g123`, `$x123`, `__chx__vt_123`) and numeric constant lines, then sort the
# lines before gzip. The JS backend's symbol numbering is instantiation-order
# dependent, so a raw byte count is not stable; the canonical size still grows
# for any real content (strings, functions, code) the feature adds.
assets="$web_build/web-client/assets"
bundle_bytes=0
if [ -d "$assets" ]; then
    bundle_bytes=$(find "$assets" -type f -name '*.mjs' -print0 2>/dev/null \
        | sort -z | xargs -0 -r cat 2>/dev/null \
        | sed -E 's/\$f[0-9]+/$f/g; s/\$k[0-9]+/$k/g; s/\$g[0-9]+/$g/g; s/\$x[0-9]+/$x/g; s/__chx__vt_[0-9]+/__chx__vt_/g' \
        | sed -E 's/^([[:space:]]*)v[0-9]+ = [-+]?[0-9][0-9a-fA-F.eE+-]*;$/\1v = NUM;/' \
        | LC_ALL=C sort | gzip -9 -c | wc -c | tr -d ' ')
fi
index_calls=$(grep -cF '[index] universal_index_module' "$web_log" 2>/dev/null || true)
index_calls=${index_calls:-0}

echo "cold build: ${cold_ms} ms"
echo "warm build: ${warm_ms} ms"
echo "plugin rebuild: ${plugin_ms} ms"
echo "bundle gz: ${bundle_bytes} bytes"
echo "index calls: ${index_calls}"
echo "warm peak RSS: ${warm_rss} kB"

# ---------------------------------------------------------------- baseline / budgets
echo
machine="$(hostname 2>/dev/null || echo unknown)"
if [ "$update" = "1" ]; then
    python3 - "$baseline" "$machine" "$cold_ms" "$warm_ms" "$plugin_ms" "$bundle_bytes" "$index_calls" "$warm_rss" <<'PY'
import json, sys
path, machine, cold, warm, plugin, bundle, calls, rss = sys.argv[1:]
data = {
    "machine": machine,
    "generated": "2026-11-25",
    "cold_ms": int(cold),
    "build_ms": int(warm),
    "plugin_ms": int(plugin),
    "bundle_gz_bytes": int(bundle),
    "index_calls": int(calls),
    "max_rss_kb": int(rss),
}
with open(path, "w", encoding="utf-8") as f:
    json.dump(data, f, indent=2, sort_keys=True)
    f.write("\n")
print(f"wrote {path} for machine {machine}")
PY
    if [ "$fail" != 0 ]; then
        echo "web-typing-perf: FAILED" >&2
        exit 1
    fi
    rm -rf "$out"
    exit 0
fi

if [ ! -f "$baseline" ]; then
    echo "web-typing-perf: no baseline at $baseline; measured only (run with --update-baseline)"
    rm -rf "$out"
    exit 0
fi

if ! python3 - "$baseline" "$machine" "$cold_ms" "$warm_ms" "$plugin_ms" "$bundle_bytes" "$index_calls" "$warm_rss" <<'PY'
import json, sys
path = sys.argv[1]
machine = sys.argv[2]
cold, warm, plugin, bundle, calls, rss = map(int, sys.argv[3:])
with open(path, encoding="utf-8") as f:
    base = json.load(f)
if base.get("machine") != machine:
    print(f"skip: baseline machine '{base.get('machine')}' != this host '{machine}'")
    sys.exit(0)
failed = False
def budget(name, now, was, pct, floor_ms):
    global failed
    limit = max(was * (1 + pct / 100.0), was + floor_ms)
    if now > limit:
        print(f"FAIL {name}: {now} > {limit:.0f} (baseline {was}, budget +{pct}% or +{floor_ms} ms)")
        failed = True
    else:
        print(f"ok   {name}: {now} (baseline {was}, budget +{pct}% or +{floor_ms} ms)")
budget("warm build ms", warm, base["build_ms"], 2, 200)
budget("plugin build ms", plugin, base["plugin_ms"], 10, 750)
if bundle > base["bundle_gz_bytes"]:
    print(f"FAIL bundle gz: {bundle} > {base['bundle_gz_bytes']} (baseline; budget 0 bytes)")
    failed = True
else:
    print(f"ok   bundle gz: {bundle} <= {base['bundle_gz_bytes']} (budget 0 bytes)")
print(f"info cold build {cold} ms (baseline {base.get('cold_ms')})")
print(f"info index calls {calls}, warm peak RSS {rss} kB (baseline {base.get('index_calls')}, {base.get('max_rss_kb')} kB)")
sys.exit(1 if failed else 0)
PY
then
    fail=1
fi

if [ "$fail" = 0 ]; then
    rm -rf "$out"
else
    echo "web-typing-perf: artifacts kept in $out" >&2
fi

if [ "$fail" != 0 ]; then
    echo "web-typing-perf: FAILED" >&2
    exit 1
fi
echo "web-typing-perf: within budget (build +2%, plugin +10%, bundle 0 bytes)"
