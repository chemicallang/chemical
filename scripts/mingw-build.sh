#!/usr/bin/env bash
set -e

CMAKE_BIN=cmake
TOOLCHAIN_ROOT=""

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
    *)
      echo "Unknown argument: $1"
      exit 1
      ;;
  esac
done

CMAKE_ARGS=(
  -B build/mingw
  -S .
  -DCMAKE_TOOLCHAIN_FILE=cmake/clang-mingw-toolchain.cmake
  -G "MinGW Makefiles"
)

if [[ -n "$TOOLCHAIN_ROOT" ]]; then
  CMAKE_ARGS+=("-DTOOLCHAIN_ROOT=$TOOLCHAIN_ROOT")
fi

"$CMAKE_BIN" "${CMAKE_ARGS[@]}"
"$CMAKE_BIN" --build build/mingw --target Compiler