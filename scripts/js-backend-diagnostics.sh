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

# ── a native-only symbol is explained as such, not as a module problem ───────
native_fixture="lang/tests/web/fixtures/js_backend_native_symbol"
native_log="$out_dir/native.log"
status=0
CHX_JS_RUNTIME="$root/scripts/js_backend/runtime.mjs" \
    "$compiler" "$native_fixture/chemical.mod" --emit-js "$out_dir/native" > "$native_log" 2>&1 || status=$?
if [ "$status" -eq 0 ]; then
    echo "FAIL: a native-only symbol compiled for JS" >&2
    exit 1
fi
log="$native_log"
require "native symbol 'getenv' has no JavaScript equivalent" "explains a native-only symbol"
require "main.ch:10:" "positions the native-symbol call"
require "@extern" "names the declaration form so the fix is obvious"

# ── `--check` runs the same rules and writes nothing ────────────────────────
check_dir="$out_dir/check-only"
check_log="$out_dir/check.log"
if ! CHX_JS_RUNTIME="$root/scripts/js_backend/runtime.mjs" \
        "$compiler" "lang/tests/web/fixtures/js_backend/chemical.mod" \
        --check --emit-js "$check_dir" > "$check_log" 2>&1; then
    echo "FAIL: --check rejected a program that builds" >&2
    cat "$check_log" >&2
    exit 1
fi
if [ -e "$check_dir" ]; then
    echo "FAIL: --check wrote output to '$check_dir'" >&2
    exit 1
fi
echo "ok   --check validates without writing"

if [ "$fail" -ne 0 ]; then
    echo
    echo "--- build log ---" >&2
    cat "$log" >&2
    exit 1
fi

echo
echo "PASS: unsupported constructs fail the build, with positions"
