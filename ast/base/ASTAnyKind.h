// Copyright (c) Chemical Language Foundation 2025.

#pragma once

#include <cstdint>

/**
 * @brief Enum class representing kind of ASTAny
 */
enum class ASTAnyKind : uint8_t {

    Value,
    Type,
    Node

};