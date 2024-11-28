// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "utils/fwd/functional.h"

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
 * escape a single escapable character at index
 *
 * @param value the string value which contains the escape sequence
 * @param index the index at which escapable char occurs
 * e.g "\r" the index would be 1, as if cursor is before r like |r
 * @return the escaped character, and whether it succeeded
 */
std::pair<char, bool> escape_single(const std::string& value, unsigned index);

/**
 * this is the function that will run when a invalid escape sequence is encountered in the string
 * the second parameter is index into the given string
 */
using InvalidSeqHandler = std::function<void(const std::string&, unsigned int)>;

/**
 * will escape all the escapable characters and return the new formed string
 * the start and size value allow to cut the string and process portion of it
 * it will also return only that portion and nothing else
 *
 * @param value the string which contains the characters
 * @param start the start at which string to append into return (inclusive)
 * @param end the end index at which string to append into return (exclusive)
 * @return a string with all escapable characters like \n escaped
 */
std::string escape_all(const std::string& value, unsigned start, unsigned end, const InvalidSeqHandler& handler);