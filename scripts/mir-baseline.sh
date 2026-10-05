# mir-baseline.sh
#
# Record the MIR performance / pass-rate / memory baseline described in
# lang/docs/mir-implementation-plan.md §11.
#
# The baseline is the contract every MIR change must match. Run this on an IDLE
# machine, before switching the pipeline to MIR (PR 0) and after each significant
# MIR milestone.
#
# Usage:
#   ./scripts/mir-baseline.sh --tcc --all
#   ./scripts/mir-baseline.sh --tcc --suite main --suite libs
#   ./scripts/mir-baseline.sh --tcc --all --no-build
#   ./scripts/mir-baseline.sh --llvm --all
#
# Outputs (default dir: lang/docs/baseline/):
#   <backend>-<suite>.log        raw suite output
#   <backend>-<suite>.json       machine-readable {suite,status,total,passed,failed,seconds,peak_bytes}
#   <backend>-summary.md         Markdown table for mir-implementation-plan.md §11.3
#
# Notes:
#   * Do NOT change the measurement procedure to make numbers look better;
#     update the baseline only with an explicit recorded decision.
#   * Memory is measured as the peak working set of the compiler process(es)
#     during the suite (Windows helper) or via GNU `time -v` where available.

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

TARGET="TCCCompiler"
COMPILER_BIN="cmake-build-debug/TCCCompiler"
BACKEND="tcc"
MODE="debug_quick"
OUT_DIR="lang/docs/baseline"
BUILD=true
JOBS="$(nproc 2>/dev/null || echo 4)"
SUITES=()
ALL=false

usage() {
  sed -n '2,40p' "$0" | sed 's/^# \{0,1\}//'
  exit 1
}

while [ $# -gt 0 ]; do
  case "$1" in
    --tcc)   TARGET="TCCCompiler"; COMPILER_BIN="cmake-build-debug/TCCCompiler"; BACKEND="tcc"; shift ;;
    --llvm)  TARGET="Compiler";    COMPILER_BIN="cmake-build-debug/Compiler";    BACKEND="llvm"; shift ;;
    --suite) SUITES+=("$2"); shift 2 ;;
    --all)   ALL=true; shift ;;
    --no-build) BUILD=false; shift ;;
    --mode)  MODE="$2"; shift 2 ;;
    --out)   OUT_DIR="$2"; shift 2 ;;
    -j)      JOBS="$2"; shift 2 ;;
    -h|--help) usage ;;
    *) echo "Unknown option: $1" >&2; usage ;;
  esac
done

if [ "$ALL" = true ]; then
  SUITES=(main interpret negative plugins async libs regexp process server webview universal)
fi
if [ "${#SUITES[@]}" -eq 0 ]; then
  echo "No suites selected. Use --all or --suite <name>." >&2
  usage
fi

case "$(uname -s)" in
  MINGW*|MSYS*|CYGWIN*) COMPILER_BIN="${COMPILER_BIN}.exe" ;;
esac

mkdir -p "$OUT_DIR"

suite_flag() {
  case "$1" in
    main) echo "" ;;
    interpret) echo "--interpret" ;;
    negative) echo "--negative" ;;
    plugins) echo "--plugins" ;;
    async) echo "--async" ;;
    libs) echo "--libs" ;;
    regexp) echo "--regexp" ;;
    process) echo "--process" ;;
    server) echo "--server" ;;
    webview) echo "--webview" ;;
    universal|universal-tests) echo "--universal" ;;
    tls) echo "--tls" ;;
    *) echo "" ;;
  esac
}

# Parse "total passed failed" from a log, or "".
parse_counts() {
  local log="$1" line t p f
  [ -f "$log" ] || return 0
  local clean
  clean="$(sed -E $'s/\033\\[[0-9;]*[A-Za-z]//g' < "$log")"
  line="$(printf '%s\n' "$clean" | grep -E 'Summary: [0-9]+ tests' | tail -n 1 || true)"
  if [ -n "$line" ]; then
    t="$(printf '%s\n' "$line" | sed -E 's/.*Summary: ([0-9]+) tests.*/\1/')"
    p="$(printf '%s\n' "$line" | sed -E 's/.* ([0-9]+) passed.*/\1/')"
    f="$(printf '%s\n' "$line" | sed -E 's/.* ([0-9]+) failed.*/\1/')"
    echo "$t $p $f"; return 0
  fi
  line="$(printf '%s\n' "$clean" | grep -E 'Total [0-9]+ Passed [0-9]+ Failed [0-9]+' | tail -n 1 || true)"
  if [ -n "$line" ]; then
    t="$(printf '%s\n' "$line" | sed -E 's/.*Total ([0-9]+).*/\1/')"
    p="$(printf '%s\n' "$line" | sed -E 's/.*Passed ([0-9]+).*/\1/')"
    f="$(printf '%s\n' "$line" | sed -E 's/.*Failed ([0-9]+).*/\1/')"
    echo "$t $p $f"; return 0
  fi
}

