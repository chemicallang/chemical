// Copyright (c) Qinetik 2024.

#include "RepresentationUtils.h"
#include <random>

std::string escape_encode(char value) {
    switch (value) {
        case '\a':
            return "\\a";
        case '\f':
            return "\\f";
        case '\r':
            return "\\r";
        case '\n':
            return "\\n";
        case '\0':
            return "\\0";
        case '\t':
            return "\\t";
        case '\v':
            return "\\v";
        case '\b':
            return "\\b";
        case '\"':
            return "\\\"";
        case '\?':
            return "\\?";
        case '\x1b':
            return "\\x1b";
        default:
            return std::string(1, value);
    }
}

int random(int min, int max) //range : [min, max]
{
    static bool first = true;
    if (first)
    {
        srand( time(NULL) ); //seeding for the first time only!
        first = false;
    }
    return min + rand() % (( max + 1 ) - min);
}