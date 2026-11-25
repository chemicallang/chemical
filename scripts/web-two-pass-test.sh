#!/usr/bin/env bash
# Two-pass gate for a SHARED `#universal` component (component body typing, H31).
#
#   shared_component   a `--web` app whose `shared` module (compiled by BOTH
#                      passes) declares a `#universal` component and renders it
#                      through `#html`. The server pass must resolve the
#                      component and emit its SSR markup; the client pass must
#                      body-check it and put its hydration JS in the bundle.
#
# The old Phase 2 checkpoint kept the component client-only, which dodged the
# two-pass bug instead of fixing it. This fixture is the real shape, and it is
# also the `import dom if client` shape of the real components library.
#
# What this gate proves:
#   * the CBI job is reused across passes (the `c4baaa1e4` guard) while its
#     `component` marker definition survives the first pass's JIT state;
#   * the server pass sees the component as a component (no
#     `symbol is not a component`) and renders it;
#   * the client pass still runs the Phase 2/3 body check on the shared
#     component (a bad prop is WEB-BODY-001 on the client only);
#   * the client bundle contains the component's emitted function.
#
#   ./scripts/web-two-pass-test.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-two-pass-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
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

# Split a `--web` log into the client and server passes (the compiler prints its
# side markers before running either job).
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

fixture="lang/tests/web/fixtures/shared_component"
out="lang/compiled/web-two-pass-gate"

clean_fixture_artifacts() {
    rm -rf "$fixture/build" "$fixture/output" \
        "$fixture/shared/build" "$fixture/client/build" \
        "$fixture/provider/build" "$fixture/src/build"
}
cleanup() {
    clean_fixture_artifacts
    if [ "$fail" = 0 ]; then
        rm -rf "$out"
    else
        echo "web-two-pass-test: artifacts kept in $out" >&2
    fi
}
trap cleanup EXIT

rm -rf "$out"
mkdir -p "$out"

# ---------------------------------------------------------------- positive
echo "== positive: shared component builds both sides, server renders it, client checks it"
pos_build="$out/positive_build"
pos_log="$out/positive.log"
if CHEM_DEBUG_INDEX=1 "$compiler" "$fixture/chemical.mod" --web -v \
        --build-dir "$pos_build" -o "$out/positive.exe" > "$pos_log" 2>&1; then
    check "the shared-component two-pass build succeeds" 1
else
    check "the shared-component two-pass build succeeds (see $pos_log)" 0
    tail -30 "$pos_log" >&2
fi
check "the build reports both sides" "$(has "$pos_log" 'web build complete')"
check "the build never reports 'symbol is not a component'" \
      "$([ "$(count "$pos_log" 'symbol is not a component')" = "0" ] && echo 1 || echo 0)"
check "the build has no unknown-annotation storm" \
      "$([ "$(count "$pos_log" 'unknown annotation')" = "0" ] && echo 1 || echo 0)"
check "the universal CBI job is built once across both passes" \
      "$([ "$(count "$pos_log" "Building CBI 'universal'")" = "1" ] && echo 1 || echo 0)"
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
check "the client indexes the provider exactly once" \
      "$([ "$(count "$pos_client" 'universal_index_module module=shared_component_provider ')" = "1" ] && echo 1 || echo 0)"
check "the server indexes the provider exactly once (imported on both sides)" \
      "$([ "$(count "$pos_server" 'universal_index_module module=shared_component_provider ')" = "1" ] && echo 1 || echo 0)"

# The body check runs on both jobs when the provider is present on both sides.
check "the client body-checks the shared component" \
      "$(has "$pos_client" '[body] component=SharedBadge')"
check "the server body-checks the shared component (provider present)" \
      "$(has "$pos_server" '[body] component=SharedBadge')"
check "the positive build has no WEB-BODY diagnostic" \
      "$([ "$(count "$pos_log" 'WEB-BODY')" = "0" ] && echo 1 || echo 0)"

# The server really resolved and rendered the component (not just compiled it).
server_run="$out/server.run.log"
if ( cd "$fixture" && "$root/$out/positive.exe" ) > "$server_run" 2>&1; then
    check "the server executable runs" 1
else
    check "the server executable runs (see $server_run)" 0
    tail -20 "$server_run" >&2
fi
server_html="$fixture/output/index.html"
check "the server emitted the shared component's markup" \
      "$(has "$server_html" '<span class="shared-badge"')"
check "the server rendered the shared component's prop" \
      "$(has "$server_html" 'from-shared')"

# The client bundle contains the component function (the hydration body).
check "the client bundle contains the shared component" \
      "$(grep -Rqs 'SharedBadge' "$pos_build/web-client/assets" && echo 1 || echo 0)"

