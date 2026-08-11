#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
#
# Shared helpers for the Chemical benchmark / release analytics collection.
# Sources of truth: the raw JSON data lives in the gh-pages branch under data/.
# Every failure is recorded as an individual data point with a status — never
# aborts the whole collection.
set -euo pipefail

BENCH_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"  # repo root (scripts live in scripts/)
# Local staging dir for collected data (gitignored; only the gh-pages branch
# ever carries data). The dashboard site lives on the gh-pages branch itself.
DATA_ROOT="${DATA_ROOT:-$BENCH_ROOT/.bench-data}"

# ─────────────────────────────────────────────────────────────────────────────
# dates / environment
# ─────────────────────────────────────────────────────────────────────────────
bm_date_utc()  { date -u +%Y-%m-%d; }
bm_datetime()  { date -u +%Y-%m-%dT%H:%M:%SZ; }

bm_platform() {
  case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
    Darwin*)              echo "macos" ;;
    Linux*)               echo "linux" ;;
    *)                    echo "unknown" ;;
  esac
}

bm_arch() {
  case "$(uname -m)" in
    x86_64|amd64) echo "x64" ;;
    aarch64|arm64) echo "arm64" ;;
    *) echo "$(uname -m)" ;;
  esac
}

# musl (alpine) detection — used to record the libc flavour in the environment
bm_libc() {
  if [ -f /etc/alpine-release ]; then echo "musl"; else echo "glibc"; fi
}

# ─────────────────────────────────────────────────────────────────────────────
# logging
# ─────────────────────────────────────────────────────────────────────────────
bm_log()  { echo "[bench] $*"; }
bm_warn() { echo "[bench] WARN: $*" >&2; }

# ─────────────────────────────────────────────────────────────────────────────
# version comparison + release-era asset expectations
# ─────────────────────────────────────────────────────────────────────────────
# NOTE: pure-bash numeric dot-separated comparison (no GNU `sort -V` — BSD /
# macOS sort rejects -V, which would make every release record "unavailable"
# on macos runners). Non-numeric segments are ignored; a missing segment
# counts as 0 (so 1.0.1 > 1). v0.0.32-style tags are supported.
ver_ge() { # <a> <b> — true if version a >= version b
  local a="${1#v}" b="${2#v}" na nb
  while :; do
    na="${a%%.*}"; nb="${b%%.*}"
    na="${na//[^0-9]/}"; nb="${nb//[^0-9]/}"
    na="${na:-0}"; nb="${nb:-0}"
    if [ "$na" -ne "$nb" ]; then
      [ "$na" -gt "$nb" ]
      return
    fi
    # strip the consumed segment; a side without a dot left is exhausted ("")
    # — ${var#*.} would return the string unchanged when no dot is present
    case "$a" in *.*) a="${a#*.}" ;; *) a="" ;; esac
    case "$b" in *.*) b="${b#*.}" ;; *) b="" ;; esac
    [ "$a" = "$b" ] && return 0
  done
}

# Expected compiler zip asset names for a release tag's era.
# Derived from the actual asset lists published per release (checked against
# the GitHub API). lsp assets are intentionally excluded (not compilers).
# A missing asset is recorded as missing_asset — the dashboard surfaces it.
expected_assets() { # <tag>
  local tag="$1"
  if ver_ge "$tag" "v0.0.32"; then
    echo "linux-x64.zip linux-x64-tcc.zip linux-arm64.zip linux-arm64-tcc.zip linux-alpine-x64.zip linux-alpine-x64-tcc.zip linux-alpine-arm64.zip linux-alpine-arm64-tcc.zip macos-x64.zip macos-x64-tcc.zip macos-arm64.zip macos-arm64-tcc.zip windows-x64.zip windows-x64-tcc.zip windows-arm64.zip windows-arm64-tcc.zip windows-mingw-x64.zip windows-mingw-x64-tcc.zip windows-mingw-arm64.zip windows-mingw-arm64-tcc.zip windows-mingw-msvcrt-x64.zip windows-mingw-msvcrt-x64-tcc.zip"
  elif ver_ge "$tag" "v0.0.31"; then
    echo "linux-x64.zip linux-x64-tcc.zip linux-arm64.zip linux-arm64-tcc.zip linux-alpine-x64.zip linux-alpine-x64-tcc.zip macos-x64.zip macos-x64-tcc.zip macos-arm64.zip macos-arm64-tcc.zip"
  elif ver_ge "$tag" "v0.0.25"; then
    echo "linux-x64.zip linux-x64-tcc.zip linux-arm64.zip linux-arm64-tcc.zip linux-alpine-x64.zip linux-alpine-x64-tcc.zip macos-x64.zip macos-x64-tcc.zip macos-arm64.zip macos-arm64-tcc.zip windows-x64.zip windows-x64-tcc.zip windows-arm64.zip windows-arm64-tcc.zip"
  elif ver_ge "$tag" "v0.0.9"; then
    echo "linux-x86-64.zip linux-x86-64-tcc.zip windows-x64.zip windows-x64-tcc.zip"
  fi
}

