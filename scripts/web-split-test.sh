#!/usr/bin/env bash
# `--web` gate: one build script, two jobs (lang/docs/web-stack-plan.md §3.1/§4).
#
#   web_split          the good tree: `shared` + `client` compile to a JavaScript
#                      bundle, `shared` + `server` to a native executable, and the
#                      two sides cannot see each other.
#   web_split_native   `shared` calls `getenv` and `client` calls that shared
#                      function: the client pass must refuse it by name and by
#                      source location, and the server side must not be built.
#   client_result      client `main` discards a `std::Result`: the client pass
#                      must reject it (`WEB-CLIENT-001`), while the same discard
#                      in server-only code is allowed. The gate then rewrites the
#                      client discard to `var _ = ...` and builds again.
#   dom_in_server      a server module calls the typed `dom` API: the per-symbol
#                      table in `MIRTargetSupport` must refuse the client-only
#                      host symbols during translation, not at link time.
#
# Both directions run through the real CLI, so this is the gate that keeps the
# two-job build honest: a regression that silently lets native code into the
# client bundle fails here.
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-split-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
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
file_has() { grep -qF -- "$2" "$1" 2>/dev/null && echo 1 || echo 0; }

out_root="lang/compiled/web-split-gate"
rm -rf "$out_root"
mkdir -p "$out_root"

echo "== web_split: shared + client (JS) and shared + server (native)"
pos_build="lang/tests/web/fixtures/web_split/build"
rm -rf "$pos_build"
pos_log="$out_root/positive.log"
if "$compiler" lang/tests/web/fixtures/web_split/chemical.mod --web \
        --build-dir "$pos_build" -o "$out_root/web_split.exe" > "$pos_log" 2>&1; then
    check "the two-job build succeeds" 1
else
    check "the two-job build succeeds (see $pos_log)" 0
    tail -25 "$pos_log" >&2
fi
check "the build reports both sides" "$(file_has "$pos_log" 'web build complete')"

bundle_dir="$pos_build/web-client/assets"
program_mjs=$(find "$bundle_dir" -maxdepth 1 -name '*.mjs' ! -name 'index.mjs' ! -name 'runtime.mjs' 2>/dev/null | head -1)
check "the client bundle was emitted ($bundle_dir)" \
      "$([ -f "$bundle_dir/index.mjs" ] && [ -n "$program_mjs" ] && echo 1 || echo 0)"
check "the client bundle carries the JS runtime" \
      "$([ -f "$bundle_dir/runtime.mjs" ] && echo 1 || echo 0)"
check "the server executable was linked" \
      "$([ -x "$out_root/web_split.exe" ] && echo 1 || echo 0)"

server_out="$out_root/server.txt"
if [ -x "$out_root/web_split.exe" ]; then
    "$out_root/web_split.exe" | tr -d '\r' > "$server_out" 2>&1
fi
check "the server job runs its own side" "$(file_has "$server_out" 'server: 5')"

if command -v node >/dev/null 2>&1; then
    client_out="$out_root/client.txt"
    if node scripts/js_backend/run.mjs "$bundle_dir" > "$client_out" 2>&1; then
        check "the client bundle runs in Node" 1
    else
        check "the client bundle runs in Node (see $client_out)" 0
    fi
    check "the client job runs shared code and its own main" \
          "$(file_has "$client_out" 'client: value=forty-two (42)')"
    check "the server job is not in the client bundle" \
          "$(grep -qF -- 'server:' "$client_out" && echo 0 || echo 1)"
else
    check "node is available to run the client bundle" 0
fi

echo
echo "== client_result: a discarded Result fails the client pass, never the server"
res_build="lang/tests/web/fixtures/client_result/build"
rm -rf "$res_build"
res_log="$out_root/client_result.log"
if "$compiler" lang/tests/web/fixtures/client_result/chemical.mod --web \
        --build-dir "$res_build" -o "$out_root/client_result.exe" > "$res_log" 2>&1; then
    check "a discarded Result in client code is rejected (it built)" 0
else
    check "a discarded Result in client code is rejected" 1
fi
check "the diagnostic names the rule" "$(file_has "$res_log" 'WEB-CLIENT-001')"
check "the diagnostic explains the fix" \
      "$(file_has "$res_log" "unhandled 'Result' in client code")"
