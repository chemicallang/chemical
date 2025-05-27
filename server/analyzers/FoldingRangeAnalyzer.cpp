// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "FoldingRangeAnalyzer.h"
#include "ast/structures/Scope.h"
#include "cst/LocationManager.h"

void FoldingRangeAnalyzer::folding_range(const Position& start, const Position& end, bool comment) {
    ranges.push_back(lsp::FoldingRange{
            start.line,
            end.line,
            start.character,
            end.character,
            comment ? lsp::FoldingRangeKind::Comment : lsp::FoldingRangeKind::Region
    });
}

void FoldingRangeAnalyzer::folding_range(SourceLocation loc, bool comment) {
    if(loc.isValid()) {
        const auto location = loc_man.getLocationPos(loc);
        folding_range(location.start, location.end, false);
    }
}

void FoldingRangeAnalyzer::VisitScope(Scope* scope) {
    folding_range(scope->encoded_location());
}

std::vector<lsp::FoldingRange> folding_analyze(LocationManager& locMan, std::vector<ASTNode*>& nodes) {
    FoldingRangeAnalyzer analyzer(locMan);
    for(const auto node : nodes) {
        analyzer.visit(node);
    }
    return std::move(analyzer.ranges);
}