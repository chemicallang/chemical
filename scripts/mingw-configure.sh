#!/usr/bin/env bash
set -e

CMAKE_BIN=cmake
TOOLCHAIN_ROOT=""
TCC_ONLY=false

while [[ $# -gt 0 ]]; do
  case "$1" in
    --cmake)
      CMAKE_BIN="$2"
      shift 2
      ;;
    --toolchain-root)
      TOOLCHAIN_ROOT="$2"
      shift 2
      ;;
    --tcc-only)
      TCC_ONLY=true
      shift
      ;;
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
DEFAULT_TOOLCHAIN="${SCRIPT_DIR}/../toolchains/llvm-mingw"

if [[ -z "$TOOLCHAIN_ROOT" ]]; then
  if [[ -d "$DEFAULT_TOOLCHAIN" ]]; then
    echo "Auto-detected llvm-mingw toolchain at ${DEFAULT_TOOLCHAIN}"
    TOOLCHAIN_ROOT="$DEFAULT_TOOLCHAIN"
  fi
fi

CMAKE_ARGS=(
  -B build/mingw
  -S .
  -G "MinGW Makefiles"
)

# Use our toolchain file only if we found a toolchain
if [[ -n "$TOOLCHAIN_ROOT" ]]; then
  CMAKE_ARGS+=("-DCMAKE_TOOLCHAIN_FILE=cmake/clang-mingw-toolchain.cmake")
  CMAKE_ARGS+=("-DTOOLCHAIN_ROOT=$TOOLCHAIN_ROOT")
fi

if [[ "$TCC_ONLY" == true ]]; then
  CMAKE_ARGS+=("-DBUILD_COMPILER=OFF")
fi

"$CMAKE_BIN" "${CMAKE_ARGS[@]}"

echo "Configuration completed. To build, run:"
if [[ "$TCC_ONLY" == true ]]; then
  echo "  $CMAKE_BIN --build build/mingw --target TCCCompiler"
else
  echo "  $CMAKE_BIN --build build/mingw --target Compiler"
fi