#!/usr/bin/env bash
# Component body checker gate (component body typing, Phase 3b).
#
# Each fixture is a `--web` app whose `#universal` component is client-only and
# whose server entry is a trivial native `main`. `dom` is imported on the client
# side only, so the client job indexes it and runs the body check while the
# server job neither imports the component nor runs the check.
#
#   body_unknown_prop      props.titel              -> WEB-BODY-001
#   body_unknown_member    document.titel (lambda)  -> WEB-BODY-002 (+ title)
#   body_no_provider       no dom import            -> WEB-BODY-008
#   body_hook_placement    useEffect inside if      -> WEB-BODY-006
#   body_ok_surface        allowed roots/built-ins  -> builds, no 001/002/006/008
#   body_ok_chains         document.body.classList  -> no 002, counted 005
#   body_props_shadow      local `props` shadow     -> no 001 (counted)
#   body_props_destructure props destructuring      -> no 001 (counted)
#   body_jsx_contexts      document.title attr/child -> no 002
#
# The log is split into the client and server passes, so the gate proves the
# diagnostic belongs to the client job and the server section has none.
#
#   ./scripts/web-body-check-test.sh
set -uo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler.exe"
if [ ! -x "$compiler" ]; then
    echo "web-body-check-test: no compiler at cmake-build-debug/TCCCompiler (build it with ./scripts/build.sh --tcc)" >&2
    exit 1
fi
compiler="$(cd "$(dirname "$compiler")" && pwd)/$(basename "$compiler")"

fail=0
check() {
    if [ "$2" = "1" ]; then
        echo "ok   $1"
    else
        echo "FAIL $1" >&2
        fail=1
    fi
}
has() { grep -qF -- "$2" "$1" 2>/dev/null && echo 1 || echo 0; }
has_re() { grep -qE -- "$2" "$1" 2>/dev/null && echo 1 || echo 0; }
count() { grep -cF -- "$2" "$1" 2>/dev/null || true; }
no_code() {
    local file="$1"
    if [ "$(count "$file" 'WEB-BODY-001')" = "0" ] \
        && [ "$(count "$file" 'WEB-BODY-002')" = "0" ] \
        && [ "$(count "$file" 'WEB-BODY-006')" = "0" ] \
        && [ "$(count "$file" 'WEB-BODY-008')" = "0" ]; then
        echo 1
    else
        echo 0
    fi
}

# Split a `--web` log into the client and server passes. The compiler prints its
# side markers before running either job, so every job_begin/index/body line
# sits in the matching section.
split_sides() {
    local log="$1" client="$2" server="$3"
    rm -f "$client" "$server"
    : > "$client"
    : > "$server"
    awk -v client="$client" -v server="$server" '
        /\[lab\] web build: client side/ { side = "client"; next }
        /\[lab\] web build: server side/ { side = "server"; next }
        side == "client" { print >> client }
        side == "server" { print >> server }
    ' "$log"
}

fixtures="lang/tests/web/fixtures"
out="lang/compiled/web-body-check-gate"
rm -rf "$out"
mkdir -p "$out"

run_case() {
    local name="$1"
    local fixture="$fixtures/$name"
    local log="$out/$name.log"
    local build="$out/${name}_build"
    local exe="$out/$name.exe"
    rm -rf "$build" "$exe"
    CHEM_DEBUG_INDEX=1 CHEM_DEBUG_BODY=1 "$compiler" "$fixture/chemical.mod" --web -v \
        --build-dir "$build" -o "$exe" > "$log" 2>&1
    local status=$?
    split_sides "$log" "$out/$name.client.log" "$out/$name.server.log"
    return $status
}

# ---------------------------------------------------------------- negative: 001
echo "== negative: props.titel is not a declared prop"
if run_case body_unknown_prop; then
    check "body_unknown_prop is rejected" 0
else
    check "body_unknown_prop is rejected" 1
fi
c="$out/body_unknown_prop.client.log"
s="$out/body_unknown_prop.server.log"
check "001 is reported once" "$([ "$(count "$c" 'WEB-BODY-001')" = "1" ] && echo 1 || echo 0)"
check "001 names the component" "$(has "$c" "component 'PropProbe'")"
check "001 names the prop" "$(has "$c" "'props.titel'")"
check "001 lists the declared props" "$(has "$c" 'declared props: title, count')"
check "the 001 is in the client section" "$(has "$c" 'WEB-BODY-001')"
check "the server section has no body diagnostic" "$(no_code "$s")"

# ---------------------------------------------------------------- negative: 002
echo
echo "== negative: document.titel has no member"
if run_case body_unknown_member; then
    check "body_unknown_member is rejected" 0
else
    check "body_unknown_member is rejected" 1
