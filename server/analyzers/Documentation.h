// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

class LexResult;

class CSTToken;

/**
 * responsible for putting the documentation of the given node into the given value
 * @param current the file user is editing currently
 * @param defined_in the file the given symbol is defined in
 * @param ref_any the token that is reference token, user is hovering at, or the ref token
 */
void markdown_documentation(std::string& value, LexResult* current, LexResult* defined_in, ASTNode* linked_node);

/**
 * a helper function
 */
void markdown_documentation(std::string& value, LexResult* current, LexResult* defined_in, ASTAny* ref_any);

/**
 * a helper function
 */
inline void markdown_documentation(std::string& value, LexResult* current, LexResult* defined_in, CSTToken* ref_tok) {
    markdown_documentation(value, current, defined_in, ref_tok->any);
}

/**
 * responsible for putting the documentation of the given node into the given value
 * @param current the file user is editing currently
 * @param defined_in the file the given symbol is defined in
 * @param parent the parent compound token of the symbol
 * @param linked the actual symbol / token for which documentation is to be added
 */
[[deprecated]]
void markdown_documentation_old(std::string& value, LexResult* current, LexResult* defined_in, CSTToken* parent, CSTToken* linked);

/**
 * get a small detail of the given token, for user's representation
 * it should be the most helpful thing about it, it's displayed
 * along with completion items
 */
void small_detail_of(std::string& value, ASTNode* linked);

/**
 * get a small detail of the given token, for user's representation
 * it should be the most helpful thing about it, it's displayed
 * along with completion items
 */
void small_detail_of_old(std::string& value, CSTToken* linked);