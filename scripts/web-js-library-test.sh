#!/usr/bin/env bash
# JS library mode gate (component body typing, Phase 0b).
#
# A `library` package has no `main`. With `--emit-js <dir>` it must build as a
# directory of ES modules, not as a shared object:
#
#   * no `main()` lookup (the build must not fail with "could not find main()")
#   * no `index.mjs` entry module
#   * one `.mjs` per module that owns reachable code, plus `runtime.mjs`
#   * `ship` files copied next to the modules
#   * no `.so` / executable
#   * `--check` lowers and diagnoses but writes no JS
#
# The Node consumer imports the emitted modules and calls the public functions
# through their `$exports` keys.
#
#   ./scripts/web-js-library-test.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-js-library-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
    exit 1
fi
compiler="$(cd "$(dirname "$compiler")" && pwd)/$(basename "$compiler")"

command -v node >/dev/null 2>&1 || {
    echo "web-js-library-test: node is required to import the emitted library" >&2
    exit 1
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
file_has() { grep -qF -- "$2" "$1" 2>/dev/null && echo 1 || echo 0; }

fixture="lang/tests/web/fixtures/js_library"
out="lang/compiled/web-js-library-gate"
rm -rf "$out"
mkdir -p "$out"

bundle="$out/bundle"
native_out="$out/library.so"
build_dir="$out/build"
log="$out/build.log"

echo "== a library package builds with --emit-js and no main"
if "$compiler" "$fixture/chemical.mod" --emit-js "$bundle" --build-dir "$build_dir" \
        -o "$native_out" > "$log" 2>&1; then
    check "the library builds as JavaScript" 1
else
    check "the library builds as JavaScript" 0
    tail -40 "$log" >&2
fi
check "the build did not look for main()" \
      "$([ "$(file_has "$log" 'could not find main()')" = "0" ] && echo 1 || echo 0)"
check "the compiler did not fall back to the native linker" \
      "$([ "$(file_has "$log" 'failed to link')" = "0" ] && echo 1 || echo 0)"

echo "== the emitted artifact is a directory of ES modules"
check "at least one module .mjs is emitted" \
      "$([ "$(ls "$bundle"/*.mjs 2>/dev/null | grep -v '/runtime.mjs$' | wc -l)" -ge 1 ] && echo 1 || echo 0)"
check "the library's module file is present" \
      "$([ -f "$bundle/js_library.mjs" ] && echo 1 || echo 0)"
check "no index.mjs entry module is written for a library" \
      "$([ ! -e "$bundle/index.mjs" ] && echo 1 || echo 0)"
check "runtime.mjs is emitted next to the modules" \
      "$([ -f "$bundle/runtime.mjs" ] && echo 1 || echo 0)"
check "the library module exposes both the full symbol table and the public API table" \
      "$([ "$(grep -lF 'export const $exports' "$bundle"/*.mjs 2>/dev/null | wc -l)" -ge 1 ] \
          && [ "$(grep -lF 'export const $api' "$bundle"/*.mjs 2>/dev/null | wc -l)" -ge 1 ] \
          && echo 1 || echo 0)"

echo "== no native artifact"
check "no .so/.dylib/.dll/.exe is produced" \
      "$([ "$(find "$bundle" -maxdepth 1 -type f \
            \( -name '*.so' -o -name '*.dylib' -o -name '*.dll' -o -name '*.exe' \) \
            | wc -l)" = "0" ] && echo 1 || echo 0)"
check "the requested native output path was not created" \
      "$([ ! -e "$native_out" ] && echo 1 || echo 0)"
check "the bundle carries no executable file" \
      "$([ "$(find "$bundle" -maxdepth 1 -type f -perm -111 | wc -l)" = "0" ] && echo 1 || echo 0)"

echo "== ship files land beside the modules"
check "the ship file is copied into the JS output directory" \
      "$([ -f "$bundle/library.txt" ] && echo 1 || echo 0)"
check "the shipped file keeps its contents" \
      "$([ "$(file_has "$bundle/library.txt" 'shipped by the js_library fixture')" = "1" ] \
          && echo 1 || echo 0)"

echo "== a Node consumer imports the module and calls the public functions"
consumer_log="$out/consumer.log"
if node "$fixture/consume.mjs" "$bundle" > "$consumer_log" 2>&1; then
    check "the Node consumer found and called the public functions" 1
else
    check "the Node consumer found and called the public functions" 0
    cat "$consumer_log" >&2
fi
check "the calls produced the deterministic output" \
      "$([ "$(file_has "$consumer_log" 'library_answer=42 library_sum(9)=45 library_async_answer=43')" = "1" ] \
          && echo 1 || echo 0)"
check "the reachable private helper is exported internally but not part of \$api" \
      "$([ "$(file_has "$consumer_log" 'hidden_in_exports=1 hidden_in_api=0')" = "1" ] \
          && echo 1 || echo 0)"

echo "== check-only lowers but writes no JavaScript"
check_bundle="$out/check_bundle"
check_log="$out/check.log"
if "$compiler" "$fixture/chemical.mod" --check --emit-js "$check_bundle" \
        --build-dir "$out/check_build" -o "$out/check.so" > "$check_log" 2>&1; then
    check "the check-only build passes" 1
else
    check "the check-only build passes" 0
    tail -40 "$check_log" >&2
fi
check "check-only writes no JS output directory" \
      "$([ ! -e "$check_bundle" ] && echo 1 || echo 0)"
check "check-only creates no native output" \
      "$([ ! -e "$out/check.so" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- cleanup
# The fixture itself must never accumulate a build directory; the gate's own
# artifacts live under lang/compiled and are removed on success.
rm -rf "$fixture/build"
if [ "$fail" = 0 ]; then
    rm -rf "$out"
else
    echo "web-js-library-test: artifacts kept in $out" >&2
fi

if [ "$fail" != 0 ]; then
    echo "web-js-library-test: FAILED" >&2
    exit 1
fi
echo "web-js-library-test: ok"
