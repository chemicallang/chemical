// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "lexer/Token.h"
#include "ast/base/BatchAllocator.h"
#include <vector>
#include "std/chem_string.h"

/**
 * lex unit represents a single file completely lexed
 */
struct LexUnit {

    /**
     * the tokens vector contains all the tokens
     */
    std::vector<Token> tokens;

    /**
     * constructor
     * The allocator is useless as it doesn't initialize itself
     */
    LexUnit() {

    }

};