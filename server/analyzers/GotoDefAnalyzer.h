// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "integration/common/Location.h"

class LexImportUnit;

class LocationManager;

class GotoDefAnalyzer {
public:

    /**
     * location manager
     */
    LocationManager& manager;

    /**
     * the position of token, at which user asked for goto def
     */
    Position position;

    /**
     * constructor
     */
    GotoDefAnalyzer(
        LocationManager& manager,
        Position position
    );

    /**
     * this function analyzes the import unit, in which last file is the one which contains the
     * token where user asked to goto def
     * It will provide locations, where that symbol has definition
     */
    std::vector<Location> analyze(LexImportUnit* unit);

};