// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//
#ifdef LSP_BUILD
#include "CompletionItemAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include <unordered_set>
#include <boost/optional.hpp>

//template<typename T>
//inline boost::optional<T> convert_optional(const std::optional<T> &std_opt) {
//    if (std_opt.has_value()) {
//        return boost::optional<T>(*std_opt);
//    } else {
//        return boost::none;
//    }
//}

void CompletionItemAnalyzer::find_completion_items() {
    unsigned int i = 0;
    auto size = tokens.size();
    std::vector<unsigned int> scopes_prev;
    std::unordered_set<unsigned int> token_positions;
//    std::unordered_set<unsigned int> upcoming_scope;
    unsigned int scope_start = 0;
    while (i < size) {
        auto token = tokens[i].get();
        if (token->lineNumber() > position.first) {
            // std::cerr << "Broke at " << token->type_string() << " because line numbers : " << std::to_string(token->lineNumber()) << " > " << std::to_string(position.first) << " \n";
            break;
        } else if (token->lineNumber() == position.first) {
            if (token->lineCharNumber() >= position.second) {
                // std::cerr << "Broke at " << token->type_string() << " because at same line number " << std::to_string(token->lineNumber()) << ", the character numbers " << std::to_string(token->lineCharNumber()) << " >= " << std::to_string(position.second) << '\n';
                break;
            }
        }
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
        } else if(token->lsp_has_comp()){
            token_positions.insert(i);
        }
        i++;
    }
    for (const auto token_pos: token_positions) {
        auto token = tokens[token_pos].get();
        if (token->lsp_comp_label().has_value()) {
            items.push_back(lsCompletionItem{
                    token->lsp_comp_label().value(),
                    token->lsp_comp_kind()
            });
        } else {
            std::cerr << "Token:" << token->type_string() << " stated has completions but returned no label.";
        }
    }
}
#endif