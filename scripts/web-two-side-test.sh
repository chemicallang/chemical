#!/usr/bin/env bash
# Two-job gate for the universal ambient index (component body typing, Phase 2).
#
#   index_two_side    a `--web` app whose client-only module declares a
#                     `#universal` component that reads `document` inside a
#                     lambda (so the render-position hydration check is exempt).
#                     The client-only `provider` module is marked
#                     `index_with "universal"` and supplies `document()`. The
#                     client job indexes it and passes; the server job neither
#                     imports nor indexes it and skips the check.
#   no provider       a copy without the provider import: the client job fails
#                     with WEB-BODY-008 naming `document`; the server side is not
#                     built.
#   native only       the negative tree without `--web`: no client/JS job exists,
#                     so the check is skipped and the build succeeds.
#   isolation         the client index is empty at job_begin, holds one ambient
#                     root after the provider is indexed, and the next job starts
#                     empty again (asserted from the debug lines).
#
# The component is client-only because `--web` reuses a plugin's component
# symbols across its two passes (a pre-existing two-pass limitation outside this
# phase). Keeping it in the client graph is also the real shape: the client is
# the side that resolves browser globals.
#
#   ./scripts/web-two-side-test.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-two-side-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
    exit 1
fi
compiler="$(cd "$(dirname "$compiler")" && pwd)/$(basename "$compiler")"

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

# Split a `--web` log into the client and server passes. The compiler prints its
# side markers before running either job, so every job_begin/index line sits in
# the matching section.
split_sides() {
    local log="$1" client="$2" server="$3"
    rm -f "$client" "$server"
    : > "$client"
    : > "$server"
    awk -v client="$client" -v server="$server" '
        /\[lab\] web build: client side/ { side = "client"; next }
        /\[lab\] web build: server side/ { side = "server"; next }
        side == "client" { print >> client }
        side == "server" { print >> server }
    ' "$log"
}

fixture="lang/tests/web/fixtures/index_two_side"
out="lang/compiled/web-two-side-gate"
rm -rf "$out"
mkdir -p "$out"

# ---------------------------------------------------------------- positive
echo "== positive: client indexes the provider, server does not"
pos_build="$out/positive_build"
pos_log="$out/positive.log"
if CHEM_DEBUG_INDEX=1 "$compiler" "$fixture/chemical.mod" --web -v \
        --build-dir "$pos_build" -o "$out/positive.exe" > "$pos_log" 2>&1; then
    check "the two-job build succeeds" 1
else
    check "the two-job build succeeds (see $pos_log)" 0
    tail -30 "$pos_log" >&2
fi
check "the build reports both sides" "$(has "$pos_log" 'web build complete')"
check "the client bundle was emitted" \
      "$([ -f "$pos_build/web-client/assets/index.mjs" ] && echo 1 || echo 0)"
check "the server executable was linked" \
      "$([ -x "$out/positive.exe" ] && echo 1 || echo 0)"

pos_client="$out/positive.client.log"
pos_server="$out/positive.server.log"
split_sides "$pos_log" "$pos_client" "$pos_server"

check "the client job is the client/JS side (client=1 js=1)" \
      "$(has_re "$pos_client" 'universal_job_begin job=web-client .*client=1 js=1')"
check "the server job is not the client side (client=0 js=0)" \
      "$(has_re "$pos_server" 'universal_job_begin .*client=0 js=0')"
check "the client indexes the provider module exactly once" \
      "$([ "$(count "$pos_client" 'universal_index_module module=index_provider ')" = "1" ] && echo 1 || echo 0)"
check "the server job never indexes the provider" \
      "$([ "$(count "$pos_server" 'universal_index_module module=index_provider')" = "0" ] && echo 1 || echo 0)"
check "the positive client build has no WEB-BODY-008" \
      "$([ "$(count "$pos_client" 'WEB-BODY-008')" = "0" ] && echo 1 || echo 0)"
check "the positive server build has no WEB-BODY-008" \
      "$([ "$(count "$pos_server" 'WEB-BODY-008')" = "0" ] && echo 1 || echo 0)"