# asset_parts <asset-name> — prints "platform arch variant" (variant: regular|tcc)
asset_parts() {
  local name="$1" base="${1%.zip}" platform="" arch="" variant="regular"
  case "$base" in
    *-tcc) variant="tcc"; base="${base%-tcc}" ;;
  esac
  case "$base" in
    linux-alpine-*)        platform="linux-alpine";        arch="${base#linux-alpine-}" ;;
    linux-*)               platform="linux";               arch="${base#linux-}" ;;
    macos-*)               platform="macos";               arch="${base#macos-}" ;;
    windows-mingw-msvcrt-*) platform="windows-mingw-msvcrt"; arch="${base#windows-mingw-msvcrt-}" ;;
    windows-mingw-*)       platform="windows-mingw";       arch="${base#windows-mingw-}" ;;
    windows-*)             platform="windows";             arch="${base#windows-}" ;;
  esac
  echo "$platform $arch $variant"
}

# ─────────────────────────────────────────────────────────────────────────────
# JSON helpers (jq required)
# ─────────────────────────────────────────────────────────────────────────────
jq_escape() { printf '%s' "$1" | jq -Rs .; }

# ─────────────────────────────────────────────────────────────────────────────
# ANSI stripping (works on GNU + BSD sed via bash $'..')
# ─────────────────────────────────────────────────────────────────────────────
strip_ansi() {
  sed -e $'s/\x1b\[[0-9;]*m//g' -e 's/\r$//'
}

