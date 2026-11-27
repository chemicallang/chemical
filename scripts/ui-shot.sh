#!/usr/bin/env bash
# P1.10 headless screenshot entry point. Renders the built-in 3-node fixture.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HEADLESS=0
THEME="light"
SCALE="1"
OUT=""
usage() {
    cat <<'USAGE'
Usage: scripts/ui-shot.sh [options]
  --headless          Render without a window (P1.10 headless host)
  --theme <name>      light | dark | high-contrast (default: light)
  --scale <n>         Logical-to-physical scale (default: 1)
  --out <path>        PNG output path (required)
  --help              Show this help
USAGE
}
while [ $# -gt 0 ]; do
    case "$1" in
        --headless) HEADLESS=1 ;;
        --theme) shift; [ $# -gt 0 ] || { echo "ui-shot: --theme needs a value" >&2; exit 2; }; THEME="$1" ;;
        --theme=*) THEME="${1#*=}" ;;
        --scale) shift; [ $# -gt 0 ] || { echo "ui-shot: --scale needs a value" >&2; exit 2; }; SCALE="$1" ;;
        --scale=*) SCALE="${1#*=}" ;;
        --out) shift; [ $# -gt 0 ] || { echo "ui-shot: --out needs a value" >&2; exit 2; }; OUT="$1" ;;
        --out=*) OUT="${1#*=}" ;;
        --help|-h) usage; exit 0 ;;
        *) echo "ui-shot: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done
case "$THEME" in light|dark|high-contrast) ;; *) echo "ui-shot: unknown theme '$THEME'" >&2; exit 2 ;; esac
if ! [[ "$SCALE" =~ ^[0-9]+([.][0-9]+)?$ ]]; then echo "ui-shot: --scale must be a positive number" >&2; exit 2; fi
if [ -z "$OUT" ]; then echo "ui-shot: --out is required" >&2; exit 2; fi
if [ -e "$OUT" ]; then echo "ui-shot: refusing to overwrite existing '$OUT'" >&2; exit 2; fi
BUILD="$ROOT/lang/tests/ui/shot/build"
mkdir -p "$BUILD"
"$ROOT/cmake-build-debug/TCCCompiler" "$ROOT/lang/tests/ui/shot/chemical.mod" \
    -o "$BUILD/ui_shot.exe" --mode debug_quick --no-cache >"$BUILD/build.log" 2>&1
CHEMICAL_UI_SHOT_OUT="$OUT" CHEMICAL_UI_SHOT_THEME="$THEME" CHEMICAL_UI_SHOT_SCALE="$SCALE" \
    "$BUILD/ui_shot.exe"
if [ ! -s "$OUT" ]; then echo "ui-shot: no PNG was written" >&2; exit 1; fi
echo "ui-shot: wrote $OUT (headless=$HEADLESS theme=$THEME scale=$SCALE)"
