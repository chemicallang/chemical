// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "cst/base/CSTVisitor.h"
#include "lexer/model/LexTokenType.h"

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
     * The function that analyzes
     */
    void analyze(std::vector<CSTToken*> &tokens);

    /**
     * this just puts the given lex token as a semantic token in the tokens vector
     * also sets prev token line number to this token
     */
    void put(CSTToken* token, unsigned int tokenType, unsigned int tokenModifiers = 0);

    /**
     * will automatically determine the token type based on token
     */
    void put_auto(CSTToken* token);

    // Visitors

    void visit(std::vector<CSTToken*> &tokens, unsigned start, unsigned end);

    void visit(std::vector<CSTToken*> &tokens, unsigned start = 0);

    void visitCommon(CSTToken *token) override;

    void visitLexTokenCommon(CSTToken *token) override;

    void visitCompoundCommon(CSTToken* compound) override;

    // Compound Visitors

    void visitVariableToken(CSTToken *token) override {
        put_auto(token);
    }

    void visitTypeToken(CSTToken *token) override {
        put_auto(token);
    }

    void visitIdentifierToken(CSTToken *token) override {
        put_auto(token);
    }

    void visitVarInit(CSTToken* varInit) override;

    void visitFunction(CSTToken* function) override;

    void visitIf(CSTToken* ifCst) override;

    void visitWhile(CSTToken* whileCst) override;

    void visitDoWhile(CSTToken* doWhileCst) override;

    void visitForLoop(CSTToken* forLoop) override;

    void visitSwitch(CSTToken* switchCst) override;

    void visitInterface(CSTToken* interface) override;

    void visitStructDef(CSTToken* structDef) override;

    void visitImpl(CSTToken* impl) override;

    void visitEnumDecl(CSTToken* enumDecl) override;

    void visitBody(CSTToken* bodyCst) override;

    // Token visitors

    void visitAnnotationToken(CSTToken *token) override;

    void visitBoolToken(CSTToken *token) override;

    void visitNullToken(CSTToken *token) override;

    void visitAccessChain(CSTToken *accessChain) override;

    void visitCharOperatorToken(CSTToken *token) override;

    void visitStringOperatorToken(CSTToken *token) override;

    void visitCharToken(CSTToken *token) override;

    void visitCommentToken(CSTToken *token) override;

    void visitMultilineComment(CSTToken *token) override;

    void visitMacroToken(CSTToken *token) override;

    void visitNumberToken(NumberToken *token) override;

    void visitOperationToken(CSTToken *token) override;

    void visitStringToken(CSTToken *token) override;

};