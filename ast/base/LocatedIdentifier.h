// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "cst/SourceLocation.h"


struct LocatedIdentifier {

    /**
     * the actual identifier
     */
    std::string identifier;

    /**
     * location of the identifier
     */
#ifdef LSP_BUILD
    SourceLocation location;
#endif

};

constexpr LocatedIdentifier ZERO_LOC_ID(std::string identifier) {
#ifdef LSP_BUILD
    return { std::move(identifier), ZERO_LOC };
#else
    return { std::move(identifier) };
#endif
}

constexpr LocatedIdentifier LOC_ID(std::string identifier, SourceLocation location) {
#ifdef LSP_BUILD
    return { std::move(identifier), location };
#else
    return { std::move(identifier) };
#endif
}