# ─────────────────────────────────────────────────────────────────────────────
# Parsing test output.
#
# The test executable prints TWO independent sections (this is how
# scripts/test.sh's compiled mode is structured, verified against its real
# output):
#
#   1) SEQUENTIAL inline tests — printed by test()/print_test_stats():
#        Test 12 [name] succeeded        /  Test 13 [name] failed
#        Total 523 Passed 520 Failed 3
#
#   2) @test RUNNER section — every @test function runs in its OWN child
#      process via posix_spawn + socketpair IPC (lang/libs/test/posix/launch.ch)
#      and the parent prints, per test:
#        Test run summary
#          Total: 523 | Passed: 517 | Failed: 6
#        Group: (no-group)
#          - test_name 1073741823
#            PASS                       <- or FAIL / [exitcode] FAIL
#        Summary: 523 tests - 517 passed, 6 failed
#
#      A non-zero exit code bracket ([139] FAIL) means the child process did
#      not exit cleanly (crash / SIGKILL on timeout). A "Test timed out after
#      10s" log line accompanies timeouts.
#
# The two sections are NOT additive in the output — the final "Total N Passed N
# Failed N" line only counts the SEQUENTIAL tests (runner results live in
# child processes and never reach print_test_stats). A parser that only read
# that line therefore MISSED every runner failure. This parser combines both.
#
# Prints JSON:
#   {"total":N,"passed":N,"failed":N,"succeeded":N,"complete":true|false,
#    "sequential":{"total":N,"passed":N,"failed":N},
#    "runner":{"total":N,"passed":N,"failed":N},
#    "failed_tests":["name",...],
#    "crashed_tests":[{"name":"...","exit_code":N}],
#    "timed_out_tests":["name",...]}
#   complete=false means no summary line was found (truncated / crashed run).
# ─────────────────────────────────────────────────────────────────────────────
parse_test_output() {
  local log="$1"
  local line
  # sequential counters
  local seq_total=0 seq_passed=0 seq_failed=0
  # runner counters (per-test lines; Summary: line is authoritative)
  local run_total=0 run_passed=0 run_failed=0
  local -a failed_names=()        # both sections
  local -a crashed_tests=()       # JSON objects
  local -a timed_out=()
  local complete=true
  local saw_seq_summary=false saw_run_summary=false
  # runner state machine
  local cur_test="" cur_block=""

  # one sed pass strips ANSI + CR (per-line subprocess spawns are far too slow
  # on multi-thousand-line logs), then pure-bash regex matching. Note: the
  # final "Total N Passed N Failed N" line from print_test_stats has NO
  # trailing newline, so the loop must process a last unterminated line too.
  while IFS= read -r line || [ -n "$line" ]; do
    # ── sequential section ──────────────────────────────────────────────────
    if [[ "$line" =~ ^Test[[:space:]]+[0-9]+[[:space:]]+\[(.*)\][[:space:]]+succeeded[[:space:]]*$ ]]; then
      seq_total=$((seq_total + 1)); seq_passed=$((seq_passed + 1))
    elif [[ "$line" =~ ^Test[[:space:]]+[0-9]+[[:space:]]+\[(.*)\][[:space:]]+failed[[:space:]]*$ ]]; then
      seq_total=$((seq_total + 1)); seq_failed=$((seq_failed + 1))
      failed_names+=("${BASH_REMATCH[1]}")
    elif [[ "$line" =~ ^Total[[:space:]]+([0-9]+)[[:space:]]+Passed[[:space:]]+([0-9]+)[[:space:]]+Failed[[:space:]]+([0-9]+) ]]; then
      # sequential summary line from print_test_stats — authoritative for the
      # sequential section ONLY (never overwrites runner counts)
      seq_total="${BASH_REMATCH[1]}"
      seq_passed="${BASH_REMATCH[2]}"
      seq_failed="${BASH_REMATCH[3]}"
      saw_seq_summary=true
    # ── runner section: header line ─────────────────────────────────────────
    elif [[ "$line" =~ ^Test[[:space:]]+run[[:space:]]+summary ]]; then
      : # section marker
    elif [[ "$line" =~ ^[[:space:]]*Total:[[:space:]]+([0-9]+)[[:space:]]+\|[[:space:]]*Passed:[[:space:]]+([0-9]+)[[:space:]]+\|[[:space:]]*Failed:[[:space:]]+([0-9]+) ]]; then
      # Display header only — do NOT seed the counters here. Per-test PASS/FAIL
      # lines increment them and the final "Summary:" line is authoritative.
      # Seeding from the header AND incrementing would double-count (and show
      # inflated numbers in a truncated/crashed run where Summary never prints).
      :
    # ── runner section: "  - test_name 1073741823" test header ──────────────
    elif [[ "$line" =~ ^[[:space:]]*-[[:space:]]+([^[:space:]].*)[[:space:]]+[0-9]+[[:space:]]*$ ]]; then
      cur_test="${BASH_REMATCH[1]}"
      cur_block=""
    # ── runner section: per-test result "    PASS" / "    FAIL" / "    [139] FAIL" ──
    elif [[ "$line" =~ ^[[:space:]]*(\[[0-9]+\])?[[:space:]]*(PASS|FAIL)[[:space:]]*$ ]] && [ -n "$cur_test" ]; then
      local exit_code="${BASH_REMATCH[1]#[}"; exit_code="${exit_code%]}"; [ -z "$exit_code" ] && exit_code=0
      if [ "${BASH_REMATCH[2]}" = "PASS" ]; then
        run_total=$((run_total + 1)); run_passed=$((run_passed + 1))
      else
        run_total=$((run_total + 1)); run_failed=$((run_failed + 1))
        failed_names+=("$cur_test")
        if [[ "$cur_block" == *"timed out"* ]]; then
          timed_out+=("$cur_test")
        elif [ "$exit_code" -ne 0 ]; then
          crashed_tests+=("{\"name\":$(jq_escape "$cur_test"),\"exit_code\":$exit_code}")
        fi
      fi
      cur_test=""
    # ── runner section: final "Summary: 523 tests - 517 passed, 6 failed" ────
    elif [[ "$line" =~ ^Summary:[[:space:]]+([0-9]+)[[:space:]]+tests[[:space:]]+-[[:space:]]+([0-9]+)[[:space:]]+passed[[:space:]]*,[[:space:]]*([0-9]+)[[:space:]]+failed ]]; then
      run_total="${BASH_REMATCH[1]}"; run_passed="${BASH_REMATCH[2]}"; run_failed="${BASH_REMATCH[3]}"
      saw_run_summary=true
    fi
    # accumulate runner block text (between test header and result) to detect
    # "timed out" log messages
    [ -n "$cur_test" ] && cur_block+="$line|"
  done < <(sed -e $'s/\x1b\[[0-9;]*m//g' -e 's/\r$//' "$log")

  local total=$((seq_total + run_total))
  local passed=$((seq_passed + run_passed))
  local failed=$((seq_failed + run_failed))

  # A run is only "complete" if every section that produced test lines also
  # produced its authoritative summary line. A truncated/crashed run (process
  # died mid-print) shows test lines but no Total / Summary — record it as
  # incomplete so the dashboard reports a crash instead of silent data loss.
  if [ "$seq_total" -gt 0 ] && [ "$saw_seq_summary" = false ]; then
    complete=false
  fi
  if [ "$run_total" -gt 0 ] && [ "$saw_run_summary" = false ]; then
    complete=false
  fi
  if [ "$total" -eq 0 ] && grep -aqE 'Test run summary|Summary: [0-9]+ tests' "$log"; then
    complete=false
  fi

  local failed_json="[]"
  if [ "${#failed_names[@]}" -gt 0 ]; then
    failed_json="["
    local i n
    for i in "${!failed_names[@]}"; do
      [ "$i" -gt 0 ] && failed_json+=","
      failed_json+="$(jq_escape "${failed_names[$i]}")"
    done
    failed_json+="]"
  fi

  local crashed_json="[]"
  if [ "${#crashed_tests[@]}" -gt 0 ]; then
    crashed_json="["
    for i in "${!crashed_tests[@]}"; do
      [ "$i" -gt 0 ] && crashed_json+=","
      crashed_json+="${crashed_tests[$i]}"
    done
    crashed_json+="]"
  fi

  local timedout_json="[]"
  if [ "${#timed_out[@]}" -gt 0 ]; then
    timedout_json="["
    for i in "${!timed_out[@]}"; do
      [ "$i" -gt 0 ] && timedout_json+=","
      timedout_json+="$(jq_escape "${timed_out[$i]}")"
    done
    timedout_json+="]"
  fi

  printf '{"total":%s,"passed":%s,"failed":%s,"succeeded":%s,"complete":%s,"sequential":{"total":%s,"passed":%s,"failed":%s},"runner":{"total":%s,"passed":%s,"failed":%s},"failed_tests":%s,"crashed_tests":%s,"timed_out_tests":%s}' \
    "$total" "$passed" "$failed" "$passed" "$complete" \
    "$seq_total" "$seq_passed" "$seq_failed" \
    "$run_total" "$run_passed" "$run_failed" \
    "$failed_json" "$crashed_json" "$timedout_json"
}

