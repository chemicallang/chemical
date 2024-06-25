// Copyright (c) Qinetik 2024.

#pragma once

#include "Position.h"

class Range {
public:
    // the range in the source file
    Position start, end;

    std::string representation() const {
        return start.representation() + " - " + end.representation();
    }
};