#!/usr/bin/env bash
# backfill-release-assets.sh — one-time repair of historical release data.
#
# The old `expected_assets()` approach left releases without a version-era
# entry (v0.0.1..v0.0.8) with EMPTY asset lists, and missed assets that used
# different naming conventions. This fetches the COMPLETE actual asset list
# for every release from the GitHub API and rewrites each release's info.json
# (keeping the existing record's fields), then publishes to gh-pages.
#
# Usage: bash scripts/backfill-release-assets.sh [--no-push]
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/bench-common.sh"

PUSH=true
[ "${1:-}" = "--no-push" ] && PUSH=false

REPO="${GITHUB_REPOSITORY:-chemicallang/chemical}"
AUTH=()
if [ -n "${GITHUB_TOKEN:-}" ]; then AUTH=(-H "Authorization: token $GITHUB_TOKEN"); fi

DATA_DIR="$DATA_ROOT"
mkdir -p "$DATA_DIR/releases"
page=1
updated=0
while :; do
  json="$(curl -s "${AUTH[@]}" "https://api.github.com/repos/${REPO}/releases?per_page=100&page=${page}")"
  count="$(printf '%s' "$json" | jq 'length')"
  [ "$count" -eq 0 ] && break

  # every release object already carries its assets — no per-tag API calls.
  # Process ALL releases in ONE jq pass (fast; per-release jq spawning is
  # needlessly slow) and write each info.json from the emitted JSON lines.
  GEN="$(bm_datetime)"
  printf '%s' "$json" | jq -r -c --arg gen "$GEN" '
    .[] |
    ( .assets | map({key:.name, value:{size_bytes:.size, url:.browser_download_url, status:"success"}}) | from_entries ) as $assets |
    "\(.tag_name)\t" +
    ({type:"release_info", tag:.tag_name, published_at:(.published_at // ""),
      commit_sha:(.target_commitish // ""), name:(.name // ""),
      generated_at:$gen, status:"success", assets:$assets} | tojson)
  ' | while IFS=$'\t' read -r tag info_json; do
    [ -z "$tag" ] || [ -z "$info_json" ] && continue
    bm_log "==> rewriting release $tag assets"
    # era-expected missing markers (explicit missing_asset, never "failed")
    EXPECTED="$(expected_assets "$tag")"
    if [ -n "$EXPECTED" ]; then
      info_json="$(printf '%s' "$info_json" | jq -c --argjson exp "$(printf '%s' "$EXPECTED" | jq -R 'split(" ") | map(select(length>0))')" \
        '.assets = (.assets + (reduce $exp[] as $a ({}; if has($a) then . else . + {($a): {status:"missing_asset",size_bytes:null,url:null}} end)))')"
    fi
    bm_write_json "$DATA_DIR/releases/$tag/info.json" "$info_json"
    echo "1" >> "$DATA_DIR/.updated"
  done
  if [ -f "$DATA_DIR/.updated" ]; then
    updated=$((updated + $(wc -l < "$DATA_DIR/.updated")))
    rm -f "$DATA_DIR/.updated"
  fi
  page=$((page + 1))
  [ "$page" -gt 20 ] && break
done

bm_log "==> updated $updated releases"
if [ "$PUSH" = true ]; then
  bash "$SCRIPT_DIR/bench-push-pages.sh"
else
  bm_log "--no-push: data updated locally only"
fi
