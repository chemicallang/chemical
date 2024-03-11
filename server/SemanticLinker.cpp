// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "SemanticLinker.h"
#include "lexer/model/tokens/CharOperatorToken.h"

void SemanticLinker::analyze_scopes() {
    unsigned int i = 0;
    auto size = tokens.size();
    // this stores the positions of scope start '{' previously, since nested '{' would overwrite, so we store
    // and pop back when the nested scope ends
    std::vector<unsigned int> scopes_prev;
    // the current identifiers found mapped to their positions in tokens vector
    std::unordered_map<std::string, unsigned int> current;
    // this tracks the current scope start
    unsigned int scope_start = 0;
    while (i < size) {
        auto token = tokens[i].get();
        if (token->type() == LexTokenType::CharOperator) {
            auto casted = as<CharOperatorToken>(i);
            if (casted->op == '{') {

                scopes_prev.push_back(scope_start);
                scope_start = i;

            } else if (casted->op == '}') {

                // remove tokens from the start of this scope till the end,
                // so these tokens are unresolvable below this scope
                unsigned int j = scope_start;
                while (j < i) {
                    auto id = tokens[j]->declaration_identifier();
                    if (id.has_value()) {
                        current.erase(id.value());
                    }
                    j++;
                }

                scope_start = scopes_prev.back();
                scopes_prev.pop_back();

            }
        } else {

            // declare and resolve the tokens
            auto id = tokens[i]->declaration_identifier();
            if (id.has_value()) {
                current[id.value()] = i;
            } else {
                // if token is in the current scope resolve it, or store it in unresolved
                id = tokens[i]->resolution_identifier();
                if (id.has_value()) {
                    auto found = current.find(id.value());
                    if (found != current.end()) {
                        resolved[i] = found->second;
                    } else {
                        if(tokens[i]->resolve_below_current_scope()) {
                            // unresolved[id.value()] = i;
                        } else {
                            not_found.insert(i);
                        }
                    }
                }
            }
        }
        i++;
    }
}