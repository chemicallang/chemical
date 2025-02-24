// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once

#include "LibLsp/lsp/textDocument/foldingRange.h"
#include "cst/base/CSTVisitor.h"
#include "cst/base/CSTToken.h"
#include "ast/utils/CommonVisitor.h"
#include "ast/base/ASTNode.h"

class LocationManager;

class FoldingRangeAnalyzer : public CommonVisitor {
public:

    /**
     * location manager is used to decode loations
     */
    LocationManager& loc_man;

    /**
     * all the folding ranges found
     */
    std::vector<FoldingRange> ranges;

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
            node->accept(this);
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

    void visit(Scope *scope) override;

    void visit(Comment *comment) override;


};