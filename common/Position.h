// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

class Position {
public:

    // line and character correspond to position in the source file
    unsigned int line, character;

    // representation of the position
    std::string representation() const {
        return std::to_string(line + 1) + ':' + std::to_string(character + 1);
    }

};