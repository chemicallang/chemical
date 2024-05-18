// Copyright (c) Qinetik 2024.

#pragma once

#include "common/Position.h"

class ImportUnit;

class HoverAnalyzer {
public:

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
    HoverAnalyzer(Position position);

    /**
     * it will analyze the position at hover occurred and return a markdown representation for the hover
     * panel in the IDE
     */
    std::string markdown_hover(ImportUnit* unit);

};