// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//
#ifdef LSP_BUILD
#include "FoldingRangeAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"

void FoldingRangeAnalyzer::analyze_scopes() {
    unsigned int i = 0;
    auto size = tokens.size();
    std::vector<unsigned int> scope_start_pos_stack;
    unsigned int scope_start_pos = 0;
    while (i < size) {
        auto token = tokens[i].get();
        if (token->type() == LexTokenType::CharOperator) {
            auto casted = as<CharOperatorToken>(i);
            if (casted->op == '{') {
                scope_start_pos_stack.push_back(scope_start_pos);
                scope_start_pos = i;
            } else if (casted->op == '}') {
                auto start = tokens[scope_start_pos].get();
                auto end = tokens[i].get();

                ranges.push_back(FoldingRange{

                        static_cast<int>(start->position.lineNumber),
                        static_cast<int>(end->position.lineNumber),
                        static_cast<int>(start->position.lineCharNumber),
                        static_cast<int>(end->position.lineCharNumber),

                        // TODO allow folding ranges of type comments
                        "region"

                });

                scope_start_pos = scope_start_pos_stack.back();
                scope_start_pos_stack.pop_back();
            }
        }
        i++;
    }
}
#endif