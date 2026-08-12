#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
#
# collect_daily.sh — build the compiler at a specific commit, benchmark it, run
# the test suite for every available backend, and emit a daily JSON record.
#
# IMPORTANT: the compiler is built AT the target commit and the test suite is
# run with THE SAME commit's test sources. A compiler from commit X cannot
# compile test sources from commit Y (tests evolve with the compiler), so the
# whole collection happens inside a worktree checked out at the target commit.
#
# Usage:
#   collect_daily.sh [--commit SHA] [--date YYYY-MM-DD] [--out DIR]
#                    [--work DIR] [--skip-llvm] [--no-build]
#                    [--backends tcc,llvm,interpret] [--quick]
#
# Emits: $OUT/daily/$DATE.json
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=bench-common.sh
source "$SCRIPT_DIR/bench-common.sh"

COMMIT=""
DATE=""
OUT=""
WORK=""
PLATFORM=""
ARCH=""
SKIP_LLVM=false
NO_BUILD=false
BACKENDS="tcc,llvm,interpret"
QUICK=false

while [ $# -gt 0 ]; do
  case "$1" in
    --commit)   COMMIT="$2"; shift 2 ;;
    --date)     DATE="$2"; shift 2 ;;
    --out)      OUT="$2"; shift 2 ;;
    --work)     WORK="$2"; shift 2 ;;
    --platform) PLATFORM="$2"; shift 2 ;;
    --arch)     ARCH="$2"; shift 2 ;;
    --skip-llvm) SKIP_LLVM=true; shift ;;
    --no-build) NO_BUILD=true; shift ;;
    --backends) BACKENDS="$2"; shift 2 ;;
    --quick)    QUICK=true; shift ;;   # skip tests, hello + modules only
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

[ -z "$OUT" ] && OUT="$DATA_ROOT"
[ -z "$DATE" ] && DATE="$(bm_date_utc)"
[ -z "$COMMIT" ] && COMMIT="$(git rev-parse HEAD 2>/dev/null || echo '')"

# Platform/arch: the workflow matrix passes them explicitly. Auto-detection via
# uname is ambiguous in containers (an alpine container reports "linux") and on
# cross-target jobs, so the explicit flags win.
[ -z "$PLATFORM" ] && PLATFORM="$(bm_platform)"
[ -z "$ARCH" ] && ARCH="$(bm_arch)"
LIBC="$(bm_libc_for "$PLATFORM")"

# MSVC toolchain only (platform "windows"): make cl.exe find INCLUDE/LIB and
# stop MSYS2 from mangling MSVC flags (same as scripts/test.sh). The daily
# workflow's msvc-dev-cmd step already set this up for the msvc variant, so
# sourcing here is a no-op on CI and only matters for local Windows runs.
# Never sourced on MinGW/msvcrt variants — they use their own toolchains, and
# msvc_env.sh prepends a Windows-style (semicolon-joined) PATH entry that bash
# splits on ':' and would shadow jq and other tools at the head of PATH.
if [ "$PLATFORM" = "windows" ]; then
  _BM_JQ_BIN="$(command -v jq || true)"
  if [ -f "$SCRIPT_DIR/msvc_env.sh" ]; then
    # shellcheck source=msvc_env.sh
    source "$SCRIPT_DIR/msvc_env.sh"
  fi
  # msvc_env.sh set PATH="<MSVC-bin>;<old PATH>"; the <MSVC-bin>;<head> part is
  # invisible to bash's ':' splitting — restore any tool it shadowed (jq).
  if [ -n "$_BM_JQ_BIN" ] && ! command -v jq >/dev/null 2>&1; then
    export PATH="$(dirname "$_BM_JQ_BIN"):$PATH"
  fi
fi

# One record PER PLATFORM per date (the daily workflow runs a platform matrix):
#   data/daily/<date>-<platform>-<arch>.json   e.g. 2026-08-12-linux-alpine-x64.json
RECORD_ID="${DATE}-${PLATFORM}-${ARCH}"
OUT_FILE="$OUT/daily/${RECORD_ID}.json"
COLLECT_START_MS="$(bm_now_ms)"

