#!/usr/bin/env bash
# Starter-template gate.
#
# `templates/web-starter` is the on-ramp: the thing a new user copies. This builds
# it, runs it, and asserts the output it promises in `lang/docs/web-getting-started.md`
# actually appears — two pages, hydration markers, and the props payload — so the
# guide and the template cannot drift apart silently.
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root" || exit 2

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || { echo "web-starter-test: no compiler at $compiler" >&2; exit 2; }

out_dir="lang/compiled/web-starter-output"
exe="lang/compiled/web-starter-gate"

fail=0
check() { # name, condition-exit
    if [ "$2" -eq 0 ]; then echo "ok   $1"; else echo "FAIL $1"; fail=1; fi
}

rm -rf "$out_dir" "$exe"
"$compiler" templates/web-starter/chemical.mod -o "$exe" --mode debug_quick \
    > lang/compiled/web-starter-build.log 2>&1
check "template compiles" $?

"$exe" > /dev/null 2>&1
check "template runs" $?

test -f "$out_dir/index.html"; check "writes index.html" $?
test -f "$out_dir/about.html"; check "writes about.html" $?
test -f "$out_dir/index.js"; check "writes index.js" $?
test -f "$out_dir/index.css"; check "writes index.css" $?

grep -q 'id="u1"' "$out_dir/index.html"; check "server-renders hydration markers" $?
grep -q 'class="counter-out"' "$out_dir/index.html"; check "server-renders the component body" $?
grep -q '"step":2' "$out_dir/index.js"; check "typed prop value reaches the client payload" $?
grep -q '"step":1' "$out_dir/index.js"; check "typed prop default reaches the client payload" $?
grep -q '<title>Starter — Home</title>' "$out_dir/index.html"; check "typed Head emits the title" $?
grep -q 'name="description"' "$out_dir/index.html"; check "typed Head emits the description" $?
grep -q 'canonical' "$out_dir/about.html"; check "typed Head emits the canonical link" $?

# The about page is static markup only: it must not need the component runtime.
if grep -q 'index.js' "$out_dir/about.html"; then
    echo "FAIL about page ships the component runtime"; fail=1
else
    echo "ok   static page ships no component runtime"
fi

rm -rf "$out_dir" "$exe"
if [ "$fail" -ne 0 ]; then
    echo
    echo "web-starter-test: FAILED"
    exit 1
fi
echo
echo "web-starter-test: the starter template builds, runs and produces the documented output"
