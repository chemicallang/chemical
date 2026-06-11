#!/usr/bin/env bash
set -euo pipefail

NO_LLVM=false

for arg in "$@"; do
  case "$arg" in
    --no-llvm) NO_LLVM=true ;;
    --help|-h)
      echo "Usage: $0 [options]"
      echo ""
      echo "Options:"
      echo "  --no-llvm       Disable LLVM, sets BUILD_COMPILER=OFF in CMake"
      echo "  --help, -h      Show this help"
      exit 0
      ;;
    *)
      echo "Unknown option: $arg"
      exit 1
      ;;
  esac
done

CMAKE_OPTIONS="-S . -B cmake-build-debug"
if [ "$NO_LLVM" = true ]; then
  CMAKE_OPTIONS="$CMAKE_OPTIONS -DBUILD_COMPILER=OFF"
fi

cmake $CMAKE_OPTIONS
