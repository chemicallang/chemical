// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "std/chem_string_view.h"

class LexResult;

/**
 * responsible for putting the documentation of the given node into the given value
 * @param current the file user is editing currently
 * @param defined_in the file the given symbol is defined in
 * @param ref_any the token that is reference token, user is hovering at, or the ref token
 */
void markdown_documentation(LocationManager& manager, std::string& value, const std::string_view& curr_file, ASTNode* linked_node);

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