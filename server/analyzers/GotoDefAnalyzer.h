// Copyright (c) Qinetik 2024.

#pragma once

#include <vector>
#include "integration/common/Location.h"

class ImportUnit;

class GotoDefAnalyzer {
public:

    /**
     * the position of token, at which user asked for goto def
     */
    Position position;

    /**
     * constructor
     */
    GotoDefAnalyzer(Position position);

    /**
     * this function analyzes the import unit, in which last file is the one which contains the
     * token where user asked to goto def
     * It will provide locations, where that symbol has definition
     */
    std::vector<Location> analyze(ImportUnit* unit);

};