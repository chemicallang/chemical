// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#include "SemanticLinker.h"

void SemanticLinker::scope_begins(unsigned int position) {
    scope_start_pos_stack.push_back(scope_start_pos);
    scope_start_pos = position;
}

void SemanticLinker::scope_ends(unsigned int position) {

    auto current_scope_end = position;

    /**
     *
     * explaining how the resolution of tokens works
     * consider the following source code :
     *
     * var a = 5;
     * print(a);
     * {
     *     var b = 3;
     *     print(a + b)
     * }
     * func print();
     *
     * when the first child scope (above the print declaration) ends, 'a' and 'b' are stored on the current map
     * tokens from map_tokens_position to current_scope_end are stored on the map
     * where map_tokens_position is last scope's end or zero (if no scope had ended before)
     * while we store the 'a' and 'b', because of their var decls, we also resolve them in the print functions that we found and we succeed
     * because these tokens appeared before their access
     *
     * the only token that won't be resolved is the function name print, because its defined below its access
     * we haven't covered how we are going to do this, until native code generation is done so we can determine
     * how many nested scope levels are we going to resolve, its probably 1
     *
     */

    std::unordered_set<std::string> current_scope_ids;

    unsigned int i = map_tokens_position;
    while (i < current_scope_end) {
        auto id = tokens[i]->declaration_identifier();
        if (id.has_value()) {
            current[id.value()] = i;
            if(i > scope_start_pos) {
                current_scope_ids.insert(id.value());
            }
        } else {
            // if token is in the current scope resolve it, or store it in unresolved
            id = tokens[i]->resolution_identifier();
            if (id.has_value()) {
                auto found = current.find(id.value());
                if (found != current.end()) {
                    resolved[i] = found->second;
                }
//                else {
//                    unresolved[id.value()] = i;
//                }
            }
        }
        i++;
    }

    // commented because we haven't yet fully worked out the resolution phase for the statements above
//    for (const auto &unres: unresolved) {
//        // only try to resolve the tokens that were above the last scope end's
//        if (unres.second < map_tokens_position) {
//            auto found = current.find(unres.first);
//            if (found != current.end()) {
//                link(unres.second, found->second);
//            } else {
//                report_unresolved(unres.second);
//            }
//            unresolved.erase(unres.first);
//        }
//    }

    // delete identifiers of the current scope only
    for(const auto& id : current_scope_ids) {
        current.erase(id);
    }

    // since tokens from the scopes above have been stored till the end of this scope, set the position
    map_tokens_position = current_scope_end;
    scope_start_pos = scope_start_pos_stack.back();
    scope_start_pos_stack.pop_back();

}
