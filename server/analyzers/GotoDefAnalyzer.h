// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include "core/diag/Location.h"
#include "lsp/types.h"
#include "lexer/Token.h"

class LocationManager;

class LexResult;

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
     * get the definition links
     */
    std::vector<lsp::DefinitionLink> analyze(std::vector<Token>& tokens);

};