# ─────────────────────────────────────────────────────────────────────────────
# Parsing compiler benchmark output (-bm / -bm-modules).
# Emitted lines look like:
#   [bm:module] 'main' completed [nano:12345] [micro:12] [milli:1] [sec:0]
# Prints a JSON array: [{"tag":"bm:module","name":"main","nanos":..,"millis":..},..]
# ─────────────────────────────────────────────────────────────────────────────
parse_bm_output() {
  local log="$1"
  local line tag name nano micro milli sec
  local -a items=()

  # one sed pass strips ANSI + CR; then pure-bash ERE per line (unquoted
  # regex fragments — quoting makes them literal in [[ =~ ]]). Emits one JSON
  # object per "[tag] 'name' completed [nano:N] [micro:N] [milli:N] [sec:N]"
  # line; name is optional.
  while IFS= read -r line; do
    if [[ "$line" =~ ^\[([^]]+)\][[:space:]]+\'(.*)\'[[:space:]]+completed[[:space:]]+\[nano:([0-9]+)\][[:space:]]+\[micro:([0-9]+)\][[:space:]]+\[milli:([0-9]+)\][[:space:]]+\[sec:([0-9]+)\]$ ]]; then
      tag="${BASH_REMATCH[1]}"; name="${BASH_REMATCH[2]}"
      nano="${BASH_REMATCH[3]}"; micro="${BASH_REMATCH[4]}"
      milli="${BASH_REMATCH[5]}"; sec="${BASH_REMATCH[6]}"
      items+=("{\"tag\":$(jq_escape "$tag"),\"name\":$(jq_escape "$name"),\"nanos\":$nano,\"micros\":$micro,\"millis\":$milli,\"secs\":$sec}")
    elif [[ "$line" =~ ^\[([^]]+)\][[:space:]]+completed[[:space:]]+\[nano:([0-9]+)\][[:space:]]+\[micro:([0-9]+)\][[:space:]]+\[milli:([0-9]+)\][[:space:]]+\[sec:([0-9]+)\]$ ]]; then
      tag="${BASH_REMATCH[1]}"
      nano="${BASH_REMATCH[2]}"; micro="${BASH_REMATCH[3]}"
      milli="${BASH_REMATCH[4]}"; sec="${BASH_REMATCH[5]}"
      items+=("{\"tag\":$(jq_escape "$tag"),\"name\":null,\"nanos\":$nano,\"micros\":$micro,\"millis\":$milli,\"secs\":$sec}")
    fi
  done < <(sed -e $'s/\x1b\[[0-9;]*m//g' -e 's/\r$//' "$log")

  if [ "${#items[@]}" -eq 0 ]; then
    echo "[]"
  else
    local out="["
    local i
    for i in "${!items[@]}"; do
      [ "$i" -gt 0 ] && out+=","
      out+="${items[$i]}"
    done
    out+="]"
    echo "$out"
  fi
}

