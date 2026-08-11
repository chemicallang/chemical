#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
#
# backfill.sh — populate the historical benchmark dataset.
#
# Commit history is collected at a *daily* cadence by default (one compiler
# build per day = the last commit on main at the end of that day), because
# per-commit building of ~1500 commits is impractical and the user's operating
# model is "one data point per date". A per-commit mode is also available.
#
# Every commit is benchmarked with ITS OWN test sources (a compiler built at
# commit X can only compile the test sources of commit X). Failures are
# recorded as data points (build_failure / unavailable) and collection
# continues. Already-collected dates/releases are skipped, so re-runs resume.
#
# GitHub Actions runner time limits: TCCCompiler builds take ~15 min and the
# LLVM Compiler ~20 min. Use --time-budget-min to stop a job before the runner
# kills it (re-dispatch with the same args to resume — collected points are
# skipped). Use --offset-days to split the range across parallel matrix jobs.
#
# Usage:
#   backfill.sh [--days N] [--mode daily|commits] [--limit N] [--out DIR]
#               [--offset-days N] [--time-budget-min N]
#               [--skip-llvm] [--quick] [--releases-only] [--commits-only]
#               [--no-releases] [--repo owner/name]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=bench-common.sh
source "$SCRIPT_DIR/bench-common.sh"

DAYS=30
MODE="daily"
LIMIT=0
OFFSET_DAYS=0
TIME_BUDGET_MIN=0
OUT=""
SKIP_LLVM=true
QUICK=false
RELEASES=true
DO_COMMITS=true
REPO="${GITHUB_REPOSITORY:-chemicallang/chemical}"
START_TS="$(date +%s)"

while [ $# -gt 0 ]; do
  case "$1" in
    --days)          DAYS="$2"; shift 2 ;;
    --mode)          MODE="$2"; shift 2 ;;
    --limit)         LIMIT="$2"; shift 2 ;;
    --offset-days)   OFFSET_DAYS="$2"; shift 2 ;;
    --time-budget-min) TIME_BUDGET_MIN="$2"; shift 2 ;;
    --out)           OUT="$2"; shift 2 ;;
    --skip-llvm)     SKIP_LLVM=true; shift ;;
    --no-skip-llvm)  SKIP_LLVM=false; shift ;;
    --quick)         QUICK=true; shift ;;
    --releases-only) RELEASES=true; DO_COMMITS=false; shift ;;
    --commits-only)  DO_COMMITS=true; RELEASES=false; shift ;;
    --no-releases)   RELEASES=false; shift ;;
    --repo)          REPO="$2"; shift 2 ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

[ -z "$OUT" ] && OUT="$DATA_ROOT"
mkdir -p "$OUT/daily" "$OUT/releases"

# True once the time budget (if any) has been exhausted. Budget is checked
# *between* points so a point already in progress always finishes.
budget_exhausted() {
  [ "$TIME_BUDGET_MIN" -le 0 ] && return 1
  local elapsed=$(( ($(date +%s) - START_TS) / 60 ))
  [ "$elapsed" -ge "$TIME_BUDGET_MIN" ]
}

MAIN_ROOT="$(git rev-parse --show-toplevel)"
WORKTREES="$(mktemp -d)"
FAILURES=0

collect_one_commit() { # <date> <sha>
  local date="$1" sha="$2"
  local out_file="$OUT/daily/$date.json"
  if [ -f "$out_file" ]; then
    bm_log "==> $date already collected, skipping"
    return 0
  fi
  bm_log "==> [$date] benchmarking commit ${sha:0:12}"
  local wt="$WORKTREES/$date"
  if ! git worktree add --detach "$wt" "$sha" >/dev/null 2>&1; then
    bm_warn "[$date] worktree checkout failed for $sha — recording build_failure"
    REC="$(jq -n --arg type daily --arg date "$date" --arg sha "$sha" \
      --arg generated_at "$(bm_datetime)" \
      '{type:$type,date:$date,generated_at:$generated_at,status:"build_failure",
        reason:"git checkout of commit failed",commit:{sha:$sha,short:($sha[0:12])},backends:{}}')"
    bm_write_json "$out_file" "$REC"
    FAILURES=$((FAILURES + 1))
    return 0
  fi
  # the compiler build needs the downloaded tcc package + the lsp submodule
  bm_link_shared_tooling "$wt" "$MAIN_ROOT"
  if [ ! -f "$wt/cmake-build-debug/CMakeCache.txt" ]; then
    ( cd "$wt" && cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_COMPILER=OFF >/dev/null 2>&1 || true )
  fi
  if bash "$SCRIPT_DIR/bench-collect-daily.sh" --commit "$sha" --date "$date" --out "$OUT" --work "$wt" --skip-llvm --backends tcc,interpret $( [ "$QUICK" = true ] && echo --quick ); then
    bm_log "==> [$date] collected"
  else
    bm_warn "[$date] collect_daily failed — recording failure record"
    REC="$(jq -n --arg type daily --arg date "$date" --arg sha "$sha" \
      --arg generated_at "$(bm_datetime)" \
      '{type:$type,date:$date,generated_at:$generated_at,status:"benchmark_failure",
        reason:"collect_daily failed",commit:{sha:$sha,short:($sha[0:12])},backends:{}}')"
    bm_write_json "$out_file" "$REC"
    FAILURES=$((FAILURES + 1))
  fi
  git worktree remove --force "$wt" >/dev/null 2>&1 || true
}

