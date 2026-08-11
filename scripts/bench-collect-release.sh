#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
#
# collect_release.sh — collect release information and benchmarks for one tag.
#
#   * asset information (sizes, missing markers) comes from the GitHub API
#   * benchmarks + test suite are run with the release binaries AND the test
#     sources from the release's own commit — a compiler built for release X
#     can only compile the test sources of release X.
#
# Usage:
#   collect_release.sh --tag <tag> [--repo owner/name] [--platform <p>]
#                      [--arch <a>] [--out DIR] [--work DIR] [--skip-llvm]
#                      [--quick] [--info-only]
#
# Emits:
#   $OUT/releases/<tag>/info.json                      (release + assets)
#   $OUT/releases/<tag>/<platform>-<arch>.json         (benchmarks/tests, unless --info-only)
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=bench-common.sh
source "$SCRIPT_DIR/bench-common.sh"

TAG=""
REPO="${GITHUB_REPOSITORY:-chemicallang/chemical}"
PLATFORM="$(bm_platform)"
ARCH="$(bm_arch)"
OUT=""
WORK=""
SKIP_LLVM=false
QUICK=false
INFO_ONLY=false

while [ $# -gt 0 ]; do
  case "$1" in
    --tag)       TAG="$2"; shift 2 ;;
    --repo)      REPO="$2"; shift 2 ;;
    --platform)  PLATFORM="$2"; shift 2 ;;
    --arch)      ARCH="$2"; shift 2 ;;
    --out)       OUT="$2"; shift 2 ;;
    --work)      WORK="$2"; shift 2 ;;
    --skip-llvm) SKIP_LLVM=true; shift ;;
    --quick)     QUICK=true; shift ;;
    --info-only) INFO_ONLY=true; shift ;;
    *) echo "Unknown option: $1" >&2; exit 2 ;;
  esac
done

[ -z "$TAG" ] && { echo "Error: --tag is required" >&2; exit 2; }
[ -z "$OUT" ] && OUT="$DATA_ROOT"
WORK_CREATED=false
if [ -z "$WORK" ]; then WORK="$(mktemp -d)"; WORK_CREATED=true; fi
# clean up temp dirs on every exit path (not user-provided --work dirs)
trap '[ "$WORK_CREATED" = true ] && rm -rf "$WORK"' EXIT

API="https://api.github.com/repos/${REPO}/releases/tags/${TAG}"

# ── fetch release info ───────────────────────────────────────────────────────
bm_log "==> Fetching release $TAG from $REPO"
AUTH=()
if [ -n "${GITHUB_TOKEN:-}" ]; then AUTH=(-H "Authorization: token $GITHUB_TOKEN"); fi
RELEASE_JSON="$(curl -s "${AUTH[@]}" "$API")"
if ! printf '%s' "$RELEASE_JSON" | jq -e '.tag_name' >/dev/null 2>&1; then
  bm_warn "release $TAG not found via API; writing unavailable record"
  REC="$(jq -n --arg tag "$TAG" --arg date "$(bm_datetime)" '{type:"release_info",tag:$tag,generated_at:$date,status:"unavailable",reason:"release not found via GitHub API",assets:{}}')"
  bm_write_json "$OUT/releases/$TAG/info.json" "$REC"
  exit 0
fi

PUBLISHED_AT="$(printf '%s' "$RELEASE_JSON" | jq -r '.published_at // ""')"
TARGET_COMMIT="$(printf '%s' "$RELEASE_JSON" | jq -r '.target_commitish // ""')"
RELEASE_NAME="$(printf '%s' "$RELEASE_JSON" | jq -r '.name // ""')"

# ── assets: expected vs actual ───────────────────────────────────────────────
ASSETS_JSON="{}"
EXPECTED="$(expected_assets "$TAG")"
if [ -n "$EXPECTED" ]; then
  for asset in $EXPECTED; do
    info="$(printf '%s' "$RELEASE_JSON" | jq -c --arg a "$asset" '.assets[]? | select(.name==$a) | {size_bytes:.size,url:.browser_download_url}')"
    if [ -n "$info" ]; then
      ASSETS_JSON="$(printf '%s' "$ASSETS_JSON" | jq --arg a "$asset" --argjson info "$info" '. + {($a): ($info + {status:"success"})}')"
    else
      ASSETS_JSON="$(printf '%s' "$ASSETS_JSON" | jq --arg a "$asset" '. + {($a): {status:"missing_asset",size_bytes:null,url:null}}')"
    fi
  done
