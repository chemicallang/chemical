#!/usr/bin/env bash
# Runs every JS-backend differential fixture.
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

fail=0
for fixture in js_backend js_backend_std js_backend_closure js_backend_iter js_backend_json js_backend_dyn; do
    echo
    echo "############ $fixture"
    if ./scripts/js-backend-test.sh "$fixture" 2>&1 | tail -4; then
        :
    else
        fail=1
    fi
done

# The runtime's own checks (data model and C integer semantics).
echo
echo "############ runtime"
node scripts/js_backend/runtime_test.mjs | tail -2 || fail=1

# Unsupported constructs must fail the build with positions, every site at once.
echo
echo "############ diagnostics"
./scripts/js-backend-diagnostics.sh | tail -2 || fail=1

if [ "$fail" -ne 0 ]; then
    echo
    echo "js-backend-suites: FAILED"
    exit 1
fi
echo
echo "js-backend-suites: all fixtures agree with the native backend"