fi
c="$out/body_unknown_member.client.log"
s="$out/body_unknown_member.server.log"
check "002 is reported once" "$([ "$(count "$c" 'WEB-BODY-002')" = "1" ] && echo 1 || echo 0)"
check "002 names the component" "$(has "$c" "component 'MemberProbe'")"
check "002 names the member" "$(has "$c" "'document.titel'")"
check "002 suggests the nearest member" "$(has "$c" "did you mean 'title'")"
check "the 002 is in the client section" "$(has "$c" 'WEB-BODY-002')"
check "the server section has no body diagnostic" "$(no_code "$s")"

# ---------------------------------------------------------------- negative: 008
echo
echo "== negative: no provider for document"
if run_case body_no_provider; then
    check "body_no_provider is rejected" 0
else
    check "body_no_provider is rejected" 1
fi
c="$out/body_no_provider.client.log"
s="$out/body_no_provider.server.log"
check "008 is reported once" "$([ "$(count "$c" 'WEB-BODY-008')" = "1" ] && echo 1 || echo 0)"
check "008 names the component" "$(has "$c" "component 'NoProviderProbe'")"
check "008 names the missing host global" "$(has "$c" "uses 'document'")"
check "008 says how to fix it" "$(has "$c" 'import a provider')"
check "the 008 is in the client section" "$(has "$c" 'WEB-BODY-008')"
check "the server section has no body diagnostic" "$(no_code "$s")"

# ---------------------------------------------------------------- negative: 006
echo
echo "== negative: useEffect inside if"
if run_case body_hook_placement; then
    check "body_hook_placement is rejected" 0
else
    check "body_hook_placement is rejected" 1
fi
c="$out/body_hook_placement.client.log"
s="$out/body_hook_placement.server.log"
check "006 is reported once" "$([ "$(count "$c" 'WEB-BODY-006')" = "1" ] && echo 1 || echo 0)"
check "006 names the component" "$(has "$c" "component 'HookProbe'")"
check "006 names the hook" "$(has "$c" "hook 'useEffect'")"
check "006 explains the rule" "$(has "$c" 'unconditionally at the top level')"
check "the 006 is in the client section" "$(has "$c" 'WEB-BODY-006')"
check "the server section has no body diagnostic" "$(no_code "$s")"

# ---------------------------------------------------------------- positive
positive_case() {
    local name="$1"
    echo
    echo "== positive: $name builds with no body diagnostics"
    if run_case "$name"; then
        check "$name builds" 1
    else
        check "$name builds (see $out/$name.log)" 0
        tail -20 "$out/$name.log" >&2
    fi
    local c="$out/$name.client.log"
    local s="$out/$name.server.log"
    check "$name has no 001/002/006/008 on the client" "$(no_code "$c")"
    check "$name has no body diagnostic on the server" "$(no_code "$s")"
    check "$name ran the body checker on the client" "$(has "$c" '[body] component=')"
    check "$name did not run the body checker on the server" \
          "$([ "$(count "$s" '[body] component=')" = "0" ] && echo 1 || echo 0)"
}

positive_case body_ok_surface
c="$out/body_ok_surface.client.log"
check "allowed surface reads props.title and document.*" \
      "$(has_re "$c" '\[body\] component=SurfaceProbe untyped=[0-9]+')"

positive_case body_ok_chains
c="$out/body_ok_chains.client.log"
check "a deeper chain is counted, not rejected" \
      "$(has_re "$c" '\[body\] component=ChainProbe untyped=[1-9][0-9]*')"

positive_case body_props_shadow
c="$out/body_props_shadow.client.log"
check "a shadowed props local produces no 001" \
      "$([ "$(count "$c" 'WEB-BODY-001')" = "0" ] && echo 1 || echo 0)"

positive_case body_props_destructure
c="$out/body_props_destructure.client.log"
check "a props destructuring read produces no 001" \
      "$([ "$(count "$c" 'WEB-BODY-001')" = "0" ] && echo 1 || echo 0)"
check "the destructuring props read is counted" \
      "$(has_re "$c" '\[body\] component=DestructureProbe untyped=[1-9][0-9]*')"

positive_case body_jsx_contexts
c="$out/body_jsx_contexts.client.log"
check "document.title in an attribute and a child produces no 002" \
      "$([ "$(count "$c" 'WEB-BODY-002')" = "0" ] && echo 1 || echo 0)"

# ---------------------------------------------------------------- cleanup
for name in body_unknown_prop body_unknown_member body_no_provider body_hook_placement \
            body_ok_surface body_ok_chains body_props_shadow body_props_destructure body_jsx_contexts; do
    rm -rf "$fixtures/$name/build" "$fixtures/$name/client/build" "$fixtures/$name/src/build"
done
if [ "$fail" = 0 ]; then
    rm -rf "$out"
else
    echo "web-body-check-test: artifacts kept in $out" >&2
fi

if [ "$fail" != 0 ]; then
    echo "web-body-check-test: FAILED" >&2
    exit 1
fi
echo "web-body-check-test: 001/002/006/008 reject, allowed surface builds, 005 is counted"
