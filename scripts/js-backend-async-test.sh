#!/usr/bin/env bash
# Promise-based async/await on the JS backend.
#
# A native build drives async with a coroutine poll loop; a JS build has real
# promises, so `async func` becomes a JS `async function` and `await` becomes a
# JS `await`. That is a different execution model, so the comparison here is
# against the fixture's expected output (and the emitted shape) rather than
# against a native build.
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(dirname "$here")"
cd "$root"

fixture=js_backend_async
fixture_dir="lang/tests/web/fixtures/$fixture"
out_root="lang/compiled/js-backend-async-test"
compiler="$root/cmake-build-debug/TCCCompiler"
if [ ! -x "$compiler" ]; then
    echo "js-backend-async-test: build the compiler first (./scripts/build.sh --tcc)" >&2
    exit 1
fi

rm -rf "$out_root"
mkdir -p "$out_root/bundle"

CHX_JS_RUNTIME="$root/scripts/js_backend/runtime.mjs" \
    "$compiler" "$fixture_dir/chemical.mod" --emit-js "$out_root/bundle" \
    > "$out_root/build.log" 2>&1 || {
    echo "js-backend-async-test: the build failed, see $out_root/build.log" >&2
    tail -20 "$out_root/build.log" >&2
    exit 1
}

node scripts/js_backend/run.mjs "$out_root/bundle" > "$out_root/output.txt" 2> "$out_root/run.log" || {
    echo "js-backend-async-test: the program failed to run" >&2
    cat "$out_root/run.log" >&2
    exit 1
}

if ! diff -u "$fixture_dir/expected.txt" "$out_root/output.txt"; then
    echo "js-backend-async-test: output does not match expected.txt" >&2
    exit 1
fi

# The point of the fixture: an async body is a JS async function suspended by
# `await`, not a poll loop that happens to work.
bundle="$out_root/bundle/js_backend_async.mjs"
if ! grep -q "async () =>" "$bundle"; then
    echo "js-backend-async-test: no async body was emitted" >&2
    exit 1
fi
if ! grep -q "await rt.promise_get" "$bundle"; then
    echo "js-backend-async-test: awaits were not lowered onto promises" >&2
    exit 1
fi
if ls "$out_root/bundle"/*.mjs | xargs grep -l "async.await" 2>/dev/null | grep -q .; then
    echo "js-backend-async-test: a poll-protocol await leaked into the bundle" >&2
    exit 1
fi

echo "PASS: async bodies are JS async functions, awaits suspend on promises, output matches"