# Index isolation: clear at job_begin, one root after the provider is indexed,
# and the next job (server) starts from an empty index again.
check "the client index is empty at job_begin" \
      "$(has_re "$pos_client" 'universal_job_begin job=web-client .*ambient=0')"
check "the provider adds one ambient root" \
      "$(has_re "$pos_client" 'universal_index_module module=index_provider .*ambient=1')"
check "the next job starts with an empty index" \
      "$(has_re "$pos_server" 'universal_job_begin .*ambient=0')"

# ---------------------------------------------------------------- negative
echo
echo "== negative: no provider import fails the client job with 008 only"
neg="$out/negative"
cp -r "$fixture" "$neg"
rm -rf "$neg/build" "$neg/provider/build" "$neg/client/build" "$neg/src/build"
# Drop the client-only provider import: the component's `document` is now
# provided by nothing.
sed -i '/import "\.\/provider" if client/d' "$neg/chemical.mod"

neg_build="$out/negative_build"
neg_log="$out/negative.log"
if CHEM_DEBUG_INDEX=1 "$compiler" "$neg/chemical.mod" --web -v \
        --build-dir "$neg_build" -o "$out/negative.exe" > "$neg_log" 2>&1; then
    check "the provider-less client build is rejected (it built)" 0
else
    check "the provider-less client build is rejected" 1
fi
check "the diagnostic names the rule" "$(has "$neg_log" 'WEB-BODY-008')"
check "the diagnostic names the component" "$(has "$neg_log" "component 'DocProbe'")"
check "the diagnostic names the missing host global" "$(has "$neg_log" "uses 'document'")"
check "the diagnostic says how to fix it" "$(has "$neg_log" 'import a provider')"
check "the diagnostic is reported once for the component+name" \
      "$([ "$(count "$neg_log" 'WEB-BODY-008')" = "1" ] && echo 1 || echo 0)"
check "the failure is attributed to the client pass" \
      "$(has "$neg_log" 'web build: the client side failed; the server side was not built')"
check "no server job ran after the client failure" \
      "$([ "$(count "$neg_log" 'web build: server side')" = "0" ] && echo 1 || echo 0)"

neg_client="$out/negative.client.log"
neg_server="$out/negative.server.log"
split_sides "$neg_log" "$neg_client" "$neg_server"
check "the 008 is in the client section" "$(has "$neg_client" 'WEB-BODY-008')"
check "the server section has no 008" \
      "$([ "$(count "$neg_server" 'WEB-BODY-008')" = "0" ] && echo 1 || echo 0)"
check "the server section has no provider index line" \
      "$([ "$(count "$neg_server" 'universal_index_module module=index_provider')" = "0" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- native only
echo
echo "== native only: the check is skipped without a client/JS job"
native_log="$out/native.log"
if CHEM_DEBUG_INDEX=1 "$compiler" "$neg/chemical.mod" \
        --build-dir "$out/native_build" -o "$out/native.exe" > "$native_log" 2>&1; then
    check "the native-only build succeeds" 1
else
    check "the native-only build succeeds (see $native_log)" 0
    tail -30 "$native_log" >&2
fi
check "the native job is not the client side" \
      "$(has_re "$native_log" 'universal_job_begin .*client=0 js=0')"
check "the native build reports no WEB-BODY-008" \
      "$([ "$(count "$native_log" 'WEB-BODY-008')" = "0" ] && echo 1 || echo 0)"
check "the native executable was produced" \
      "$([ -x "$out/native.exe" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- cleanup
rm -rf "$fixture/build" "$fixture/provider/build" "$fixture/client/build" "$fixture/src/build"
if [ "$fail" = 0 ]; then
    rm -rf "$out"
else
    echo "web-two-side-test: artifacts kept in $out" >&2
fi

if [ "$fail" != 0 ]; then
    echo "web-two-side-test: FAILED" >&2
    exit 1
fi
echo "web-two-side-test: the client indexes providers, the server skips the check"
