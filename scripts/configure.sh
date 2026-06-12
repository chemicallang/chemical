#!/usr/bin/env bash
set -euo pipefail

NO_LLVM=false
GENERATOR=""

while [ $# -gt 0 ]; do
  case "$1" in
    --no-llvm) NO_LLVM=true; shift ;;
    --generator|-G)
      if [ -n "${2-}" ]; then
        GENERATOR="$2"; shift 2
      else
        echo "Error: --generator/-G requires a value" >&2; exit 1
      fi
      ;;
    --help|-h)
      echo "Usage: $0 [options]"
      echo ""
      echo "Options:"
      echo "  --no-llvm                 Disable LLVM, sets BUILD_COMPILER=OFF"
      echo "  --generator <name>, -G    CMake generator (Ninja, Unix Makefiles, etc.)"
      echo "  --help, -h                Show this help"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

CMAKE_ARGS=(-S . -B cmake-build-debug)
if [ -n "$GENERATOR" ]; then
  CMAKE_ARGS+=(-G "$GENERATOR")
fi
if [ "$NO_LLVM" = true ]; then
  CMAKE_ARGS+=(-DBUILD_COMPILER=OFF)
fi

cmake "${CMAKE_ARGS[@]}"
