#!/usr/bin/env bash
# Embedded-bundle gate: the two-phase build (lang/docs/web-stack-plan.md Phase 4).
#
# Phase 1 (`fixtures/embedded_producer`) builds a site, writes it to disk, and
# emits the same bytes as Chemical source — `StaticSite.write_assets_module`.
# Phase 2 (`fixtures/embedded_consumer`) is compiled against that generated module
# and serves requests from a `page.Bundle`. It contains no site, page or component,
# so anything it answers correctly exists **in the binary**: the generated source
# is the only path the bytes could have taken.
#
# The gate compares each response body with the file phase 1 wrote, byte for byte,
# and checks the serving contract on the embedded copy (ETag/304, immutable assets,
# directory-form routes, 404).
#
#   ./scripts/web-embedded-test.sh
set -eu

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

compiler="${CHX_COMPILER:-cmake-build-debug/TCCCompiler.exe}"
[ -x "$compiler" ] || compiler="cmake-build-debug/TCCCompiler"

out_dir="lang/compiled/embedded_out"
gen_dir="lang/compiled/embedded_gen"
consumer_dir="lang/compiled/embedded_consumer"

rm -rf "$out_dir" "$gen_dir" "$consumer_dir" \
       lang/tests/web/fixtures/embedded_producer/build \
       lang/tests/web/fixtures/embedded_consumer/build
mkdir -p "$gen_dir"

fail=0
check() {
    if [ "$2" = "1" ]; then
        echo "ok   $1"
    else
        echo "FAIL $1" >&2
        fail=1
    fi
}
contains() { grep -qF "$2" "$1" && echo 1 || echo 0; }
lacks() { grep -qF "$2" "$1" && echo 0 || echo 1; }

echo "== phase 1: build the site and generate its source =="
producer=lang/compiled/embedded_producer.exe
rm -f "$producer"
"$compiler" lang/tests/web/fixtures/embedded_producer/chemical.mod -o "$producer" >/tmp/web_embedded_build.txt 2>&1 || true
if [ ! -x "$producer" ]; then
    echo "FAIL the producer did not compile" >&2
    tail -20 /tmp/web_embedded_build.txt >&2
    exit 1
fi
./"$producer" >/tmp/web_embedded_producer.txt 2>&1

check "the producer built the site and wrote a generated module" \
      "$([ -f "$gen_dir/assets_gen.ch" ] && [ -f "$out_dir/index.html" ] && echo 1 || echo 0)"
check "the generated module carries every artifact" \
      "$(grep -qE 'embedded_asset_count\(\) : ubigint \{ return [1-9][0-9]* as ubigint \}' "$gen_dir/assets_gen.ch" && echo 1 || echo 0)"
check "the producer's bundle is a separate vendor file or route chunks" \
      "$(grep -qE 'producer: artifacts=[0-9]+' /tmp/web_embedded_producer.txt && echo 1 || echo 0)"

echo "== phase 2: compile a server against the generated source =="
mkdir -p "$consumer_dir/src"
cp lang/tests/web/fixtures/embedded_consumer/src/main.ch "$consumer_dir/src/main.ch"
cp "$gen_dir/assets_gen.ch" "$consumer_dir/src/assets_gen.ch"
cat > "$consumer_dir/chemical.mod" <<'MOD'
application embedded_consumer

source "src"

import cstd
import std
import page
MOD

consumer=lang/compiled/embedded_consumer.exe
rm -f "$consumer"
"$compiler" "$consumer_dir/chemical.mod" -o "$consumer" >/tmp/web_embedded_consumer_build.txt 2>&1 || true
if [ ! -x "$consumer" ]; then
    echo "FAIL the consumer did not compile" >&2
    tail -20 /tmp/web_embedded_consumer_build.txt >&2
    exit 1
fi
./"$consumer" >/tmp/web_embedded_consumer.txt 2>&1

log=/tmp/web_embedded_consumer.txt
line_of() { grep -F "request $1 ->" "$log" | head -1; }
status_of() { line_of "$1" | awk -F' \\| ' '{split($1,a," -> "); print a[2]}'; }
cc_of() { line_of "$1" | awk -F' \\| ' '{print $3}'; }
bytes_of() { line_of "$1" | awk -F' \\| ' '{print $4}' | sed 's/ B//'; }
body_of() { awk -v p="$1" '$0 == "---body " p "---" {f=1; next} $0 == "---end---" {f=0} f' "$log"; }

check "the consumer serves every generated artifact" \
      "$([ "$(grep -o 'embedded artifacts=[0-9]*' "$log" | head -1)" = "embedded artifacts=6" ] && echo 1 || echo 0)"
check "an embedded page is a 200 HTML response" \
      "$([ "$(status_of /)" = "200" ] && [ "$(cc_of /)" = "no-cache" ] && echo 1 || echo 0)"
check "an embedded page carries a strong ETag" \
      "$(grep -qE 'request / -> 200 .*etag="[0-9a-f]{12}"' "$log" && echo 1 || echo 0)"
check "a nested route resolves with and without its trailing slash" \
      "$([ "$(status_of /stats/)" = "200" ] && [ "$(grep -o 'stats bare -> 200' "$log")" = "stats bare -> 200" ] && echo 1 || echo 0)"

# The embedded bytes must be the built bytes: compare the served body with the
# file phase 1 wrote. This is what makes "embedded" verifiable rather than
# plausible.
# Command substitution drops the trailing newline the printer adds, so this
# compares bytes rather than line endings.
[ "$(body_of /)" = "$(cat "$out_dir/index.html")" ] && index_same=1 || index_same=0
check "the embedded / body is byte-identical to the built index.html" "$index_same"
[ "$(body_of /stats/)" = "$(cat "$out_dir/stats/index.html")" ] && stats_same=1 || stats_same=0
check "the embedded /stats/ body is byte-identical to the built page" "$stats_same"

check "revalidating from the embedded copy is a 304 with no body" \
      "$([ "$(status_of '/ conditional')" = "304" ] && [ "$(bytes_of '/ conditional')" = "0" ] && echo 1 || echo 0)"
check "a content-hashed asset is served immutable" \
      "$([ "$(status_of /first-asset)" = "200" ] && [ "$(cc_of /first-asset)" = "public, max-age=31536000, immutable" ] && echo 1 || echo 0)"
check "an unknown path is a 404 from the embedded copy" \
      "$(grep -qF 'missing -> 404' "$log" && echo 1 || echo 0)"
check "no script is inlined into an embedded page" \
      "$(printf '%s' "$(body_of /)" | grep -q '<script>' && echo 0 || echo 1)"

if [ "$fail" = "0" ]; then
    echo
    echo "web-embedded-test: PASS (two-phase build; the server serves only compiled-in bytes)"
fi
exit "$fail"
