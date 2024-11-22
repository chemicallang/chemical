// Copyright (c) Qinetik 2024.

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
     * contains the multi string allocation
     */
    chem::string allocated_multi_str;

    /**
     * the tokens vector contains all the tokens
     */
    std::vector<Token> tokens;

};