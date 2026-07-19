// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>
#include "std/chem_string_view.h"

/**
 * This C++ struct may be exposed to generated C code via the CBI interface.
 * If you add, remove, or change a field here and also define a matching
 * struct in Chemical (for CBI access), ensure the field ORDER, NAMES,
 * and TYPES match exactly, or the struct layout will be incompatible.
 */

/// Nested checks options.
struct LabModuleChecksOptions {
    bool bounds = false;
    bool overflow = false;
    bool null = false;
};

/// Module-level options struct.
/// WARNING: Keep in sync with lang/libs/lab/src/lab.ch :: ModuleOptions
struct LabModuleOptions {
    bool safety = true;
    LabModuleChecksOptions checks;
    chem::string_view safe_mode;       // "off" | "warn" | "enforce" | "" (not set)
    chem::string_view stack_protector; // "none" | "standard" | "strong" | "all" | "" (not set)
    int optimization_level = 0;    // 0-3
};
