// Copyright (c) Chemical Language Foundation 2025.

#include "StringHelpers.h"
#include <functional>
#include "stream/SourceProvider.h"

char escaped_char(char current) {
    switch (current) {
        case 'a':
            return '\a';
        case 'f':
            return '\f';
        case 'r':
            return '\r';
        case 'n':
            return '\n';
        case '0':
            return '\0';
        case 't':
            return '\t';
        case 'v':
            return '\v';
        case 'b':
            return '\b';
        case '\\':
            return '\\';
        case '"':
            return '"';
        case '\'':
            return '\'';
        case '?':
            return '\?';
        default:
            return current;
    }
}

std::pair<char, int> escapable_char(const std::string_view &value, unsigned index) {
    char current = value[index];
    if(current != 'x') {
        auto next = escaped_char(current);
        if(next != current || next == '\\') {
            return {next, index + 1 };
        } else {
            return {current, -1 };
        }
    } else if (index + 2 < value.size() && value[index + 1] == '1' && value[index + 2] == 'b') {
        return { '\x1b', index + 3 };
    }
    return { current, -1 };
}

std::pair<char, int> escapable_char(const chem::string_view &value, unsigned index) {
    char current = value[index];
    if(current != 'x') {
        auto next = escaped_char(current);
        if(next != current || next == '\\' || next == '\'') {
            return {next, index + 1 };
        } else {
            return {current, -1 };
        }
    } else if (index + 2 < value.size() && value[index + 1] == '1' && value[index + 2] == 'b') {
        return { '\x1b', index + 3 };
    }
    return { current, -1 };
}

char escapable_char(SourceProvider& provider, char current) {
    if(current == 'x') {
        if(provider.increment('1') && provider.increment('b')) {
            return '\x1b';
        } else {
            return current;
        }
    } else {
        return escaped_char(current);
    }
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