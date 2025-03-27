// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "libtcc.h"
#include <vector>

struct CBIData {

    /**
     * the module in which public functions are searched
     */
    TCCState* module = nullptr;

};