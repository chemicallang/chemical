// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "core/diag/Diagnostic.h"
#include "FlatIGFile.h"
#include "lexer/Token.h"
#include "ast/base/BatchAllocator.h"
#include <memory>

struct LexResult {

    /**
     * this allocator is the allocator all strings live on, that are present inside
     * the tokens, this is composed inside the lexer, however moved here when lexing is done
     */
    BatchAllocator allocator;
    /**
     * this allocator is used inside the lexer to provide support for allocating
     * things to the nested user lexers
     */
    BatchAllocator fileAllocator;
    /**
     * the absolute path to the file
     */
    std::string abs_path;
    /**
     * the lexed tokens
     */
    std::vector<Token> tokens;
    /**
     * diagnostics when tokens were lexed / resolved
     */
    std::vector<Diag> diags;

    /**
     * constructor
     * by default 5000 bytes = 5kb is allocated for tokens of each file in batches
     */
    LexResult() : allocator(0), fileAllocator(0) {

    }

};
