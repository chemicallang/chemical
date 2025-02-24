// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <string>
#include "std/chem_string_view.h"

class SourceProvider;

/**
 * very simple, for 'n' it returns backslash n, for 'r' it returns backslash r
 * 'x' is not supported
 */
char escaped_char(char current);

/**
 * if current is 'n' it returns backslash n
 * current is the character to escape, provider can be provided to support
 * x1b, current should be 'x' and provider must be at peek == 1 so we can increment both
 */
char escapable_char(SourceProvider& provider, char current);

/**
 * will return the escaped character, for 'r' given at index, it will return '\r'
 *
 * @param value the string value which contains the escape sequence
 * @param index the index at which escapable char occurs
 * e.g "\r" the index would be 1, as if cursor is before r like |r
 * @return new index, at where the escaped char ends
 */
std::pair<char, int> escapable_char(const std::string_view& value, unsigned index);

/**
 * will return the escaped character, for 'r' given at index, it will return '\r'
 *
 * @param value the string value which contains the escape sequence
 * @param index the index at which escapable char occurs
 * e.g "\r" the index would be 1, as if cursor is before r like |r
 * @return new index, at where the escaped char ends
 */
std::pair<char, int> escapable_char(const chem::string_view& value, unsigned index);

/**
 * escape a single escapable character at index
 *
 * @param value the string value which contains the escape sequence
 * @param index the index at which escapable char occurs
 * e.g "\r" the index would be 1, as if cursor is before r like |r
 * @return the escaped character, and whether it succeeded
 */
std::pair<char, bool> escape_single(const std::string& value, unsigned index);