# ─────────────────────────────────────────────────────────────────────────────
# Run a command with a timeout, tee to a log, and produce a status.
# usage: bm_run <out_status> <out_duration_ms> <timeout_secs> <log> <cmd...>
#   out_status receives: success | timeout | failed
#   The command's real exit code is stored in BM_EXIT_CODE.
# ─────────────────────────────────────────────────────────────────────────────
BM_EXIT_CODE=0
# Portable epoch-milliseconds: GNU `date +%s%N` gives ns (divided down to ms);
# BSD/macOS `date` has no %N (it prints a literal %N), so fall back to seconds
# precision — coarse but correct on every platform.
bm_now_ms() {
  local v
  v="$(date +%s%N 2>/dev/null || true)"
  case "$v" in
    ''|*[!0-9]*) date +%s000 ;;
    *) printf '%s' "$(( v / 1000000 ))" ;;
  esac
}

bm_run() {
  local out_status="$1" out_duration="$2" timeout_secs="$3" log="$4"; shift 4
  local start end
  start="$(bm_now_ms)"
  set +e
  if command -v timeout >/dev/null 2>&1; then
    timeout "$timeout_secs" "$@" > "$log" 2>&1
    BM_EXIT_CODE=$?
  else
    "$@" > "$log" 2>&1
    BM_EXIT_CODE=$?
  fi
  set -e
  end="$(bm_now_ms)"
  printf -v "$out_duration" "%d" $(( end - start ))

  if [ "$BM_EXIT_CODE" -eq 124 ]; then
    printf -v "$out_status" "%s" "timeout"
  elif [ "$BM_EXIT_CODE" -eq 0 ]; then
    printf -v "$out_status" "%s" "success"
  else
    printf -v "$out_status" "%s" "failed"
  fi
}

