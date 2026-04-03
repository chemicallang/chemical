// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "core/diag/Diagnostic.h"
#include "lexer/Token.h"
#include "ast/base/BatchAllocator.h"
#include <memory>

#include "stream/FileInputSource.h"

struct LexResult {
    /**
     * must be assigned
     */
    unsigned int fileId;
    /**
     * if any error occurred during lexing, this will be made true
     */
    bool has_errors = false;
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
     * this is only used if overridden source doesn't exit, meaning we read from the file
     */
    FileInputSource fileSource;

    /**
     * when lex source is overridden source
     * IDE has edited changes, that haven't been saved to disk, we store the source
     * in this variable, then we lex this source, it has to be valid, because tokens above contain
     * pointers into this string, IT must not change once initialized
     */
    std::string overridden_source;

    /**
     * constructor
     * by default 5000 bytes = 5kb is allocated for tokens of each file in batches
     */
    LexResult(unsigned int fileId) : fileId(fileId), allocator(0), fileAllocator(0) {

    }

};
