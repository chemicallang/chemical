#!/usr/bin/env bash
# JS backend diagnostic gate.
#
# A construct the JS backend cannot emit must fail the *build* with a positioned
# message (function name + file:line:column) for every offending site — never a
# runtime error in a browser. This compiles a fixture that contains two such
# sites and requires both to be reported, with positions.
#
#   ./scripts/js-backend-diagnostics.sh
set -eu

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler.exe}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler"
fixture="lang/tests/web/fixtures/js_backend_diagnostics"
out_dir="lang/compiled/js-backend-test/diagnostics"
log="$out_dir/build.log"
rm -rf "$out_dir"
mkdir -p "$out_dir"

status=0
CHX_JS_RUNTIME="$root/scripts/js_backend/runtime.mjs" \
    "$compiler" "$fixture/chemical.mod" --emit-js "$out_dir/bundle" > "$log" 2>&1 || status=$?

if [ "$status" -eq 0 ]; then
    echo "FAIL: the fixture compiled for JS; inline assembly must be rejected" >&2
    exit 1
fi

fail=0
require() {
    if grep -qF "$1" "$log"; then
        echo "ok   $2"
    else
        echo "FAIL $2: expected '$1' in the build log" >&2
        fail=1
    fi
}

# Both sites, each with its function name and a position.
require "inline assembly is not supported by the JS backend" "reports the unsupported construct"
require "uses_asm_a" "names the first function"
require "uses_asm_b" "names the second function"
require "main.ch:10:" "positions the first site"
require "main.ch:16:" "positions the second site"
if [ "$fail" -ne 0 ]; then
    echo
    echo "--- build log ---" >&2
    cat "$log" >&2
    exit 1
fi

echo
echo "PASS: unsupported constructs fail the build, with positions"
