// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once

#include "lsp/types.h"
#include "preprocess/visitors/RecursiveVisitor.h"
#include "ast/base/ASTNode.h"
#include "integration/common/Position.h"
#include "FoldingRangeAnalyzerApi.h"

class LocationManager;

class FoldingRangeAnalyzer : public RecursiveVisitor<FoldingRangeAnalyzer> {
public:

    /**
     * location manager is used to decode loations
     */
    LocationManager& loc_man;

    /**
     * all the folding ranges found
     */
    std::vector<lsp::FoldingRange> ranges;

    /**
     * constructor
     * @param tokens
     */
    FoldingRangeAnalyzer(LocationManager& loc_man) : loc_man(loc_man) {

    }

    /**
     * will analyze the given nodes of the current document
     * @param nodes
     */
    void analyze(std::vector<ASTNode*>& nodes) {
        for(auto node : nodes) {
            visit(node);
        }
    }

    /**
     * will add a folding range from start to end token
     */
    void folding_range(const Position& start, const Position& end, bool comment = false);

    /**
     * create a folding range for the given source location
     */
    void folding_range(SourceLocation location, bool comment = false);

    // Visitor functions

    void VisitScope(Scope* node);


};