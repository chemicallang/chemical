#!/usr/bin/env bash
#
# Copyright (c) Chemical Language Foundation 2025.
#
# Get the directory of this script (resolves symlinks too)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Run the fetch script located in the same directory
"${SCRIPT_DIR}/setup-tcc.sh"