// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <bit>

struct TargetData {

    // compilers
    bool tcc = false;
    bool clang = false;

    // job specific
    bool lsp = false;
    bool test = false;
    bool debug = false;
    bool debug_quick = false;
    bool debug_complete = false;
    bool release = false;
    bool release_safe = false;
    bool release_small = false;
    bool release_fast = false;

    bool posix = false;
    bool gnu = false;

    // basic platform flags
    bool is64Bit = false;
    bool little_endian = false;
    bool big_endian = false;
    bool windows = false;
    bool win32 = false;
    bool win64 = false;
    bool linux = false;
    bool macos = false;
    bool freebsd = false;
    bool unix = false;
    bool android = false;
    bool cygwin = false;
    bool mingw32 = false;
    bool mingw64 = false;
    bool emscripten = false;

    // architecture
    bool x86_64 = false;
    bool x86 = false;
    bool i386 = false;
    bool arm = false;
    bool aarch64 = false;
    bool powerpc = false;
    bool powerpc64 = false;
    bool riscv = false;
    bool s390x = false;
    bool wasm32 = false;
    bool wasm64 = false;

};

/**
 * we use this to create target data, it may not be accurate for
 * target triple, but its a starting point
 */
consteval TargetData create_target_data() {
    TargetData d{};

    // by default debug flag is true
    d.debug = true;

#ifdef COMPILER_BUILD
    d.clang = true;
#else
    d.tcc = true;
#endif

#ifdef LSP_BUILD
    d.lsp = true;
#endif

    // OS / platform detection via macros
#ifdef _WIN32
    d.windows = true;
    d.win32 = true;
#endif

#ifdef _WIN64
    d.windows = true;
    d.win64 = true;
#endif

    d.posix = !d.windows;

#ifdef __linux__
    d.linux = true;
    d.unix = true;
#endif

#ifdef __APPLE__
    d.macos = true;
    d.unix = true;
#endif

#ifdef __FreeBSD__
    d.freebsd = true;
    d.unix = true;
#endif

#ifdef __ANDROID__
    d.android = true;
    d.unix = true;
#endif

#ifdef __CYGWIN__
    d.cygwin = true;
    d.unix = true;
#endif

#ifdef __MINGW32__
    d.mingw32 = true;
    d.win32 = true;
    d.windows = true;
#endif

#ifdef __MINGW64__
    d.mingw64 = true;
    d.win64 = true;
    d.windows = true;
#endif

#ifdef __EMSCRIPTEN__
    d.emscripten = true;
    // Emscripten typically targets WASM and behaves Unix-like for tooling
    d.unix = true;
#endif

    // Architecture detection
#if defined(__x86_64__) || defined(_M_X64) || defined(__amd64__)
    d.x86_64 = true;
#endif

#if defined(__i386__) || defined(_M_IX86)
    d.i386 = true;
    d.x86 = true;
#endif

#if defined(__arm__) || defined(__ARM_ARCH) || defined(_M_ARM)
    d.arm = true;
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
    d.aarch64 = true;
#endif

#if defined(__powerpc__) || defined(__powerpc) || defined(__ppc__)
    d.powerpc = true;
#endif

#if defined(__powerpc64__) || defined(__ppc64__)
    d.powerpc64 = true;
#endif

#if defined(__riscv) || defined(__riscv__)
    d.riscv = true;
#endif

#if defined(__s390x__)
    d.s390x = true;
#endif

#if defined(__wasm32__)
    d.wasm32 = true;
#endif

#if defined(__wasm64__)
    d.wasm64 = true;
#endif

    // Bitness (pointer size)
    d.is64Bit = (sizeof(void*) >= 8);

    // is the system little endian
    d.little_endian = std::endian::native == std::endian::little;
    d.big_endian = std::endian::native != std::endian::little;

    return d;
}