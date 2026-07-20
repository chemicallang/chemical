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
TEST_NEGATIVE=false
MODE="debug_quick"
NO_CACHE="--no-cache"
INCREMENTAL=false
EMIT_C=false
USE_C=false
DEBUG_FLAG=false
GDB=false
BT_MODE="none"
RECOMPILE_PLUGINS="-frecompile-plugins"
BENCHMARK=false
BENCHMARK_FILES=false
BENCHMARK_MODULES=false
VERBOSE=false
PRINT_CMD=false

usage() {
  echo "Usage: $0 [options]"
  echo ""
  echo "Options:"
  echo "  --tcc                   Use TCCCompiler (TinyCC backend)"
  echo "  --llvm                  Use Compiler (LLVM/Clang backend)"
  echo "  --interpret             Run tests via interpretation job (no executable produced)"
  echo "  --negative              Run negative (safety) tests only"
  echo "  --libs                  Include library tests (passes --arg-test-libs)"
  echo "  -o <path>               Custom output executable path"
  echo "  --no-run                Build test executable only, do not run"
  echo "  --no-build              Skip building compiler target, use existing binary"
  echo "                          WARNING: Only use when no C++ changes have been made."
  echo "  --mode <mode>           Compilation mode (default: debug_quick)"
  echo "  --cache                 Use cached objects (default: --no-cache)"
  echo "  --incremental           Use incremental compilation"
  echo "  --emit-c                Emit C translation output"
  echo "  --use-c                 Translate to C and compile with embedded Clang (Compiler only)"
  echo "  --cached-plugins        Skip recompiling CBI plugins (default: -frecompile-plugins)"
  echo "  --bm                    Run compilation benchmark (print times per phase)"
  echo "  --bm-files              Run per-file compilation benchmark"
  echo "  --bm-modules            Run per-module compilation benchmark"
  echo "  -g                      Pass -g to the compiler (debug symbols)"
  echo "  --gdb                   Run tests under interactive GDB (implies -g)"
  echo "  -bt, --bt               Run tests under GDB -batch, print backtrace on crash (implies -g)"
  echo "  -bt-full, --bt-full     Run tests under GDB -batch with full bt, registers, disasm (implies -g)"
  echo "  -v                      Pass -v to the compiler (verbose output)"
  echo "  --print-command         Print the compiler command without running it"
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
    --negative) TEST_NEGATIVE=true ;;
    --libs) TEST_LIBS=true ;;
    -o) TEST_OUT_NAME="$2"; shift ;;
    --no-run) RUN_TESTS=false ;;
    --no-build) BUILD_TARGET=false ;;
    --mode) MODE="$2"; shift ;;
    --cache) NO_CACHE="" ;;
    --emit-c) EMIT_C=true ;;
    --use-c) USE_C=true ;;
    --incremental) INCREMENTAL=true ;;
    --cached-plugins) RECOMPILE_PLUGINS="" ;;
    --bm) BENCHMARK=true ;;
    --bm-files) BENCHMARK_FILES=true ;;
    --bm-modules) BENCHMARK_MODULES=true ;;
    -g) DEBUG_FLAG=true ;;
    --gdb) GDB=true; DEBUG_FLAG=true ;;
    -bt|--bt) BT_MODE="bt"; DEBUG_FLAG=true ;;
    -bt-full|--bt-full) BT_MODE="bt-full"; DEBUG_FLAG=true ;;
    -v) VERBOSE=true ;;
    --print-command) PRINT_CMD=true ;;
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

