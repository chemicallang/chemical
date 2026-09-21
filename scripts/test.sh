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
TEST_PLUGINS=false
TEST_INTERPRET=false
TEST_NEGATIVE=false
TEST_TLS=false
TEST_PROCESS=false
TEST_WEBVIEW=false
TEST_UNIVERSAL_TESTS=false
TEST_SERVER=false
TEST_LIBS=false
TEST_ASYNC=false
RUN_ALL=false
INCLUDE_TLS=false
COMPILE_TARGET=""
MODE="debug_quick"
NO_CACHE="--no-cache"
INCREMENTAL=false
EMIT_C=false
USE_C=false
SANITIZER=""
DEBUG_FLAG=false
GDB=false
BT_MODE="none"
RECOMPILE_PLUGINS="-frecompile-plugins"
BENCHMARK=false
BENCHMARK_FILES=false
BENCHMARK_MODULES=false
VERBOSE=false
PRINT_CMD=false
TEST_IDS=""
TEST_NAMES=""
SKIP_SEQUENTIAL=false
UT_HEADED=false

usage() {
  echo "Usage: $0 [options]"
  echo ""
  echo "Options:"
  echo "  --tcc                   Use TCCCompiler (TinyCC backend)"
  echo "  --llvm                  Use Compiler (LLVM/Clang backend)"
  echo "  --interpret             Run tests via interpretation job (no executable produced)"
  echo "  --negative              Run negative (safety) tests only"
  echo "  --plugins               Include compiler plugin tests (passes --arg-test-plugins)"
  echo "  --tls                   Build & run the TLS integration test suite (passes --arg-test-tls)"
  echo "  --process               Build & run the process/environment test suite (passes --arg-test-process)"
  echo "  --webview               Build & run the webview test suite (passes --arg-test-webview)"
  echo "  --universal-tests       Build & run the universal component tests in a WebView (passes --arg-test-universal-tests)"
  echo "  --ut-headed             With --universal-tests, show the WebView window (default: hidden)"
  echo "  --server                Build & run the server test suite (passes --arg-test-server)"
  echo "  --libs                  Build & run the library test suite (passes --arg-test-libs)"
  echo "  --async                 Build & run the async/await suite (C or LLVM)"
  echo "  --all                   Run every suite (main, interpret, negative, plugins, async,"
  echo "                          libs, process, server, webview) and print a summary table."
  echo "                          Uses --tcc unless --llvm is also given. The slow 'tls'"
  echo "                          suite is SKIPPED unless --include-tls is given."
  echo "  --include-tls           With --all, also run the slow 'tls' suite (minutes)."
  echo "  --target <triple>       Pass --target <triple> to the compiler (optional, omitted if empty)"
  echo "  -o <path>               Custom output executable path"
  echo "  --no-run                Build test executable only, do not run"
  echo "  --no-build              Skip building compiler target, use existing binary"
  echo "                          WARNING: Only use when no C++ changes have been made."
  echo "  --mode <mode>           Compilation mode (default: debug_quick)"
  echo "  --cache                 Use cached objects (default: --no-cache)"
  echo "  --incremental           Use incremental compilation"
  echo "  --emit-c                Emit C translation output"
  echo "  --use-c                 Translate to C and compile with embedded Clang (Compiler only)"
  echo "  --sanitize <sanitizer>  Enable sanitizer on generated binary: address, thread, memory, undefined, leak (LLVM only)"
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
  echo "  --test-ids <ids>        Comma-separated list of @test IDs to run (e.g. --test-ids 1,2,3)"
  echo "  --test-names <names>    Comma-separated list of @test function names to run"
  echo "  --skip-sequential       Skip sequential inline tests, only run @test runner"
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
    --plugins) TEST_PLUGINS=true ;;
    --tls) TEST_TLS=true ;;
    --process) TEST_PROCESS=true ;;
    --webview) TEST_WEBVIEW=true ;;
    --universal-tests) TEST_UNIVERSAL_TESTS=true ;;
    --server) TEST_SERVER=true ;;
    --libs) TEST_LIBS=true ;;
    --async) TEST_ASYNC=true ;;
    --all) RUN_ALL=true ;;
    --include-tls) INCLUDE_TLS=true ;;
    --target) COMPILE_TARGET="$2"; shift ;;
    -o) TEST_OUT_NAME="$2"; shift ;;
    --no-run) RUN_TESTS=false ;;
    --no-build) BUILD_TARGET=false ;;
    --mode) MODE="$2"; shift ;;
    --cache) NO_CACHE="" ;;
    --emit-c) EMIT_C=true ;;
    --use-c) USE_C=true ;;
    --sanitize) SANITIZER="$2"; shift ;;
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
    --test-ids) TEST_IDS="$2"; shift ;;
    --test-names) TEST_NAMES="$2"; shift ;;
    --skip-sequential) SKIP_SEQUENTIAL=true ;;
    --ut-headed) UT_HEADED=true ;;
    --help|-h) usage ;;
    *) echo "Unknown option: $1"; usage ;;
  esac
  shift
