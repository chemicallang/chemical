// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "FoldingRangeAnalyzer.h"
#include "ast/structures/Scope.h"
#include "core/source/LocationManager.h"

void FoldingRangeAnalyzer::folding_range(const Position& start, const Position& end, bool comment) {
    ranges.push_back(lsp::FoldingRange{
            start.line,
            end.line,
            start.character,
            end.character,
            comment ? lsp::FoldingRangeKind::Comment : lsp::FoldingRangeKind::Region
    });
}

void FoldingRangeAnalyzer::analyze(std::vector<Token>& tokens) {
    ranges.reserve(120);
    std::vector<Position> bracesStack;
    bracesStack.reserve(20);
    for(auto& token : tokens) {
        switch(token.type) {
            case TokenType::LBrace:
                bracesStack.emplace_back(token.position);
                break;
            case TokenType::RBrace:
                if(!bracesStack.empty()) {
                    const auto opening = bracesStack.back();
                    bracesStack.pop_back();
                    folding_range(opening, token.position, false);
                }
                break;
            default:
                break;
        }
    }
}