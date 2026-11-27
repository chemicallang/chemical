#!/usr/bin/env bash
# Screenshot/headless-shot entry point.
#
# P0.3a scaffold: the real headless host and PNG capture land in P1.10. This
# script validates its arguments and exits 78 (EX_CONFIG), writing no image.
set -euo pipefail

HEADLESS=0
THEME="light"
SCALE="1"
OUT=""

usage() {
    cat <<'EOF'
Usage: scripts/ui-shot.sh [options]

  --headless          Run without a real window (accepted; P1.10 implements it)
  --theme <name>      light | dark | high-contrast (default: light)
  --scale <n>         Logical-to-physical scale (default: 1)
  --out <path>        PNG output path (no file is written before P1.10)
  --help              Show this help

Exit code 78 means "not implemented before P1.10". No fake image is written.
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --headless) HEADLESS=1 ;;
        --theme) shift; [ $# -gt 0 ] || { echo "ui-shot: --theme needs a value" >&2; exit 2; }; THEME="$1" ;;
        --theme=*) THEME="${1#*=}" ;;
        --scale) shift; [ $# -gt 0 ] || { echo "ui-shot: --scale needs a value" >&2; exit 2; }; SCALE="$1" ;;
        --scale=*) SCALE="${1#*=}" ;;
        --out) shift; [ $# -gt 0 ] || { echo "ui-shot: --out needs a path" >&2; exit 2; }; OUT="$1" ;;
        --out=*) OUT="${1#*=}" ;;
        --help|-h) usage; exit 0 ;;
        *) echo "ui-shot: unknown argument '$1'" >&2; usage >&2; exit 2 ;;
    esac
    shift
done

case "$THEME" in
    light|dark|high-contrast) ;;
    *) echo "ui-shot: unknown theme '$THEME'" >&2; exit 2 ;;
esac
if ! [[ "$SCALE" =~ ^[0-9]+([.][0-9]+)?$ ]]; then
    echo "ui-shot: --scale must be a positive number" >&2
    exit 2
fi
if [ -n "$OUT" ] && [ -e "$OUT" ]; then
    echo "ui-shot: refusing to overwrite existing '$OUT'" >&2
    exit 2
fi

echo "ui-shot: not implemented before P1.10 (headless=$HEADLESS theme=$THEME scale=$SCALE${OUT:+ out=$OUT})" >&2
exit 78
