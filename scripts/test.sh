#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# On Windows (Git Bash / MSYS2), ensure MSVC flags are not mangled by MSYS2
# and INCLUDE/LIB are set up correctly for cl.exe
source "$SCRIPT_DIR/msvc_env.sh"

BUILD_DIR="cmake-build-debug"
JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
TARGET=""
TEST_BUILD_LAB="lang/tests/build.lab"
TEST_OUT_DIR="lang/tests/build"
TEST_OUT_NAME=""
RUN_TESTS=true
BUILD_TARGET=true
TEST_LIBS=false
TEST_INTERPRET=false
MODE="debug_quick"
NO_CACHE="--no-cache"
EMIT_C=false
USE_C=false
DEBUG_FLAG=false
GDB=false
RECOMPILE_PLUGINS="-frecompile-plugins"

usage() {
  echo "Usage: $0 [options]"
  echo ""
  echo "Options:"
  echo "  --tcc                   Use TCCCompiler (TinyCC backend)"
  echo "  --llvm                  Use Compiler (LLVM/Clang backend)"
  echo "  --interpret             Run tests via interpretation job (no executable produced)"
  echo "  --libs                  Include library tests (passes --arg-test-libs)"
  echo "  -o <path>               Custom output executable path"
  echo "  --no-run                Build test executable only, do not run"
  echo "  --no-build              Skip building compiler target, use existing binary"
  echo "                          WARNING: Only use when no C++ changes have been made."
  echo "  --mode <mode>           Compilation mode (default: debug_quick)"
  echo "  --cache                 Use cached objects (default: --no-cache)"
  echo "  --emit-c                Emit C translation output"
  echo "  --use-c                 Translate to C and compile with embedded Clang (Compiler only)"
  echo "  --cached-plugins        Skip recompiling CBI plugins (default: -frecompile-plugins)"
  echo "  -g                      Pass -g to the compiler (debug symbols)"
  echo "  --gdb                   Run tests under GDB (implies -g)"
  echo "  -j N                    Number of parallel jobs (default: $JOBS)"
  echo "  --help, -h              Show this help"
  exit 1
}

while [ $# -gt 0 ]; do
  case "$1" in
    --tcc)
      TARGET="TCCCompiler"
      COMPILER_BIN="$BUILD_DIR/TCCCompiler"
      ;;
    --llvm)
      TARGET="Compiler"
      COMPILER_BIN="$BUILD_DIR/Compiler"
      ;;
    --interpret) TEST_INTERPRET=true ;;
    --libs) TEST_LIBS=true ;;
    -o) TEST_OUT_NAME="$2"; shift ;;
    --no-run) RUN_TESTS=false ;;
    --no-build) BUILD_TARGET=false ;;
    --mode) MODE="$2"; shift ;;
    --cache) NO_CACHE="" ;;
    --emit-c) EMIT_C=true ;;
    --use-c) USE_C=true ;;
    --cached-plugins) RECOMPILE_PLUGINS="" ;;
    -g) DEBUG_FLAG=true ;;
    --gdb) GDB=true; DEBUG_FLAG=true ;;
    -j) JOBS="$2"; shift ;;
    --help|-h) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
  shift
done

if [ -z "$TARGET" ]; then
  echo "Error: Specify --tcc or --llvm"
  usage
fi

# Append .exe on Windows
case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) COMPILER_BIN="${COMPILER_BIN}.exe" ;;
esac

# Build the compiler target if requested
if [ "$BUILD_TARGET" = true ]; then
  echo "==> Building $TARGET..."
  cmake --build "$BUILD_DIR" --config Debug --target "$TARGET" -j "$JOBS"
fi

if [ ! -f "$COMPILER_BIN" ]; then
  echo "Error: Compiler binary not found at $COMPILER_BIN"
  echo "Build it with: $0 --$([ "$TARGET" = "Compiler" ] && echo "llvm" || echo "tcc") (without --no-build)"
  exit 1
fi

# Determine output path
if [ -n "$TEST_OUT_NAME" ]; then
  TEST_OUT="$TEST_OUT_NAME"
else
  if [ "$TARGET" = "Compiler" ]; then
    TEST_OUT="$TEST_OUT_DIR/tests.exe"
  else
    TEST_OUT="$TEST_OUT_DIR/tests-tcc.exe"
  fi
fi

# Build the test command
if [ "$TEST_INTERPRET" = true ]; then
  # Interpretation mode: run the compiler which interprets main() directly
  CMD=("$COMPILER_BIN" "$TEST_BUILD_LAB" --mode "$MODE" --arg-interpret)
  [ -n "$NO_CACHE" ] && CMD+=("$NO_CACHE")
  [ "$EMIT_C" = true ] && CMD+=("--emit-c")
  [ "$DEBUG_FLAG" = true ] && CMD+=("-g")
  echo "==> Interpreting tests..."
  if [ "$GDB" = true ]; then
    echo "gdb --args ${CMD[@]}"
    gdb --args "${CMD[@]}"
  else
    echo "${CMD[@]}"
    "${CMD[@]}"
  fi
else
  # Normal compilation mode: build executable then run
  CMD=("$COMPILER_BIN" "$TEST_BUILD_LAB" -o "$TEST_OUT" --mode "$MODE")
  [ -n "$NO_CACHE" ] && CMD+=("$NO_CACHE")
  [ "$EMIT_C" = true ] && CMD+=("--emit-c")
  [ "$USE_C" = true ] && CMD+=("--use-c")
  [ "$DEBUG_FLAG" = true ] && CMD+=("-g")
  if [ "$TEST_LIBS" = true ]; then
    CMD+=("--arg-test-libs")
    [ -n "$RECOMPILE_PLUGINS" ] && CMD+=("$RECOMPILE_PLUGINS")
  fi

  echo "==> Compiling tests..."
  echo "${CMD[@]}"
  "${CMD[@]}"

  if [ "$RUN_TESTS" = true ]; then
    if [ ! -f "$TEST_OUT" ]; then
      echo "Error: Test executable not found at $TEST_OUT"
      exit 1
    fi
    echo "==> Running tests..."
    if [ "$GDB" = true ]; then
      gdb "$TEST_OUT"
    else
      "$TEST_OUT"
    fi
  fi
fi
