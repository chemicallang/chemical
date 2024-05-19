// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

class LexResult;

class CompoundCSTToken;

class CSTToken;

/**
 * responsible for putting the documentation of the given node into the given value
 * @param current the file user is editing currently
 * @param defined_in the file the given symbol is defined in
 * @param parent the parent compound token of the symbol
 * @param linked the actual symbol / token for which documentation is to be added
 */
void markdown_documentation(std::string& value, LexResult* current, LexResult* defined_in, CompoundCSTToken* parent, CSTToken* linked);

/**
 * get a small detail of the given token, for user's representation
 * it should be the most helpful thing about it, it's displayed
 * along with completion items
 */
void small_detail_of(std::string& value, CSTToken* token);