fi

INFO_REC="$(jq -n \
  --arg type "release_info" \
  --arg tag "$TAG" \
  --arg published_at "$PUBLISHED_AT" \
  --arg commit_sha "$TARGET_COMMIT" \
  --arg name "$RELEASE_NAME" \
  --arg generated_at "$(bm_datetime)" \
  --argjson assets "$ASSETS_JSON" \
  '{type:$type,tag:$tag,published_at:$published_at,commit_sha:$commit_sha,name:$name,generated_at:$generated_at,status:"success",assets:$assets}')"
bm_write_json "$OUT/releases/$TAG/info.json" "$INFO_REC"
bm_log "==> wrote $OUT/releases/$TAG/info.json"

if [ "$INFO_ONLY" = true ]; then
  bm_log "==> info-only; done"
  exit 0
fi

# ── pick the assets for this platform ────────────────────────────────────────
pick_asset_name() { # <variant: regular|tcc>
  local variant="$1"
  local asset
  for asset in $EXPECTED; do
    local p a v
    read -r p a v <<< "$(asset_parts "$asset")"
    if [ "$p" = "$PLATFORM" ] && [ "$v" = "$variant" ] && [ "$a" = "$ARCH" ]; then
      echo "$asset"
      return 0
    fi
  done
  return 1
}

REGULAR_ASSET="$(pick_asset_name regular || true)"
TCC_ASSET="$(pick_asset_name tcc || true)"

if [ -z "$REGULAR_ASSET" ] && [ -z "$TCC_ASSET" ]; then
  bm_warn "no assets expected for platform $PLATFORM/$ARCH in release $TAG; writing empty platform record"
  PLAT_REC="$(jq -n \
    --arg type "release_platform" --arg tag "$TAG" --arg platform "$PLATFORM" --arg arch "$ARCH" \
    --arg libc "$(bm_libc)" --arg generated_at "$(bm_datetime)" \
    --argjson assets "$ASSETS_JSON" \
    '{type:$type,tag:$tag,platform:$platform,arch:$arch,libc:$libc,generated_at:$generated_at,status:"unavailable",reason:"no expected assets for this platform",assets:$assets,backends:{}}')"
  bm_write_json "$OUT/releases/$TAG/$PLATFORM-$ARCH.json" "$PLAT_REC"
  exit 0
fi

# ── download + extract ───────────────────────────────────────────────────────
unzip_dir() { # <zip> <dest>
  if command -v unzip >/dev/null 2>&1; then
    unzip -o -q "$1" -d "$2"
  elif command -v tar >/dev/null 2>&1 && tar --version 2>/dev/null | grep -qi bsdtar; then
    # Windows runners: bsdtar handles zip; unzip and python3 may be absent
    tar -xf "$1" -C "$2"
  else
    python3 -m zipfile -e "$1" "$2"
  fi
}

download_asset() { # <asset> <destdir>
  local asset="$1" dest="$2"
  local url
  url="$(printf '%s' "$RELEASE_JSON" | jq -r --arg a "$asset" '.assets[] | select(.name==$a) | .browser_download_url')"
  [ -z "$url" ] && return 1
  curl -sL -o "$dest/$asset" "$url"
}

DL_DIR="$WORK/dl"
EXTRACT_DIR="$WORK/extract"
mkdir -p "$DL_DIR" "$EXTRACT_DIR"

DL_STATUS="success"
if [ -n "$REGULAR_ASSET" ] && [ "$SKIP_LLVM" = false ]; then
  bm_log "==> downloading $REGULAR_ASSET"
  download_asset "$REGULAR_ASSET" "$DL_DIR" || DL_STATUS="failed"
fi
if [ -n "$TCC_ASSET" ] && [ "$DL_STATUS" = "success" ]; then
  bm_log "==> downloading $TCC_ASSET"
  download_asset "$TCC_ASSET" "$DL_DIR" || DL_STATUS="failed"
fi

