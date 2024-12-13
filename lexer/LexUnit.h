// Copyright (c) Qinetik 2024.

#pragma once

#include "lexer/Token.h"
#include "ast/base/BatchAllocator.h"
#include <vector>
#include "std/chem_string.h"
#include "std/alloc/SerialStrAllocator.h"

/**
 * lex unit represents a single file completely lexed
 */
struct LexUnit {

    /**
     * strings present in source code are allocated using this allocator
     */
    SerialStrAllocator allocator;

    /**
     * the tokens vector contains all the tokens
     */
    std::vector<Token> tokens;

    /**
     * constructor
     * The allocator is useless as it doesn't initialize itself
     */
    LexUnit() : allocator(0) {

    }

};