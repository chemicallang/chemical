#!/usr/bin/env bash
# M6.2: build and run the safe-thread data-race smoke test under TSAN.
#
# The bundled LLVM prebuild does not always ship libclang_rt.tsan.a. This
# script uses a system LLVM resource directory when one is available and fails
# loudly rather than silently running without the sanitizer.
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
COMPILER="${COMPILER:-$ROOT_DIR/cmake-build-debug/Compiler}"
MODULE="$ROOT_DIR/lang/tests/thread_tsan/chemical.mod"
OUT="${OUT:-${TMPDIR:-/tmp}/chemical-thread-tsan}"

if [ ! -x "$COMPILER" ]; then
  echo "error: compiler not found at $COMPILER" >&2
  exit 1
fi

RESOURCE_DIR="${CHEMICAL_TSAN_RESOURCES:-}"
if [ -z "$RESOURCE_DIR" ]; then
  for candidate in /usr/lib/llvm-*/lib/clang/*; do
    if [ -f "$candidate/lib/linux/libclang_rt.tsan-x86_64.a" ]; then
      RESOURCE_DIR="$candidate"
      break
    fi
  done
fi
if [ -z "$RESOURCE_DIR" ]; then
  echo "error: no libclang_rt.tsan runtime found; set CHEMICAL_TSAN_RESOURCES" >&2
  exit 1
fi

mkdir -p "$(dirname "$OUT")"
"$COMPILER" "$MODULE" -o "$OUT" --mode debug_quick --tsan --no-cache \
  --resources "$RESOURCE_DIR"
"$OUT"