# ── commits ──────────────────────────────────────────────────────────────────
if [ "$DO_COMMITS" = true ]; then
  if [ "$MODE" = "daily" ]; then
    bm_log "==> daily mode: days ${OFFSET_DAYS}+1 .. $((OFFSET_DAYS + DAYS)) (offset $OFFSET_DAYS, budget ${TIME_BUDGET_MIN}min)"
    done_days=0
    for i in $(seq $((OFFSET_DAYS + 1)) $((OFFSET_DAYS + DAYS))); do
      [ "$LIMIT" -gt 0 ] && [ "$done_days" -ge "$LIMIT" ] && break
      if budget_exhausted; then
        bm_log "==> time budget (${TIME_BUDGET_MIN}min) reached; stopping (re-dispatch to resume)"
        break
      fi
      d="$(date -u -d "-$i days" +%Y-%m-%d 2>/dev/null || date -u -v-${i}d +%Y-%m-%d)"
      sha="$(git log -1 --before="$d 23:59:59" --format=%H 2>/dev/null || true)"
      [ -z "$sha" ] && continue
      collect_one_commit "$d" "$sha"
      done_days=$((done_days + 1))
    done
  else
    bm_log "==> commits mode (offset $OFFSET_DAYS, budget ${TIME_BUDGET_MIN}min)"
    n=150
    [ "$LIMIT" -gt 0 ] && n="$LIMIT"
    skipped=0
    while IFS= read -r sha; do
      [ -z "$sha" ] && continue
      if [ "$skipped" -lt "$OFFSET_DAYS" ]; then
        skipped=$((skipped + 1))
        continue
      fi
      if budget_exhausted; then
        bm_log "==> time budget (${TIME_BUDGET_MIN}min) reached; stopping (re-dispatch to resume)"
        break
      fi
      d="$(git log -1 --format=%ci "$sha" 2>/dev/null | cut -d' ' -f1)"
      collect_one_commit "$d" "$sha"
    done < <(git log --format=%H -n "$n" 2>/dev/null || true)
  fi
fi

# ── releases ─────────────────────────────────────────────────────────────────
if [ "$RELEASES" = true ]; then
  bm_log "==> collecting releases"
  AUTH=()
  if [ -n "${GITHUB_TOKEN:-}" ]; then AUTH=(-H "Authorization: token $GITHUB_TOKEN"); fi
  page=1
  while :; do
    if budget_exhausted; then
      bm_log "==> time budget (${TIME_BUDGET_MIN}min) reached during releases; stopping (re-dispatch to resume)"
      break
    fi
    json="$(curl -s "${AUTH[@]}" "https://api.github.com/repos/${REPO}/releases?per_page=100&page=$page")"
    count="$(printf '%s' "$json" | jq 'length')"
    [ "$count" -eq 0 ] && break
    while IFS= read -r tag; do
      [ -z "$tag" ] && continue
      if budget_exhausted; then
        bm_log "==> time budget (${TIME_BUDGET_MIN}min) reached during releases; stopping (re-dispatch to resume)"
        break 2
      fi
      if [ -f "$OUT/releases/$tag/info.json" ]; then
        bm_log "==> release $tag already collected, skipping"
        continue
      fi
      bm_log "==> collecting release $tag"
      if bash "$SCRIPT_DIR/bench-collect-release.sh" --tag "$tag" --out "$OUT" --repo "$REPO" --skip-llvm $( [ "$QUICK" = true ] && echo --quick ); then
        bm_log "==> release $tag collected"
      else
        bm_warn "release $tag collection failed"
        FAILURES=$((FAILURES + 1))
      fi
    done < <(printf '%s' "$json" | jq -r '.[].tag_name')
    page=$((page + 1))
    [ "$page" -gt 20 ] && break
  done
fi

rm -rf "$WORKTREES"
ELAPSED_MIN=$(( ($(date +%s) - START_TS) / 60 ))
bm_log "==> backfill finished in ${ELAPSED_MIN}min with $FAILURES recorded failure(s)"
[ "$FAILURES" -eq 0 ] || bm_warn "some entries were recorded with failure status (see data/)"
exit 0