done

if [ -z "$TARGET" ]; then
  if [ "$RUN_ALL" = true ]; then
    # A fresh-clone sanity run should work without LLVM; default to TinyCC.
    TARGET="TCCCompiler"
    COMPILER_BIN="$BUILD_DIR/TCCCompiler"
  else
    echo "Error: Specify --tcc or --llvm"
    usage
  fi
fi

if [ -n "$SANITIZER" ] && [ "$TARGET" != "Compiler" ]; then
  echo "Error: --sanitize requires --llvm (TCC does not support sanitizers)"
  exit 1
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

# ----------------------------------------------------------
# --all : run every suite, stream output, report a summary
# ----------------------------------------------------------
if [ -t 1 ]; then
  C_BOLD=$'\033[1m'; C_DIM=$'\033[2m'; C_RESET=$'\033[0m'
  C_RED=$'\033[31m'; C_GREEN=$'\033[32m'; C_YELLOW=$'\033[33m'
  C_CYAN=$'\033[36m'; C_MAGENTA=$'\033[35m'; C_BLUE=$'\033[34m'
else
  C_BOLD=""; C_DIM=""; C_RESET=""; C_RED=""; C_GREEN=""; C_YELLOW=""
  C_CYAN=""; C_MAGENTA=""; C_BLUE=""
fi

strip_ansi() { sed -E $'s/\033\\[[0-9;]*[A-Za-z]//g'; }

# Echo "<total> <passed> <failed>" parsed from a suite log, or nothing if the
# suite never printed a summary (e.g. it failed to compile).
parse_suite_counts() {
  local log="$1"
  [ -f "$log" ] || return 0
  local clean line t p f
  clean=$(strip_ansi < "$log" || true)
  # Dedicated suites (test library): "Summary: N tests - P passed, F failed"
  line=$(printf '%s\n' "$clean" | grep -E 'Summary: [0-9]+ tests' | tail -n 1 || true)
  if [ -n "$line" ]; then
    t=$(printf '%s\n' "$line" | sed -E 's/.*Summary: ([0-9]+) tests.*/\1/')
    p=$(printf '%s\n' "$line" | sed -E 's/.* ([0-9]+) passed.*/\1/')
    f=$(printf '%s\n' "$line" | sed -E 's/.* ([0-9]+) failed.*/\1/')
    echo "$t $p $f"
    return 0
  fi
  # Main / interpret suites: "Total N Passed P Failed F"
  line=$(printf '%s\n' "$clean" | grep -E 'Total [0-9]+ Passed [0-9]+ Failed [0-9]+' | tail -n 1 || true)
  if [ -n "$line" ]; then
    t=$(printf '%s\n' "$line" | sed -E 's/.*Total ([0-9]+).*/\1/')
    p=$(printf '%s\n' "$line" | sed -E 's/.*Passed ([0-9]+).*/\1/')
    f=$(printf '%s\n' "$line" | sed -E 's/.*Failed ([0-9]+).*/\1/')
    echo "$t $p $f"
    return 0
  fi
  return 0
}

