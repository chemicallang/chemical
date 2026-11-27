#!/usr/bin/env bash
# P0.1 environment probe for the Chemical native UI library.
#
# Safe/read-only: prints host facts, tool availability and a PASS/FAIL/BLOCKED
# matrix. It never installs, deletes, or modifies anything. Missing platform
# probes are reported BLOCKED with the reason, never silently skipped.
#
# Usage:
#   scripts/ui-probe-env.sh            # human-readable
#   scripts/ui-probe-env.sh --json     # machine-readable JSON (requires jq)
#   scripts/ui-probe-env.sh --strict   # exit 1 if a Linux-required tool fails
#
# The package-install commands for missing tools are in
# lang/tests/ui_probe/ENV.md.

set -u

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JSON=0
STRICT=0
for arg in "$@"; do
    case "$arg" in
        --json) JSON=1 ;;
        --strict) STRICT=1 ;;
        *) echo "ui-probe-env: unknown argument '$arg'" >&2; exit 2 ;;
    esac
done

# Keep the original stdout for --json; human progress goes to stderr then.
exec 3>&1
if [ "$JSON" = "1" ]; then exec 1>&2; fi

pass=0
fail=0
blocked=0
rows=()

row() { # row <state> <id> <detail>
    local state="$1" id="$2" detail="$3"
    rows+=("${state}|${id}|${detail}")
    case "$state" in
        PASS) pass=$((pass + 1)) ;;
        FAIL) fail=$((fail + 1)) ;;
        BLOCKED) blocked=$((blocked + 1)) ;;
    esac
}

have() { command -v "$1" >/dev/null 2>&1; }

tool() { # tool <id> <binary> [required]
    local id="$1" bin="$2" required="${3:-optional}"
    if have "$bin"; then
        local v
        v="$("$bin" --version 2>&1 | head -1 | tr -d '\r')"
        row PASS "$id" "$v"
    elif [ "$required" = "required" ]; then
        row FAIL "$id" "missing ($bin)"
    else
        row BLOCKED "$id" "missing ($bin)"
    fi
}

json_escape() {
    local s="$1"
    s="${s//\\/\\\\}"
    s="${s//\"/\\\"}"
    s="${s//$'\n'/\\n}"
    printf '%s' "$s"
}

cpu_model="$(awk -F': ' '/model name/ {print $2; exit}' /proc/cpuinfo 2>/dev/null || echo unknown)"
cpu_cores="$(nproc 2>/dev/null || echo unknown)"
mem_kb="$(awk '/MemTotal/ {print $2; exit}' /proc/meminfo 2>/dev/null || echo unknown)"
mem_gb="unknown"
if [ "$mem_kb" != "unknown" ]; then mem_gb="$((mem_kb / 1024 / 1024))"; fi
kernel="$(uname -srm)"
distro="unknown"
if [ -r /etc/os-release ]; then
    # shellcheck disable=SC1091
    . /etc/os-release
    distro="${PRETTY_NAME:-unknown}"
fi
governor="unknown"
for f in /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor /sys/devices/system/cpu/cpufreq/policy0/scaling_governor; do
    if [ -r "$f" ]; then governor="$(cat "$f")"; break; fi
done
gpu="$( (lspci 2>/dev/null || true) | grep -Ei 'vga|3d|display' | head -3 | paste -sd ';' - )"
[ -z "$gpu" ] && gpu="unavailable (lspci missing)"
display="${DISPLAY:-unset}"
wayland="${WAYLAND_DISPLAY:-unset}"

echo "=== Chemical UI P0.1 environment probe ==="
echo "date:        $(date -u +%Y-%m-%dT%H:%M:%SZ)"
echo "kernel:      $kernel"
echo "distro:      $distro"
echo "cpu:         $cpu_model ($cpu_cores logical cores)"
echo "ram:         ${mem_gb} GB"
echo "governor:    $governor"
echo "gpu:         $gpu"
echo "display:     DISPLAY=$display WAYLAND_DISPLAY=$wayland"
echo

# ---- compilers -------------------------------------------------------------
for c in TCCCompiler Compiler ChemicalLsp; do
    p="$ROOT/cmake-build-debug/$c"
    if [ -x "$p" ]; then
        row PASS "compiler.$c" "$("$p" --version 2>&1 | head -1)"
    else
        row FAIL "compiler.$c" "not built ($p)"
    fi
done

