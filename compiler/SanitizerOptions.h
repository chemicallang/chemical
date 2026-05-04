// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

/**
 * Supported LLVM sanitizer types
 * These map to LLVM's -fsanitize= flags and corresponding passes
 */
enum class SanitizerType : uint32_t {
    None = 0,

    // Thread Sanitizer - detects data races
    // -fsanitize=thread
    Thread = 1 << 0,

    // Address Sanitizer - detects memory errors (use-after-free, buffer overflow)
    // -fsanitize=address
    Address = 1 << 1,

    // Memory Sanitizer - detects uninitialized memory reads
    // -fsanitize=memory
    Memory = 1 << 2,

    // Undefined Behavior Sanitizer - detects undefined behavior
    // -fsanitize=undefined
    UndefinedBehavior = 1 << 3,

    // Leak Sanitizer - detects memory leaks
    // -fsanitize=leak
    Leak = 1 << 4,

    // Hardware Address Sanitizer - similar to ASan but uses hardware features (AArch64)
    // -fsanitize=hwaddress
    HWAddress = 1 << 5,

    // DataFlow Sanitizer - taint tracking
    // -fsanitize=dataflow
    DataFlow = 1 << 6,
};

// Bitwise OR operator for combining sanitizers
inline SanitizerType operator|(SanitizerType a, SanitizerType b) {
    return static_cast<SanitizerType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// Bitwise AND operator for checking sanitizer flags
inline SanitizerType operator&(SanitizerType a, SanitizerType b) {
    return static_cast<SanitizerType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline SanitizerType& operator|=(SanitizerType& a, SanitizerType b) {
    a = a | b;
    return a;
}

// Check if a specific sanitizer is enabled
inline bool has_sanitizer(SanitizerType flags, SanitizerType check) {
    return (flags & check) != SanitizerType::None;
}

// Get the -fsanitize= flag string for clang linking
// Some sanitizers can be combined: -fsanitize=address,undefined
inline const char* get_sanitizer_flag(SanitizerType type) {
    switch (type) {
        case SanitizerType::Thread:          return "-fsanitize=thread";
        case SanitizerType::Address:         return "-fsanitize=address";
        case SanitizerType::Memory:          return "-fsanitize=memory";
        case SanitizerType::UndefinedBehavior: return "-fsanitize=undefined";
        case SanitizerType::Leak:            return "-fsanitize=leak";
        case SanitizerType::HWAddress:       return "-fsanitize=hwaddress";
        case SanitizerType::DataFlow:        return "-fsanitize=dataflow";
        default:                             return nullptr;
    }
}

// Get the library name for linking (for lld linker)
inline const char* get_sanitizer_lib(SanitizerType type) {
    switch (type) {
        case SanitizerType::Thread:          return "-ltsan";
        case SanitizerType::Address:         return "-lasan";
        case SanitizerType::Memory:          return "-lmsan";
        case SanitizerType::Leak:            return "-llsan";
        case SanitizerType::UndefinedBehavior: return "-lubsan";
        default:                             return nullptr;
    }
}
