// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once

#include "lsp/types.h"
#include "ast/base/ASTNode.h"
#include "core/diag/Position.h"
#include "lexer/Token.h"

class FoldingRangeAnalyzer {
public:

    /**
     * all the folding ranges found
     */
    std::vector<lsp::FoldingRange> ranges;

    /**
     * constructor
     * @param tokens
     */
    FoldingRangeAnalyzer() {

    }

    /**
     * tokens can be analyzed to provide folding ranges
     */
    void analyze(std::vector<Token>& tokens);

    /**
     * will add a folding range from start to end token
     */
    void folding_range(const Position& start, const Position& end, bool comment = false);


};