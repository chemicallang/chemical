#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# On Windows (Git Bash / MSYS2), prevent MSYS2 from mangling MSVC flags
source "$SCRIPT_DIR/msvc_env.sh"

BUILD_DIR="cmake-build-debug"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
TARGET=""
CLEAN=false
CONFIG="Debug"

usage() {
  echo "Usage: $0 [options]"
  echo ""
  echo "Options:"
  echo "  --tcc, --TCCCompiler     Build TCCCompiler (TinyCC backend)"
  echo "  --llvm, --Compiler       Build Compiler (LLVM/Clang backend)"
  echo "  --lsp, --ChemicalLsp     Build ChemicalLsp (LSP server)"
  echo "  --all                    Build all targets"
  echo "  --clean                  Clean target artifacts only (no build)"
  echo "  --config Debug           Set config (default: Debug)"
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
    --clean) CLEAN=true ;;
    --config) CONFIG="$2"; shift ;;
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

# ── Clean step (exit after clean — does NOT trigger a build) ───────────────
if [ "$CLEAN" = true ]; then
  if [ "$TARGET" = "all" ]; then
    echo "==> Cleaning all targets..."
    cmake --build "$BUILD_DIR" --config "$CONFIG" --target clean 2>/dev/null || true
  else
    echo "==> Cleaning target $TARGET..."
    # Delete compiled object files for this target
    # (keeps CMake build.make / link.txt etc. so cmake --build still works)
    find "$BUILD_DIR/CMakeFiles/${TARGET}.dir" \(
        -name "*.obj" -o
        -name "*.o" -o
        -name "*.pdb"
    \) -exec rm -f {} + 2>/dev/null || true
    # Delete target binary
    rm -f "$BUILD_DIR/${TARGET}.exe" "$BUILD_DIR/${TARGET}" "$BUILD_DIR/${TARGET}.pdb"
  fi
  echo "==> Done cleaning."
  exit 0
fi

# ── Build step (skipped when --clean passed) ──────────────────────────────────
if [ "$TARGET" = "all" ]; then
  cmake --build "$BUILD_DIR" --config "$CONFIG" -j "$JOBS"
else
  cmake --build "$BUILD_DIR" --config "$CONFIG" --target "$TARGET" -j "$JOBS"
fi
