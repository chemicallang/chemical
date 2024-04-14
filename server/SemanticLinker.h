// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include <unordered_map>
#include <unordered_set>
#include "lexer/model/tokens/LexToken.h"

class SemanticLinker {
public:

    // constructor
    SemanticLinker(std::vector<std::unique_ptr<LexToken>> &tokens) {

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
     * tokens that aren't found, thus can be classified as errors
     */
    std::unordered_set<unsigned int> not_found;

    /**
     * tokens that couldn't be resolved in the nested child scopes
     * these tokens may be found in the future if they require
     */
    std::unordered_map<std::string, unsigned int> unresolved;

};