format_duration() {
  local secs="$1"
  if [ "$secs" -ge 60 ]; then
    echo "$((secs / 60))m$((secs % 60))s"
  else
    echo "${secs}s"
  fi
}

run_all_suites() {
  local backend_flag="$1"
  local backend_name="$2"

  local log_dir="$TEST_OUT_DIR/all-logs"
  rm -rf "$log_dir"
  mkdir -p "$log_dir"

  echo ""
  echo "${C_BOLD}${C_CYAN}================================================================${C_RESET}"
  echo "${C_BOLD}${C_CYAN}  Chemical test suite — running ALL suites${C_RESET}"
  echo "${C_BOLD}${C_CYAN}  backend: ${backend_name}${C_RESET}"
  echo "${C_BOLD}${C_CYAN}  logs:    ${log_dir}${C_RESET}"
  if [ "$INCLUDE_TLS" = false ]; then
    echo "${C_DIM}  note: the slow 'tls' suite is skipped.${C_RESET}"
    echo "${C_DIM}        pass --include-tls to run it as well (it takes minutes).${C_RESET}"
  fi
  echo "${C_BOLD}${C_CYAN}================================================================${C_RESET}"

  # Build the compiler once; the per-suite runs use --no-build.
  if [ "$BUILD_TARGET" = true ]; then
    echo ""
    echo "${C_BOLD}==> Building ${backend_name}...${C_RESET}"
    if ! cmake --build "$BUILD_DIR" --config Debug --target "$TARGET" -j "$JOBS"; then
      echo "${C_RED}${C_BOLD}Failed to build ${backend_name}. Aborting --all.${C_RESET}"
      return 1
    fi
  fi

  # The slow `tls` suite is opt-in (`--include-tls`); everything else always runs.
  local names=("main" "interpret" "negative" "plugins" "async" "libs" "process" "server")
  local flags=("" "--interpret" "--negative" "--plugins" "--async" "--libs" "--process" "--server")
  if [ "$INCLUDE_TLS" = true ]; then
    names+=("tls")
    flags+=("--tls")
  fi
  names+=("webview")
  flags+=("--webview")

  local n_suites=${#names[@]}
  local any_failed=0
  local idx=0
  local -a r_name=() r_status=() r_total=() r_passed=() r_failed=() r_time=()

  while [ "$idx" -lt "$n_suites" ]; do
    local name="${names[$idx]}"
    local flag="${flags[$idx]}"
    idx=$((idx + 1))

    echo ""
    echo "${C_BOLD}${C_MAGENTA}----------------------------------------------------------------${C_RESET}"
    echo "${C_BOLD}${C_MAGENTA}  [${idx}/${n_suites}] suite: ${name}${C_RESET}"
    echo "${C_BOLD}${C_MAGENTA}----------------------------------------------------------------${C_RESET}"

    local log="$log_dir/${name}.log"
    # No-cache is the default; do not forward a flag the parser rejects.
    local -a sub=("$0" "$backend_flag" "--no-build" "--mode" "$MODE")
    if [ -n "$flag" ]; then sub+=("$flag"); fi

    local start_ts end_ts dur status
    start_ts=$(date +%s)
    status=0
    # Quiet by default (log only) so the summary stays readable; `-v` streams
    # the suite output live. Never let a failing suite abort the whole run.
    set +e
    if [ "$VERBOSE" = true ]; then
      "${sub[@]}" 2>&1 | tee "$log"
      status=${PIPESTATUS[0]}
    else
      echo "${C_DIM}  running...${C_RESET}"
      "${sub[@]}" > "$log" 2>&1
      status=$?
    fi
    set -e
    end_ts=$(date +%s)
    dur=$((end_ts - start_ts))

    local counts total passed failed suite_status
    counts=$(parse_suite_counts "$log")
    total=""; passed=""; failed=""
    if [ -n "$counts" ]; then
      read -r total passed failed <<< "$counts" || true
    fi
    if [ "$status" -eq 0 ] && [ -n "$total" ] && [ "${failed:-1}" -eq 0 ]; then
      suite_status="ok"
    elif [ -n "$total" ]; then
      suite_status="fail"
    else
      suite_status="error"
    fi

    r_name+=("$name"); r_status+=("$suite_status")
    r_total+=("${total:-?}"); r_passed+=("${passed:-?}"); r_failed+=("${failed:-?}")
    r_time+=("$(format_duration "$dur")")

    case "$suite_status" in
      ok)    echo "${C_GREEN}${C_BOLD}  ==> ${name}: ${passed}/${total} passed  ($(format_duration "$dur"))${C_RESET}" ;;
      fail)  echo "${C_RED}${C_BOLD}  ==> ${name}: ${failed} failed, ${passed}/${total} passed  ($(format_duration "$dur"))${C_RESET}"; any_failed=1 ;;
      error) echo "${C_RED}${C_BOLD}  ==> ${name}: did not run (compile/build error?) — see ${log}  ($(format_duration "$dur"))${C_RESET}"; any_failed=1 ;;
    esac
    # On failure, name the failing tests when the suite printed them; otherwise
    # (compile/build error) show the tail of the log.
    if [ "$suite_status" != "ok" ]; then
      local fails
      fails=$(strip_ansi < "$log" | awk '/^  - /{name=$0} /^    FAIL$/{print name}' | head -n 20 || true)
      if [ -n "$fails" ]; then
        echo "${C_DIM}  --- failing tests (${name}) ---${C_RESET}"
        printf '%s\n' "$fails" | sed 's/^/    /'
      else
        echo "${C_DIM}  --- last log lines (${name}) ---${C_RESET}"
        strip_ansi < "$log" | grep -v '^[[:space:]]*$' | tail -n 15 | sed 's/^/    /'
      fi
      echo "${C_DIM}  --- full log: ${log} ---${C_RESET}"
    fi
  done

  # ---- final table ----
  echo ""
  echo "${C_BOLD}${C_CYAN}================================================================${C_RESET}"
  echo "${C_BOLD}${C_CYAN}  SUMMARY  (backend: ${backend_name})${C_RESET}"
  echo "${C_BOLD}${C_CYAN}================================================================${C_RESET}"
  printf "${C_BOLD}  %-10s %-7s %8s %8s %8s %8s${C_RESET}\n" "SUITE" "STATUS" "TOTAL" "PASSED" "FAILED" "TIME"
  printf "  %s\n" "------------------------------------------------------------"

  local grand_total=0 grand_passed=0 grand_failed=0
  local i=0
  while [ "$i" -lt "$n_suites" ]; do
    local color
    case "${r_status[$i]}" in
      ok)    color="$C_GREEN" ;;
      fail)  color="$C_RED" ;;
      *)     color="$C_YELLOW" ;;
    esac
    printf "  ${C_BOLD}%-10s${C_RESET} ${color}%-7s${C_RESET} %8s %8s %8s %8s\n" \
      "${r_name[$i]}" "${r_status[$i]}" "${r_total[$i]}" "${r_passed[$i]}" "${r_failed[$i]}" "${r_time[$i]}"
    if [ "${r_total[$i]}" != "?" ]; then
      grand_total=$((grand_total + r_total[i]))
      grand_passed=$((grand_passed + r_passed[i]))
      grand_failed=$((grand_failed + r_failed[i]))
    fi
    i=$((i + 1))
  done

  printf "  %s\n" "------------------------------------------------------------"
  local grand_color="$C_GREEN"
  [ "$any_failed" -ne 0 ] && grand_color="$C_RED"
  printf "  ${C_BOLD}%-10s${C_RESET} ${grand_color}%-7s${C_RESET} %8s %8s %8s\n" \
    "TOTAL" "$([ "$any_failed" -eq 0 ] && echo 'PASS' || echo 'FAIL')" \
    "$grand_total" "$grand_passed" "$grand_failed" ""

  if [ "$any_failed" -eq 0 ]; then
    echo ""
    echo "${C_GREEN}${C_BOLD}  All suites passed.${C_RESET}"
  else
    echo ""
    echo "${C_RED}${C_BOLD}  Some suites failed — see ${log_dir} for full logs.${C_RESET}"
  fi
  if [ "$INCLUDE_TLS" = false ]; then
    echo "${C_YELLOW}  The 'tls' suite was skipped. Re-run with --include-tls to test it too${C_RESET}"
    echo "${C_YELLOW}  (it is slow — minutes on a loaded machine).${C_RESET}"
  fi

  return "$any_failed"
}