# ─────────────────────────────────────────────────────────────────────────────
# Hello-world benchmarks.
# usage: bench_hello <backend> <bin> <bare|std> <bench_name> <outvar> <logdir>
# Measures the *compilation* time of the program (the thing the compilers do).
# The interpreter cannot run a standalone file (it needs a build.lab
# interpretation job) so its hello benchmark is recorded as "unavailable".
# ─────────────────────────────────────────────────────────────────────────────
bench_hello() {
  local backend="$1" bin="$2" kind="$3" bench_name="$4" outvar="$5" logdir="$6"
  local work=".bench_tmp/hello_$kind"
  rm -rf "$work"
  mkdir -p "$work"

  if [ "$kind" = "bare" ]; then
    cat > "$work/main.ch" <<'EOF'
@extern public func printf(format : *char, _ : any...) : int
public func main() : int {
    printf("hello world")
    return 0
}
EOF
    local hello_src="$work/main.ch"
  else
    mkdir -p "$work/src"
    cat > "$work/chemical.mod" <<'EOF'
application hello_std
source "src"
import std
EOF
    cat > "$work/src/main.ch" <<'EOF'
public func main() : int {
    println(`hello from chemical`)
    return 0
}
EOF
    hello_src="$work/chemical.mod"
  fi

  local log="$logdir/${backend}_hello_${kind}.log"
  local status="" dur_ms=0

  if [ "$backend" = "Interpreter" ]; then
    printf -v "$outvar" '{"name":%s,"status":"unavailable","reason":"interpreter requires a build.lab interpretation job; standalone hello world is not interpretable via the CLI","duration_ms":null}' "$(jq_escape "$bench_name")"
    return
  fi
  if [ ! -f "$bin" ]; then
    printf -v "$outvar" '{"name":%s,"status":"unavailable","reason":"compiler binary not built","duration_ms":null}' "$(jq_escape "$bench_name")"
    return
  fi

  local out_exe="$work/hello_$kind.exe"
  bm_run status dur_ms 600 "$log" "$bin" "$hello_src" -o "$out_exe" --mode debug_quick --no-cache

  local reason="null"
  [ "$status" != "success" ] && reason=$(jq_escape "compiler exited with code $BM_EXIT_CODE")
  printf -v "$outvar" '{"name":%s,"status":%s,"reason":%s,"duration_ms":%s}' \
    "$(jq_escape "$bench_name")" "$(jq_escape "$status")" "$reason" "$dur_ms"
}

