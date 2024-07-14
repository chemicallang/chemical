// Copyright (c) Qinetik 2024.

//
// Created by Waqas Tahir on 21/02/2024.
//
#pragma once

#include <string>

struct StreamPosition {

    unsigned int pos;
    unsigned int line;
    unsigned int character;

//    [[nodiscard]]
//    std::string formatted() const {
//        std::string format;
//        format.append("at position " + std::to_string(pos) + " line " + std::to_string(line) + " character " + std::to_string(character));
//        return format;
//    }

};