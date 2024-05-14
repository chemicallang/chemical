// Copyright (c) Qinetik 2024.

#pragma once

#include <string>
#include "utils/functionalfwd.h"

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