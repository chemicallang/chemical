// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "SemanticAnalyzer.h"
#include <unordered_map>
#include <unordered_set>

class SemanticLinker : public SemanticAnalyzer {
public:

    // constructor
    SemanticLinker(std::vector<std::unique_ptr<LexToken>> &tokens) : SemanticAnalyzer(tokens) {

    }

    /**
     * this stack stores the start positions of the nested scopes
     * since one scope starts, we set scope_start_pos, another nested starts, we must store the previous one on the stack
     * when nested ends, we must get the last position from the scope_start_pos_stack and set it to scope_start_pos, or zero if stack is empty
     */
    std::vector<unsigned int> scope_start_pos_stack;

    /**
     * The tokens that have resolution
     * the key is unsigned int (position inside the tokens vector of the token that has resolution identifier)
     * the value (position inside the tokens vector of the token that has declaration identifier)
     *
     * these tokens have same resolution & declaration identifiers
     * when we say var x = 5; it means identifier(x) has a declaration identifier = "x"
     * when we say print(x); it means identifier(x) has a resolution identifier = "x"
     * to link the two, we get the resolution identifier and match it with declaration identifier present in the current map
     */
    std::unordered_map<unsigned int, unsigned int> resolved;

    /**
     * the start position (inside the tokens vector) of the current scope
     */
    unsigned int scope_start_pos = 0;

    /**
     * the identifier map till the current position (map_tokens_position)
     * unsigned int is the position
     */
    std::unordered_map<std::string, unsigned int> current;

    /**
     * the position (inside the vector tokens) which tells at the position we have analyzed all the tokens above that position in the current map
     * this doesn't include tokens of the nested scopes, because when a scope ends, its keys will be deleted from the unordered_map
     */
    unsigned int map_tokens_position = 0;

    /**
     * tokens that couldn't be resolved in the nested child scopes
     */
    std::unordered_map<std::string, unsigned int> unresolved;

    /**
     * this function is called with the position of the token that couldn't be resolved
     * @param position
     */
    virtual void report_unresolved(unsigned int position) {

    }

    /**
     * The function that analyzes
     */
    inline void analyze() {
        analyze_scopes();
    }

    void scope_begins(unsigned int position) override;

    void scope_ends(unsigned int position) override;

};
