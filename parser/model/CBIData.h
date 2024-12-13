// Copyright (c) Qinetik 2024.

#pragma once

#include "libtcc.h"
#include <vector>

struct CBIData {

    /**
     * all the modules in this CBI
     */
    std::vector<TCCState*> modules;

    /**
     * the module in which public functions are searched
     */
    TCCState* entry_module = nullptr;

};