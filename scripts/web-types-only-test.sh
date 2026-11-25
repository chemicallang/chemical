#!/usr/bin/env bash
# Phase 4b gate: declarations-only host modules (`option host = "js"`).
#
#   host_types_only   a `--web` app whose server module names `*dom::Document` in
#                    a signature but never calls a wrapper. `dom` imports into
#                    the native server job, where it is included for its
#                    declarations only: the build succeeds, the server binary
#                    carries no `chx_dom_*` symbol (and no dom object), and the
#                    client pass still emits dom normally.
#   dom_in_server    the negative half: an actual server call into a `dom`
#                    wrapper must be refused at the CALLER (`module server`,
#                    `server/routes.ch`) with the client-only reason, not with
#                    an unresolved symbol at link time.
#
#   ./scripts/web-types-only-test.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-types-only-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
    exit 1
fi
compiler="$(cd "$(dirname "$compiler")" && pwd)/$(basename "$compiler")"

if ! command -v nm >/dev/null 2>&1; then
    echo "web-types-only-test: 'nm' is required to inspect the server binary" >&2
    exit 1
fi

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
has_re() { grep -qE -- "$2" "$1" 2>/dev/null && echo 1 || echo 0; }
count() { grep -cF -- "$2" "$1" 2>/dev/null || true; }
bundle_has() {
    local dir="$1" needle="$2"
    grep -rqF --include='*.mjs' -- "$needle" "$dir" 2>/dev/null && echo 1 || echo 0
}

fixture="lang/tests/web/fixtures/host_types_only"
native_fixture="lang/tests/web/fixtures/host_types_only_native"
negative="lang/tests/web/fixtures/dom_in_server"
out="lang/compiled/web-types-only-gate"

cleanup() {
    rm -rf "$fixture/build" "$fixture/client/build" "$fixture/src/build"
    rm -rf "$native_fixture/build" "$native_fixture/src/build"
    rm -rf "$negative/build" "$negative/client/build" "$negative/server/build" "$negative/src/build"
    if [ "$fail" = 0 ]; then
        rm -rf "$out"
    else
        echo "web-types-only-test: artifacts kept in $out" >&2
    fi
}
trap cleanup EXIT

rm -rf "$out"
mkdir -p "$out"

# ---------------------------------------------------------------- positive
echo "== positive: a server job names dom types, no dom implementation"
build_dir="$out/positive_build"
log="$out/positive.log"
if "$compiler" "$fixture/chemical.mod" --web -v \
        --build-dir "$build_dir" -o "$out/positive.exe" > "$log" 2>&1; then
    check "the two-job build succeeds with dom declarations-only on the server" 1
else
    check "the two-job build succeeds with dom declarations-only on the server (see $log)" 0
    tail -30 "$log" >&2
fi
check "the build reports both sides" "$(has "$log" 'web build complete')"
check "the server job marks dom declarations-only" \
      "$(has "$log" "module 'dom' declares host 'js' and is declarations-only on this job")"
check "the server executable was linked" \
      "$([ -x "$out/positive.exe" ] && echo 1 || echo 0)"
check "the server executable runs" \
      "$("$out/positive.exe" > /dev/null 2>&1 && echo 1 || echo 0)"
check "the types-only identity is recorded" \
      "$(has "$build_dir/web-server/types_only.identity" 'dom')"

# The server binary must not carry any host implementation or wrapper.
check "the server binary has no chx_dom_* symbol" \
      "$([ "$(nm -a "$out/positive.exe" 2>/dev/null | grep -c 'chx_dom_')" = "0" ] && echo 1 || echo 0)"
check "the server binary has no dom wrapper symbol" \
      "$([ "$(nm -a "$out/positive.exe" 2>/dev/null | grep -c 'dom_dom')" = "0" ] && echo 1 || echo 0)"
check "the server job emits no dom object" \
      "$([ "$(find "$build_dir/web-server" \( -name 'dom.o' -o -name 'dom.bc' -o -name 'dom.obj' \) 2>/dev/null | wc -l)" = "0" ] && echo 1 || echo 0)"

# The client side is a JS job (it implements host "js") and emits dom as usual.
assets="$build_dir/web-client/assets"
check "the client job emits the dom module" \
      "$([ -f "$assets/dom.mjs" ] && echo 1 || echo 0)"
check "the client bundle maps the dom host calls to the JS runtime" \
      "$([ "$(bundle_has "$assets" 'rt.dom.get_element_by_id')" = "1" ] && [ "$(bundle_has "$assets" 'rt.dom.document')" = "1" ] && echo 1 || echo 0)"
