// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>
#include <type_traits>
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
    // The host a module's implementation is written for:
    // 0 = any/unset, 1 = js, 2 = native.
    // ABI: this must stay an `int`, matching `ModuleOptions.host : int` in
    // lang/libs/lab/src/lab.ch. `ModulegetOptions` hands this struct to the
    // Chemical build.lab, so a C++ type whose layout differs from the Chemical
    // mirror (e.g. `chem::string` vs libstdc++ `std::string`) is not allowed.
    // A module with host 1 (js) is declarations-only on a non-JS job (see
    // `LabBuildCompiler::mark_declaration_only_modules`).
    int host = 0;

    // Proposal 0005 module policy defaults. 1 means every function body defined
    // in this module forbids the effect unless it declares the positive effect
    // (`[panic]`, `[alloc]`, ...). ABI: `int`, matching
    // `ModuleOptions.no_panic : int` in lang/libs/lab/src/lab.ch.
    int no_panic = 0;
    int no_alloc = 0;
    int no_unsafe = 0;
    int no_blocking = 0;
    int pure = 0;
};

static_assert(std::is_same_v<decltype(LabModuleOptions::host), int>,
              "LabModuleOptions::host must stay an int to match lab.ch::ModuleOptions");