if [ "$RUN_ALL" = true ]; then
  rc=0
  set +e
  if [ "$TARGET" = "Compiler" ]; then
    run_all_suites "--llvm" "Compiler (LLVM)"
  else
    run_all_suites "--tcc" "TCCCompiler (TinyCC)"
  fi
  rc=$?
  set -e
  exit "$rc"
fi

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
  if [ -n "$COMPILE_TARGET" ]; then
    CMD+=("--target" "$COMPILE_TARGET")
  fi
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
  if [ -n "$COMPILE_TARGET" ]; then
    CMD+=("--target" "$COMPILE_TARGET")
  fi
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
  [ -n "$SANITIZER" ] && CMD+=("--sanitize" "$SANITIZER")
  [ "$DEBUG_FLAG" = true ] && CMD+=("-g")
  [ "$BENCHMARK" = true ] && CMD+=("-bm")
  [ "$BENCHMARK_FILES" = true ] && CMD+=("-bm-files")
  [ "$BENCHMARK_MODULES" = true ] && CMD+=("-bm-modules")
  [ "$VERBOSE" = true ] && CMD+=("-v")
  if [ "$TEST_PLUGINS" = true ]; then
    CMD+=("--arg-test-plugins")
    [ -n "$RECOMPILE_PLUGINS" ] && CMD+=("$RECOMPILE_PLUGINS")
  fi
  if [ "$TEST_TLS" = true ]; then
    CMD+=("--arg-test-tls")
  fi
  if [ "$TEST_PROCESS" = true ]; then
    CMD+=("--arg-test-process")
  fi
  if [ "$TEST_WEBVIEW" = true ]; then
    CMD+=("--arg-test-webview")
  fi
  if [ "$TEST_UNIVERSAL_TESTS" = true ]; then
    CMD+=("--arg-test-universal-tests")
  fi
  if [ "$TEST_SERVER" = true ]; then
    CMD+=("--arg-test-server")
  fi
  if [ "$TEST_LIBS" = true ]; then
    CMD+=("--arg-test-libs")
  fi
  if [ "$TEST_ASYNC" = true ]; then
    CMD+=("--arg-test-async")
  fi
  if [ -n "$COMPILE_TARGET" ]; then
    CMD+=("--target" "$COMPILE_TARGET")
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
    # Build test runner arguments
    declare -a TEST_ARGS=()
    if [ -n "$TEST_IDS" ]; then
      TEST_ARGS+=("--test-ids" "$TEST_IDS")
    fi
    if [ -n "$TEST_NAMES" ]; then
      TEST_ARGS+=("--test-names" "$TEST_NAMES")
    fi
    if [ "$SKIP_SEQUENTIAL" = true ]; then
      TEST_ARGS+=("--skip-sequential")
    fi
    if [ "$UT_HEADED" = true ]; then
      TEST_ARGS+=("--ut-headed")
    fi
    echo "==> Running tests${TEST_ARGS[*]+ ${TEST_ARGS[*]}}..."
    if [ "$GDB" = true ]; then
      gdb --args "$TEST_OUT" "${TEST_ARGS[@]}"
    elif [ "$BT_MODE" != "none" ]; then
      run_under_gdb_batch "$BT_MODE" "$TEST_OUT" "${TEST_ARGS[@]}"
    else
      "$TEST_OUT" "${TEST_ARGS[@]}"
    fi
  fi
fi
