#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

NO_LLVM=false
RELEASE_BUILD=false
GENERATOR=""

while [ $# -gt 0 ]; do
  case "$1" in
    --no-llvm) NO_LLVM=true; shift ;;
    --release) RELEASE_BUILD=true; shift ;;
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
      echo "  --release                 Sets CMAKE_BUILD_TYPE to Release instead of Debug"
      echo "  --generator <name>, -G    CMake generator (Ninja, Unix Makefiles, etc.)"
      echo "  --help, -h                Show this help"
      echo ""
      echo "Environment variables:"
      echo "  CHEMICAL_MSVC_AUTO=0    Disable automatic MSVC environment setup on Windows"
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2; exit 1 ;;
  esac
done

# On Windows (Git Bash / MSYS2), ensure MSVC flags are not mangled by MSYS2
source "$SCRIPT_DIR/msvc_env.sh"

CMAKE_ARGS=(-S . -B cmake-build-debug)
if [ -n "$GENERATOR" ]; then
  CMAKE_ARGS+=(-G "$GENERATOR")
elif [ -n "${MSYSTEM-}" ] && [ -n "${INCLUDE-}" ]; then
  # Windows Git Bash + MSVC environment (set by msvc_env.sh)
  # → use MinGW Makefiles (matches CLion default)
  CMAKE_ARGS+=(-G "MinGW Makefiles")
fi
if [ "$NO_LLVM" = true ]; then
  CMAKE_ARGS+=(-DBUILD_COMPILER=OFF)
fi
if [ "$RELEASE_BUILD" = true ]; then
  CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Release)
else
  CMAKE_ARGS+=(-DCMAKE_BUILD_TYPE=Debug)
fi

cmake "${CMAKE_ARGS[@]}"
