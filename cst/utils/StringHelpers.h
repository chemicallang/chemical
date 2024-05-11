// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

/**
 * will return the escaped character, for 'r' given at index, it will return '\r'
 *
 * @param value the string value which contains the escape sequence
 * @param index the index at which escapable char occurs
 * e.g "\r" the index would be 1, as if cursor is before r like |r
 * @return new index, at where the escaped char ends
 */
std::pair<char, unsigned int> escapable_char(const std::string& value, unsigned index);

/**
 * escape a single escapable character at index
 *
 * @param value the string value which contains the escape sequence
 * @param index the index at which escapable char occurs
 * e.g "\r" the index would be 1, as if cursor is before r like |r
 * @return the escaped character, and whether it succeeded
 */
std::pair<char, bool> escape_single(const std::string& value, unsigned index);