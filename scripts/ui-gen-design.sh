#!/usr/bin/env bash
# Generate the P0.4 static design references from tokens.json.
#   scripts/ui-gen-design.sh            # regenerate
#   scripts/ui-gen-design.sh --check    # fail on drift
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
exec python3 "$ROOT/scripts/ui_gen_design.py" "$@"
