// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>

class Value;

namespace chem {
    class string;
}

struct ValueSpan {
    Value** ptr;
    size_t size;
};

void take_chemical_values(std::vector<Value*>& values, ValueSpan* chemical_values);