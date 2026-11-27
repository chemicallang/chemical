#!/usr/bin/env bash
# Render the P0.4 static design references at 1x.
# Requires google-chrome/chromium; no renderer library is used.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHROME="${CHROME:-$(command -v google-chrome || command -v chromium || command -v chromium-browser || true)}"
[ -n "$CHROME" ] || { echo "ui-render-design: no Chrome/Chromium found" >&2; exit 1; }
OUT="$ROOT/lang/docs/ui/design"
render() {
    local url="$1" out="$2" size="$3"
    "$CHROME" --headless=new --no-sandbox --disable-gpu --hide-scrollbars \
        --force-device-scale-factor=1 --window-size="$size" \
        --virtual-time-budget=3000 --screenshot="$out" "$url" >/dev/null 2>&1
    echo "rendered $out"
}
render "file://$OUT/reference-settings.html" "$OUT/reference-settings.png" 1280,800
render "file://$OUT/reference-dashboard.html" "$OUT/reference-dashboard.png" 1280,800
render "file://$OUT/reference-chat.html" "$OUT/reference-chat.png" 1280,800
render "file://$OUT/reference-dashboard.html?theme=dark" "$OUT/reference-dashboard-dark.png" 1280,800
render "file://$OUT/tokens.html" "$OUT/tokens.png" 1280,3245
