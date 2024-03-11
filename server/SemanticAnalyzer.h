// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#pragma once

#include "lexer/model/tokens/LexToken.h"
#include <vector>
#include <memory>

class SemanticAnalyzer {
public:

    /**
     * tokens
     */
    std::vector<std::unique_ptr<LexToken>> &tokens;

    /**
     * constructor
     * @param tokens
     */
    SemanticAnalyzer(std::vector<std::unique_ptr<LexToken>> &tokens) : tokens(tokens) {

    }

    /**
     * this will return a raw pointer to the token at position
     * as derived token of class given type parameter T
     * This will fail if the token at current position is not of derived class of given type parameter
     * The raw pointer may become dangling, if unique_ptr is destroyed !
     */
    template<typename T>
    inline T *as(unsigned int position);

};

template<typename T>
T *SemanticAnalyzer::as(unsigned int position) {
    return static_cast<T *>(tokens[position].get());
}