# ── failure guard (registered EARLY — failures before the backend collection
# -- checkout, worktree, cmake configure -- must still produce a visible
# record with status "failed" + a reason; a crashed platform would otherwise
# silently disappear from the dashboard) ─────────────────────────────────────
BACKEND_TMP=""
bm_on_exit() {
  if [ -n "$BACKEND_TMP" ]; then rm -rf "$BACKEND_TMP"; fi
  if [ ! -f "$OUT_FILE" ]; then
    local code="${1:-1}"
    local sha short
    sha="$(git rev-parse HEAD 2>/dev/null || echo '')"
    short="${sha:0:12}"
    local rec
    rec="$(jq -n \
      --arg type "daily" --arg id "$RECORD_ID" --arg date "$DATE" \
      --arg generated_at "$(bm_datetime)" \
      --arg platform "$PLATFORM" --arg arch "$ARCH" --arg libc "$LIBC" \
      --arg sha "$sha" --arg short "$short" \
      --argjson duration_ms "$(( $(bm_now_ms) - COLLECT_START_MS ))" \
      --arg status "failed" \
      --arg status_reason "collection aborted before completion (exit $code)" \
      '{type:$type,id:$id,date:$date,generated_at:$generated_at,duration_ms:$duration_ms,status:$status,status_reason:$status_reason,platform:{os:$platform,arch:$arch,libc:$libc},commit:{sha:$sha,short:$short,subject:null,author_date:null},compiler_version:{},backends:{}}')"
    mkdir -p "$(dirname "$OUT_FILE")"
    printf '%s\n' "$rec" > "$OUT_FILE"
  fi
}
trap 'bm_on_exit "$?"' EXIT

# where the checked-out commit lives (defaults to the current checkout)
REPO_ROOT="$(git rev-parse --show-toplevel)"
MAIN_ROOT="$REPO_ROOT"
if [ -n "$WORK" ]; then
  REPO_ROOT="$WORK"
  if [ ! -d "$REPO_ROOT/.git" ] && [ ! -f "$REPO_ROOT/.git" ]; then
    bm_log "work dir '$REPO_ROOT' is not a git checkout; creating worktree at $COMMIT"
    rm -rf "$REPO_ROOT"
    git worktree add --detach "$REPO_ROOT" "$COMMIT" >/dev/null
  fi
  # old commits have no lib/ tooling; link the shared libtcc + submodule
  bm_link_shared_tooling "$REPO_ROOT" "$MAIN_ROOT"
fi

cd "$REPO_ROOT"

# ── cmake configure (TCC-only by default; the LLVM workflow configures itself) ──
# When run from the main checkout (daily workflow), cmake-build-debug is
# configured by the workflow with BUILD_COMPILER=ON so both backends build.
# Worktrees configure themselves here; failures are surfaced in the
# build log instead of silently hiding the reason.
if [ ! -f cmake-build-debug/CMakeCache.txt ]; then
  bm_log "configuring cmake (BUILD_COMPILER=OFF)..."
  mkdir -p "$OUT/logs/$DATE"
  if ! cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_COMPILER=OFF > "$OUT/logs/$DATE/cmake_configure.log" 2>&1; then
    bm_warn "cmake configure failed — see cmake_configure.log; builds will be recorded as failures"
  fi
fi

# ── commit metadata ──────────────────────────────────────────────────────────
SHA="$(git rev-parse HEAD)"
SHORT="${SHA:0:12}"
SUBJECT="$(git log -1 --format=%s 2>/dev/null | head -c 200 || echo '')"
AUTHOR_DATE="$(git log -1 --format=%aI 2>/dev/null || echo '')"

bm_log "==> Benchmarking commit $SHORT ($SUBJECT) on $PLATFORM/$ARCH ($LIBC)"

case "$PLATFORM" in
  windows) TCC_BIN="cmake-build-debug/TCCCompiler.exe"; LLVM_BIN="cmake-build-debug/Compiler.exe" ;;
  *)       TCC_BIN="cmake-build-debug/TCCCompiler";     LLVM_BIN="cmake-build-debug/Compiler" ;;
