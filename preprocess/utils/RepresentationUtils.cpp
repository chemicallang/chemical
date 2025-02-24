// Copyright (c) Chemical Language Foundation 2025.

#include "RepresentationUtils.h"
#include <random>
#include <ostream>

void write_escape_encoded(std::ostream& stream, char value) {
    switch (value) {
        case '\a':
            stream << "\\a";
            return;
        case '\f':
            stream << "\\f";
            return;
        case '\r':
            stream << "\\r";
            return;
        case '\n':
            stream << "\\n";
            return;
        case '\0':
            stream << "\\0";
            return;
        case '\t':
            stream << "\\t";
            return;
        case '\v':
            stream << "\\v";
            return;
        case '\b':
            stream << "\\b";
            return;
        case '\"':
            stream << "\\\"";
            return;
        case '\?':
            stream << "\\?";
            return;
        case '\x1b':
            stream << "\\x1b";
            return;
        case '\'':
            stream << "\\'";
            return;
        case '\\':
            stream << "\\\\";
            return;
        default:
            stream << value;
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