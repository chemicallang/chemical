// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "cst/base/CSTVisitor.h"
#include "parser/model/LexTokenType.h"

class SemanticTokensAnalyzer : public CSTVisitor {
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
     * all the items that were found when analyzer completed
     */
    std::vector<SemanticToken> tokens;

    /**
     * constructor
     */
    SemanticTokensAnalyzer() {}

    /**
     * put a lsp token
     */
    void put(
        unsigned int lineNumber,
        unsigned int lineCharNumber,
        unsigned int length,
        unsigned int tokenType,
        unsigned int tokenModifiers
    );

    /**
     * this just puts the given lex token as a semantic token in the tokens vector
     * also sets prev token line number to this token
     */
    inline void put(CSTToken* token, unsigned int tokenType, unsigned int tokenModifiers = 0);

    /**
     * will automatically determine the token type based on token
     */
    void put_auto(CSTToken* token);

    // Visitors

    /**
     * @param tokens the tokens vector
     * @param start inclusive start
     * @param till exclusive end
     */
    void visit(std::vector<CSTToken*> &tokens, unsigned start, unsigned till);

    inline void visit(std::vector<CSTToken*> &tokens_vec, unsigned start = 0) {
        visit(tokens_vec, start, tokens_vec.size());
    }

    inline void analyze(std::vector<CSTToken*> &tokens_vec) {
        visit(tokens_vec, 0, tokens_vec.size());
    }

    void visitCommon(CSTToken *token) final;

    void visitLexTokenCommon(CSTToken *token) final;

    void visitCompoundCommon(CSTToken* compound) final;

    // Compound Visitors

    void visitVariableToken(CSTToken *token) final {
        put_auto(token);
    }

    void visitTypeToken(CSTToken *token) final {
        put_auto(token);
    }

    void visitIdentifierToken(CSTToken *token) final {
        put_auto(token);
    }

    void visitFunction(CSTToken* function) final;

    void visitInterface(CSTToken* interface) final;

    void visitStructDef(CSTToken* structDef) final;

    void visitImpl(CSTToken* impl) final;

    void visitEnumDecl(CSTToken* enumDecl) final;

    void visitBody(CSTToken* bodyCst) final;

    // Token visitors

    void visitMultilineComment(CSTToken *token) final;

};