esac

LOGS="$OUT/logs/$DATE"
mkdir -p "$LOGS"
BACKEND_TMP="$(mktemp -d)"

# ── build helpers ────────────────────────────────────────────────────────────
build_backend() { # <backend> <target> <bin> <log>
  local backend="$1" target="$2" bin="$3" log="$4"
  if [ "$NO_BUILD" = true ]; then
    if [ -f "$bin" ]; then
      bm_log "  [build $backend] --no-build, using existing binary" >&2
      echo "success"
      return
    fi
  fi
  bm_log "  [build $backend] building..." >&2
  # bm_run is timeout-aware: macOS has no coreutils `timeout`, so a direct
  # `timeout ... cmake --build` would make EVERY build record "failed" there.
  local st ms
  bm_run st ms 1800 "$log" cmake --build cmake-build-debug --config Debug --target "$target" -j "$(bm_jobs)"
  echo "$st"
}


# ── per-backend collection ───────────────────────────────────────────────────
collect_backend() { # <backend> <binary> <json_out_file>
  local backend="$1" bin="$2" out_file="$3"
  bm_log "==> Backend: $backend"
  local build_status
  if [ "$backend" = "Compiler" ] && [ "$SKIP_LLVM" = true ]; then
    build_status="unavailable"
    bm_log "  [build] skipped (--skip-llvm)"
  elif [ "$NO_BUILD" = true ] && [ ! -f "$bin" ]; then
    # --no-build with nothing to use: nothing to benchmark
    build_status="unavailable"
    bm_log "  [build] --no-build and binary not present -> unavailable"
  else
    build_status="$(build_backend "$backend" "$( [ "$backend" = "Compiler" ] && echo Compiler || echo TCCCompiler )" "$bin" "$LOGS/${backend}_build.log")"
  fi

  local build_reason="null"
  if [ "$build_status" != "success" ]; then
    build_reason="$(jq_escape "compiler build failed; see ${backend}_build.log")"
  fi

  local bm_bare="" bm_std="" mods_tests=""
  local benchmarks_json="[]"

  if [ "$build_status" = "success" ]; then
    bench_hello "$backend" "$bin" bare "hello_bare" bm_bare "$LOGS"
    bench_hello "$backend" "$bin" std  "hello_std"  bm_std "$LOGS"
    benchmarks_json="[$bm_bare,$bm_std]"
    bench_modules_and_tests "$backend" "$bin" "$backend" mods_tests "$LOGS" "$QUICK"
  else
    mods_tests='{"modules":[],"files":[],"tests":{"status":"unavailable","reason":"compiler build failed","total":null,"passed":null,"failed":null,"duration_ms":null,"failed_tests":[]},"build_ms":null,"build_status":"unavailable"}'
  fi

  local record
  record="$(printf '%s' "$mods_tests" | jq --arg backend "$backend" \
    --arg build_status "$build_status" \
    --argjson build_reason "$build_reason" \
    --argjson benchmarks "$benchmarks_json" \
    '{build:{status:$build_status,reason:$build_reason},
      benchmarks:$benchmarks,
      modules:.modules,
      files:.files,
      tests:.tests,
      tests_build_ms:.build_ms,
      tests_build_status:.build_status}')"

  bm_write_json "$out_file" "$record"
  bm_log "==> wrote $out_file"
}

# ── assemble the daily record ────────────────────────────────────────────────
mkdir -p "$OUT/daily"

rm -rf .bench_tmp
mkdir -p .bench_tmp

if [[ ",$BACKENDS," == *",tcc,"* ]]; then
  collect_backend TCCCompiler "$TCC_BIN" "$BACKEND_TMP/backend_tcc.json"
fi
if [[ ",$BACKENDS," == *",llvm,"* ]]; then
  collect_backend Compiler "$LLVM_BIN" "$BACKEND_TMP/backend_llvm.json"
