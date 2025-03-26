// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "std/chem_string_view.h"
#include <vector>
#include <span>

struct ModuleFileData {
public:

    /**
     * interface declarations in chemical.mod file allowing user to import
     * compiler interfaces easily
     */
    std::vector<std::span<const std::pair<chem::string_view, void*>>> compiler_interfaces;

};