// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 11/03/2024.
//

#include "SemanticTokensAnalyzer.h"
#include "lexer/model/tokens/CharOperatorToken.h"
#include <unordered_set>
#include "cst/base/CompoundCSTToken.h"
#include "cst/utils/CSTUtils.h"

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
        case LexTokenType::Identifier:
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

void SemanticTokensAnalyzer::visitBody(CompoundCSTToken *bodyCst) {
    analyze(bodyCst->tokens);
}

void SemanticTokensAnalyzer::visitEnumDecl(CompoundCSTToken *enumDecl) {
    enumDecl->tokens[0]->accept(this);
    put(enumDecl->tokens[1]->start_token(), SemanticTokenType::ls_enum);
    visit(enumDecl->tokens, 2);
}

void SemanticTokensAnalyzer::visitVarInit(CompoundCSTToken *varInit) {
    visitCompoundCommon((CompoundCSTToken *) varInit);
}

void SemanticTokensAnalyzer::visitFunction(CompoundCSTToken *function) {
    function->tokens[0]->accept(this);
    put(function->tokens[1]->start_token(), SemanticTokenType::ls_function);
    visit(function->tokens, 2);
};

void SemanticTokensAnalyzer::visitIf(CompoundCSTToken *ifCst) {
    visitCompoundCommon((CompoundCSTToken *) ifCst);
};

void SemanticTokensAnalyzer::visitWhile(CompoundCSTToken *whileCst) {
    visitCompoundCommon((CompoundCSTToken *) whileCst);
};

void SemanticTokensAnalyzer::visitDoWhile(CompoundCSTToken *doWhileCst) {
    visitCompoundCommon((CompoundCSTToken *) doWhileCst);
};

void SemanticTokensAnalyzer::visitForLoop(CompoundCSTToken *forLoop) {
    visitCompoundCommon((CompoundCSTToken *) forLoop);
};

void SemanticTokensAnalyzer::visitSwitch(CompoundCSTToken *switchCst) {
    visitCompoundCommon((CompoundCSTToken *) switchCst);
};

void SemanticTokensAnalyzer::visitInterface(CompoundCSTToken *cst) {
    cst->tokens[0]->accept(this);
    put(cst->tokens[1].get(), SemanticTokenType::ls_interface);
    visit(cst->tokens, 2);
}

void SemanticTokensAnalyzer::visitImpl(CompoundCSTToken *cst) {
    bool has_for = is_keyword(cst->tokens[2].get(), "for");
    cst->tokens[0]->accept(this);
    put(cst->tokens[1].get(), SemanticTokenType::ls_interface);
    cst->tokens[2]->accept(this);
    if(has_for) {
        put(cst->tokens[3].get(), SemanticTokenType::ls_struct);
        visit(cst->tokens, 4);
    } else {
        visit(cst->tokens, 3);
    }
}

void SemanticTokensAnalyzer::visitStructDef(CompoundCSTToken *cst) {
    cst->tokens[0]->accept(this);
    put(cst->tokens[1].get(), SemanticTokenType::ls_struct);
    visit(cst->tokens, 2);
};

void SemanticTokensAnalyzer::analyze(std::vector<std::unique_ptr<CSTToken>> &cstTokens) {
    for (auto &token: cstTokens) {
        token->accept(this);
    }
}

void SemanticTokensAnalyzer::visitMultilineComment(LexToken *token) {
    put(token, SemanticTokenType::ls_comment);
};

void SemanticTokensAnalyzer::visitBoolToken(LexToken *token) {
    put(token, SemanticTokenType::ls_keyword);
};

void SemanticTokensAnalyzer::visitNullToken(LexToken *token) {
    put(token, SemanticTokenType::ls_keyword);
}

void SemanticTokensAnalyzer::visitCharOperatorToken(LexToken *token) {
    put(token, SemanticTokenType::ls_operator);
};

void SemanticTokensAnalyzer::visitStringOperatorToken(LexToken *token) {
    put(token, SemanticTokenType::ls_operator);
}

void SemanticTokensAnalyzer::visitCharToken(LexToken *token) {
    put(token, SemanticTokenType::ls_string);
};

void SemanticTokensAnalyzer::visitCommentToken(LexToken *token) {
    put(token, SemanticTokenType::ls_comment);
};

void SemanticTokensAnalyzer::visitMacroToken(LexToken *token) {
    put(token, SemanticTokenType::ls_macro);
};

void SemanticTokensAnalyzer::visitAnnotationToken(LexToken *token) {
    put(token, SemanticTokenType::ls_macro);
}

void SemanticTokensAnalyzer::visitNumberToken(NumberToken *token) {
    put((LexToken *) token, SemanticTokenType::ls_number);
};

void SemanticTokensAnalyzer::visitOperationToken(LexToken *token) {
    put(token, SemanticTokenType::ls_operator);
};

void SemanticTokensAnalyzer::visitStringToken(LexToken *token) {
    put(token, SemanticTokenType::ls_string);
};