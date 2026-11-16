#!/usr/bin/env bash
# Instantiation-name gate: content-derived names, one definition per name.
#
# The compiler names a generic instantiation after its *content* — the generic
# declaration's module-qualified name plus its concrete type arguments — not after
# the position it happened to take in the declaring module's instantiation list.
# The position is only unique within one module, and every module instantiates its
# own copies, so two different types could print one name; the C compiler then kept
# whichever definition it saw first and the program was wrong (and the numbering
# depended on how the parallel symbol-resolution pass interleaved files, so the
# same source could build once and break once).
#
# This gate builds the `instantiation_names` fixture — a module that instantiates
# `vector<T>`, `Option<T>` and `unordered_map<K, V>` with different arguments than
# `std` does, and reaches instantiations `std` also reaches — twice and checks:
#
#   1. every instantiation name is a content hash, never a position;
#   2. two independent builds emit exactly the same set of names (determinism,
#      which is what makes the per-module translation cache safe);
#   3. no name is defined twice in the single translation unit, and no name has
#      two different signatures;
#   4. an incremental build that mixes cached and freshly translated modules still
#      links and runs (a cached module re-claims the definitions it owns).
#
#   ./scripts/instantiation-names-test.sh
set -eu

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler.exe}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler"

fixture="lang/tests/fixtures/instantiation_names"
out="lang/compiled/instantiation_names"
rm -rf "$out" "$fixture/build"

fail=0
check() {
    if [ "$2" = "1" ]; then
        echo "ok   $1"
    else
        echo "FAIL $1" >&2
        fail=1
    fi
}

build() {
    local exe="$1"
    shift
    "$compiler" "$fixture/chemical.mod" -o "$exe" --mode debug_quick --emit-c "$@" \
        >/dev/null 2>&1
}

echo "== two independent builds =="
build "$out.exe" --no-cache
cp "$fixture/build/Translated.c" "$out.one.c"
build "$out.exe" --no-cache
cp "$fixture/build/Translated.c" "$out.two.c"

names() { grep -oE '__cgs__[0-9a-f]+|__cfg_[0-9a-f]+' "$1" | sort -u; }
names "$out.one.c" > "$out.names1"
names "$out.two.c" > "$out.names2"

check "the build emits instantiation names at all" \
      "$([ "$(wc -l < "$out.names1")" -gt 20 ] && echo 1 || echo 0)"
check "two builds emit the identical name set" \
      "$(diff -q "$out.names1" "$out.names2" >/dev/null && echo 1 || echo 0)"

# A struct tag ends at the hash (`struct X__cgs__<hash> {`), so this is the one
# emitted name where the suffix can be checked exactly: a positional index would
# be a short decimal number (`__cgs__4`), never twelve hex digits.
struct_hashes() {
    grep -oE '^struct [A-Za-z_0-9]+__cgs__[0-9a-f]+ \{' "$1" \
        | sed -E 's/^struct ([A-Za-z_0-9]+) \{/\1/'
}
struct_hashes "$out.one.c" > "$out.structs"
check "there are instantiated structs to name" \
      "$([ "$(wc -l < "$out.structs")" -gt 2 ] && echo 1 || echo 0)"
check "every instantiated struct name ends in a 12-hex content hash" \
      "$([ -z "$(grep -vE '__cgs__[0-9a-f]{12}$' "$out.structs")" ] && echo 1 || echo 0)"

echo "== one definition per name =="
defs() {
    grep -oE '^[A-Za-z_][^;{}]*__cg[sf]__[0-9a-f]+[^;{}]*\([^;{}]*\)[[:space:]]*\{' "$1" \
        | sed -E 's/.*[ *]([A-Za-z_0-9]+)\(.*/\1/' | sort
}
defs "$out.one.c" > "$out.defs"
check "no instantiation function is defined twice" \
      "$([ -z "$(uniq -d "$out.defs")" ] && echo 1 || echo 0)"
check "no struct body is defined twice" \
      "$([ -z "$(grep -oE '^struct [A-Za-z_0-9]+ \{' "$out.one.c" | sort | uniq -d)" ] && echo 1 || echo 0)"
# Distinct prototypes of one name collapse with `sort -u`, so any name that is
# still listed twice was declared with two different signatures.
protos() {
    grep -oE '^[A-Za-z_][^;{}]*__cg[sf]__[0-9a-f]+[^;{}]*\([^;{}]*\);' "$1" | sort -u
}
protos "$out.one.c" > "$out.protos"
check "no symbol has two signatures" \
      "$([ -z "$(sed -E 's/.*[ *]([A-Za-z_0-9]+)\(.*/\1/' "$out.protos" | sort | uniq -d)" ] && echo 1 || echo 0)"

echo "== the program runs =="
./"$out.exe"
check "the built program returns 0" "$([ $? -eq 0 ] && echo 1 || echo 0)"

echo "== incremental build (cached modules re-claim their definitions) =="
rm -rf "$fixture/build"
build "$out.exe" >/dev/null
# `touch`, not an edit: the module must be re-translated by its *timestamp* while
# its content (and the checked-in fixture) stays untouched.
touch "$fixture/src/main.ch"
build "$out.exe" > /dev/null
cp "$fixture/build/Translated.c" "$out.mixed.c"
defs "$out.mixed.c" > "$out.mixed.defs"
check "a mixed cached/fresh build defines each name once" \
      "$([ -z "$(uniq -d "$out.mixed.defs")" ] && echo 1 || echo 0)"
check "a mixed cached/fresh build defines each struct once" \
      "$([ -z "$(grep -oE '^struct [A-Za-z_0-9]+ \{' "$out.mixed.c" | sort | uniq -d)" ] && echo 1 || echo 0)"

if [ "$fail" -ne 0 ]; then
    echo "FAIL: instantiation names are not content-derived and unique" >&2
    exit 1
fi
echo "PASS: instantiation names are content-derived, deterministic and defined once"
