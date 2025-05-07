// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "integration/common/Position.h"

class LexImportUnit;

class LocationManager;

class LexResult;

class HoverAnalyzer {
public:

    /**
     * location manager is used to decode locations
     */
    LocationManager& loc_man;

    /**
     * the position user hovered at
     */
    Position position;

    /**
     * a string value is built for the hover, this is usually some markup content
     * currently : markdown being used
     */
    std::string value;

    /**
     * constructor
     */
    HoverAnalyzer(LocationManager& locMan, Position position);

    /**
     * it will analyze the position at hover occurred and return a markdown representation for the hover
     * panel in the IDE
     */
    std::string markdown_hover(LexResult* current_file);

};