# ─────────────────────────────────────────────────────────────────────────────
# Module benchmarks + test suite for one backend.
# usage: bench_modules_and_tests <backend> <bin> <log_base> <outvar> <logdir> <quick>
#   1) compiles lang/tests/build.lab with -bm-modules (yields per-module
#      timings + the test executable for compiled backends)
#   2) runs the test suite (compiled: run the exe; interpreter: --arg-interpret)
# Emits JSON: {"modules":[...],"tests":{...},"build_ms":N,"build_status":"..."}
# The caller must already be inside the repo/checkout that owns the test sources.
# ─────────────────────────────────────────────────────────────────────────────
bench_modules_and_tests() {
  local backend="$1" bin="$2" log_base="$3" outvar="$4" logdir="$5" quick="$6"
  local build_log="$logdir/${log_base}_build.log"
  local test_log="$logdir/${log_base}_tests.log"
  local build_status="" build_ms=0 test_status="" test_ms=0
  local modules_json="[]" files_json="[]" tests_json='{"status":"unavailable","reason":"not run","total":null,"passed":null,"failed":null,"duration_ms":null,"failed_tests":[]}'
  local exe=""

  if [ "$quick" = "true" ]; then
    printf -v "$outvar" '{"modules":%s,"files":%s,"tests":%s,"build_ms":null,"build_status":"skipped"}' "[]" "[]" "$tests_json"
    return
  fi

  # -bm-modules gives module-level timings (tag bm:module); -bm-files adds
  # per-file phase timings (Lexer / Parser / SymRes:* / 2cTranslation:*) in
  # the same output format — both are parsed below and kept separate.
  if [ "$backend" = "Interpreter" ]; then
    bm_run build_status build_ms 1800 "$build_log" "$bin" lang/tests/build.lab --mode debug_quick --arg-interpret --no-cache -bm-modules -bm-files
  else
    case "$backend" in
      TCCCompiler) exe="lang/tests/build/tests-tcc.exe" ;;
      Compiler)    exe="lang/tests/build/tests.exe" ;;
    esac
    bm_run build_status build_ms 1800 "$build_log" "$bin" lang/tests/build.lab -o "$exe" --mode debug_quick --no-cache -bm-modules -bm-files
  fi

  if [ "$build_status" = "success" ]; then
    local raw_mods
    raw_mods="$(parse_bm_output "$build_log")"
    # split module-level entries from per-file entries; keep the slowest 50
    # files per phase so records stay small while still surfacing regressions
    # (a full run is files × ~10 phases, i.e. thousands of entries)
    modules_json="$(printf '%s' "$raw_mods" | jq -c '[.[] | select(.tag == "bm:module")]')"
    files_json="$(printf '%s' "$raw_mods" | jq -c '[.[] | select(.tag != "bm:module")] | group_by(.tag) | map(sort_by(-.nanos) | .[0:50]) | add')"
    if [ "$backend" = "Interpreter" ]; then
      # interpret mode runs the tests inside the compiler process
      bm_run test_status test_ms 900 "$test_log" "$bin" lang/tests/build.lab --mode debug_quick --arg-interpret --no-cache
    else
      if [ -f "$exe" ]; then
        bm_run test_status test_ms 900 "$test_log" "$exe"
      else
        test_status="failed"
        test_ms=0
      fi
    fi

    if [ -f "$test_log" ]; then
      local counts
      counts="$(parse_test_output "$test_log")"
      # NOTE: the test executable's main() always returns 0 even when tests
      # fail (verified in lang/tests/src/tests.ch), so the process exit code
      # can NOT determine pass/fail. Status is derived from the parsed counts
      # and the crash/timeout signals instead.
      local status="success" reason="null"
      local n_failed n_crashed n_timed
      n_failed="$(printf '%s' "$counts" | jq -r '.failed // 0' 2>/dev/null || echo 0)"
      n_crashed="$(printf '%s' "$counts" | jq -r '.crashed_tests | length' 2>/dev/null || echo 0)"
      n_timed="$(printf '%s' "$counts" | jq -r '.timed_out_tests | length' 2>/dev/null || echo 0)"
      if [ "$test_status" = "timeout" ]; then
        status="timeout"
        reason="$(jq_escape "test suite timed out after 900s")"
      elif [ "$test_status" != "success" ]; then
        # the test executable itself died (signal / non-zero exit)
        status="test_crash"
        reason="$(jq_escape "test executable crashed with exit code $BM_EXIT_CODE")"
      elif [ "$(printf '%s' "$counts" | jq -r '.complete // "false"' 2>/dev/null || echo false)" != "true" ]; then
        status="test_crash"
        reason="$(jq_escape "test run did not complete (output truncated); possible crash mid-run")"
      elif [ "$n_failed" -gt 0 ]; then
        status="test_failure"
        reason="$(jq_escape "$n_failed test(s) failed")"
      fi
      local extra=""
      [ "$n_crashed" -gt 0 ] && extra+="$n_crashed crashed; "
      [ "$n_timed" -gt 0 ] && extra+="$n_timed timed out; "
      if [ -n "$extra" ]; then
        extra="${extra%, }"
        if [ "$status" = "success" ]; then status="test_failure"; fi
        if [ "$reason" = "null" ]; then
          reason="$(jq_escape "$extra")"
        else
          reason="$(printf '%s' "$reason" | jq -c --arg e "$extra" '. + " — " + $e' 2>/dev/null || echo "$reason")"
        fi
      fi
      tests_json="$(printf '%s' "$counts" | jq --arg st "$status" --argjson reason "$reason" --argjson ms "$test_ms" \
        '{status:$st,reason:$reason,total:.total,passed:.passed,failed:.failed,succeeded:.succeeded,duration_ms:$ms,complete:.complete,sequential:.sequential,runner:.runner,failed_tests:.failed_tests,crashed_tests:.crashed_tests,timed_out_tests:.timed_out_tests}')"
    else
      tests_json="$(printf '{"status":"test_crash","reason":"no test output","total":null,"passed":null,"failed":null,"succeeded":null,"duration_ms":%s,"complete":false,"sequential":null,"runner":null,"failed_tests":[],"crashed_tests":[],"timed_out_tests":[]}' "$test_ms")"
    fi
  else
    # compiler build of the test suite failed — surface the tail of the build
    # log so compiler crashes / errors are visible instead of a bare status
    local build_tail=""
    if [ -f "$build_log" ]; then
      build_tail="$(tail -3 "$build_log" | grep -aE 'error|Error|crash|signal|Segmentation|assert' | tail -1 || true)"
    fi
    local build_reason
    if [ -n "$build_tail" ]; then
      build_reason="$(jq_escape "test suite compilation failed (exit $BM_EXIT_CODE): $build_tail")"
    else
      build_reason="$(jq_escape "test suite compilation failed with exit code $BM_EXIT_CODE")"
    fi
    tests_json="$(printf '{"status":"build_failure","reason":%s,"total":null,"passed":null,"failed":null,"succeeded":null,"duration_ms":%s,"complete":false,"sequential":null,"runner":null,"failed_tests":[],"crashed_tests":[],"timed_out_tests":[]}' \
      "$build_reason" "$build_ms")"
  fi

  printf -v "$outvar" '{"modules":%s,"files":%s,"tests":%s,"build_ms":%s,"build_status":%s}' \
    "$modules_json" "$files_json" "$tests_json" "$build_ms" "$(jq_escape "$build_status")"
}

