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
    --skip-llvm) SKIP_LLVM=true; shift ;;
    --no-build) NO_BUILD=true; shift ;;
    --backends) BACKENDS="$2"; shift 2 ;;
    --quick)    QUICK=true; shift ;;   # skip tests, hello + modules only
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

[ -z "$OUT" ] && OUT="$DATA_ROOT"
[ -z "$DATE" ] && DATE="$(bm_date_utc)"
[ -z "$COMMIT" ] && COMMIT="$(git rev-parse HEAD)"

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
if [ ! -f cmake-build-debug/CMakeCache.txt ]; then
  bm_log "configuring cmake (BUILD_COMPILER=OFF)..."
  cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_COMPILER=OFF >/dev/null 2>&1 \
    || bm_warn "cmake configure failed; builds will be recorded as failures"
fi

# ── commit metadata ──────────────────────────────────────────────────────────
SHA="$(git rev-parse HEAD)"
SHORT="${SHA:0:12}"
SUBJECT="$(git log -1 --format=%s 2>/dev/null | head -c 200 || echo '')"
AUTHOR_DATE="$(git log -1 --format=%aI 2>/dev/null || echo '')"

bm_log "==> Benchmarking commit $SHORT ($SUBJECT) on $(bm_platform)/$(bm_arch)"

PLATFORM="$(bm_platform)"
ARCH="$(bm_arch)"
LIBC="$(bm_libc)"

case "$PLATFORM" in
  windows) TCC_BIN="cmake-build-debug/TCCCompiler.exe"; LLVM_BIN="cmake-build-debug/Compiler.exe" ;;
  *)       TCC_BIN="cmake-build-debug/TCCCompiler";     LLVM_BIN="cmake-build-debug/Compiler" ;;
esac

LOGS="$OUT/logs/$DATE"
mkdir -p "$LOGS"
BACKEND_TMP="$(mktemp -d)"
trap 'rm -rf "$BACKEND_TMP"' EXIT

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
  if timeout 1800 cmake --build cmake-build-debug --config Debug --target "$target" -j "$(nproc 2>/dev/null || echo 2)" > "$log" 2>&1; then
    echo "success"
  else
    echo "failed"
  fi
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
    mods_tests='{"modules":[],"tests":{"status":"unavailable","reason":"compiler build failed","total":null,"passed":null,"failed":null,"duration_ms":null,"failed_tests":[]},"build_ms":null,"build_status":"unavailable"}'
  fi

  local record
  record="$(printf '%s' "$mods_tests" | jq --arg backend "$backend" \
    --arg build_status "$build_status" \
    --argjson build_reason "$build_reason" \
    --argjson benchmarks "$benchmarks_json" \
    '{build:{status:$build_status,reason:$build_reason},
      benchmarks:$benchmarks,
      modules:.modules,
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

RECORD="$(jq -n \
  --arg type "daily" \
  --arg date "$DATE" \
  --arg generated_at "$(bm_datetime)" \
  --arg platform "$PLATFORM" \
  --arg arch "$ARCH" \
  --arg libc "$LIBC" \
  --arg sha "$SHA" \
  --arg short "$SHORT" \
  --arg subject "$SUBJECT" \
  --arg author_date "$AUTHOR_DATE" \
  --arg tcc_ver "$COMPILER_VERSION_TCC" \
  --arg llvm_ver "$COMPILER_VERSION_LLVM" \
  --argjson backends "$BACKENDS_JSON" \
  '{type:$type,date:$date,generated_at:$generated_at,
    platform:{os:$platform,arch:$arch,libc:$libc},
    commit:{sha:$sha,short:$short,subject:$subject,author_date:$author_date},
    compiler_version:{TCCCompiler:$tcc_ver,Compiler:$llvm_ver},
    backends:$backends}')"

OUT_FILE="$OUT/daily/$DATE.json"
bm_write_json "$OUT_FILE" "$RECORD"
bm_log "==> Done: $OUT_FILE"
