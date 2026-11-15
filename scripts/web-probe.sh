#!/usr/bin/env bash
# Web-stack probe — thin wrapper around scripts/web_probe.py.
#
# Phase 0 of lang/docs/web-stack-plan.md. Reports the numbers the plan makes
# promises about (runtime share, dispatch-text share, cross-page duplication,
# minify headroom, hashed assets, CSP-safe output) and gates on the ones that
# are already invariants today.
#
#   ./scripts/web-probe.sh                          # analyze the components app output
#   ./scripts/web-probe.sh analyze <dir>            # analyze any build output dir
#   ./scripts/web-probe.sh analyze <dir> --strict   # exit non-zero on a violation
#   ./scripts/web-probe.sh compare a.js b.js        # cross-page duplication
set -u

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cmd="${1:-analyze}"

case "$cmd" in
    analyze|compare)
        shift
        exec python3 "$here/web_probe.py" "$cmd" "$@"
        ;;
    -h|--help|help)
        sed -n '2,16p' "${BASH_SOURCE[0]}"
        ;;
    *)
        # Bare directory argument is treated as `analyze <dir>`.
        if [ -d "$cmd" ]; then
            exec python3 "$here/web_probe.py" analyze "$@"
        fi
        echo "web-probe: unknown command '$cmd' (try --help)" >&2
        exit 2
        ;;
esac