if [ "$DL_STATUS" != "success" ]; then
  bm_warn "asset download failed; writing platform record with failure status"
  PLAT_REC="$(jq -n \
    --arg type "release_platform" --arg tag "$TAG" --arg platform "$PLATFORM" --arg arch "$ARCH" \
    --arg libc "$(bm_libc)" --arg generated_at "$(bm_datetime)" \
    --argjson assets "$ASSETS_JSON" \
    '{type:$type,tag:$tag,platform:$platform,arch:$arch,libc:$libc,generated_at:$generated_at,status:"benchmark_failure",reason:"release asset download failed",assets:$assets,backends:{}}')"
  bm_write_json "$OUT/releases/$TAG/$PLATFORM-$ARCH.json" "$PLAT_REC"
  exit 0
fi

for zip in "$DL_DIR"/*.zip; do
  [ -f "$zip" ] || continue
  bm_log "==> extracting $(basename "$zip")"
  if ! unzip_dir "$zip" "$EXTRACT_DIR"; then
    DL_STATUS="failed"
    bm_warn "extraction failed for $(basename "$zip")"
    break
  fi
done

if [ "$DL_STATUS" != "success" ]; then
  bm_warn "asset extraction failed; writing platform record with failure status"
  PLAT_REC="$(jq -n \
    --arg type "release_platform" --arg tag "$TAG" --arg platform "$PLATFORM" --arg arch "$ARCH" \
    --arg libc "$(bm_libc)" --arg generated_at "$(bm_datetime)" \
    --argjson assets "$ASSETS_JSON" \
    '{type:$type,tag:$tag,platform:$platform,arch:$arch,libc:$libc,generated_at:$generated_at,status:"benchmark_failure",reason:"release asset extraction failed",assets:$assets,backends:{}}')"
  bm_write_json "$OUT/releases/$TAG/$PLATFORM-$ARCH.json" "$PLAT_REC"
  exit 0
fi

# determine release roots (fixed names inside the archives)
REGULAR_ROOT=""
TCC_ROOT=""
if [ -n "$REGULAR_ASSET" ] && [ -d "$EXTRACT_DIR/chemical" ]; then REGULAR_ROOT="$EXTRACT_DIR/chemical"; fi
if [ -n "$TCC_ASSET" ] && [ -d "$EXTRACT_DIR/chemical-tcc" ]; then TCC_ROOT="$EXTRACT_DIR/chemical-tcc"; fi

# binary name inside the archive (windows ships .exe)
case "$PLATFORM" in
  windows|windows-mingw|windows-mingw-msvcrt) RELEASE_BIN="chemical.exe" ;;
  *) RELEASE_BIN="chemical" ;;
esac

# ── release-commit test sources ──────────────────────────────────────────────
# A compiler built for release X can only compile the test sources of release X.
# Extract lang/tests at the release tag from the git checkout (the workflow
# clones with tags) and merge them into the release roots.
TESTS_SRC=""
if git rev-parse --verify "refs/tags/$TAG" >/dev/null 2>&1; then
  TESTS_SRC="$WORK/tag-tests"
  bm_log "==> extracting lang/tests at tag $TAG"
  if git archive "refs/tags/$TAG" lang/tests 2>/dev/null | tar -x -C "$WORK" 2>/dev/null; then
    [ -d "$WORK/lang/tests" ] && mv "$WORK/lang/tests" "$TESTS_SRC"
  fi
fi

merge_tests() { # <release_root>
  local root="$1"
  if [ -n "$TESTS_SRC" ] && [ -d "$TESTS_SRC" ] && [ ! -d "$root/lang/tests" ]; then
    mkdir -p "$root/lang"
    cp -r "$TESTS_SRC" "$root/lang/tests"
  fi
}

# ── per-backend benchmarks (release binaries) ────────────────────────────────
collect_backend_release() { # <backend> <release_root> <binary> <json_out>
  local backend="$1" root="$2" bin="$3" out_file="$4"
  bm_log "==> Backend: $backend (release $TAG)"
  merge_tests "$root"

  if [ ! -f "$root/$bin" ]; then
    REC="$(jq -n --arg backend "$backend" \
      '{build:{status:"unavailable",reason:"binary not present in release asset"},
        benchmarks:[],modules:[],files:[],tests:{status:"unavailable",reason:"binary not present in release asset","total":null,"passed":null,"failed":null,"duration_ms":null,"failed_tests":[]},
        tests_build_ms:null,tests_build_status:"unavailable"}')"
    bm_write_json "$out_file" "$REC"
    return
  fi

  local logs="$OUT/logs/release-$TAG/$PLATFORM-$ARCH"
  mkdir -p "$logs"

  # hello benchmarks + module/tests run inside the release root so that the
  # bundled libs and relative libtcc resolution work.
  local bm_bare="" bm_std="" mods_tests=""
  ( cd "$root" && bench_hello "$backend" "$bin" bare "hello_bare" bm_bare "$logs"
    bench_hello "$backend" "$bin" std "hello_std" bm_std "$logs"
    bench_modules_and_tests "$backend" "$bin" "$backend" mods_tests "$logs" "$QUICK"
    printf '{"bare":%s,"std":%s,"mods":%s}' "$bm_bare" "$bm_std" "$mods_tests" ) > "$WORK/backend_out.json"

  # the subshell can't propagate the outvars; parse the emitted JSON instead
  local em
  em="$(cat "$WORK/backend_out.json")"
  local benchmarks_json
  benchmarks_json="[$(printf '%s' "$em" | jq -c '.bare'),$(printf '%s' "$em" | jq -c '.std')]"
  local mods_tests_json
  mods_tests_json="$(printf '%s' "$em" | jq -c '.mods')"

  local build_status
  build_status="$(printf '%s' "$mods_tests_json" | jq -r '.build_status // "unavailable"')"
  local build_reason="null"
  if [ "$build_status" != "success" ]; then
    build_reason="$(jq_escape "test suite build status: $build_status")"
  fi

  local rec
  rec="$(printf '%s' "$mods_tests_json" | jq --arg backend "$backend" \
    --arg build_status "$build_status" --argjson build_reason "$build_reason" \
    --argjson benchmarks "$benchmarks_json" \
    '{build:{status:$build_status,reason:$build_reason},
      benchmarks:$benchmarks,
      modules:.modules,
      files:.files,
      tests:.tests,
      tests_build_ms:.build_ms,
      tests_build_status:.build_status}')"
  bm_write_json "$out_file" "$rec"
  bm_log "==> wrote $out_file"
}

BACKENDS_JSON="{}"
if [ -n "$TCC_ROOT" ] && [ -f "$TCC_ROOT/$RELEASE_BIN" ]; then
  collect_backend_release TCCCompiler "$TCC_ROOT" "$RELEASE_BIN" "$WORK/backend_tcc.json"
  BACKENDS_JSON="$(printf '%s' "$BACKENDS_JSON" | jq --slurpfile bf "$WORK/backend_tcc.json" '. + {TCCCompiler: $bf[0]}')"
fi
if [ -n "$TCC_ROOT" ] && [ -f "$TCC_ROOT/$RELEASE_BIN" ]; then
  collect_backend_release Interpreter "$TCC_ROOT" "$RELEASE_BIN" "$WORK/backend_interpret.json"
  BACKENDS_JSON="$(printf '%s' "$BACKENDS_JSON" | jq --slurpfile bf "$WORK/backend_interpret.json" '. + {Interpreter: $bf[0]}')"
fi
if [ "$SKIP_LLVM" = false ] && [ -n "$REGULAR_ROOT" ] && [ -f "$REGULAR_ROOT/$RELEASE_BIN" ]; then
  collect_backend_release Compiler "$REGULAR_ROOT" "$RELEASE_BIN" "$WORK/backend_llvm.json"
  BACKENDS_JSON="$(printf '%s' "$BACKENDS_JSON" | jq --slurpfile bf "$WORK/backend_llvm.json" '. + {Compiler: $bf[0]}')"
fi

PLAT_REC="$(jq -n \
  --arg type "release_platform" \
  --arg tag "$TAG" \
  --arg platform "$PLATFORM" \
  --arg arch "$ARCH" \
  --arg libc "$(bm_libc)" \
  --arg generated_at "$(bm_datetime)" \
  --argjson assets "$ASSETS_JSON" \
  --argjson backends "$BACKENDS_JSON" \
  '{type:$type,tag:$tag,platform:$platform,arch:$arch,libc:$libc,generated_at:$generated_at,status:"success",assets:$assets,backends:$backends}')"
bm_write_json "$OUT/releases/$TAG/$PLATFORM-$ARCH.json" "$PLAT_REC"
bm_log "==> Done: $OUT/releases/$TAG/$PLATFORM-$ARCH.json"