check "the client bundle carries no raw chx_dom_ symbol" \
      "$([ "$(bundle_has "$assets" 'chx_dom_')" = "0" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- plain native
# No `--web`: an ordinary native build that imports dom must not break, and it
# exercises the per-module (non-single-file) object path too.
echo
echo "== plain native: import dom for its types only"
native_build="$out/native_build"
native_log="$out/native.log"
if "$compiler" "$native_fixture/chemical.mod" -v \
        --build-dir "$native_build" -o "$out/native.exe" > "$native_log" 2>&1; then
    check "a plain native build with dom declarations-only succeeds" 1
else
    check "a plain native build with dom declarations-only succeeds (see $native_log)" 0
    tail -20 "$native_log" >&2
fi
check "the native job marks dom declarations-only" \
      "$(has "$native_log" "module 'dom' declares host 'js' and is declarations-only on this job")"
check "the native executable runs" \
      "$("$out/native.exe" > /dev/null 2>&1 && echo 1 || echo 0)"
check "the native binary has no chx_dom_* symbol" \
      "$([ "$(nm -a "$out/native.exe" 2>/dev/null | grep -c 'chx_dom_')" = "0" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- cache bypass
# A declarations-only module emits declarations, not a cached object/partial C.
# Put a poisoned stale artifact where a cache would look and force the job to
# reprocess: the build must ignore it (and stay green).
echo
echo "== cache: a stale declarations-only artifact is never reused"
cache_fixture="$out/cache_fixture"
cp -r "$fixture" "$cache_fixture"
rm -rf "$cache_fixture/build" "$cache_fixture/client/build" "$cache_fixture/src/build"
cache_build="$out/cache_build"
cache_log1="$out/cache_first.log"
if "$compiler" "$cache_fixture/chemical.mod" --web \
        --build-dir "$cache_build" -o "$out/cache.exe" > "$cache_log1" 2>&1; then
    check "the cache fixture builds once" 1
else
    check "the cache fixture builds once (see $cache_log1)" 0
    tail -20 "$cache_log1" >&2
fi
mkdir -p "$cache_build/web-server/modules/dom"
printf '#error stale types-only cache must not be reused\n' \
    > "$cache_build/web-server/modules/dom/partial.2c.c"
printf 'int stale_types_only_object = 1;\n' \
    > "$cache_build/web-server/modules/dom/object_tcc.o"
# Force a module change so the job cannot take the whole-job reuse shortcut.
printf '\n// force reprocessing\n' >> "$cache_fixture/src/main.ch"
cache_log2="$out/cache_second.log"
if "$compiler" "$cache_fixture/chemical.mod" --web \
        --build-dir "$cache_build" -o "$out/cache.exe" > "$cache_log2" 2>&1; then
    check "a rebuild ignores the stale declarations-only artifact" 1
else
    check "a rebuild ignores the stale declarations-only artifact (see $cache_log2)" 0
    tail -20 "$cache_log2" >&2
fi
check "the rebuild still has no chx_dom_* symbol" \
      "$([ "$(nm -a "$out/cache.exe" 2>/dev/null | grep -c 'chx_dom_')" = "0" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- negative
echo
echo "== negative: a server call into a dom wrapper is refused at the caller"
neg_build="$out/negative_build"
neg_log="$out/negative.log"
if "$compiler" "$negative/chemical.mod" --web -v \
        --build-dir "$neg_build" -o "$out/negative.exe" > "$neg_log" 2>&1; then
    check "a server call into dom is rejected (it built)" 0
else
    check "a server call into dom is rejected" 1
fi
check "the diagnostic names the client-only reason" "$(has "$neg_log" 'client-only')"
check "the diagnostic names the caller module" \
      "$(has "$neg_log" 'failure during translation to c in the module server')"
check "the diagnostic names the caller source" "$(has "$neg_log" 'server/routes.ch')"
check "the diagnostic says where to move the call" \
      "$(has "$neg_log" "call it from a 'client' module")"
check "the refusal is not attributed to the dom module" \
      "$([ "$(count "$neg_log" 'failure during translation to c in the module dom')" = "0" ] && echo 1 || echo 0)"
check "no link error is reached" \
      "$([ "$(count "$neg_log" 'unresolved reference')" = "0" ] && echo 1 || echo 0)"
check "no server executable was linked" \
      "$([ -x "$out/negative.exe" ] && echo 0 || echo 1)"

if [ "$fail" -ne 0 ]; then
    echo "web-types-only-test: FAILED" >&2
    exit 1
fi
echo "web-types-only-test: host declarations-only modules build without host symbols"
