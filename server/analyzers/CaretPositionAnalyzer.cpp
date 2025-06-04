// Copyright (c) Chemical Language Foundation 2025.

#include "CaretPositionAnalyzer.h"
#include "core/source/LocationManager.h"

bool CaretPositionAnalyzer::is_caret_inside(SourceLocation location) {
    const auto data = loc_man.getLocationPos(location);
    return is_caret_inside(data.start, data.end);
}

Token* CaretPositionAnalyzer::token_before_caret(std::vector<Token> &tokens) {
    int i = 0;
    while (i < tokens.size()) {
        if (is_caret_eq_or_behind(&tokens[i])) {
            return &tokens[i - 1];
        }
        i++;
    }
    return nullptr;
}

Token* CaretPositionAnalyzer::chain_before_caret(std::vector<Token> &tokens) {
    auto token = token_before_caret(tokens);
    if (token) {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
        std::cout << "token before index : " + token->representation() << " type " << token->type_string() << " parent type " << parent->type_string() << std::endl;
#endif
        return token;
    } else {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
        std::cout << "no token before the caret position" << std::endl;
#endif
        return nullptr;
    }
}