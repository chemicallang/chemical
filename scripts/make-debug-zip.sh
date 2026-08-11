#!/usr/bin/env bash
# Copyright (c) Chemical Language Foundation 2025.
#
# make-debug-zip.sh — build a debug release package from a release package.
#
# The release packaging (scripts/release.sh) already produces a complete
# package (compiler binary + lang/libs + tcc package + resources). Debug
# builds only differ in the compiler binary itself (built with
# CMAKE_BUILD_TYPE=RelWithDebInfo so it carries full debug symbols). This
# helper swaps the debug binary into a copy of the release zip, keeping the
# package structure identical.
#
# Usage:
#   make-debug-zip.sh <release-zip> <debug-binary> <output-zip> <binary-path-in-zip>
#
#   <release-zip>        the release package zip (e.g. out/release/linux-x64-tcc.zip)
#   <debug-binary>       the debug-built compiler binary (e.g. out/build-debug/TCCCompiler)
#   <output-zip>         where to write the debug package (e.g. out/release/linux-x64-tcc-debug.zip)
#   <binary-path-in-zip> path of the compiler inside the zip, e.g. chemical/chemical
#                        or chemical-tcc/chemical (windows: .../chemical.exe)
#
# Depends on: unzip, zip, cp.
set -euo pipefail

if [ "$#" -ne 4 ]; then
  echo "usage: make-debug-zip.sh <release-zip> <debug-binary> <output-zip> <binary-path-in-zip>" >&2
  exit 2
fi

release_zip="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
debug_bin="$2"
output_zip="$(cd "$(dirname "$3")" && pwd)/$(basename "$3")"
inner_bin="$4"

[ -f "$release_zip" ] || { echo "error: release zip not found: $release_zip" >&2; exit 1; }
[ -f "$debug_bin" ]   || { echo "error: debug binary not found: $debug_bin" >&2; exit 1; }

tmp="$(mktemp -d)"
trap 'rm -rf "$tmp"' EXIT

unzip -o -q "$release_zip" -d "$tmp"

if [ ! -f "$tmp/$inner_bin" ]; then
  echo "error: '$inner_bin' not found in $release_zip" >&2
  echo "archive top-level entries:" >&2
  ls "$tmp" >&2
  exit 1
fi

cp -f "$debug_bin" "$tmp/$inner_bin"
chmod +x "$tmp/$inner_bin"

top="${inner_bin%%/*}"
( cd "$tmp" && zip -r -q "$output_zip" "$top" )

echo "wrote $output_zip ($(du -h "$output_zip" | cut -f1))"
