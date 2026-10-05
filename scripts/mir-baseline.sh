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
#   ./scripts/mir-baseline.sh --tcc --all --repeat 3      # average 3 runs
#   ./scripts/mir-baseline.sh --tcc --suite main --suite libs
#   ./scripts/mir-baseline.sh --tcc --all --no-build
#   ./scripts/mir-baseline.sh --llvm --all
#
# Outputs (default dir: lang/tests/build/mir-baseline, which is gitignored):
#   <backend>-<suite>.log        raw suite output (last run when --repeat > 1)
#   <backend>-<suite>.runN.log   per-run output when --repeat > 1
#   <backend>-<suite>.json       machine-readable summary (incl. per-run times)
#   <backend>-summary.md         Markdown table for mir-implementation-plan.md §11.3
#   <backend>-size.txt           compiler binary sizes
#
# Notes:
#   * Record the OS/host in the report. Baselines are machine-specific; this
#     project records at least one Windows and one Linux measurement and the
#     final MIR result must stay under both.
#   * With --repeat N, per-suite time is reported as min/median/mean/max across
#     the N runs so a single slow run does not set the baseline; peak memory is
#     the maximum observed (memory must not be trimmed by averaging).
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
OUT_DIR="lang/tests/build/mir-baseline"
BUILD=true
JOBS="$(nproc 2>/dev/null || echo 4)"
SUITES=()
ALL=false
PARSE_ONLY=false
REPEAT=1

usage() {
  sed -n '2,45p' "$0" | sed 's/^# \{0,1\}//'
  exit 1
}

while [ $# -gt 0 ]; do
  case "$1" in
    --tcc)   TARGET="TCCCompiler"; COMPILER_BIN="cmake-build-debug/TCCCompiler"; BACKEND="tcc"; shift ;;
    --llvm)  TARGET="Compiler";    COMPILER_BIN="cmake-build-debug/Compiler";    BACKEND="llvm"; shift ;;
    --suite) SUITES+=("$2"); shift 2 ;;
    --all)   ALL=true; shift ;;
    --parse-only) PARSE_ONLY=true; shift ;;
    --repeat) REPEAT="$2"; shift 2 ;;
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
# Mirrors scripts/test.sh's preference: the dedicated-suite "Summary:" line
# first, then the main/interpret "Total N Passed N Failed N" line. Strips ANSI
# escapes, CR, and NUL bytes (the compiler can emit NULs in warnings).
parse_counts() {
  local log="$1" line
  [ -f "$log" ] || return 0
  line="$(
    tr -d '\000' < "$log" \
      | sed -e $'s/\033\\[[0-9;]*[A-Za-z]//g' -e 's/\r$//' \
      | grep -aE 'Summary: [0-9]+ tests|Total [0-9]+ Passed [0-9]+ Failed [0-9]+' \
      | tail -n 1 || true
  )"
  [ -n "$line" ] || return 0
  if [[ "$line" =~ Summary:[[:space:]]+([0-9]+)[[:space:]]+tests[[:space:]]*-[[:space:]]+([0-9]+)[[:space:]]+passed[[:space:]]*,[[:space:]]*([0-9]+)[[:space:]]+failed ]]; then
    echo "${BASH_REMATCH[1]} ${BASH_REMATCH[2]} ${BASH_REMATCH[3]}"
  elif [[ "$line" =~ Total[[:space:]]+([0-9]+)[[:space:]]+Passed[[:space:]]+([0-9]+)[[:space:]]+Failed[[:space:]]+([0-9]+) ]]; then
    echo "${BASH_REMATCH[1]} ${BASH_REMATCH[2]} ${BASH_REMATCH[3]}"
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
    local start end
    start="$(date +%s%N)"
    /usr/bin/time -v -o "$tf" bash -c "$cmdline" > "$logfile" 2>&1
    local rc=$?
    end="$(date +%s%N)"
    local kb; kb="$(grep -E 'Maximum resident set size' "$tf" | grep -oE '[0-9]+' | head -n1 || echo 0)"
    rm -f "$tf"
    local ms=$(( (end - start) / 1000000 ))
    printf '{"exit":%s,"peak_bytes":%s,"ms":%s}\n' "$rc" "$(( ${kb:-0} * 1024 ))" "$ms"
    return 0
  fi
  local start end rc
  start="$(date +%s)"
  bash -c "$cmdline" > "$logfile" 2>&1; rc=$?
  end="$(date +%s)"
  printf '{"exit":%s,"peak_bytes":null,"ms":%s}\n' "$rc" "$(( (end - start) * 1000 ))"
}

# --parse-only: re-parse existing per-suite logs without re-running anything.
# Useful for regenerating the summary after a parser fix.
if [ "$PARSE_ONLY" = true ]; then
  echo "suite      total passed failed"
  for suite in "${SUITES[@]}"; do
    log="$OUT_DIR/${BACKEND}-${suite}.log"
    counts="$(parse_counts "$log")"
    printf '%-10s %s\n' "$suite" "${counts:-?}"
  done
  exit 0
fi

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
  echo "Recorded $(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo
  echo "- Host OS: \`$(uname -s) $(uname -m)\`"
  echo "- Host: \`${HOSTNAME:-unknown}\`"
  echo "- Runs per suite: $REPEAT"
  echo
  echo "| Suite | Status | Total | Passed | Failed | Min s | Median s | Mean s | Max s | Peak MB |"
  echo "|-------|--------|------:|-------:|-------:|------:|---------:|-------:|------:|--------:|"
} > "$SUMMARY"

