// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include <vector>
#include <cstdint>
#include "lexer/Token.h"

class ASTNode;

class CompilerBinder;

class SemanticTokensAnalyzer {
private:

    /**
     * previous token's line number
     */
    unsigned int prev_token_line_num = 0;

    /**
     * previous token's line number
     */
    unsigned int prev_token_char_num = 0;

public:

    /**
     * this is a cache variable
     */
    Token* current_token;

    /**
     * this is a cache variable
     */
    Token* end_token;

    /**
     * compiler binder is required to support nested token support
     */
    CompilerBinder& binder;

    /**
     * all the items that were found when analyzer completed
     */
    std::vector<uint32_t> tokens;

    /**
     * constructor
     */
    SemanticTokensAnalyzer(CompilerBinder& binder) : binder(binder) {}

    /**
     * put a lsp token
     */
    void put(
        uint32_t lineNumber,
        uint32_t lineCharNumber,
        uint32_t length,
        uint32_t tokenType,
        uint32_t tokenModifiers
    );

    /**
     * this just puts the given lex token as a semantic token in the tokens vector
     * also sets prev token line number to this token
     */
    void put(Token* token, uint32_t tokenType, uint32_t tokenModifiers = 0);

    /**
     * will put the token, highlighting it according to given node
     */
    void put_node_token(Token* token, ASTNode* node);

    /**
     * will automatically determine the token type based on token
     */
    void put_auto(Token* token);

    /**
     * analyzes the given tokens
     */
    void analyze(std::vector<Token> &tokens);

    /**
     * put a multiline token
     */
    void putMultilineToken(
            Token *token,
            uint32_t tokenType,
            uint32_t tokenModifiers,
            unsigned int charStartOffset,
            unsigned int charEndOffset
    );

    /**
     * helper function
     */
    inline void putMultilineToken(Token *token, uint32_t tokenType, uint32_t tokenModifiers = 0) {
        putMultilineToken(token, tokenType, tokenModifiers, 0, 0);
    }

};