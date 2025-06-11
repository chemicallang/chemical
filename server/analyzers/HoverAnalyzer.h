// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string_view>
#include "core/diag/Position.h"
#include <vector>
#include <lexer/Token.h>

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
     * markdown hover tokens
     */
    std::string markdown_hover(const std::string_view& abs_path, std::vector<Token>& tokens);

};