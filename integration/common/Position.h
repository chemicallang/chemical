// Copyright (c) Qinetik 2024.

#pragma once

#include <string>

class Position {
public:

    // line and character correspond to position in the source file
    unsigned int line, character;

    /**
     * is this position, ahead of the given position
     */
    bool is_ahead(const Position& position) const;

    /**
     * is this position, behind the given position
     */
    bool is_behind(const Position& position) const;

    /**
     * is this position, equal to the given position
     */
    bool is_equal(const Position& position) const;

    /**
     * representation of the position
     */
    std::string representation() const {
        return std::to_string(line + 1) + ':' + std::to_string(character + 1);
    }

};