// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "std/chem_string_view.h"
#include <span>

struct ImportSymbol {
    std::span<chem::string_view> parts;
    chem::string_view alias;
};