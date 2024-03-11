// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "SemanticAnalyzer.h"
#include "ScopeAnalyzer.h"
#include <unordered_map>
#include <unordered_set>

class SemanticLinker : public SemanticAnalyzer {
public:

    // constructor
    SemanticLinker(std::vector<std::unique_ptr<LexToken>> &tokens) : SemanticAnalyzer(tokens) {

    }

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
     * tokens that couldn't be resolved in the nested child scopes
     */
    std::unordered_map<std::string, unsigned int> unresolved;

    /**
     * this function is called with the position of the token that couldn't be resolved
     * @param position
     */
    virtual void report_unresolved(unsigned int position) {

    }

    void analyze_scopes();

    /**
     * The function that analyzes
     */
    inline void analyze() {
        analyze_scopes();
    }

};
