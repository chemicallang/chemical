#!/usr/bin/env bash
# JS backend differential test (Phase 1 gate).
#
# Compiles one fixture twice — once with the native backend, once with the JS
# backend (`--emit-js`) — runs both, and requires byte-identical output. This is
# the gate that turns "the ABI spike works" into "the backend works": the same
# MIR, two backends, one observed behaviour.
#
#   ./scripts/js-backend-test.sh [fixture-name]
#
# Default fixture: js_backend.
set -eu

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

fixture="${1:-js_backend}"
fixture_dir="lang/tests/web/fixtures/$fixture"
compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler.exe}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler"
out_root="lang/compiled/js-backend-test"
mkdir -p "$out_root"

command -v node >/dev/null 2>&1 || { echo "js-backend-test: node not found" >&2; exit 2; }
[ -f "$fixture_dir/chemical.mod" ] || { echo "js-backend-test: no fixture at $fixture_dir" >&2; exit 2; }

echo "== native backend =="
"$compiler" "$fixture_dir/chemical.mod" -o "$out_root/$fixture.exe" > "$out_root/native-build.log" 2>&1 || {
    echo "js-backend-test: native build failed, see $out_root/native-build.log" >&2
    tail -20 "$out_root/native-build.log" >&2
    exit 1
}
# Normalize the platform line ending: the comparison is about program output.
"$out_root/$fixture.exe" | tr -d '\r' > "$out_root/native.txt"

echo "== js backend =="
rm -rf "$out_root/bundle"
CHX_JS_RUNTIME="$root/scripts/js_backend/runtime.mjs" \
    "$compiler" "$fixture_dir/chemical.mod" --emit-js "$out_root/bundle" \
    > "$out_root/js-build.log" 2>&1 || {
    echo "js-backend-test: JS build failed, see $out_root/js-build.log" >&2
    tail -20 "$out_root/js-build.log" >&2
    exit 1
}

node scripts/js_backend/run.mjs "$out_root/bundle" > "$out_root/js.txt" || {
    echo "js-backend-test: the emitted program failed to run" >&2
    cat "$out_root/js.txt" >&2
    exit 1
}

if diff -u "$out_root/native.txt" "$out_root/js.txt"; then
    modules=$(ls "$out_root/bundle"/*.mjs | wc -l)
    bytes=$(cat "$out_root/bundle"/*.mjs | wc -c)
    echo
    echo "PASS: native and JS output agree byte-for-byte ($modules modules, $bytes bytes)"
    # The source map is decoded, not just checked for existence: every mapping
    # must be in range and one must land on the line that produced the output.
    node scripts/js_backend/sourcemap_check.mjs "$out_root/bundle" main.ch printf | tail -2
    exit 0
fi

echo
echo "FAIL: the JS backend does not reproduce the native output" >&2
exit 1
