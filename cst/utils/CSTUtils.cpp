// Copyright (c) Qinetik 2024.

#include "CSTUtils.h"
#include "lexer/model/tokens/VariableToken.h"

bool is_var_init_const(CompoundCSTToken* cst) {
    return str_token(cst->tokens[0].get()) == "const";
}

void visit(CSTVisitor* visitor, std::vector<std::unique_ptr<CSTToken>>& tokens, unsigned int start, unsigned int end) {
    while(start < end) {
        tokens[start]->accept(visitor);
        start++;
    }
}

CSTToken* find_identifier(std::vector<std::unique_ptr<CSTToken>>& tokens, const std::string& identifier, unsigned start) {
    CSTToken* token;
    while(start < tokens.size()) {
        token = tokens[start].get();
        if(token->type() == LexTokenType::Identifier && token->as_lex_token()->value == identifier) {
            return token;
        }
        start++;
    }
    return nullptr;
}

CSTToken* link_child(CSTToken* parent, CSTToken* token) {
    switch(parent->type()) {
        case LexTokenType::CompEnumDecl:
            if(token->type() != LexTokenType::Variable) {
                return nullptr;
            }
            ((VariableToken*) token)->linked = find_identifier(parent->as_compound()->tokens, token->as_lex_token()->value, 3);
            return ((VariableToken*) token)->linked;
        default:
            return nullptr;
    }
}