// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>
#include <string>

enum class AccessSpecifier : uint8_t {
    Private = 0,
    Protected,
    Internal,
    Public
};

std::string to_string(AccessSpecifier specifier);