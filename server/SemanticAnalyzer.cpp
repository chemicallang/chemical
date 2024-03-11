// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "SemanticAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"

void SemanticAnalyzer::analyze_scopes() {
    unsigned int i = 0;
    auto size = tokens.size();
    while (i < size) {
        auto token = tokens[i].get();
        if (token->type() == LexTokenType::CharOperator) {
            auto casted = as<CharOperatorToken>(i);
            if (casted->op == '{') {
                scope_begins(i);
            } else if (casted->op == '}') {
                scope_ends(i);
            }
        }
        i++;
    }
}