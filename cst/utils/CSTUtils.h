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

inline std::string typealias_name(CompoundCSTToken* cst) {
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

/**
 * what is the parameter name in given comp param token
 */
std::string param_name(CompoundCSTToken* param);

/**
 * is given comp var init token a const
 */
bool is_var_init_const(CompoundCSTToken* cst);

/**
 * visits a range of tokens, from the given vector tokens, starting at start (inclusive) and end (exclusive)
 */
void visit(CSTVisitor* visitor, std::vector<std::unique_ptr<CSTToken>>& tokens, unsigned int start, unsigned int end);

/**
 * a helper for visit
 */
inline void visit(CSTVisitor* visitor, std::vector<std::unique_ptr<CSTToken>>& tokens, unsigned int start) {
    visit(visitor, tokens, start, tokens.size());
}

/**
 * a helper for visit
 */
inline void visit(CSTVisitor* visitor, std::vector<std::unique_ptr<CSTToken>>& tokens) {
    visit(visitor, tokens, 0, tokens.size());
}

/**
 * find's the token with given identifier inside the given tokens vector starting at start
 */
CSTToken* find_identifier(std::vector<std::unique_ptr<CSTToken>>& tokens, const std::string& identifier, unsigned start = 0);

/**
 * link's the child 'token' which is present in the given parent token
 * here the parent maybe a compound enum decl token, and token may be an identifier token present inside the enum
 * the function will find the enum member in the child and link the ref token with it
 */
CSTToken* link_child(CSTToken* parent, CSTToken* token);

/**
 * gets the linked node from given ref, this ref is a ref token
 * meaning a referenced variable token, like TypeToken or VariableToken
 */
CSTToken* get_linked(CSTToken* ref);

/**
 * get linked node from var init
 * this linked node is either present in type (if user gave it) or the value
 */
CSTToken* get_linked_from_var_init(std::vector<std::unique_ptr<CSTToken>>& tokens);

/**
 * get the linked node from given typealias compound tokens
 */
CSTToken* get_linked_from_typealias(std::vector<std::unique_ptr<CSTToken>>& tokens);

/**
 * get the linked node from given function compound tokens
 */
CSTToken* get_linked_from_func(std::vector<std::unique_ptr<CSTToken>>& tokens);

/**
 * get linked node from the given CST node (var init, struct member)
 */
CSTToken* get_linked_from_node(CSTToken* token);

/**
 * get's the child type, for example if it's an array
 */
CSTToken* get_child_type(CSTToken* token);

/**
 * first is the container token, which is the direct parent of the token
 * the second is the index in the tokens vector present in the first compound token
 */
using token_with_parent = std::pair<CompoundCSTToken*, unsigned int>;

/**
 * get the token's parent and index into it's tokens vector at where the token at given position is located
 * will return nullptr and -1 if not present
 * @param container this can be a nullptr but it's the token that will be returned, if the token has no parent !
 * @param tokens the vector in which to look for the position
 * @param position the position at which token should exist
 */
token_with_parent get_token_at_position(CompoundCSTToken* container, std::vector<std::unique_ptr<CSTToken>>& tokens, const Position& position);

/**
 * get the token at position or nullptr
 */
LexToken* get_token_at_position(std::vector<std::unique_ptr<CSTToken>>& tokens, const Position& position);

/**
 * will look for token in a lex result, and if found will return it, can be useful
 * to find which file a token belongs to, since lex result provides absolute path to it
 */
LexResult* find_containing_file(ImportUnit* unit, CSTToken* token);

/**
 * it will provide token with parent and also a pointer to file
 * so it gives you 1. token, 2. it's parent token, 3. the file it belongs to
 */
using token_parent_file = std::pair<LexResult*, token_with_parent>;

/**
 * given a token, it'll find the file it belongs to and it's parent and index in it's parent
 */
token_parent_file find_token_parent(ImportUnit* unit, CSTToken* token);