fi
if [[ ",$BACKENDS," == *",interpret,"* ]]; then
  collect_backend Interpreter "$TCC_BIN" "$BACKEND_TMP/backend_interpret.json"
fi
rm -rf .bench_tmp

# merge backend files under canonical names (same as release records + dashboard)
BACKENDS_JSON="{}"
for f in "$BACKEND_TMP"/backend_*.json; do
  [ -f "$f" ] || continue
  local_bn="$(basename "$f" .json)"
  case "${local_bn#backend_}" in
    tcc)       key="TCCCompiler" ;;
    llvm)      key="Compiler" ;;
    interpret) key="Interpreter" ;;
    *)         key="${local_bn#backend_}" ;;
  esac
  BACKENDS_JSON="$(printf '%s' "$BACKENDS_JSON" | jq --arg bn "$key" --slurpfile bf "$f" '. + {($bn): $bf[0]}')"
done

COMPILER_VERSION_TCC="$( [ -f "$TCC_BIN" ] && "$TCC_BIN" --version 2>/dev/null | head -1 | tr -d '\r' || echo "n/a" )"
COMPILER_VERSION_LLVM="$( [ -f "$LLVM_BIN" ] && "$LLVM_BIN" --version 2>/dev/null | head -1 | tr -d '\r' || echo "n/a" )"

# The backends blob can be large (module + per-file timings from every
# backend); passing it via --argjson would exceed the ~128 KB per-argument
# limit (E2BIG: "Argument list too long"). Pipe it through stdin instead and
# reference it as the input (`.`), which has no size limit.
# ── overall per-platform run status ──────────────────────────────────────────
# success = every requested backend built; partial = at least one backend
# failed; failed = nothing could be built. A backend skipped on purpose
# (--skip-llvm -> build.status "unavailable") is NOT a failure.
RUN_STATUS="success"
RUN_REASON="null"
n_ok=0 n_bad=0
for f in "$BACKEND_TMP"/backend_*.json; do
  [ -f "$f" ] || continue
  st="$(jq -r '.build.status // "unavailable"' "$f")"
  if [ "$st" = "success" ]; then n_ok=$((n_ok+1));
  elif [ "$st" = "failed" ]; then n_bad=$((n_bad+1)); fi
done
if [ "$n_ok" -eq 0 ]; then
  RUN_STATUS="failed"
  RUN_REASON="$(jq_escape "no backend could be built (see per-backend build/test logs)")"
elif [ "$n_bad" -gt 0 ]; then
  RUN_STATUS="partial"
  RUN_REASON="$(jq_escape "$n_bad backend(s) failed — see per-backend build/test reasons")"
fi

RECORD="$(printf '%s' "$BACKENDS_JSON" | jq \
  --arg type "daily" \
  --arg id "$RECORD_ID" \
  --arg date "$DATE" \
  --arg generated_at "$(bm_datetime)" \
  --argjson duration_ms "$(( $(bm_now_ms) - COLLECT_START_MS ))" \
  --arg status "$RUN_STATUS" \
  --argjson status_reason "$RUN_REASON" \
  --arg platform "$PLATFORM" \
  --arg arch "$ARCH" \
  --arg libc "$LIBC" \
  --arg sha "$SHA" \
  --arg short "$SHORT" \
  --arg subject "$SUBJECT" \
  --arg author_date "$AUTHOR_DATE" \
  --arg tcc_ver "$COMPILER_VERSION_TCC" \
  --arg llvm_ver "$COMPILER_VERSION_LLVM" \
  '{type:$type,id:$id,date:$date,generated_at:$generated_at,duration_ms:$duration_ms,status:$status,status_reason:$status_reason,
    platform:{os:$platform,arch:$arch,libc:$libc},
    commit:{sha:$sha,short:$short,subject:$subject,author_date:$author_date},
    compiler_version:{TCCCompiler:$tcc_ver,Compiler:$llvm_ver},
    backends:.}')"

OUT_FILE="$OUT/daily/$RECORD_ID.json"
bm_write_json "$OUT_FILE" "$RECORD"
bm_log "==> Done: $OUT_FILE"
