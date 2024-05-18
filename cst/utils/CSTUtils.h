// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <string>
#include "cst/base/CompoundCSTToken.h"
#include "lexer/model/tokens/CharOperatorToken.h"

class ImportUnit;

class LexResult;

inline std::string str_token(CSTToken *token) {
    return static_cast<LexToken *>(token)->value;
}

inline std::string escaped_str_token(CSTToken *token) {
    auto str = str_token(token);
    return str.substr(1, str.size() - 2);
}

inline char char_op(CSTToken *token) {
    return static_cast<CharOperatorToken *>(token)->value[0];
}

inline bool is_keyword(CSTToken *token, const std::string &x) {
    return token->type() == LexTokenType::Keyword && str_token(token) == x;
}

inline bool is_variable(CSTToken *token, const std::string &x) {
    return token->type() == LexTokenType::Variable && str_token(token) == x;
}

inline bool is_char_op(CSTToken *token, char x) {
    return token->type() == LexTokenType::CharOperator && char_op(token) == x;
}

inline bool is_str_op(CSTToken *token, const std::string &x) {
    return token->type() == LexTokenType::StringOperator && str_token(token) == x;
}

inline std::string var_init_identifier(CompoundCSTToken* cst) {
    return str_token(cst->tokens[1].get());
}

inline std::string func_name(CompoundCSTToken* func) {
    return str_token(func->tokens[1].get());
}

inline std::string enum_name(CompoundCSTToken* _enum) {
    return str_token(_enum->tokens[1].get());
}

inline std::string struct_name(CompoundCSTToken* _struct) {
    return str_token(_struct->tokens[1].get());
}

inline std::string interface_name(CompoundCSTToken* interface) {
    return str_token(interface->tokens[1].get());
}

std::string param_name(CompoundCSTToken* param);

bool is_var_init_const(CompoundCSTToken* cst);

void visit(CSTVisitor* visitor, std::vector<std::unique_ptr<CSTToken>>& tokens, unsigned int start, unsigned int end);

inline void visit(CSTVisitor* visitor, std::vector<std::unique_ptr<CSTToken>>& tokens, unsigned int start) {
    visit(visitor, tokens, start, tokens.size());
}

inline void visit(CSTVisitor* visitor, std::vector<std::unique_ptr<CSTToken>>& tokens) {
    visit(visitor, tokens, 0, tokens.size());
}

CSTToken* find_identifier(std::vector<std::unique_ptr<CSTToken>>& tokens, const std::string& identifier, unsigned start = 0);

CSTToken* link_child(CSTToken* parent, CSTToken* token);

CSTToken* get_linked(CSTToken* ref);

CSTToken* get_linked_from_var_init(std::vector<std::unique_ptr<CSTToken>>& tokens);

LexToken* get_token_at_position(std::vector<std::unique_ptr<CSTToken>>& tokens, const Position& position);

/**
 * will look for token in a lex result, and if found will return it, can be useful
 * to find which file a token belongs to, since lex result provides absolute path to it
 */
LexResult* find_container(ImportUnit* unit, CSTToken* token);

/**
 * will find the parent compound token of the given token in the ImportUnit, can be useful
 *  to find the parent of a enum member (identifier token), which is a enum token
 */
//CompoundCSTToken* find_parent(ImportUnit* unit, CSTToken* token);