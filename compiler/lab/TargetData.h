// Copyright (c) Qinetik 2024.

#pragma once

/**
 * when you use def.windows or def.linux, this target data
 * is what was used to calculate these native definitions that are provided
 * in the chemical compiler
 */
struct TargetData {
    bool is_64Bit = false;
    bool is_win32 = false;
    bool is_win64 = false;
    bool is_windows = false;
    bool is_linux = false;
    bool is_macos = false;
    bool is_freebsd = false;
    bool is_unix = false;
    bool is_android = false;
    bool is_cygwin = false;
    bool is_mingw32 = false;
    bool is_x86_64 = false;
    bool is_i386 = false;
    bool is_arm = false;
    bool is_aarch64 = false;
};