// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "CompletionItemAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include <unordered_set>

template<typename T>
inline boost::optional<T> convert_optional(const std::optional<T> &std_opt) {
    if (std_opt.has_value()) {
        return boost::optional<T>(*std_opt);
    } else {
        return boost::none;
    }
}

void CompletionItemAnalyzer::find_completion_items() {
    unsigned int i = 0;
    auto size = tokens.size();
    std::vector<unsigned int> scopes_prev;
    std::unordered_set<unsigned int> token_positions;
//    std::unordered_set<unsigned int> upcoming_scope;
    unsigned int scope_start = 0;
    while (i < size) {
        auto token = tokens[i].get();
        if (token->type() == LexTokenType::CharOperator) {
            auto casted = as<CharOperatorToken>(i);
            if (casted->op == '{') { // a scope begins

                scopes_prev.push_back(scope_start);
                scope_start = i;

//                for(const auto t : upcoming_scope) {
//                    token_positions.insert(t);
//                }
//                upcoming_scope.clear();

            } else if (casted->op == '}') { // a scope ends

                auto scope_end = i;
                unsigned int j = scope_start; // from last scope to this scope's end
                // remove tokens as the nested scope ended before the cursor position
                while (j < scope_end) {
                    token_positions.erase(j);
                    j++;
                }

                scope_start = scopes_prev.back();
                scopes_prev.pop_back();

            }
        } else {
            token_positions.insert(i);
        }
        if (token->lineNumber() > position.first) {
            break;
        } else if (token->lineNumber() == position.first) {
            if (token->lineCharNumber() >= position.second) {
                break;
            }
        }
        i++;
    }
    for (const auto token_pos: token_positions) {
        auto token = tokens[token_pos].get();
        if (token->lsp_comp_label().has_value()) {
            items.push_back(lsCompletionItem{
                    token->lsp_comp_label().value(),
                    convert_optional(token->lsp_comp_kind())
            });
        }
    }
}