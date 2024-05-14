// Copyright (c) Qinetik 2024.

#include "StringHelpers.h"
#include <functional>

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
            if (index + 2 < value.size() && value[index + 1] == '1' && value[index + 2] == 'b') {
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

std::string escape_all(const std::string& value, unsigned index, unsigned end, const InvalidSeqHandler& handler) {
    std::string ret;
    char current;
    while(index < end) {
        current = value[index];
        if(current == '\\') {
            if(index + 1 < end) {
                auto result = escapable_char(value, index + 1);
                if(result.second == -1) {
                    handler(value, index + 1);
                } else {
                    ret.append(1, result.first);
                }
                index = result.second;
                continue;
            } else {
                handler(value, index + 1);
                break;
            }
        } else {
            ret.append(1, current);
        }
        index++;
    }
    return ret;
}