# ---- core toolchain --------------------------------------------------------
tool "tool.cc" cc required
tool "tool.clang" clang required
tool "tool.python3" python3 required
tool "tool.node" node required
tool "tool.npx" npx optional
tool "tool.gh" gh optional
tool "tool.git" git required
tool "tool.cmake" cmake optional
tool "tool.fc-list" fc-list optional
if have wasm-ld; then
    row PASS "tool.wasm-ld" "$(wasm-ld --version | head -1)"
elif [ -x "$ROOT/out/host/bin/wasm-ld" ]; then
    row PASS "tool.wasm-ld" "$($ROOT/out/host/bin/wasm-ld --version | head -1) (out/host/bin)"
else
    row BLOCKED "tool.wasm-ld" "missing"
fi
if clang --print-targets 2>/dev/null | grep -q 'wasm32'; then
    row PASS "tool.clang-wasm32" "wasm32 target available"
else
    row FAIL "tool.clang-wasm32" "clang has no wasm32 target"
fi
if python3 -c 'import fontTools' >/dev/null 2>&1; then
    row PASS "tool.fonttools" "$(python3 -c 'import fontTools; print(fontTools.version)' 2>/dev/null)"
else
    row BLOCKED "tool.fonttools" "python3 -c 'import fontTools' failed (pip install fonttools)"
fi
if [ -x "$ROOT/lang/compiled/wasm_ui/node_modules/.bin/playwright" ]; then
    row PASS "tool.playwright-local" "$ROOT/lang/compiled/wasm_ui/node_modules/.bin/playwright"
elif have npx && npx --no-install playwright --version >/dev/null 2>&1; then
    row PASS "tool.playwright" "$(npx --no-install playwright --version 2>&1 | head -1)"
else
    row BLOCKED "tool.playwright" "no local/global playwright (npm i -D @playwright/test && npx playwright install)"
fi

# ---- Linux desktop host ----------------------------------------------------
if pkg-config --exists gtk+-3.0 2>/dev/null; then
    row PASS "linux.gtk3-dev" "$(pkg-config --modversion gtk+-3.0)"
else
    row FAIL "linux.gtk3-dev" "pkg-config gtk+-3.0 missing (runtime may still exist)"
fi
if [ -e /usr/lib/x86_64-linux-gnu/libgtk-3.so.0 ] || ldconfig -p 2>/dev/null | grep -q 'libgtk-3.so.0'; then
    row PASS "linux.gtk3-runtime" "libgtk-3.so.0 present"
else
    row FAIL "linux.gtk3-runtime" "libgtk-3.so.0 missing"
fi
if have Xvfb; then row PASS "linux.xvfb" "$(Xvfb -help 2>&1 | head -1)"; else row BLOCKED "linux.xvfb" "missing (apt install xvfb)"; fi
if [ "$display" != "unset" ] || [ "$wayland" != "unset" ]; then
    row PASS "linux.display" "DISPLAY=$display WAYLAND_DISPLAY=$wayland"
else
    row BLOCKED "linux.display" "no DISPLAY/WAYLAND_DISPLAY (use xvfb-run or a session)"
fi
if [ -e /usr/lib/x86_64-linux-gnu/dri/swrast_dri.so ] || [ -e /usr/lib/x86_64-linux-gnu/dri/llvmpipe_dri.so ]; then
    row PASS "linux.llvmpipe" "Mesa swrast/llvmpipe DRI present"
elif have glxinfo; then
    row PASS "linux.llvmpipe" "$(glxinfo -B 2>/dev/null | grep -m1 'OpenGL renderer' || echo present)"
else
    row BLOCKED "linux.llvmpipe" "no swrast_dri.so and no glxinfo"
fi
if ldconfig -p 2>/dev/null | grep -q 'libGL.so.1'; then row PASS "linux.opengl" "libGL.so.1 present"; else row BLOCKED "linux.opengl" "missing"; fi
if ldconfig -p 2>/dev/null | grep -q 'libEGL.so.1'; then row PASS "linux.egl" "libEGL.so.1 present"; else row BLOCKED "linux.egl" "missing"; fi
if pkg-config --exists sdl3 2>/dev/null; then row PASS "linux.sdl3-dev" "$(pkg-config --modversion sdl3)"; else row BLOCKED "linux.sdl3-dev" "pkg-config sdl3 missing"; fi

# ---- Vulkan ----------------------------------------------------------------
if ldconfig -p 2>/dev/null | grep -q 'libvulkan.so.1'; then
    row PASS "vulkan.loader" "$(ldconfig -p | grep 'libvulkan.so.1' | head -1 | awk '{print $NF}')"
else
    row BLOCKED "vulkan.loader" "libvulkan.so.1 missing"
