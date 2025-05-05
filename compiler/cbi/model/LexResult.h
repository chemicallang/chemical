// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "integration/common/Diagnostic.h"
#include "FlatIGFile.h"
#include "lexer/Token.h"
#include "ast/base/BatchAllocator.h"
#include <memory>

struct LexResult {

    /**
     * the allocator is where the token strings are stored for the file
     */
    BatchAllocator allocator;
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
    LexResult(std::size_t batchSize = 5000) : allocator(batchSize) {

    }

};
