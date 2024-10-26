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
    SourceLocation location;

};