fi
icd_dir=/usr/share/vulkan/icd.d
if [ -d "$icd_dir" ] && ls "$icd_dir"/*.json >/dev/null 2>&1; then
    row PASS "vulkan.icd" "$(ls "$icd_dir" | paste -sd, -)"
else
    row BLOCKED "vulkan.icd" "no ICD manifests in $icd_dir"
fi
if [ -e "$icd_dir/lvp_icd.json" ]; then row PASS "vulkan.lavapipe" "lvp_icd.json present"; else row BLOCKED "vulkan.lavapipe" "lvp_icd.json missing"; fi
if have vulkaninfo; then row PASS "vulkan.vulkaninfo" "$(vulkaninfo --version 2>&1 | head -1)"; else row BLOCKED "vulkan.vulkaninfo" "missing (apt install vulkan-tools)"; fi

# ---- API/platform probes ---------------------------------------------------
if [ "$(uname -s)" = "Linux" ]; then
    row PASS "platform.linux" "native host probe runs"
else
    row BLOCKED "platform.linux" "not running on Linux"
fi
if [ "$(uname -s)" = "Darwin" ]; then
    row PASS "platform.macos" "native host probe available"
else
    row BLOCKED "platform.macos" "not macOS: Cocoa window backend/IME missing in this tree"
fi
case "$(uname -s)" in
    MINGW*|MSYS*|CYGWIN*) row PASS "platform.windows" "native host probe available" ;;
    *) row BLOCKED "platform.windows" "not Windows: Win32 IME/user-data probe not runnable here" ;;
esac
if [ -e "$ROOT/lang/libs/window/win/win.ch" ]; then
    row PASS "window.win32-source" "lang/libs/window/win/win.ch present"
else
    row FAIL "window.win32-source" "missing"
fi
if [ -e "$ROOT/lang/libs/window/posix/linux.ch" ]; then
    row PASS "window.gtk3-source" "lang/libs/window/posix/linux.ch present"
else
    row FAIL "window.gtk3-source" "missing"
fi
if [ -e "$ROOT/lang/libs/window/cocoa" ] || [ -e "$ROOT/lang/libs/window/macos" ]; then
    row PASS "window.cocoa-source" "macOS backend present"
else
    row BLOCKED "window.cocoa-source" "no macOS backend in lang/libs/window (RESEARCH §7 confirmed)"
fi

# ---- gh --------------------------------------------------------------------
if have gh; then
    if gh auth status >/dev/null 2>&1; then
        row PASS "gh.auth" "authenticated ($(gh api user --jq .login 2>/dev/null))"
        org="$(gh api orgs/chemicallang --jq .login 2>/dev/null || true)"
        if [ -n "$org" ]; then
            perms="$(gh api repos/chemicallang/components-e2e --jq '.permissions|tostring' 2>/dev/null || true)"
            if [ -n "$perms" ]; then
                row PASS "gh.org-permission" "chemicallang visible; components-e2e permissions=$perms"
            else
                row BLOCKED "gh.org-permission" "org visible, repo permissions unavailable"
            fi
        else
            row FAIL "gh.org-permission" "org chemicallang not visible"
        fi
    else
        row BLOCKED "gh.auth" "not logged in (gh auth login)"
    fi
else
    row BLOCKED "gh.auth" "gh missing"
fi

echo "=== matrix ==="
printf '%-8s %-28s %s\n' "STATE" "PROBE" "DETAIL"
for r in "${rows[@]}"; do
    IFS='|' read -r state id detail <<< "$r"
    printf '%-8s %-28s %s\n' "$state" "$id" "$detail"
done
echo
echo "summary: PASS=$pass FAIL=$fail BLOCKED=$blocked"

if [ "$JSON" = "1" ]; then
    if ! have jq; then
        echo "ui-probe-env: jq not installed; JSON mode unavailable" >&2
        exit 2
    fi
    printf '%s\n' "${rows[@]}" | jq -Rn \
        --arg kernel "$kernel" --arg distro "$distro" --arg cpu "$cpu_model" \
        --arg cores "$cpu_cores" --arg ram "$mem_gb" --arg governor "$governor" \
        --arg gpu "$gpu" '
        [inputs | split("|") | {state: .[0], probe: .[1], detail: .[2]}] as $r
        | {host: {kernel: $kernel, distro: $distro, cpu: $cpu, cores: $cores,
                  ram_gb: $ram, governor: $governor, gpu: $gpu},
           results: $r}
    ' >&3
fi

if [ "$STRICT" = "1" ] && [ "$fail" -gt 0 ]; then
    exit 1
fi
exit 0
