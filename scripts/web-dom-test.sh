#!/usr/bin/env bash
# `dom` gate: browser code written in `client/*.ch`, compiled by `--web`'s client job
# and run against a real host API.
#
# The fixture (`lang/tests/web/fixtures/dom_client`) is one application, two jobs:
#   client pass — `shared` + `client` → JavaScript (typed `dom` calls)
#   server pass — `shared` + `server` + `src` → native executable
#
# The bundle is then executed by `scripts/js_backend/dom_harness.mjs`, which installs a
# small DOM, `localStorage` and `fetch` on `globalThis` and dispatches the clicks a
# page would deliver. That makes the check about the emitted JavaScript — handle
# plumbing, the `$fns` callback table, string marshalling — rather than about HTML.
#
#   ./scripts/web-dom-test.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-dom-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
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
# A client bundle is one `.mjs` per Chemical module (plus `index.mjs` and the
# runtime), so "the emitted JavaScript does X" is a property of the union of them.
bundle_has() {
    local dir="$1" needle="$2"
    grep -rqF --include='*.mjs' -- "$needle" "$dir" 2>/dev/null && echo 1 || echo 0
}
bundle_file() { ls "$1"/*.mjs 2>/dev/null | head -1; }

command -v node >/dev/null 2>&1 || {
    echo "web-dom-test: node is required to run the emitted bundle" >&2
    exit 1
}

out="lang/compiled/web-dom-gate"
rm -rf "$out"
mkdir -p "$out"
build_dir="$out/build"
log="$out/build.log"

echo "== two-job build of the dom fixture"
if "$compiler" lang/tests/web/fixtures/dom_client/chemical.mod --web \
        --build-dir "$build_dir" -o "$out/dom_client.exe" > "$log" 2>&1; then
    check "the two-job build succeeds" 1
else
    check "the two-job build succeeds" 0
    tail -20 "$log" >&2
fi
assets="$build_dir/web-client/assets"
check "the client job emits a JavaScript bundle" \
      "$([ -f "$assets/index.mjs" ] && [ -f "$assets/client.mjs" ] && [ -f "$assets/shared.mjs" ] && echo 1 || echo 0)"
check "the client job emits one module file per Chemical module" \
      "$([ -f "$assets/dom.mjs" ] && [ -f "$assets/std.mjs" ] && echo 1 || echo 0)"
check "the client job emits a source map per module" \
      "$([ -f "$assets/client.mjs.map" ] && [ -f "$assets/shared.mjs.map" ] && echo 1 || echo 0)"
check "every module links its own source map" \
      "$([ "$(grep -l 'sourceMappingURL=' "$assets"/*.mjs | wc -l)" = "$(ls "$assets"/*.mjs | grep -v 'runtime.mjs\|index.mjs\|boot.mjs' | wc -l)" ] && echo 1 || echo 0)"
check "the server job emits a native executable" \
      "$([ -x "$out/dom_client.exe" ] && echo 1 || echo 0)"

# The two jobs share `shared` and nothing else: the server's HTML must not appear in
# the client bundle, and the client's DOM calls must not appear in the server binary.
check "server-only code is absent from the client bundle" \
      "$([ "$(bundle_has "$assets" 'DOCTYPE html')" = "0" ] && echo 1 || echo 0)"
check "client-only code is absent from the server executable" \
      "$([ "$(file_has "$out/dom_client.exe" 'chx_dom_get_element_by_id')" = "0" ] && echo 1 || echo 0)"

echo "== server side"
server_out="$out/server.txt"
"$out/dom_client.exe" > "$server_out" 2>&1 || true
check "the server executable runs and renders the page" \
      "$([ "$(file_has "$server_out" 'server:')" = "1" ] && [ "$(file_has "$server_out" 'bytes of HTML')" = "1" ] && echo 1 || echo 0)"

echo "== client side: emitted JavaScript against a host DOM"
check "the emitted bundle resolves the shared rule through the JS backend" \
      "$(bundle_has "$assets" 'shared_next')"
check "the emitted bundle calls the dom host (not raw externs)" \
      "$([ "$(bundle_has "$assets" 'rt.dom.get_element_by_id')" = "1" ] && [ "$(bundle_has "$assets" 'chx_dom_get_element_by_id')" = "0" ] && echo 1 || echo 0)"
check "the emitted bundle registers its function table for host callbacks" \
      "$(bundle_has "$assets" 'rt.register_fns($fns)')"

harness_out="$out/harness.txt"
if node scripts/js_backend/dom_harness.mjs "$build_dir/web-client/assets" > "$harness_out" 2>&1; then
    check "the emitted bundle runs against the DOM harness" 1
else
    check "the emitted bundle runs against the DOM harness" 0
    tail -20 "$harness_out" >&2
fi

expected="$out/expected.txt"
cat > "$expected" <<'EOF'
dom found=1 missing=0
dom text=Hello dom role=card has=1
dom value=typed
dom storage=dark removed=1
dom url origin=https://example.com path=/a/b q=7
dom click count=1
dom click count=2
dom fetch status=200 body={"ok":true,"url":"/api/ok"}
dom-harness: done
EOF

# The harness prints `dom-harness: done` last; compare the program's lines exactly,
# so a silently-missing call (an empty tree walk, a dead callback) fails here.
grep -F 'dom ' "$harness_out" > "$out/actual.txt" 2>/dev/null || true
grep -F 'dom ' "$expected" > "$out/expected_dom.txt"
if diff -u "$out/expected_dom.txt" "$out/actual.txt" > "$out/diff.txt" 2>&1; then
    check "every dom call produced the expected result (document, element, event, fetch, storage, URL)" 1
else
    check "every dom call produced the expected result (document, element, event, fetch, storage, URL)" 0
    cat "$out/diff.txt" >&2
fi
check "the listener fired for both dispatched clicks" \
      "$([ "$(grep -c 'dom click count=' "$harness_out")" = "2" ] && echo 1 || echo 0)"
check "the fetch completion ran with the response body" \
      "$(file_has "$harness_out" 'dom fetch status=200 body={"ok":true,"url":"/api/ok"}')"

echo "== source map decodes to the client source"
if node scripts/js_backend/sourcemap_check.mjs "$build_dir/web-client/assets" main.ch on_click \
        > "$out/sourcemap.txt" 2>&1; then
    check "the source map points at the code that produced the dom listener" 1
else
    check "the source map points at the code that produced the dom listener" 0
    tail -10 "$out/sourcemap.txt" >&2
fi

if [ "$fail" -ne 0 ]; then
    echo "web-dom-test: FAILED" >&2
    exit 1
fi
echo "web-dom-test: PASS (client/*.ch compiles to JS, typed dom runs against the host)"
