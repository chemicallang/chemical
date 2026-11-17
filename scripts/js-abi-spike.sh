#!/usr/bin/env bash
# JS backend ABI spike gate (lang/docs/js-backend-abi.md §9).
#
# Compiles and runs the spike program with the native backend, runs the
# hand-written JavaScript over the emulated linear-memory ABI, and requires
# byte-identical output. This is Phase 1 step 0's gate: if it fails, the ABI is
# wrong and no emitter code should be written.
#
#   ./scripts/js-abi-spike.sh
set -eu

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler.exe}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler"
spike_dir="lang/tests/web/fixtures/js_abi_spike"

if [ ! -f "$spike_dir/chemical.mod" ]; then
    echo "js-abi-spike: missing $spike_dir/chemical.mod" >&2
    exit 2
fi
command -v node >/dev/null 2>&1 || { echo "js-abi-spike: node not found" >&2; exit 2; }

echo "== compiling the spike with the native backend =="
"$compiler" "$spike_dir/chemical.mod" -o "$spike_dir/spike.exe" >/dev/null
# Normalize the platform line ending: the comparison is about program output, not
# about whether stdout was opened in text mode.
"$spike_dir/spike.exe" | tr -d '\r' > "$spike_dir/native.txt"

echo "== running the hand-written JS ABI =="
node scripts/js_abi_spike/spike.mjs > "$spike_dir/js.txt"

if diff -u "$spike_dir/native.txt" "$spike_dir/js.txt"; then
    size=$(wc -c < scripts/js_abi_spike/spike.mjs)
    echo
    echo "PASS: native and JS ABI agree byte-for-byte ($size bytes of JS)"
    exit 0
fi

echo
echo "FAIL: the JS ABI does not reproduce the native output" >&2
exit 1
