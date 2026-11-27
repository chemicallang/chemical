#!/usr/bin/env bash
# Generate or check lang/libs/ui/src/theme.ch from lang/docs/ui/tokens.json.
#
#   scripts/ui-gen-theme.sh            # regenerate theme.ch
#   scripts/ui-gen-theme.sh --check    # fail on drift
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec python3 "$ROOT/scripts/ui_gen_theme.py" "$@"
