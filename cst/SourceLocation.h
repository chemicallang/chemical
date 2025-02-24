// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

class SourceLocation {
public:

    uint64_t encoded;

    constexpr SourceLocation(uint64_t encoded) : encoded(encoded) {

    }

    inline bool isValid() const { return encoded != 0; }
    inline bool isInvalid() const { return encoded == 0; }

};

/**
 * this is the location constant that is used to express
 * location at file-id = 0, line-start = 0, char-start = 0, line-end = 0, char-end = 0
 * file-id being zero, means invalid location, this is used when location is unknown
 */
inline constexpr uint64_t ZERO_LOC = 0;