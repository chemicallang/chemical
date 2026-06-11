#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="cmake-build-debug"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
TARGET=""

usage() {
  echo "Usage: $0 [options]"
  echo ""
  echo "Options:"
  echo "  --tcc, --TCCCompiler     Build TCCCompiler (TinyCC backend)"
  echo "  --llvm, --Compiler       Build Compiler (LLVM/Clang backend)"
  echo "  --lsp, --ChemicalLsp     Build ChemicalLsp (LSP server)"
  echo "  --all                    Build all targets"
  echo "  -j N                     Number of parallel jobs (default: $JOBS)"
  echo "  --help, -h               Show this help"
  exit 1
}

while [ $# -gt 0 ]; do
  case "$1" in
    --tcc|--TCCCompiler) TARGET="TCCCompiler" ;;
    --llvm|--Compiler) TARGET="Compiler" ;;
    --lsp|--ChemicalLsp) TARGET="ChemicalLsp" ;;
    --all) TARGET="all" ;;
    -j) JOBS="$2"; shift ;;
    --help|-h) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
  shift
done

if [ -z "$TARGET" ]; then
  echo "Error: No target specified. Use --tcc, --llvm, --lsp, or --all."
  usage
fi

if [ "$TARGET" = "all" ]; then
  cmake --build "$BUILD_DIR" --config Debug -j "$JOBS"
else
  cmake --build "$BUILD_DIR" --config Debug --target "$TARGET" -j "$JOBS"
fi
