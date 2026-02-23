// Copyright (c) Chemical Language Foundation 2026.

#pragma once

#include "ImportSymbol.h"
#include "core/source/SourceLocation.h"

struct DependencySymbolInfo {
    std::span<ImportSymbol> symbols;
    chem::string_view alias;
    SourceLocation location;
};