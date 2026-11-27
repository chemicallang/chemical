# P0.1 environment (verified, not assumed)

Host probed on **2026-11-27** by `scripts/ui-probe-env.sh`. Raw output:
`lang/docs/ui/evidence/P0.1/env.log`.

## Actual host

| Item | Value |
|------|-------|
| OS | Ubuntu 26.04.1 LTS (Resolute Raccoon) |
| Kernel | Linux 7.0.0-43-generic x86_64 |
| CPU | AMD Ryzen 5 7430U with Radeon Graphics, 12 logical cores |
| RAM | 14 GB |
| GPU | AMD Barcelo (RADV RENOIR) + Mesa llvmpipe/lavapipe |
| CPU governor | `powersave` (measurements are noisy; pin with `taskset -c N`) |
| Display | `DISPLAY=:0`, `WAYLAND_DISPLAY=wayland-0` (a live session) |
| Compilers | `cmake-build-debug/TCCCompiler` and `Compiler`, Chemical v0.6.0 |
| `pkg-config gtk+-3.0` | **absent** (runtime `libgtk-3.so.0` present; the current `window` backend links by soname, so it still builds) |
| Xvfb | **absent** (a real session display is present) |
| Vulkan | loader `libvulkan.so.1` 1.4.341; ICDs include `lvp_icd.json` (lavapipe), `radeon_icd.json` (RADV), Intel/NVIDIA/Asahi/virtio |
| `vulkaninfo` | **absent** (Vulkan probe uses ctypes, see `lang/tests/ui_probe/gpu/`) |
| Mesa | 26.0.8 (llvmpipe + RADV) |
| OpenGL/EGL | `libGL.so.1`, `libEGL.so.1` present |
| SDL | SDL2 runtime present; SDL3 + dev files **absent** |
| clang | 21.1.8 (system), wasm32 target present |
| wasm-ld | LLD 22.1.8 at `out/host/bin/wasm-ld` |
| Python | 3.14.4; `fontTools` **absent** |
| Node | v22.22.1; Playwright present only in `lang/compiled/wasm_ui/node_modules` |
| gh | 2.46.0, authenticated as `wakaztahir`; `chemicallang` org admin active, `components-e2e` has `admin/push/pull` |
| mac OS | **BLOCKED** (not macOS; no Cocoa backend) |
| Windows | **BLOCKED** (not Windows; Win32 source exists but cannot run here) |

## Missing tools and exact install commands

### Ubuntu / Debian (this host)

```bash
# GTK3 headers (needed for C host shims and pkg-config, not for the current
# soname-linked window library)
sudo apt-get update
sudo apt-get install -y libgtk-3-dev

# Headless X server for CI
sudo apt-get install -y xvfb

# Vulkan loader + tools; Mesa already ships lavapipe in mesa-vulkan-drivers
sudo apt-get install -y libvulkan-dev vulkan-tools mesa-vulkan-drivers

# Python font oracle (fontTools ttLib) for P1.5b metrics
sudo apt-get install -y python3-fonttools
# or: python3 -m pip install --user fonttools

# SDL3 (only if the SDL3 GPU path is chosen)
sudo apt-get install -y libsdl3-dev

# Playwright browsers (Node package is already vendored under
# lang/compiled/wasm_ui/node_modules)
npx playwright install --with-deps chromium

# gh (already installed here)
sudo apt-get install -y gh
```

### macOS (BLOCKED on this host)

```bash
brew install gtk+3 vulkan-loader molten-vk sdl3 python fonttools node gh
# Xcode command line tools provide clang + the Cocoa frameworks.
```

### Windows (BLOCKED on this host)

```powershell
choco install -y mingw cmake ninja python nodejs gh
choco install -y vulkan-sdk
# GTK3 on Windows is not the planned host (Win32 is); SDL3 is optional.
```

## Notes

- The **reference laptop in `BENCHMARKS.md` is an i5-1035G1**; this host is a
  different AMD CPU. Absolute numbers are therefore *not* directly comparable
  to the published budgets. P0.1 records this host's numbers plus the
  CPU/GPU/governor so the parent can decide whether to invest in an equivalent
  runner.
- `powersave` governor + a shared session means the raster numbers have
  run-to-run spread; `raster_probe` prints min/median/max over 11 iterations
  and was pinned with `taskset -c 2`.
- The GTK3 **runtime** is enough for `lang/libs/window` because its `.mod` links
  `:libgtk-3.so.0` by soname. A C/Objective-C shim or a GTK widget host would
  need `libgtk-3-dev` (headers), which is why `linux.gtk3-dev` is FAIL in the
  matrix while the window probe still runs.
