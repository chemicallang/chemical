// Copyright (c) Qinetik 2024.

#include "StringHelpers.h"

std::pair<char, unsigned int> escapable_char(const std::string &value, unsigned index) {
    char actualChar;
    switch (value[index]) {
        case 'a':
            actualChar = '\a';
            break;
        case 'f':
            actualChar = '\f';
            break;
        case 'r':
            actualChar = '\r';
            break;
        case 'n':
            actualChar = '\n';
            break;
        case '0':
            actualChar = '\0';
            break;
        case 't':
            actualChar = '\t';
            break;
        case 'v':
            actualChar = '\v';
            break;
        case 'b':
            actualChar = '\b';
            break;
        case '\\':
            actualChar = '\\';
            break;
        case '"':
            actualChar = '"';
            break;
        case '?':
            actualChar = '\?';
            break;
        case 'x':
            if (value.size() < index + 2 && value[index + 1] == '1' && value[index + 2] == 'b') {
                actualChar = '\x1b';
                index = index + 2;
            }
            break;
        default:
            return {value[index], -1};
    }
    return {actualChar, index + 1};
}

std::pair<char, bool> escape_single(const std::string &value, unsigned i) {
    if (i < value.size()) {
        auto result = escapable_char(value, i);
        if (result.second == -1) {
            return {value[i], false};
        } else {
            return {result.first, true};
        }
    } else {
        return {'\0', false};
    }
}