// Copyright (c) Qinetik 2024.

#include "CaretPositionAnalyzer.h"


bool CaretPositionAnalyzer::is_eq_caret(CSTToken* token) const {
    return is_eq_caret(token->position());
}

bool CaretPositionAnalyzer::is_ahead(CSTToken *token) const {
    return is_ahead(token->position());
}

bool CaretPositionAnalyzer::is_caret_inside(CSTToken *token) {
    return is_behind(token->start_token()->position()) && !is_behind(token->end_token()->position());
}

CSTToken* CaretPositionAnalyzer::child_container(CSTToken* compound) {
    for (auto &token: compound->tokens) {
        if (token->compound() && is_caret_inside(token)) {
            return token;
        }
    }
    return nullptr;
}

CSTToken* CaretPositionAnalyzer::direct_parent(std::vector<CSTToken*> &tokens) {
    CSTToken* child;
    CSTToken* nested;
    for (auto &token: tokens) {
        if (token->compound() && is_caret_inside(token)) {
            child = (CSTToken* ) token;
            while (true) {
                nested = child_container(child);
                if (nested == nullptr) {
                    return child;
                } else {
                    child = nested;
                }
            }
        }
    }
    return nullptr;
}

CSTToken* last_direct_parent(CSTToken* token) {
    if(token->compound()) {
        auto last = token->tokens[token->tokens.size() - 1];
        if(last->compound()) {
            return last_direct_parent(last);
        } else {
            return token;
        }
    } else {
        return token;
    }
}

CSTToken* CaretPositionAnalyzer::token_before_caret(std::vector<CSTToken*> &tokens) {
    int i = 0;
    while (i < tokens.size()) {
        if (is_caret_eq_or_behind(tokens[i]->start_token())) {
            return last_direct_parent(tokens[i - 1]);
        }
        i++;
    }
    return nullptr;
}

CSTToken* CaretPositionAnalyzer::chain_before_caret(std::vector<CSTToken*> &tokens) {
    auto parent = direct_parent(tokens);
    if (parent == nullptr) {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
        std::cout << "Couldn't find direct parent" << std::endl;
#endif
        return nullptr;
    } else {
        auto token = token_before_caret(parent->tokens);
        if (token) {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
            std::cout << "token before index : " + token->representation() << " type " << token->type_string() << " parent type " << parent->type_string() << std::endl;
#endif
            if(token->type() == LexTokenType::CompAccessChain || token->type() == LexTokenType::CompAccessChainNode) {
                return (CSTToken*) token;
            }
            return nullptr;
        } else {
#if defined DEBUG_COMPLETION && DEBUG_COMPLETION
            std::cout << "no token before the caret position" << std::endl;
#endif
            return nullptr;
        }
    }
}