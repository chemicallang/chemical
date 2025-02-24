// Copyright (c) Chemical Language Foundation 2025.

//
// Created by Waqas Tahir on 21/02/2024.
//
#pragma once

#include <string>

struct StreamPosition {

    off_t pos;
    unsigned int line;
    unsigned int character;
    size_t bufferPos;  // Position within the buffer

//    [[nodiscard]]
//    std::string formatted() const {
//        std::string format;
//        format.append("at position " + std::to_string(pos) + " line " + std::to_string(line) + " character " + std::to_string(character));
//        return format;
//    }

};