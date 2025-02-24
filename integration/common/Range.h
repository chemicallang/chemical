// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include "Position.h"

class Range {
public:
    // the range in the source file
    Position start, end;

    std::string representation() const {
        if(start.is_equal(end)) {
            return start.representation();
        } else {
            return start.representation() + " - " + end.representation();
        }
    }
};