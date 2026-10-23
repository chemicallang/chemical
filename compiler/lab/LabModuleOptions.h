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

/// Module-level options struct.
/// WARNING: Keep in sync with lang/libs/lab/src/lab.ch :: ModuleOptions
struct LabModuleOptions {
    chem::string_view stack_protector; // "none" | "standard" | "strong" | "all" | "" (not set)
    int optimization_level = 0;    // 0-3
};
