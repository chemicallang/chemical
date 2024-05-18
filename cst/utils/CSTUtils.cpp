// Copyright (c) Qinetik 2024.

#include "CSTUtils.h"
#include "lexer/model/tokens/RefToken.h"

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

CSTToken* find_func_or_var_init(std::vector<std::unique_ptr<CSTToken>>& tokens, const std::string& name, unsigned start) {
    CSTToken* token;
    while(start < tokens.size()) {
        token = tokens[start].get();
        if(token->is_var_init() || token->is_func_decl()) {
            if(str_token(token->as_compound()->tokens[1].get()) == name) {
                return token;
            }
        }
        start++;
    }
    return nullptr;
}

CSTToken* get_linked(CSTToken* ref) {
    switch(ref->type()) {
        case LexTokenType::Variable:
        case LexTokenType::Type:
            return ((RefToken*) ref)->linked;
        default:
            return nullptr;
    }
}

CSTToken* link_child(CSTToken* parent, CSTToken* token) {
    switch(parent->type()) {
        case LexTokenType::CompEnumDecl:
            if(token->type() != LexTokenType::Variable) {
                return nullptr;
            }
            ((RefToken*) token)->linked = find_identifier(parent->as_compound()->tokens, token->as_lex_token()->value, 3);
            return ((RefToken*) token)->linked;
        case LexTokenType::CompStructDef:
        case LexTokenType::CompInterface:
            if(token->type() != LexTokenType::Variable) {
                return nullptr;
            }
            return find_func_or_var_init(parent->as_compound()->tokens, token->as_lex_token()->value, 3);
        case LexTokenType::CompVarInit:
            if (is_char_op(parent->as_compound()->tokens[2].get(), ':')) {
                auto linked = get_linked(parent->as_compound()->tokens[3].get());
                if(linked) {
                    return link_child(linked, token);
                } else {
                    return nullptr;
                }
            } else {
                // TODO use the value
                return nullptr;
            }
        default:
            return nullptr;
    }
}