# ─────────────────────────────────────────────────────────────────────────────
# Write a JSON file atomically (tmp + mv)
# ─────────────────────────────────────────────────────────────────────────────
bm_write_json() { # <file> <json>
  local tmp
  tmp="$(mktemp)"
  printf '%s\n' "$2" > "$tmp"
  mkdir -p "$(dirname "$1")"
  mv "$tmp" "$1"
}

# ─────────────────────────────────────────────────────────────────────────────
# Link shared tooling (libtcc + the lsp submodule) from the main checkout into
# a worktree checked out at an older commit. Old commits do not contain
# lib/tcc or the submodule, but the compiler build needs them to link.
# usage: bm_link_shared_tooling <worktree_root> <main_root>
# ─────────────────────────────────────────────────────────────────────────────
bm_link_shared_tooling() {
  local wt="$1" main_root="$2"
  # git worktrees do NOT materialize submodules: lib/lsp-framework (and any
  # other submodule) appears as an EMPTY directory stub, which makes cmake
  # configure fail ("does not contain a CMakeLists.txt") and every compiler
  # build record a failure. Link the whole lib/ tree from the main checkout
  # instead of individual entries: the stub dir must be removed first or the
  # `ln -s` target already exists (broken/empty).
  if [ -d "$main_root/lib" ]; then
    if [ -e "$wt/lib" ] && [ ! -L "$wt/lib" ]; then
      rm -rf "$wt/lib"
    fi
    if [ ! -e "$wt/lib" ]; then
      mkdir -p "$(dirname "$wt/lib")"
      ln -s "$main_root/lib" "$wt/lib"
    fi
  fi
  # prebuilt LLVM — only needed when the LLVM Compiler is built in the worktree
  # (--no-skip-llvm / commit collection with LLVM enabled)
  if [ -d "$main_root/out/host" ] && [ ! -e "$wt/out/host" ]; then
    mkdir -p "$wt/out"
    ln -s "$main_root/out/host" "$wt/out/host"
  fi
}
