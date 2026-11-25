#!/usr/bin/env bash
# `index_with` module metadata gate (component body typing, Phase 0a).
#
# Proves the generic `index_with "<name>", ...` directive is parsed, propagated
# into the module record, and surfaced by the `-v` build diagnostic:
#
#   [lab] module 'index_parse' indexers: universal, docs, alpha
#
# Also round-trips through the mod -> build.lab converter (Module.addIndexer)
# and checks a non-string value is rejected with a parse diagnostic.
#
#   ./scripts/web-index-test.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-index-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
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

fixture="lang/tests/web/fixtures/index_parse"
out="lang/compiled/web-index-gate"
rm -rf "$out"
mkdir -p "$out"

# ---------------------------------------------------------------- parse + build
build_dir="$out/build"
log="$out/build.log"

echo "== parsed metadata is copied into the module record"
if "$compiler" "$fixture/chemical.mod" -v --build-dir "$build_dir" \
        -o "$out/index_parse.exe" > "$log" 2>&1; then
    check "the fixture builds" 1
else
    check "the fixture builds" 0
    tail -30 "$log" >&2
fi

check "a comma list and a repeated index_with parse in order" \
      "$(has "$log" "[lab] module 'index_parse' indexers: universal, docs, alpha")"
check "a module without index_with reports (none)" \
      "$(has "$log" "indexers: (none)")"
check "the executable is produced" \
      "$([ -x "$out/index_parse.exe" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- converter
lab_out="$out/index_parse.lab"
if "$compiler" "$fixture/chemical.mod" -o "$lab_out" > "$out/convert.log" 2>&1; then
    check "the mod -> build.lab conversion succeeds" 1
else
    check "the mod -> build.lab conversion succeeds" 0
    tail -20 "$out/convert.log" >&2
fi
check "the converter emits one addIndexer per value" \
      "$([ "$(grep -cF 'mod.addIndexer(' "$lab_out" 2>/dev/null || true)" = "3" ] && echo 1 || echo 0)"
check "the converter preserves the indexer names" \
      "$([ "$(has "$lab_out" 'mod.addIndexer("universal");')" = "1" ] \
          && [ "$(has "$lab_out" 'mod.addIndexer("docs");')" = "1" ] \
          && [ "$(has "$lab_out" 'mod.addIndexer("alpha");')" = "1" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- enum sync
if ./scripts/check-cbi-enum-sync.sh > "$out/enum-sync.log" 2>&1; then
    check "the C++/Chemical CBIFunctionType enums match" 1
else
    check "the C++/Chemical CBIFunctionType enums match" 0
    cat "$out/enum-sync.log" >&2
fi

# ---------------------------------------------------------------- hook probe
# universal_cbi is loaded, so JobBegin runs, but IndexModule only runs for a
# module that declares `index_with "universal"`.
probe_build() {
    local fixture="$1" name="$2" log="$3"
    CHEM_DEBUG_INDEX=1 "$compiler" "$fixture/chemical.mod" -v \
        --build-dir "$out/${name}_build" -o "$out/${name}.exe" > "$log" 2>&1
}

probe="lang/tests/web/fixtures/index_probe"
probe_log="$out/index_probe.log"
if probe_build "$probe" index_probe "$probe_log"; then
    check "the marked index_probe fixture builds" 1
else
    check "the marked index_probe fixture builds" 0
    tail -30 "$probe_log" >&2
fi
check "universal_job_begin runs exactly once for the job" \
      "$([ "$(grep -cF '[index] universal_job_begin' "$probe_log" 2>/dev/null || true)" = "1" ] && echo 1 || echo 0)"
check "universal_index_module runs exactly once for the marked module" \
      "$([ "$(grep -cF '[index] universal_index_module module=index_probe ' "$probe_log" 2>/dev/null || true)" = "1" ] && echo 1 || echo 0)"
check "the marked module has a non-zero declaration count" \
      "$(grep -qE '\[index\] universal_index_module module=index_probe count=[1-9][0-9]* decls=' "$probe_log" 2>/dev/null && echo 1 || echo 0)"
check "the indexed declarations include a public struct" \
      "$(has "$probe_log" "StructDecl:ProbeWidgetProps")"
check "the indexed declarations include a public function" \
      "$(has "$probe_log" "FunctionDecl:probe_helper")"

unmarked="lang/tests/web/fixtures/index_probe_unmarked"
unmarked_log="$out/index_probe_unmarked.log"
if probe_build "$unmarked" index_probe_unmarked "$unmarked_log"; then
    check "the unmarked index_probe fixture builds" 1
else
    check "the unmarked index_probe fixture builds" 0
    tail -30 "$unmarked_log" >&2
fi
check "universal_job_begin runs exactly once for the unmarked job" \
      "$([ "$(grep -cF '[index] universal_job_begin' "$unmarked_log" 2>/dev/null || true)" = "1" ] && echo 1 || echo 0)"
check "the unmarked module gets no index call" \
      "$([ "$(grep -cF 'universal_index_module' "$unmarked_log" 2>/dev/null || true)" = "0" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- diagnostics
make_bad_fixture() {
    local dir="$1" body="$2"
    rm -rf "$dir"
    mkdir -p "$dir/src"
    printf '%s\n' "$body" > "$dir/chemical.mod"
    printf 'public func main() : int { return 0 }\n' > "$dir/src/main.ch"
}

bad="$out/bad_identifier"
make_bad_fixture "$bad" 'application index_bad
source "src"
import std

index_with universal'
bad_log="$out/bad_identifier.log"
if "$compiler" "$bad/chemical.mod" --build-dir "$out/bad_identifier_build" \
        -o "$out/bad_identifier.exe" > "$bad_log" 2>&1; then
    check "a non-string index_with is rejected" 0
else
    check "a non-string index_with is rejected" 1
fi
check "the non-string diagnostic is clear" \
      "$(has "$bad_log" "expected a string value for 'index_with'")"

bad_end="$out/bad_end"
make_bad_fixture "$bad_end" 'application index_bad
source "src"
import std

index_with "universal",'
bad_end_log="$out/bad_end.log"
if "$compiler" "$bad_end/chemical.mod" --build-dir "$out/bad_end_build" \
        -o "$out/bad_end.exe" > "$bad_end_log" 2>&1; then
    check "a missing value after a comma is rejected" 0
else
    check "a missing value after a comma is rejected" 1
fi
check "the unexpected-end diagnostic is clear" \
      "$(has "$bad_end_log" "unexpected end of file in 'index_with' statement, expected a string")"

# ---------------------------------------------------------------- cleanup
# The fixture itself must never accumulate a build directory; the gate's own
# artifacts live under lang/compiled and are removed on success.
rm -rf "$fixture/build" "$probe/build" "$unmarked/build"
if [ "$fail" = 0 ]; then
    rm -rf "$out"
else
    echo "web-index-test: artifacts kept in $out" >&2
fi

if [ "$fail" != 0 ]; then
    echo "web-index-test: FAILED" >&2
    exit 1
fi
echo "web-index-test: ok"