# ---------------------------------------------------------------- negative
echo
echo "== negative: the client body-checks the shared component (bad prop)"
neg="$out/negative"
cp -r "$fixture" "$neg"
rm -rf "$neg/build" "$neg/output" "$neg/shared/build" "$neg/client/build" "$neg/provider/build" "$neg/src/build"
# A declared prop is read with a typo: WEB-BODY-001 must fire on the client pass.
sed -i 's/{props.label}/{props.labelz}/' "$neg/shared/badge.ch"

neg_log="$out/negative.log"
if CHEM_DEBUG_INDEX=1 "$compiler" "$neg/chemical.mod" --web -v \
        --build-dir "$out/negative_build" -o "$out/negative.exe" > "$neg_log" 2>&1; then
    check "the typo'd shared component is rejected (it built)" 0
else
    check "the typo'd shared component is rejected" 1
fi
check "the diagnostic is WEB-BODY-001" "$(has "$neg_log" 'WEB-BODY-001')"
check "the diagnostic names the shared component" \
      "$(has "$neg_log" "component 'SharedBadge'")"
check "the failure is attributed to the client pass" \
      "$(has "$neg_log" 'web build: the client side failed; the server side was not built')"
check "no server job ran after the client failure" \
      "$([ "$(count "$neg_log" 'web build: server side')" = "0" ] && echo 1 || echo 0)"

neg_client="$out/negative.client.log"
neg_server="$out/negative.server.log"
split_sides "$neg_log" "$neg_client" "$neg_server"
check "the 001 is in the client section" "$(has "$neg_client" 'WEB-BODY-001')"
check "the server section has no 001" \
      "$([ "$(count "$neg_server" 'WEB-BODY-001')" = "0" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- server-side check
# The server pass runs the same body check when a provider is present. A native
# build of the same shared component (server-shaped: `src` included, provider
# imported) proves the non-client branch reports the same typo, with
# `client=0 js=0`.
echo
echo "== negative: the server/native job body-checks too (provider present)"
neg_native="$out/negative_native"
cp -r "$neg" "$neg_native"
rm -rf "$neg_native/build" "$neg_native/output" "$neg_native/shared/build" \
       "$neg_native/client/build" "$neg_native/provider/build" "$neg_native/src/build"
# `src` is the server entry; include it without `--web` for this native job.
sed -i 's/^source "src" if server$/source "src"/' "$neg_native/chemical.mod"
neg_native_log="$out/negative_native.log"
if CHEM_DEBUG_INDEX=1 "$compiler" "$neg_native/chemical.mod" -v \
        --build-dir "$out/negative_native_build" -o "$out/negative_native.exe" > "$neg_native_log" 2>&1; then
    check "the typo'd shared component fails the native/server job (it built)" 0
else
    check "the typo'd shared component fails the native/server job" 1
fi
check "the native job is not the client side (client=0 js=0)" \
      "$(has_re "$neg_native_log" 'universal_job_begin .*client=0 js=0')"
check "the native/server job body-checks the shared component" \
      "$(has "$neg_native_log" '[body] component=SharedBadge')"
check "the native/server job reports WEB-BODY-001" \
      "$(has "$neg_native_log" 'WEB-BODY-001')"

# ---------------------------------------------------------------- no provider on the server
# `shared` imports the provider only on the client side: the server job has an
# empty index and must skip the check (no false 008), while the client indexes
# it and checks the component.
echo
echo "== no provider on the server: the server skips, the client checks"
skip="$out/no_provider"
cp -r "$fixture" "$skip"
rm -rf "$skip/build" "$skip/output" "$skip/shared/build" "$skip/client/build" \
       "$skip/provider/build" "$skip/src/build"
sed -i 's|^import "\.\./provider"$|import "../provider" if client|' "$skip/shared/chemical.mod"
skip_log="$out/no_provider.log"
if CHEM_DEBUG_INDEX=1 "$compiler" "$skip/chemical.mod" --web -v \
        --build-dir "$out/no_provider_build" -o "$out/no_provider.exe" > "$skip_log" 2>&1; then
    check "the no-server-provider build succeeds" 1
else
    check "the no-server-provider build succeeds (see $skip_log)" 0
    tail -20 "$skip_log" >&2
fi
skip_client="$out/no_provider.client.log"
skip_server="$out/no_provider.server.log"
split_sides "$skip_log" "$skip_client" "$skip_server"
check "the client still body-checks the shared component" \
      "$(has "$skip_client" '[body] component=SharedBadge')"
check "the server (no provider) skips the body check" \
      "$([ "$(count "$skip_server" '[body] component=')" = "0" ] && echo 1 || echo 0)"
check "the no-server-provider build has no WEB-BODY diagnostic" \
      "$([ "$(count "$skip_log" 'WEB-BODY')" = "0" ] && echo 1 || echo 0)"

if [ "$fail" != 0 ]; then
    echo "web-two-pass-test: FAILED" >&2
    exit 1
fi
echo "web-two-pass-test: a shared #universal component compiles in both --web passes"