check "the diagnostic names the discard idiom" "$(file_has "$res_log" "var _ = ...")"
check "the diagnostic points at the client source" "$(file_has "$res_log" 'client/main.ch')"
check "the diagnostic is attributed to the client pass" \
      "$(file_has "$res_log" 'web build: the client side failed')"
check "no client bundle was emitted for the rejected client" \
      "$([ -f "$res_build/web-client/assets/client.mjs" ] && echo 0 || echo 1)"

# The escape hatch: the same tree with `var _ = shared_parse("x")` must build,
# which also proves the server-side discard in `src/main.ch` is tolerated.
res_ok="$out_root/client_result_ok"
rm -rf "$res_ok"
cp -r lang/tests/web/fixtures/client_result "$res_ok"
rm -rf "$res_ok/build"
sed -i 's/^    shared_parse("x")$/    var _ = shared_parse("x")/' "$res_ok/client/main.ch"
res_ok_log="$out_root/client_result_ok.log"
if "$compiler" "$res_ok/chemical.mod" --web --build-dir "$res_ok/build" \
        -o "$res_ok/client_result.exe" > "$res_ok_log" 2>&1; then
    check "a deliberate discard (var _ = ...) builds in both jobs" 1
else
    check "a deliberate discard (var _ = ...) builds in both jobs (see $res_ok_log)" 0
    tail -25 "$res_ok_log" >&2
fi
check "the server side is built once the client is accepted" \
      "$([ -x "$res_ok/client_result.exe" ] && echo 1 || echo 0)"
check "the server job runs despite discarding a Result" \
      "$("$res_ok/client_result.exe" > /dev/null 2>&1 && echo 1 || echo 0)"

echo
echo "== dom_in_server: the typed DOM has no native implementation"
dom_build="lang/tests/web/fixtures/dom_in_server/build"
rm -rf "$dom_build"
dom_log="$out_root/dom_in_server.log"
if "$compiler" lang/tests/web/fixtures/dom_in_server/chemical.mod --web \
        --build-dir "$dom_build" -o "$out_root/dom_in_server.exe" > "$dom_log" 2>&1; then
    check "a server module using dom is rejected (it built)" 0
else
    check "a server module using dom is rejected" 1
fi
check "the diagnostic names the reason" "$(file_has "$dom_log" 'client-only')"
check "the diagnostic names the dom host API" \
      "$(file_has "$dom_log" "the 'dom' host API")"
check "the diagnostic says where to move the call" \
      "$(file_has "$dom_log" "call it from a 'client' module")"
check "the refusal happens during translation, not at link time" \
      "$(file_has "$dom_log" 'failure during translation to c in the module dom')"
check "no link error is reached" \
      "$(grep -qF -- 'unresolved reference' "$dom_log" && echo 0 || echo 1)"
check "no server executable was linked" \
      "$([ -x "$out_root/dom_in_server.exe" ] && echo 0 || echo 1)"

echo
echo "== web_split_native: a native symbol reached from shared must fail the client pass"
neg_build="lang/tests/web/fixtures/web_split_native/build"
rm -rf "$neg_build"
neg_log="$out_root/negative.log"
if "$compiler" lang/tests/web/fixtures/web_split_native/chemical.mod --web \
        --build-dir "$neg_build" -o "$out_root/web_split_native.exe" > "$neg_log" 2>&1; then
    check "the client pass rejects a native symbol (it built)" 0
else
    check "the client pass rejects a native symbol" 1
fi
check "the diagnostic names the native symbol" "$(file_has "$neg_log" 'getenv')"
check "the diagnostic points at the shared source" "$(file_has "$neg_log" 'shared/calc.ch')"
check "the diagnostic is attributed to the client pass" \
      "$(file_has "$neg_log" 'web build: the client side failed')"
check "the server side is not built after a client failure" \
      "$(file_has "$neg_log" 'the server side was not built')"
check "no client bundle was emitted" \
      "$([ -f "$neg_build/web-client/assets/client.mjs" ] && echo 0 || echo 1)"

echo
if [ "$fail" -ne 0 ]; then
    echo "web-split-test: FAILED" >&2
    exit 1
fi
echo "web-split-test: the two-job build splits cleanly in both directions"