# ----------------------------------------------------------
# GDB backtrace helpers
# ----------------------------------------------------------
run_under_gdb_batch() {
  local mode="$1"
  shift
  # Tests run in child processes via IPC (test_runner forks @test functions),
  # so we need follow-fork-mode child to catch crashes in the actual test process.
  # Use || true to prevent set -e from aborting on gdb's non-zero exit (program crashed).
  if [ "$mode" = "bt" ]; then
    gdb -batch \
      -ex "set follow-fork-mode child" \
      -ex "run" \
      -ex "bt full" \
      --args "$@" || true
  elif [ "$mode" = "bt-full" ]; then
    gdb -batch \
      -ex "set pagination off" \
      -ex "set follow-fork-mode child" \
      -ex "run" \
      -ex "thread apply all bt full" \
      -ex "info registers" \
      -ex "x/16i \$pc" \
      -ex "info locals" \
      -ex "info args" \
      --args "$@" || true
  fi
}

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
if [ "$TEST_NEGATIVE" = true ]; then
  NEG_OUT="$TEST_OUT_DIR/negative-tests-tcc.exe"
  CMD=("$COMPILER_BIN" "$TEST_BUILD_LAB" -o "$NEG_OUT" --mode "$MODE" --arg-negative)
  [ -n "$NO_CACHE" ] && CMD+=("$NO_CACHE")
  [ "$EMIT_C" = true ] && CMD+=("--emit-c")
  [ "$DEBUG_FLAG" = true ] && CMD+=("-g")
  [ "$VERBOSE" = true ] && CMD+=("-v")
  if [ "$PRINT_CMD" = true ]; then
    echo "${CMD[@]}"
    exit 0
  fi
  echo "==> Building negative tests..."
  echo "${CMD[@]}"
  "${CMD[@]}"
  if [ "$RUN_TESTS" = true ]; then
    if [ ! -f "$NEG_OUT" ]; then
      echo "Error: Negative test executable not found at $NEG_OUT"
      exit 1
    fi
    echo "==> Running negative tests..."
    "$NEG_OUT"
  fi
elif [ "$TEST_INTERPRET" = true ]; then
  # Interpretation mode: run the compiler which interprets main() directly
  CMD=("$COMPILER_BIN" "$TEST_BUILD_LAB" --mode "$MODE" --arg-interpret)
  [ -n "$NO_CACHE" ] && CMD+=("$NO_CACHE")
  [ "$EMIT_C" = true ] && CMD+=("--emit-c")
  [ "$DEBUG_FLAG" = true ] && CMD+=("-g")
  [ "$BENCHMARK" = true ] && CMD+=("-bm")
  [ "$BENCHMARK_FILES" = true ] && CMD+=("-bm-files")
  [ "$BENCHMARK_MODULES" = true ] && CMD+=("-bm-modules")
  [ "$VERBOSE" = true ] && CMD+=("-v")
  if [ "$PRINT_CMD" = true ]; then
    echo "${CMD[@]}"
    exit 0
  fi
  echo "==> Interpreting tests..."
  if [ "$GDB" = true ]; then
    echo "gdb --args ${CMD[@]}"
    gdb --args "${CMD[@]}"
  elif [ "$BT_MODE" != "none" ]; then
    run_under_gdb_batch "$BT_MODE" "${CMD[@]}"
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
  [ "$INCREMENTAL" = true ] && CMD+=("--incremental")
  [ "$DEBUG_FLAG" = true ] && CMD+=("-g")
  [ "$BENCHMARK" = true ] && CMD+=("-bm")
  [ "$BENCHMARK_FILES" = true ] && CMD+=("-bm-files")
  [ "$BENCHMARK_MODULES" = true ] && CMD+=("-bm-modules")
  [ "$VERBOSE" = true ] && CMD+=("-v")
  if [ "$TEST_LIBS" = true ]; then
    CMD+=("--arg-test-libs")
    [ -n "$RECOMPILE_PLUGINS" ] && CMD+=("$RECOMPILE_PLUGINS")
  fi
  if [ "$PRINT_CMD" = true ]; then
    echo "${CMD[@]}"
    exit 0
  fi
  echo "==> Compiling tests..."
  echo "${CMD[@]}"
  if [ "$BT_MODE" != "none" ]; then
    # Crash happens in the COMPILER BINARY during compilation, not the test executable
    run_under_gdb_batch "$BT_MODE" "${CMD[@]}"
  else
    "${CMD[@]}"
  fi

  if [ "$RUN_TESTS" = true ]; then
    if [ ! -f "$TEST_OUT" ]; then
      echo "Error: Test executable not found at $TEST_OUT"
      exit 1
    fi
    echo "==> Running tests..."
    if [ "$GDB" = true ]; then
      gdb "$TEST_OUT"
    elif [ "$BT_MODE" != "none" ]; then
      run_under_gdb_batch "$BT_MODE" "$TEST_OUT"
    else
      "$TEST_OUT"
    fi
  fi
fi
