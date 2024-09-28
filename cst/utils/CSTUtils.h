// Copyright (c) Qinetik 2024.

#pragma once

#include <memory>
#include <string>
#include "cst/base/CSTToken.h"
#include "ast/base/AccessSpecifier.h"
#include <optional>

class LexImportUnit;

class LexResult;

inline const std::string& str_token(CSTToken *token) {
    return token->value();
}

inline std::string escaped_str_token(CSTToken *token) {
    auto& str = str_token(token);
    return str.substr(1, str.size() - 2);
}

inline char char_op(CSTToken *token) {
    return token->value()[0];
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

/**
 * get the access specifier, otherwise returns 99
 */
std::optional<AccessSpecifier> specifier_token(CSTToken* token);

/**
 * get index + 1 if it's a specifier (public / private) otherwise index
 */
inline unsigned int specifier_index(CSTToken* spec_token, unsigned index = 0) {
    return specifier_token(spec_token).has_value() ? index + 1 : index;
}

inline unsigned int var_init_name_index(CSTToken* cst) {
    return specifier_index(cst->tokens[0], 1);
}

inline CSTToken* var_init_name_tok(CSTToken* cst) {
    return cst->tokens[var_init_name_index(cst)];
}

inline const std::string& var_init_identifier(CSTToken* cst) {
    return str_token(var_init_name_tok(cst));
}

inline unsigned int typealias_name_index(CSTToken* cst) {
    return specifier_index(cst->tokens[0], 1);
}

inline CSTToken* typealias_name_tok(CSTToken* cst) {
    return cst->tokens[typealias_name_index(cst)];
}

inline const std::string& typealias_name(CSTToken* cst) {
    return str_token(typealias_name_tok(cst));
}

unsigned int func_name_index(CSTToken* function);

inline CSTToken* func_name_tok(CSTToken* function) {
    return function->tokens[func_name_index(function)];
}

inline const std::string& func_name(CSTToken* func) {
    return str_token(func_name_tok(func));
}

inline unsigned int enum_name_index(CSTToken* _enum) {
    return specifier_index(_enum->tokens[0], 1);
}

inline CSTToken* enum_name_tok(CSTToken* _enum) {
    return _enum->tokens[enum_name_index(_enum)];
}

inline const std::string& enum_name(CSTToken* _enum) {
    return str_token(enum_name_tok(_enum));
}

inline unsigned int struct_name_index(CSTToken* _struct) {
    return specifier_index(_struct->tokens[0], 1);
}

inline CSTToken* struct_name_tok(CSTToken* _struct) {
    return _struct->tokens[struct_name_index(_struct)];
}

inline const std::string& struct_name(CSTToken* _struct) {
    return str_token(struct_name_tok(_struct));
}

inline unsigned int interface_name_index(CSTToken* interface) {
    return specifier_index(interface->tokens[0], 1);
}

inline CSTToken* interface_name_tok(CSTToken* interface) {
    return interface->tokens[interface_name_index(interface)];
}

inline const std::string& interface_name(CSTToken* interface) {
    return str_token(interface_name_tok(interface));
}

/**
 * what is the parameter name in given comp param token
 */
const std::string& param_name(CSTToken* param);

/**
 * is given comp var init token a const
 */
bool is_var_init_const(CSTToken* cst);

/**
 * visits a range of tokens, from the given vector tokens, starting at start (inclusive) and end (exclusive)
 */
void visit(CSTVisitor* visitor, std::vector<CSTToken*>& tokens, unsigned int start, unsigned int end);

/**
 * a helper for visit
 */
inline void visit(CSTVisitor* visitor, std::vector<CSTToken*>& tokens, unsigned int start) {
    visit(visitor, tokens, start, tokens.size());
}

/**
 * a helper for visit
 */
inline void visit(CSTVisitor* visitor, std::vector<CSTToken*>& tokens) {
    visit(visitor, tokens, 0, tokens.size());
}

/**
 * find's the token with given identifier inside the given tokens vector starting at start
 */
CSTToken* find_identifier(std::vector<CSTToken*>& tokens, const std::string& identifier, unsigned start = 0);

#ifdef LSP_BUILD

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
CSTToken* get_linked_from_var_init(std::vector<CSTToken*>& tokens);

/**
 * get the linked node from given typealias compound tokens
 */
CSTToken* get_linked_from_typealias(std::vector<CSTToken*>& tokens);

/**
 * get the linked node from given function compound tokens
 */
CSTToken* get_linked_from_func(std::vector<CSTToken*>& tokens);

/**
 * get linked node from the given CST node (var init, struct member)
 */
CSTToken* get_linked_from_node(CSTToken* token);

#endif

/**
 * get's the child type, for example if it's an array
 */
CSTToken* get_child_type(CSTToken* token);

/**
 * first is the container token, which is the direct parent of the token
 * the second is the index in the tokens vector present in the first compound token
 */
using token_with_parent = std::pair<CSTToken*, unsigned int>;

/**
 * get the token's parent and index into it's tokens vector at where the token at given position is located
 * will return nullptr and -1 if not present
 * @param container this can be a nullptr but it's the token that will be returned, if the token has no parent !
 * @param tokens the vector in which to look for the position
 * @param position the position at which token should exist
 */
token_with_parent get_token_at_position(CSTToken* container, std::vector<CSTToken*>& tokens, const Position& position);

/**
 * get the token at position or nullptr
 */
CSTToken* get_token_at_position(std::vector<CSTToken*>& tokens, const Position& position);

/**
 * will look for token in a lex result, and if found will return it, can be useful
 * to find which file a token belongs to, since lex result provides absolute path to it
 */
LexResult* find_containing_file(LexImportUnit* unit, CSTToken* token);

/**
 * it will provide token with parent and also a pointer to file
 * so it gives you 1. token, 2. it's parent token, 3. the file it belongs to
 */
using token_parent_file = std::pair<LexResult*, token_with_parent>;

/**
 * given a token, it'll find the file it belongs to and it's parent and index in it's parent
 */
token_parent_file find_token_parent(LexImportUnit* unit, CSTToken* token);

/**
 * get the annotation token at index in annotation compound token
 */
CSTToken* annotation_arg(unsigned index, CSTToken* token);

/**
 * get the annotation parameter at index in annotation compound token
 */
std::optional<bool> annotation_bool_arg(unsigned index, CSTToken* token);

/**
 * get the string parameter at index in annotation compound token
 */
std::string annotation_str_arg(unsigned index, CSTToken* token);