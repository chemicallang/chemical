// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <vector>
#include <cstdint>

class Value;
class BaseType;

namespace chem {
    struct string;
}

struct ValueSpan {
    Value** ptr;
    size_t size;
};

struct BaseTypeSpan {
    BaseType** ptr;
    size_t size;
};

struct UbigintSpan {
    uint64_t* ptr;
    size_t size;
};

void take_chemical_values(std::vector<Value*>& values, ValueSpan* chemical_values);