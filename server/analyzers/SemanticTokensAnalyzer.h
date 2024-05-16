// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 10/03/2024.
//

#pragma once

#include "LibLsp/lsp/textDocument/SemanticTokens.h"
#include "cst/base/CSTVisitor.h"

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
    void analyze(std::vector<std::unique_ptr<CSTToken>> &tokens);

    /**
     * this just puts the given lex token as a semantic token in the tokens vector
     * also sets prev token line number to this token
     */
    void put(LexToken* token, unsigned int tokenType, unsigned int tokenModifiers = 0);

    // Visitors

    void visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned start, unsigned end);

    void visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned start = 0);

    void visitCommon(CSTToken *token) override;

    void visitLexTokenCommon(LexToken *token) override;

    void visitCompoundCommon(CompoundCSTToken *compound) override;

    // Compound Visitors

    void visitVarInit(CompoundCSTToken *varInit) override;

    void visitFunction(CompoundCSTToken *function) override;

    void visitIf(CompoundCSTToken *ifCst) override;

    void visitWhile(CompoundCSTToken *whileCst) override;

    void visitDoWhile(CompoundCSTToken *doWhileCst) override;

    void visitForLoop(CompoundCSTToken *forLoop) override;

    void visitSwitch(CompoundCSTToken *switchCst) override;

    void visitStructDef(CompoundCSTToken *structDef) override;

    void visitEnumDecl(CompoundCSTToken *enumDecl) override;

    void visitBody(CompoundCSTToken *bodyCst) override;

    // Token visitors

    void visit(AnnotationToken *token) override;

    void visit(BoolToken *token) override;

    void visit(NullToken *token) override;

    void visit(CharOperatorToken *token) override;

    void visit(StringOperatorToken *token) override;

    void visit(CharToken *token) override;

    void visit(CommentToken *token) override;

    void visit(MultilineCommentToken *token) override;

    void visit(MacroToken *token) override;

    void visit(NumberToken *token) override;

    void visit(OperationToken *token) override;

    void visit(StringToken *token) override;

};