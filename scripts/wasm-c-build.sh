#!/usr/bin/env bash
# Build a Chemical app for wasm32 through the C emitter:
#
#   scripts/wasm-c-build.sh <app.mod> <out.wasm> [--export NAME]... [--export-all]
#
# TCCCompiler translates the whole app to one C file (no TCC compile/link, no
# host flags); this script compiles that C and lang/runtime/wasm/runtime.c with
# clang for wasm32 and links them with wasm-ld.
#
# Environment overrides:
#   WASM_CC           clang driver command (default: cmake-build-debug/Compiler cc, else clang)
#   WASM_LD           wasm-ld path (default: out/host/bin/wasm-ld, else wasm-ld)
#   WASM_TCC_COMPILER TCCCompiler binary (default: cmake-build-debug/TCCCompiler)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

usage() {
    sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
    exit 1
}

[ $# -ge 2 ] || usage
MOD="$1"
OUT="$2"
shift 2

EXPORT_ALL=false
EXPORTS=()
while [ $# -gt 0 ]; do
    case "$1" in
        --export) EXPORTS+=("$2"); shift 2 ;;
        --export-all) EXPORT_ALL=true; shift ;;
        *) echo "wasm-c-build: unknown argument '$1'" >&2; usage ;;
    esac
done

TCC="${WASM_TCC_COMPILER:-$ROOT/cmake-build-debug/TCCCompiler}"
if [ ! -x "$TCC" ]; then
    echo "wasm-c-build: TCCCompiler not found at '$TCC' (build with ./scripts/build.sh --tcc)" >&2
    exit 1
fi

if [ -n "${WASM_CC:-}" ]; then
    read -r -a CC_CMD <<<"$WASM_CC"
elif [ -x "$ROOT/cmake-build-debug/Compiler" ]; then
    CC_CMD=("$ROOT/cmake-build-debug/Compiler" cc)
else
    CC_CMD=(clang)
fi

WASM_LD="${WASM_LD:-}"
if [ -z "$WASM_LD" ]; then
    if [ -x "$ROOT/out/host/bin/wasm-ld" ]; then
        WASM_LD="$ROOT/out/host/bin/wasm-ld"
    else
        WASM_LD="wasm-ld"
    fi
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "==> emitting C from $MOD"
"$TCC" "$MOD" --target wasm32-unknown-unknown --emit-c -o "$TMP/app.c" --mode debug_quick

echo "==> compiling C for wasm32"
"${CC_CMD[@]}" --target=wasm32-unknown-unknown -nostdlib -Os -std=c11 \
    -Wno-incompatible-library-redeclaration -Wno-duplicate-decl-specifier \
    -c "$TMP/app.c" -o "$TMP/app.o"
"${CC_CMD[@]}" --target=wasm32-unknown-unknown -nostdlib -Os \
    -c "$ROOT/lang/runtime/wasm/runtime.c" -o "$TMP/runtime.o"

echo "==> linking $OUT"
LINK=( "$WASM_LD" --no-entry --export-memory --allow-undefined --gc-sections
       -z stack-size=1048576 --stack-first )
if [ "$EXPORT_ALL" = true ]; then
    LINK+=( --export-all )
elif [ ${#EXPORTS[@]} -gt 0 ]; then
    for name in "${EXPORTS[@]}"; do LINK+=( "--export=$name" ); done
else
    LINK+=( --export=main )
fi
LINK+=( "$TMP/app.o" "$TMP/runtime.o" -o "$OUT" )
"${LINK[@]}"

echo "==> built $OUT"
