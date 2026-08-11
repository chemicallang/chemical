#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
#
# bench-push-pages.sh — publish the benchmark data to the gh-pages branch of
# this repository. The branch layout is:
#
#   gh-pages/
#     index.html            # dashboard (site lives ON this branch)
#     assets/               # css/js/Chart.js
#     README.md             # dashboard/system docs
#     data/manifest.json    # index of available data files
#     data/daily/<date>.json
#     data/releases/<tag>/info.json
#     data/releases/<tag>/<platform>-<arch>.json
#
# The dashboard site, README and assets are maintained ON the gh-pages branch
# itself (they are NOT kept on main). This script only ever writes data/ (and
# the manifest) — it never touches the site files unless --site is passed
# explicitly, and it refuses to push to main/master/default.
#
# GitHub Pages serves the branch root; the dashboard is pure static JS that
# fetches data/*.json at runtime. Deployment is therefore just this push.
#
# Usage:
#   bench-push-pages.sh [--data DIR] [--site DIR] [--branch gh-pages]
#                       [--message MSG] [--no-push]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=bench-common.sh
source "$SCRIPT_DIR/bench-common.sh"

DATA_DIR="${DATA_DIR:-$DATA_ROOT}"
SITE_DIR="${SITE_DIR:-}"          # optional: refresh dashboard files too
BRANCH="${BRANCH:-gh-pages}"
MESSAGE="${MESSAGE:-Update benchmark data $(bm_date_utc)}"
PUSH=true

while [ $# -gt 0 ]; do
  case "$1" in
    --data)    DATA_DIR="$2"; shift 2 ;;
    --site)    SITE_DIR="$2"; shift 2 ;;
    --branch)  BRANCH="$2"; shift 2 ;;
    --message) MESSAGE="$2"; shift 2 ;;
    --no-push) PUSH=false; shift ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

# ── absolutize paths AFTER arg parsing ───────────────────────────────────────
# The script cd's into a temp worktree later; a relative --data/--site path
# would silently resolve against the wrong directory. If the data dir does
# not exist (e.g. collection failed before writing anything), fall back to an
# empty overlay — with the non-destructive merge below that is a harmless
# no-op publish rather than a hard failure.
if [ ! -d "$DATA_DIR" ]; then
  bm_warn "data dir '$DATA_DIR' not found — publishing an empty overlay (existing published data is preserved)"
  DATA_DIR="$(mktemp -d)"
else
  DATA_DIR="$(cd "$DATA_DIR" && pwd)"
fi
if [ -n "$SITE_DIR" ] && [ ! -d "$SITE_DIR" ]; then
  bm_warn "site dir '$SITE_DIR' not found — skipping site refresh"
  SITE_DIR=""
elif [ -n "$SITE_DIR" ]; then
  SITE_DIR="$(cd "$SITE_DIR" && pwd)"
fi

# ── never publish to the default branch ─────────────────────────────────────
# Benchmark data must never land on main. Guard against both the obvious names
# and whatever the remote actually considers its default branch.
DEFAULT_BRANCH="$(git symbolic-ref refs/remotes/origin/HEAD 2>/dev/null | sed 's|.*/||' || true)"
case "$BRANCH" in
  main|master) echo "Refusing to publish to '$BRANCH' — benchmark data must never be committed to the default branch" >&2; exit 3 ;;
esac
if [ -n "$DEFAULT_BRANCH" ] && [ "$BRANCH" = "$DEFAULT_BRANCH" ]; then
  echo "Refusing to publish to default branch '$BRANCH'" >&2
  exit 3
fi

COMMITTER_NAME="${COMMITTER_NAME:-github-actions[bot]}"
COMMITTER_EMAIL="${COMMITTER_EMAIL:-41898282+github-actions[bot]@users.noreply.github.com}"

# ── prepare the gh-pages worktree ────────────────────────────────────────────
WT="$(mktemp -d)"
trap 'git worktree remove --force "$WT" >/dev/null 2>&1 || true; rm -rf "$WT"' EXIT

