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

inline bool is_linkage_public(AccessSpecifier specifier) {
    switch(specifier) {
        case AccessSpecifier::Public:
        case AccessSpecifier::Protected:
            return true;
        default:
            return false;
    }
}

std::string to_string(AccessSpecifier specifier);