failures=0
for suite in "${SUITES[@]}"; do
  flag="$(suite_flag "$suite")"
  canonical="$OUT_DIR/${BACKEND}-${suite}.log"
  echo ""
  echo "==> suite: $suite (x$REPEAT)"
  cmdline="bash '$SCRIPT_DIR/test.sh' --$BACKEND --no-build --mode $MODE"
  [ -n "$flag" ] && cmdline="$cmdline $flag"

  times=()
  peaks=()
  last_exit="null"
  for ((r=1; r<=REPEAT; r++)); do
    if [ "$REPEAT" -gt 1 ]; then log="$OUT_DIR/${BACKEND}-${suite}.run${r}.log"; else log="$canonical"; fi
    measured="$(measure_wrapped "$cmdline" "$log")"
    cexit="$(printf '%s' "$measured" | sed -E 's/.*"exit":([0-9-]+).*/\1/')"
    cpeak="$(printf '%s' "$measured" | sed -E 's/.*"peak_bytes":([0-9]+|null).*/\1/')"
    cms="$(printf '%s' "$measured" | sed -E 's/.*"ms":([0-9]+|null).*/\1/')"
    [ -n "$cexit" ] && last_exit="$cexit"
    t="null"
    if [ -n "$cms" ] && [ "$cms" != "null" ]; then t="$(awk "BEGIN{printf \"%.2f\", $cms/1000.0}")"; fi
    times+=("$t")
    if [ -n "$cpeak" ] && [ "$cpeak" != "null" ]; then peaks+=("$cpeak"); fi
    echo "    run $r: ${t}s, peak ${cpeak} bytes"
  done
  if [ "$REPEAT" -gt 1 ]; then cp "$OUT_DIR/${BACKEND}-${suite}.run${REPEAT}.log" "$canonical"; fi

  counts="$(parse_counts "$canonical")"
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

  # Time aggregation across runs (nulls ignored): min / median / mean / max.
  vals="$(printf '%s\n' "${times[@]}" | grep -v '^null$' || true)"
  tmin="$(printf '%s\n' "$vals" | awk 'NR==1{m=$1} {if($1<m)m=$1} END{if(NR)printf "%.2f",m}')"
  tmax="$(printf '%s\n' "$vals" | awk '{if(NR==1||$1>m)m=$1} END{if(NR)printf "%.2f",m}')"
  tmean="$(printf '%s\n' "$vals" | awk '{s+=$1} END{if(NR)printf "%.2f",s/NR}')"
  tmed="$(printf '%s\n' "$vals" | sort -n | awk '{a[NR]=$1} END{if(NR==0)exit; if(NR%2)printf "%.2f",a[(NR+1)/2]; else printf "%.2f",(a[NR/2]+a[NR/2+1])/2}')"

  # Peak memory is the maximum observed, never averaged.
  peakmax="null"
  for p in ${peaks[@]+"${peaks[@]}"}; do
    if [ "$peakmax" = "null" ] || [ "$p" -gt "$peakmax" ]; then peakmax="$p"; fi
  done

  runs_json="$(printf '%s,' "${times[@]}" | sed 's/,$//')"
  printf '{"suite":"%s","status":"%s","total":%s,"passed":%s,"failed":%s,"seconds_min":%s,"seconds_median":%s,"seconds_mean":%s,"seconds_max":%s,"seconds_runs":[%s],"peak_bytes":%s,"exit":%s}\n' \
    "$suite" "$status" "${total:-null}" "${passed:-null}" "${failed:-null}" \
    "${tmin:-null}" "${tmed:-null}" "${tmean:-null}" "${tmax:-null}" "$runs_json" \
    "${peakmax:-null}" "${last_exit:-null}" > "$OUT_DIR/${BACKEND}-${suite}.json"

  peak_mb="?"
  [ "$peakmax" != "null" ] && peak_mb="$(awk "BEGIN{printf \"%.1f\", $peakmax/1048576.0}")"
  printf '| %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |\n' \
    "$suite" "$status" "${total:-?}" "${passed:-?}" "${failed:-?}" \
    "${tmin:-?}" "${tmed:-?}" "${tmean:-?}" "${tmax:-?}" "$peak_mb" >> "$SUMMARY"

  echo "    $suite: $status ${passed:-?}/${total:-?} (mean ${tmean:-?}s, range ${tmin:-?}-${tmax:-?}s, peak ${peak_mb} MB)"
done

echo ""
echo "==> summary written to $SUMMARY"

# --- compiler binary sizes (hard < 4 MB release requirement) ---
SIZE_FILE="$OUT_DIR/${BACKEND}-size.txt"
{
  echo "backend=$BACKEND"
  echo "date=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  for b in TCCCompiler Compiler; do
    for p in "cmake-build-debug/$b" "cmake-build-debug/$b.exe"; do
      if [ -f "$p" ]; then
        echo "$b $(wc -c < "$p" | tr -d ' ') bytes ($p)"
      fi
    done
  done
} > "$SIZE_FILE"
cat "$SIZE_FILE"
echo "==> binary sizes written to $SIZE_FILE"

exit "$(( failures > 0 ? 1 : 0 ))"