# Run a command line (capturing output to $2) and echo JSON
# {exit,peak_bytes,ms}. Prefers the PowerShell sampler on Windows; falls back to
# GNU time -v, then to plain wall-clock timing.
measure_wrapped() {
  local cmdline="$1" logfile="$2" out_json
  local ps
  ps="$(command -v pwsh || command -v powershell || true)"
  if [ -n "$ps" ] && [ -f "$SCRIPT_DIR/mir-baseline-mem.ps1" ]; then
    out_json="$("$ps" -NoProfile -File "$SCRIPT_DIR/mir-baseline-mem.ps1" -CommandLine "$cmdline" -Log "$logfile" 2>/dev/null | tail -n 1 || true)"
    if [ -n "$out_json" ]; then echo "$out_json"; return 0; fi
  fi
  if command -v /usr/bin/time >/dev/null 2>&1 && /usr/bin/time -v true >/dev/null 2>&1; then
    local tf; tf="$(mktemp)"
    /usr/bin/time -v -o "$tf" bash -c "$cmdline" > "$logfile" 2>&1
    local rc=$?
    local kb; kb="$(grep -E 'Maximum resident set size' "$tf" | grep -oE '[0-9]+' | head -n1 || echo 0)"
    rm -f "$tf"
    printf '{"exit":%s,"peak_bytes":%s,"ms":null}\n' "$rc" "$(( ${kb:-0} * 1024 ))"
    return 0
  fi
  local start end rc
  start="$(date +%s)"
  bash -c "$cmdline" > "$logfile" 2>&1; rc=$?
  end="$(date +%s)"
  printf '{"exit":%s,"peak_bytes":null,"ms":%s}\n' "$rc" "$(( (end - start) * 1000 ))"
}

if [ "$BUILD" = true ]; then
  echo "==> Building $TARGET ..."
  cmake --build cmake-build-debug --config Debug --target "$TARGET" -j "$JOBS" || {
    echo "Build failed" >&2; exit 1; }
fi

if [ ! -f "$COMPILER_BIN" ]; then
  echo "Compiler binary not found: $COMPILER_BIN (build with scripts/build.sh)" >&2
  exit 1
fi

SUMMARY="$OUT_DIR/${BACKEND}-summary.md"
{
  echo "# MIR baseline summary ($BACKEND, $MODE)"
  echo
  echo "Recorded $(date -u +%Y-%m-%dT%H:%M:%SZ) on $(uname -s) $(uname -m)"
  echo
  echo "| Suite | Status | Total | Passed | Failed | Seconds | Peak bytes |"
  echo "|-------|--------|------:|-------:|-------:|--------:|-----------:|"
} > "$SUMMARY"

failures=0
for suite in "${SUITES[@]}"; do
  flag="$(suite_flag "$suite")"
  log="$OUT_DIR/${BACKEND}-${suite}.log"
  echo ""
  echo "==> suite: $suite"
  cmdline="bash '$SCRIPT_DIR/test.sh' --$BACKEND --no-build --mode $MODE"
  [ -n "$flag" ] && cmdline="$cmdline $flag"

  measured="$(measure_wrapped "$cmdline" "$log")"
  exit_code="$(printf '%s' "$measured" | sed -E 's/.*"exit":([0-9-]+).*/\1/')"
  peak="$(printf '%s' "$measured" | sed -E 's/.*"peak_bytes":([0-9]+|null).*/\1/')"
  ms="$(printf '%s' "$measured" | sed -E 's/.*"ms":([0-9]+|null).*/\1/')"

  counts="$(parse_counts "$log")"
  total=""; passed=""; failed=""
  if [ -n "$counts" ]; then read -r total passed failed <<< "$counts"; fi
  if [ -z "$total" ]; then
    status="error"
  elif [ "${failed:-1}" -eq 0 ]; then
    status="ok"
  else
    status="fail"
  fi
  [ "$status" != "ok" ] && failures=$((failures + 1))

  secs="null"
  if [ -n "$ms" ] && [ "$ms" != "null" ]; then secs="$(awk "BEGIN{printf \"%.2f\", $ms/1000.0}")"; fi

  printf '{"suite":"%s","status":"%s","total":%s,"passed":%s,"failed":%s,"seconds":%s,"peak_bytes":%s,"exit":%s}\n' \
    "$suite" "$status" "${total:-null}" "${passed:-null}" "${failed:-null}" "${secs}" "${peak:-null}" "${exit_code:-null}" \
    > "$OUT_DIR/${BACKEND}-${suite}.json"

  printf '| %s | %s | %s | %s | %s | %s | %s |\n' \
    "$suite" "$status" "${total:-?}" "${passed:-?}" "${failed:-?}" "${secs:-?}" "${peak:-?}" >> "$SUMMARY"

  echo "    $suite: $status ${passed:-?}/${total:-?} (${secs:-?}s, peak ${peak:-?} bytes)"
done

echo ""
echo "==> summary written to $SUMMARY"
exit "$(( failures > 0 ? 1 : 0 ))"
