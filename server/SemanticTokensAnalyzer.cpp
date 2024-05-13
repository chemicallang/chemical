// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "SemanticTokensAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include <unordered_set>
#include "cst/structures/BodyCST.h"
#include "cst/statements/VarInitCST.h"
#include "cst/structures/ForLoopCST.h"
#include "cst/structures/WhileCST.h"
#include "cst/structures/DoWhileCST.h"
#include "cst/statements/IfCST.h"
#include "cst/structures/FunctionCST.h"
#include "cst/structures/EnumDeclCST.h"

#define DEBUG false

void SemanticTokensAnalyzer::put(LexToken *token, unsigned int tokenType, unsigned int tokenModifiers) {
    tokens.emplace_back(
            token->lineNumber() - prev_token_line_num, (
                    token->lineNumber() == prev_token_line_num ? (
                            // on the same line
                            token->lineCharNumber() - prev_token_char_num
                    ) : (
                            // on a different line
                            token->lineCharNumber()
                    )
            ), token->length(), tokenType, tokenModifiers
    );
    prev_token_char_num = token->lineCharNumber();
    prev_token_line_num = token->lineNumber();
}

void SemanticTokensAnalyzer::visitCommon(CSTToken *token) {
    throw std::runtime_error("[SemanticTokensAnalyzer] VISIT_COMMON called ! It shouldn't have when it's overridden");
}

void SemanticTokensAnalyzer::visitLexTokenCommon(LexToken *token) {
    switch (token->type()) {
        case LexTokenType::Keyword:
            put(token, SemanticTokenType::ls_keyword);
            break;
        case LexTokenType::Variable:
            put(token, SemanticTokenType::ls_variable);
            break;
        case LexTokenType::Type:
            put(token, SemanticTokenType::ls_type);
            break;
        default:
            std::string s;
            token->append_representation(s);
            std::cerr << "[SemanticTokensAnalyzer] : Token with representation " << s
                      << " and type " << token->type_string()
                      << " called visit common, has been appended as a comment token" << std::endl;
            put(token, SemanticTokenType::ls_comment);
    }
}

void SemanticTokensAnalyzer::visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned start, unsigned end) {
    unsigned int i = start;
    unsigned int till = end;
    while (i < till) {
        tokens[i]->accept(this);
        i++;
    }
}

void SemanticTokensAnalyzer::visit(std::vector<std::unique_ptr<CSTToken>> &tokens, unsigned start) {
    visit(tokens, start, tokens.size());
}

void SemanticTokensAnalyzer::visitCompoundCommon(CompoundCSTToken *compound) {
    visit(compound->tokens);
}

void SemanticTokensAnalyzer::visit(BodyCST *bodyCst) {
    analyze(bodyCst->tokens);
}

void SemanticTokensAnalyzer::visit(EnumDeclCST *enumDecl) {
    enumDecl->tokens[0]->accept(this);
    put(enumDecl->tokens[1]->start_token(), SemanticTokenType::ls_enum);
    visit(enumDecl->tokens, 2);
}

void SemanticTokensAnalyzer::visit(VarInitCST *varInit) {
    visitCompoundCommon((CompoundCSTToken *) varInit);
}

void SemanticTokensAnalyzer::visit(FunctionCST *function) {
    function->tokens[0]->accept(this);
    put(function->tokens[1]->start_token(), SemanticTokenType::ls_function);
    visit(function->tokens, 2);
};

void SemanticTokensAnalyzer::visit(IfCST *ifCst) {
    visitCompoundCommon((CompoundCSTToken *) ifCst);
};

void SemanticTokensAnalyzer::visit(WhileCST *whileCst) {
    visitCompoundCommon((CompoundCSTToken *) whileCst);
};

void SemanticTokensAnalyzer::visit(DoWhileCST *doWhileCst) {
    visitCompoundCommon((CompoundCSTToken *) doWhileCst);
};

void SemanticTokensAnalyzer::visit(ForLoopCST *forLoop) {
    visitCompoundCommon((CompoundCSTToken *) forLoop);
};

void SemanticTokensAnalyzer::visit(SwitchCST *switchCst) {
    visitCompoundCommon((CompoundCSTToken *) switchCst);
};

void SemanticTokensAnalyzer::visit(StructDefCST *structDef) {
    visitCompoundCommon((CompoundCSTToken *) structDef);
};

void SemanticTokensAnalyzer::analyze(std::vector<std::unique_ptr<CSTToken>> &cstTokens) {
    for (auto &token: cstTokens) {
        token->accept(this);
    }
}

void SemanticTokensAnalyzer::visit(MultilineCommentToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_comment);
};

void SemanticTokensAnalyzer::visit(BoolToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_keyword);
};

void SemanticTokensAnalyzer::visit(NullToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_keyword);
}

void SemanticTokensAnalyzer::visit(CharOperatorToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_operator);
};

void SemanticTokensAnalyzer::visit(StringOperatorToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_operator);
}

void SemanticTokensAnalyzer::visit(CharToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_string);
};

void SemanticTokensAnalyzer::visit(CommentToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_comment);
};

void SemanticTokensAnalyzer::visit(MacroToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_macro);
};

void SemanticTokensAnalyzer::visit(NumberToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_number);
};

void SemanticTokensAnalyzer::visit(OperationToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_operator);
};

void SemanticTokensAnalyzer::visit(StringToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_string);
};