git fetch origin "$BRANCH:$BRANCH" >/dev/null 2>&1 || true
git worktree add --detach "$WT" HEAD >/dev/null 2>&1
cd "$WT"
if git rev-parse --verify "refs/heads/$BRANCH" >/dev/null 2>&1; then
  # Safety: if the checkout fails (e.g. the branch is checked out in another
  # local worktree) we must NOT fall through to a detached HEAD at main's
  # tree and push that to gh-pages. Fail loudly instead.
  if ! git checkout -f "$BRANCH" >/dev/null 2>&1 || [ "$(git branch --show-current 2>/dev/null || true)" != "$BRANCH" ]; then
    echo "Refusing to publish: could not check out '$BRANCH' (is it open in another worktree?)" >&2
    exit 3
  fi
else
  git checkout --orphan "$BRANCH" >/dev/null 2>&1
  git rm -rf --quiet . >/dev/null 2>&1 || true
fi

# ── optional site refresh (maintainers editing the dashboard) ───────────────
if [ -n "$SITE_DIR" ] && [ -d "$SITE_DIR" ]; then
  bm_log "refreshing dashboard site from $SITE_DIR"
  cp -r "$SITE_DIR"/. .
fi

# ── data only (site files above are preserved unless --site was given) ───────
# Overlay the newly collected data on top of whatever is already published —
# never wipe. A publish with an empty or partial local collection (e.g. only
# releases collected) must NOT delete daily files published earlier; losing
# history was the root cause of the empty dashboard.
mkdir -p data
if [ -d "$DATA_DIR/daily" ]; then cp -r "$DATA_DIR/daily" data/; fi
if [ -d "$DATA_DIR/releases" ]; then cp -r "$DATA_DIR/releases" data/; fi

# ── build the manifest from the MERGED data tree ─────────────────────────────
# (computed after the overlay so the manifest always matches what is actually
# being committed; see bench-common.sh for the parse helpers).
# NOTE: under set -euo pipefail a non-matching glob makes `ls` fail, so the
# whole pipeline reports failure — guard with `|| true` (NOT `|| echo '[]'`,
# which would append a second value after jq's own [] on empty input).
DAILY_JSON="$(ls data/daily/*.json 2>/dev/null | sed 's|.*/||; s|\.json$||' | sort -r | jq -R -s 'split("\n") | map(select(length>0))' 2>/dev/null || true)"
[ -z "$DAILY_JSON" ] && DAILY_JSON='[]'

RELEASES_JSON="{}"
for tagdir in data/releases/*/; do
  [ -d "$tagdir" ] || continue
  tag="$(basename "$tagdir")"
  PLATFORMS="[]"
  for pf in "$tagdir"*.json; do
    [ -f "$pf" ] || continue
    base="$(basename "$pf" .json)"
    [ "$base" = "info" ] && continue
    PLATFORMS="$(printf '%s' "$PLATFORMS" | jq --arg p "$base" '. + [$p]')"
  done
  RELEASES_JSON="$(printf '%s' "$RELEASES_JSON" | jq --arg t "$tag" --argjson pf "$PLATFORMS" '. + {($t): $pf}')"
done

MANIFEST="$(jq -n \
  --arg generated_at "$(bm_datetime)" \
  --argjson daily "$DAILY_JSON" \
  --argjson releases "$RELEASES_JSON" \
  '{generated_at:$generated_at,daily:$daily,releases:$releases}')"
printf '%s\n' "$MANIFEST" > data/manifest.json

git config user.name "$COMMITTER_NAME"
git config user.email "$COMMITTER_EMAIL"
git add -A
if git diff --cached --quiet; then
  bm_log "no changes to publish"
  exit 0
fi
git commit -q -m "$MESSAGE"

if [ "$PUSH" = true ]; then
  bm_log "pushing to $BRANCH"
  ATTEMPTS=0
  while [ "$ATTEMPTS" -lt 5 ]; do
    if git push origin "HEAD:$BRANCH" 2>/dev/null; then
      bm_log "pushed to $BRANCH"
      exit 0
    fi
    ATTEMPTS=$((ATTEMPTS + 1))
    sleep 3
    git pull --rebase origin "$BRANCH" >/dev/null 2>&1 || true
  done
  bm_warn "failed to push after $ATTEMPTS attempts"
  exit 1
fi
bm_log "--no-push: published to local worktree only"
